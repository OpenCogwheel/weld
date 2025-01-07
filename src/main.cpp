#include <boost/process.hpp>
#include <boost/process/v1/args.hpp>
#include <boost/process/v1/exe.hpp>
#include <boost/process/v1/system.hpp>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <vector>

#include "toml.hpp"

std::vector<std::filesystem::path> get_args_with_extension(const std::filesystem::path& dir, const std::string& extension) {
    std::vector<std::filesystem::path> result;

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (std::filesystem::is_regular_file(entry) && entry.path().extension() == extension) {
            result.push_back(entry.path());
        }
    }

    return result;
}

void build_project_gcc(std::string src_dir, std::string out_dir, std::string proj_name) {
    std::string srcd = std::filesystem::current_path().string() + "/" + src_dir;
    std::string outd = std::filesystem::current_path().string() + "/" + out_dir;
    
    std::filesystem::create_directory(outd);
    std::filesystem::create_directory(outd + "/int");
    
    std::vector<std::filesystem::path> files = get_args_with_extension(srcd, ".cpp");
    #ifdef __linux__
        for (auto file : files) {
            boost::process::system(
                "/usr/bin/gcc",
                boost::process::args({
                    "-c", file,
                    "-o", outd + "/int/" + file.replace_extension(".o").filename().string()
                })
            );
        }
        
        for (auto file : files) {
            file = std::filesystem::path(outd + "/int/").concat(file.replace_extension(".o").filename().string());
        }
        
        boost::process::system(
            "/usr/bin/gcc",
            boost::process::args(files),
            boost::process::args({"-o", outd + "/" + proj_name})
        );
    #endif
    
}

void build_project(std::string path) {
    auto weld_build_data = toml::parse(path + "/weld.toml", toml::spec::v(1, 1, 0));
    
    std::string project_name;
    std::string src_dir;
    std::string out_dir;
    
    std::string toolset;
    if (weld_build_data.contains("project")) {
        auto project_data = toml::find(weld_build_data, "project");
        project_name = toml::find<std::string>(project_data, "name");
        
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
    
    if (toolset == "gcc") {
        build_project_gcc(src_dir, out_dir, project_name);
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
    
    if (argc < 1) {
        build_project(std::filesystem::current_path().string());
    } else {
        while (argc > 0) {
            char *flag = shift(argc, &argv);
            
            // TODO: Implement flags
        }
    }
}