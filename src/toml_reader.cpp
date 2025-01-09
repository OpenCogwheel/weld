#include "toml_reader.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

#include "toml.hpp"
#include "dependencies.hpp"
#include "weld.hpp"

TOMLReader::TOMLReader(std::filesystem::path path) {
    auto weld_build_data = toml::parse(path.string() + "/weld.toml", toml::spec::v(1, 1, 0));
    m_Data.project_path = path.string();
    
    if (weld_build_data.contains("project")) {
        auto project_settings = toml::find(weld_build_data, "project");
        m_Data.project_name = toml::find<std::string>(project_settings, "name");
        m_Data.project_type = toml::find<std::string>(project_settings, "type");
    } else if (weld_build_data.contains("workspace")) {
        auto workspace_settings = toml::find(weld_build_data, "workspace");
        m_Data.is_workspace = true;
        m_Data.members = toml::find<std::vector<std::string>>(workspace_settings, "members");
        
        if (workspace_settings.contains("out_dir")) {
            m_Data.out_dir = toml::find<std::string>(workspace_settings, "out_dir");
        } else {
            m_Data.out_dir = "out";
        }
    } else {
        std::cerr << "error: no project or workspace found" << std::endl;
        exit(1);
    }
    
    if (weld_build_data.contains("settings")) {
        if (m_Data.is_workspace) {
            std::cout << "error: settings is project only, not workspace!" << std::endl;
            exit(1);
        }
        
        auto settings = toml::find(weld_build_data, "settings");
        
        if (settings.contains("toolset")) {
            m_Data.toolset = toml::find<std::string>(settings, "toolset");
        } else { m_Data.toolset = "gcc"; }
        
        if (settings.contains("src_dir")) {
            m_Data.src_dir = toml::find<std::string>(settings, "src_dir");
        } else { m_Data.src_dir = "src"; }
        if (settings.contains("out_dir")) {
            m_Data.out_dir = toml::find<std::string>(settings, "out_dir");
        } else { m_Data.src_dir = "bin"; }  
    } else {
        m_Data.src_dir = "src";
        m_Data.out_dir = "bin";
    }
    
    if (weld_build_data.contains("lib")) {
        if (m_Data.is_workspace) {
            std::cout << "error: lib is project only, not workspace!" << std::endl;
            exit(1);
        }
        
        if (m_Data.project_type == "SharedLib"
            || m_Data.project_type == "StaticLib"
            || m_Data.project_type == "Utility") {
            auto lib_settings = toml::find(weld_build_data, "lib");
            
            if (lib_settings.contains("include_dir")) {
                m_Data.include_dir = toml::find<std::string>(lib_settings, "include_dir");
            } else {
                m_Data.include_dir = "src";
            }
        } else {
            std::cerr << "error: lib header is only for libraries!" << std::endl;
            exit(1);
        }
    } else {
        m_Data.include_dir = "src";
    }
    
    if (weld_build_data.contains("files")) {
        if (m_Data.is_workspace) {
            std::cout << "error: files is project only, not workspace!" << std::endl;
            exit(1);
        }
        
        auto file_settings = toml::find(weld_build_data, "files");
        
        if (file_settings.contains("cextensions")) {
            m_Data.cextensions = toml::find<std::vector<std::string>>(file_settings, "cextensions");
        } else { m_Data.cextensions = {".c", ".cpp"}; }
        
        if (file_settings.contains("exclude")) {
            m_Data.exclude = toml::find<std::vector<std::string>>(file_settings, "exclude");
        }
    } else {
        m_Data.src_dir = "src";
        m_Data.out_dir = "bin";
    }
    
    if (weld_build_data.contains("gnuc")) {
        if (m_Data.is_workspace) {
            std::cout << "error: gnuc is project only, not workspace!" << std::endl;
            exit(1);
        }
        
        if ((m_Data.toolset == "gcc" || m_Data.toolset == "g++")) {
            auto gnuc_settings = toml::find(weld_build_data, "gnuc");
            
            if (gnuc_settings.contains("cflags")) { 
                std::vector<std::string> cflags = toml::find<std::vector<std::string>>(gnuc_settings, "cflags");
                for (auto cflag : cflags) {
                    m_Data.cflags.push_back(cflag);
                }
            }
            
            if (gnuc_settings.contains("lflags")) {
                std::vector<std::string> lflags = toml::find<std::vector<std::string>>(gnuc_settings, "cflags");
                for (auto lflag : lflags) {
                    m_Data.cflags.push_back(lflag);
                }
            }
        } else {
            std::cerr << "error: gnuc requires toolset gcc or g++" << std::endl;
            exit(1);
        }
    }
    
    if (weld_build_data.contains("dependencies")) {
        if (m_Data.is_workspace) {
            std::cout << "error: dependencies is project only, not workspace!" << std::endl;
            exit(1);
        }
        
        Dependencies dependencies;
        dependencies.read(path);
        
        for (std::tuple<std::string, bool> dep: dependencies.m_Dependencies) {
            TOMLReader reader(m_Data.project_path + "/" + std::get<0>(dep));
            TOMLData data = reader.get_data();
            
            if (data.toolset == "gcc" || data.toolset == "g++" || data.project_type != "Utility") {
                build_project_gnuc(data);
            }
            
            #ifdef __linux__
                if (data.project_type == "SharedLib") {
                    if (std::get<1>(dep)) {
                        m_Data.cflags.push_back("-I" + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.include_dir);
                        m_Data.lflags.push_back("-I" + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.include_dir);
                    }
                    
                    m_Data.lflags.push_back("-L" + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.out_dir);
                    m_Data.lflags.push_back("-Wl,-rpath," + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.out_dir);
                    m_Data.lflags.push_back("-l" + data.project_name);
                } else if (data.project_type == "StaticLib") {
                    if (std::get<1>(dep)) {
                        m_Data.cflags.push_back("-I" + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.include_dir);
                        m_Data.lflags.push_back("-I" + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.include_dir);
                    }
                    
                    m_Data.lflags.push_back("-L" + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.out_dir);
                    m_Data.lflags.push_back("-l" + data.project_name);
                } else if (data.project_type == "Utility") {
                    if (std::get<1>(dep)) {
                        m_Data.cflags.push_back("-I" + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.include_dir);
                        m_Data.lflags.push_back("-I" + m_Data.project_path + "/" + std::get<0>(dep) + "/" + data.include_dir);
                    }
                }
            #endif
        }
    }
}