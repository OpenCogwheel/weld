#include "weld.hpp"
#include "command.hpp"
#include <algorithm>
#include <filesystem>

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
    std::string full_src_path = data.project_path + "/" + data.src_dir;
    std::string full_out_path = data.project_path + "/" + data.out_dir;
    
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
            out_name = "lib" + data.project_name + ".so";
        } else if (data.project_type == "StaticLib") {
            out_name = "lib" + data.project_name + ".a";
        } else {
            if (data.project_type != "ConsoleApp") {
                std::cerr << "error: invalid project type!" << std::endl;
                exit(1);
            }
        }

        
        for (auto &file : files) {
            std::filesystem::path out_file = file.filename(); out_file.replace_extension(".o");
            std::cout << "Building ---> " + file.filename().string() + "\n";
            pool.enqueue([=]() {
                Commands::run(
                    gnuc_path,
                    data.cflags,
                    "-c", file,
                    "-o", full_out_path + "/genobjs/" + out_file.string()
                );
                
                std::cout << "Finished ---> " << out_file.string() << std::endl;
            });
        }
        
        pool.get();
        
        for (auto &file : files) {
            file = std::filesystem::path(full_out_path + "/genobjs/").concat(file.filename().replace_extension(".o").string());
        }
        
        if (data.project_type == "StaticLib") {
            std::cout << "Creating ---> " << out_name << std::endl;
            Commands::run(
                find_exec_path("ar"),
                "rcs",
                full_out_path + "/" + out_name,
                files
            );
            std::cout << "Finished Creating Static" << std::endl;
        } else {
            std::cout << "Linking ---> " << data.project_name << std::endl;
            Commands::run(
                gnuc_path,
                files,
                data.lflags,
                "-o", full_out_path + "/" + out_name
            );
            std::cout << "Finished Linking" << std::endl;
        }
    #endif
}

void create_project(std::string toolset, std::string project_name) {
    std::filesystem::create_directory(std::filesystem::current_path() / project_name);
    std::filesystem::create_directory(std::filesystem::current_path() / project_name / "src");
    
    std::ofstream file(project_name + "/weld.toml");
    
    if (file.is_open()) {
        file << "[project]\n";
        file << "name = \"" << project_name << "\"\n";
        file << "type = \"ConsoleApp\"\n\n";
        
        file << "[files]\n";
        if (toolset == "gcc") {
            file << "cextensions = [\".c\"]\n\n";
        } else if (toolset == "g++") {
            file << "cextensions = [\".cpp\"]\n\n";
        }
        
        file << "[settings]\n";
        file << "toolset = \"" << toolset << "\"\n";
        file << "src_dir = \"src\"\n";
        file << "out_dir = \"bin\"\n\n";
        
        file << "[gnuc]\n";
        file << "cflags = [\"\"]\n";
        file << "lflags = [\"\"]\n";
    } else {
        std::cerr << "error: failed to write weld.toml!";
        exit(1);
    }
    
    file.close();
    
    if (toolset == "gcc") {
        std::ofstream lfile(std::filesystem::current_path() / project_name / "src/main.c");
        
        if (lfile.is_open()) {
            std::filesystem::path current_file = __FILE__;
            std::ifstream ctemp(current_file.parent_path() / "templates" / "ctemp");
            
            lfile << ctemp.rdbuf();
            
            ctemp.close();
        } else {
            std::cerr << "error: failed to write main.c!";
            exit(1);
        }
        
        lfile.close();
    } else if (toolset == "g++") {
        if (toolset == "g++") {
            std::ofstream lfile(std::filesystem::current_path() / project_name / "src/main.cpp");
            
            if (lfile.is_open()) {
                std::filesystem::path current_file = __FILE__;
                std::ifstream ctemp(current_file.parent_path() / "templates" / "cpptemp");
                
                lfile << ctemp.rdbuf();
                
                ctemp.close();
            } else {
                std::cerr << "error: failed to write main.cpp!";
                exit(1);
            }
            
            lfile.close();
        }
    }
}