#include <boost/process.hpp>
#include <boost/process/v1/args.hpp>
#include <boost/process/v1/detail/child_decl.hpp>
#include <boost/process/v1/exe.hpp>
#include <boost/process/v1/system.hpp>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <vector>

#include "toml.hpp"
#include "threadpool.hpp"

std::vector<std::filesystem::path> get_args_with_extension(const std::filesystem::path& dir, const std::string& extension) {
    std::vector<std::filesystem::path> result;

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (std::filesystem::is_regular_file(entry) && entry.path().extension() == extension) {
            result.push_back(entry.path());
        }
    }

    return result;
}

void build_project_gcc(
    std::string src_dir,
    std::string out_dir,
    std::string proj_name,
    std::string proj_type,
    std::vector<std::string> cflags,
    std::vector<std::string> lflags
) {
    std::string srcd = std::filesystem::current_path().string() + "/" + src_dir;
    std::string outd = std::filesystem::current_path().string() + "/" + out_dir;
    
    std::filesystem::create_directory(outd);
    std::filesystem::create_directory(outd + "/int");
    
    std::vector<std::filesystem::path> files = get_args_with_extension(srcd, ".c");
    
    const size_t max_threads = std::thread::hardware_concurrency();
    ThreadPool pool(max_threads);
    
    std::vector<boost::process::child> processes;
    std::mutex child_mutex;
    #ifdef __linux__
        if (proj_type == "ConsoleApp" || proj_type == "StaticLib") {
            for (auto &file : files) {
                pool.enqueue([&] {
                    boost::process::child process(
                        "/usr/bin/gcc",
                        boost::process::args(cflags),
                        boost::process::args({
                            "-c", file,
                            "-o", outd + "/int/" + file.replace_extension(".o").filename().string()
                        })
                    );
                    {
                        std::lock_guard<std::mutex> lock(child_mutex);
                        processes.push_back(std::move(process));
                    }
                });
            }
        } else if (proj_type == "SharedLib") {
            for (auto &file : files) {
                pool.enqueue([&] {
                    boost::process::child process(
                        "/usr/bin/gcc",
                        "-fPIC",
                        boost::process::args(cflags),
                        boost::process::args({
                            "-c", file,
                            "-o", outd + "/int/" + file.replace_extension(".o").filename().string()
                        })
                    );
                    {
                        std::lock_guard<std::mutex> lock(child_mutex);
                        processes.push_back(std::move(process));
                    }
                });
            }
        } else {
            std::cerr << "error: invalid project type!" << std::endl;
        }
        
        pool.enqueue([&] {
            std::lock_guard<std::mutex> lock(child_mutex);
            for (auto& process : processes) {
                process.wait();
            }
        });
        
        for (auto file : files) {
            file = std::filesystem::path(outd + "/int/").concat(file.replace_extension(".o").filename().string());
        }
        
        if (proj_type == "ConsoleApp") {
            boost::process::system(
                "/usr/bin/gcc",
                boost::process::args(lflags),
                boost::process::args(files),
                boost::process::args({"-o", outd + "/" + proj_name})
            );
        } else if (proj_type == "SharedLib") {
            boost::process::system(
                "/usr/bin/gcc",
                "-fPIC",
                "-shared",
                boost::process::args(lflags),
                boost::process::args(files),
                boost::process::args({"-o", outd + "/" + proj_name + ".so"})
            );
        } else if (proj_type == "StaticLib") {
            boost::process::system(
                "/usr/bin/ar",
                "rcs",
                boost::process::args({outd + "/" + proj_name + ".a"}),
                boost::process::args(files)
            );
        }
    #endif
    
}

void build_project_gpp(
    std::string src_dir,
    std::string out_dir,
    std::string proj_name,
    std::string proj_type,
    std::vector<std::string> cflags,
    std::vector<std::string> lflags
) {
    std::string srcd = std::filesystem::current_path().string() + "/" + src_dir;
    std::string outd = std::filesystem::current_path().string() + "/" + out_dir;
    
    std::filesystem::create_directory(outd);
    std::filesystem::create_directory(outd + "/int");
    
    std::vector<std::filesystem::path> files = get_args_with_extension(srcd, ".cpp");
    
    const size_t max_threads = std::thread::hardware_concurrency();
    ThreadPool pool(max_threads);
    
    std::vector<boost::process::child> processes;
    std::mutex child_mutex;
    #ifdef __linux__
        if (proj_type == "ConsoleApp" || proj_type == "StaticLib") {
            for (auto &file : files) {
                pool.enqueue([&] {
                    boost::process::child process(
                        "/usr/bin/g++",
                        boost::process::args(cflags),
                        boost::process::args({
                            "-c", file,
                            "-o", outd + "/int/" + file.replace_extension(".o").filename().string()
                        })
                    );
                    {
                        std::lock_guard<std::mutex> lock(child_mutex);
                        processes.push_back(std::move(process));
                    }
                });
            }
        } else if (proj_type == "SharedLib") {
            for (auto &file : files) {
                pool.enqueue([&] {
                    boost::process::child process(
                        "/usr/bin/g++",
                        "-fPIC",
                        boost::process::args(cflags),
                        boost::process::args({
                            "-c", file,
                            "-o", outd + "/int/" + file.replace_extension(".o").filename().string()
                        })
                    );
                    {
                        std::lock_guard<std::mutex> lock(child_mutex);
                        processes.push_back(std::move(process));
                    }
                });
            }
        } else {
            std::cerr << "error: invalid project type!" << std::endl;
        }
        
        pool.enqueue([&] {
            std::lock_guard<std::mutex> lock(child_mutex);
            for (auto& process : processes) {
                process.wait();
            }
        });
        
        for (auto file : files) {
            file = std::filesystem::path(outd + "/int/").concat(file.replace_extension(".o").filename().string());
        }
        
        if (proj_type == "ConsoleApp") {
            boost::process::system(
                "/usr/bin/g++",
                boost::process::args(lflags),
                boost::process::args(files),
                boost::process::args({"-o", outd + "/" + proj_name})
            );
        } else if (proj_type == "SharedLib") {
            boost::process::system(
                "/usr/bin/g++",
                "-fPIC",
                "-shared",
                boost::process::args(lflags),
                boost::process::args(files),
                boost::process::args({"-o", outd + "/" + proj_name + ".so"})
            );
        } else if (proj_type == "StaticLib") {
            boost::process::system(
                "/usr/bin/ar",
                "rcs",
                boost::process::args({outd + "/" + proj_name + ".a"}),
                boost::process::args(files)
            );
        }
    #endif
    
}

void build_project(std::string path) {
    auto weld_build_data = toml::parse(path + "/weld.toml", toml::spec::v(1, 1, 0));
    
    std::string project_name;
    std::string project_type;
    std::string src_dir;
    std::string out_dir;
    
    std::string toolset;
    std::vector<std::string> cflags;
    std::vector<std::string> lflags;
    if (weld_build_data.contains("project")) {
        auto project_data = toml::find(weld_build_data, "project");
        project_name = toml::find<std::string>(project_data, "name");
        project_type = toml::find<std::string>(project_data, "type");
        
        std::cout << "found project: " << project_name << std::endl;
    }
    
    if (weld_build_data.contains("settings")) {
        auto project_settings_data = toml::find(weld_build_data, "settings");
        src_dir = toml::find<std::string>(project_settings_data, "src_dir");
        out_dir = toml::find<std::string>(project_settings_data, "out_dir");
        
        if (project_settings_data.contains("toolset")) {
            toolset = toml::find<std::string>(project_settings_data, "toolset");
        } else {
            toolset = "gcc";
        }
        
        std::cout << "source dir: " << src_dir << std::endl;
        std::cout << "output dir: " << out_dir << std::endl;
        std::cout << "toolset: " << toolset << std::endl;
    }
    
    if (toolset == "gcc" || toolset == "g++") {
        if (weld_build_data.contains("gnuc")) {
            auto gnuc_data = toml::find(weld_build_data, "gnuc");
           
            if (gnuc_data.contains("cflags")) {
                cflags = toml::find<std::vector<std::string>>(gnuc_data, "cflags");
            }
            
            if (gnuc_data.contains("lflags")) {
                lflags = toml::find<std::vector<std::string>>(gnuc_data, "lflags");
            }
        }
        
        if (toolset == "gcc") {
            build_project_gcc(
                src_dir, out_dir,
                project_name, project_type,
                cflags, lflags
            );
        } else if (toolset == "g++") {
            build_project_gpp(
                src_dir, out_dir,
                project_name, project_type,
                cflags, lflags
            );
        }
    } else {
        std::cerr << "error: invalid or no toolset provided!" << std::endl;
    }
}

static char *shift(int &argc, char ***argv) {
    char *arg_s = **argv;
    *argv -= 1;
    argc -= 1;
    return arg_s;
}

int main(int argc, char **argv) {
    char *program = shift(argc, &argv);
    
    while (argc > 0) {
        char *flag = shift(argc, &argv);
        
        // TODO: Implement flags
    }
    
    build_project(std::filesystem::current_path().string());
}