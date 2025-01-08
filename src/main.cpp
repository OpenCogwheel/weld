#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

#include "toml_reader.hpp"
#include "command.hpp"
#include "threadpool.hpp"

std::vector<std::filesystem::path> get_args_with_extensions(const std::filesystem::path& dir, const std::vector<std::string>& extensions) {
    std::vector<std::filesystem::path> result;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (std::filesystem::is_regular_file(entry)) {
            std::string ext = entry.path().extension().string();

            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                result.push_back(entry.path());
            }
        }
    }

    return result;
}

std::string find_exec_path(std::string name) {
    std::string path;
    try {
        const char* path_env = std::getenv("PATH");
        if (!path_env) {
            throw std::runtime_error("PATH environment variable is not set");
        }

        std::string path_list = path_env;
        char path_seperator =
        #if defined(_WIN32) || defined(_WIN64)
            ';';
        #else
            ':';
        #endif
        
        size_t pos = 0;
        while ((pos = path_list.find(path_seperator)) != std::string::npos) {
            std::string dir = path_list.substr(0, pos);
            path_list.erase(0, pos + 1);
            std::filesystem::path exec_path = std::filesystem::path(dir) / name;
            if (std::filesystem::exists(exec_path) && std::filesystem::is_regular_file(exec_path)) {
                return exec_path;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: failed to find executable " + name + " in path: " << e.what() << std::endl;
    }

    return "";
}

void exclude_files_and_folders(
    const std::filesystem::path &root_dir, 
    std::vector<std::filesystem::path> &files, 
    const std::vector<std::string> &exclude
) {
    std::filesystem::path root_path(root_dir);
    
    for (auto it = files.begin(); it != files.end(); ) {
        bool should_exclude = false;
        
        for (const auto &excluded : exclude) {
            std::filesystem::path excluded_path = root_path / excluded;
            
            if (excluded_path.has_filename() == false) {
                excluded_path = excluded_path.parent_path();
            }
            
            excluded_path = excluded_path.lexically_normal();
            
            std::filesystem::path current_path = *it;
            
            current_path = current_path.lexically_normal(); 
            
            if (current_path == excluded_path) {
                should_exclude = true;
                break;
            }
            
            if (current_path.string().find(excluded_path.string() + std::filesystem::path::preferred_separator) == 0) {
                should_exclude = true;
                break;
            }
        }
        
        if (should_exclude) {
            it = files.erase(it);
        } else {
            ++it;
        }
    }
}

void build_project_gnuc(TOMLData data) {
    std::string full_src_path = std::filesystem::current_path().string() + "/" + data.src_dir;
    std::string full_out_path = std::filesystem::current_path().string() + "/" + data.out_dir;
    
    std::vector<std::filesystem::path> files 
        = get_args_with_extensions(full_src_path, data.cextensions);
    
    ThreadPool pool(std::thread::hardware_concurrency());
    std::vector<std::future<void>> futures;
    
    // Create the required output directory
    std::filesystem::create_directory(full_out_path);
    std::filesystem::create_directory(full_out_path + "/genobjs");
    
    std::string gnuc_path = find_exec_path(data.toolset);
    
    exclude_files_and_folders(full_src_path, files, data.exclude);

    std::string out_name = data.project_name;
    #ifdef __linux__
        if (data.project_type == "SharedLib") {
            data.cflags.push_back("-fPIC");
            data.lflags.push_back("-fPIC");
            data.lflags.push_back("-shared");
            data.project_name += ".so";
        } else if (data.project_type == "StaticLib") {
            data.project_name += ".a";
        } else {
            if (data.project_type != "ConsoleApp") {
                std::cerr << "error: invalid project type!" << std::endl;
                exit(1);
            }
        }
        
        for (auto &file : files) {
            std::filesystem::path out_file = file; out_file.replace_extension(".o");
            std::cout << "Building ---> " + file.string() + "\n";
            pool.enqueue([=]() {
                Commands::run(
                    gnuc_path,
                    data.cflags,
                    "-c", file,
                    "-o", full_out_path + "/genobjs/" + out_file.filename().string()
                );
                
                std::cout << "Finished ---> " << out_file.string() << std::endl;
            });
        }
        
        pool.get();
        
        for (auto file : files) {
            file = std::filesystem::path(full_out_path + "/genobjs/").concat(file.filename().string());
        }
        
        std::cout << "Linking ---> " << data.project_name << std::endl;
        if (data.project_type == "StaticLib") {
            Commands::run(
                find_exec_path("ar"),
                "rcs",
                full_out_path + "/" + out_name + ".a",
                files
            );
        } else {
            Commands::run(
                gnuc_path,
                data.lflags,
                files,
                "-o", full_out_path + "/" + out_name
            );
        }
        
        std::cout << "Finished Linking" << std::endl;
    #endif
}

int main(int argc, char **argv) {
    TOMLReader toml_reader(std::filesystem::current_path());
    
    if (toml_reader.get_data().toolset == "gcc" || toml_reader.get_data().toolset == "g++") {
        TOMLData data = toml_reader.get_data();
        build_project_gnuc(data);
    } else {
        std::cerr << "error: invalid toolset" << std::endl;
        exit(1);
    }
}