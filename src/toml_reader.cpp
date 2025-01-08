#include "toml_reader.hpp"

#include "toml.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>

TOMLReader::TOMLReader(std::filesystem::path path) {
    auto weld_build_data = toml::parse(path.string() + "/weld.toml", toml::spec::v(1, 1, 0));
    
    if (weld_build_data.contains("project")) {
        auto project_settings = toml::find(weld_build_data, "project");
        m_Data.project_name = toml::find<std::string>(project_settings, "name");
        m_Data.project_type = toml::find<std::string>(project_settings, "type");
    } else if (weld_build_data.contains("workspace")) {
        std::cerr << "error: workspace isnt implemented yet" << std::endl;
        exit(1);
    } else {
        std::cerr << "error: no project or workspace found" << std::endl;
        exit(1);
    }
    
    if (weld_build_data.contains("settings")) {
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
    
    if (weld_build_data.contains("files")) {
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
        if ((m_Data.toolset == "gcc" || m_Data.toolset == "g++")) {
            auto gnuc_settings = toml::find(weld_build_data, "gnuc");
            
            if (gnuc_settings.contains("cflags")) {
                m_Data.cflags = toml::find<std::vector<std::string>>(gnuc_settings, "cflags");
            }
            if (gnuc_settings.contains("lflags")) {
                m_Data.lflags = toml::find<std::vector<std::string>>(gnuc_settings, "lflags");
            }
        } else {
            std::cerr << "error: gnuc requires toolset gcc or g++" << std::endl;
            exit(1);
        }
    }
}