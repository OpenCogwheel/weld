#pragma once

#include "dependencies.hpp"
#include <filesystem>
#include <string>
#include <vector>

struct TOMLData {
    // Workspace stuff
    bool is_workspace = false;
    std::vector<std::string> members;
    
    // Project stuff
    std::string project_name, project_type, project_path;
    std::string src_dir, out_dir, include_dir;
    std::string toolset;
    
    std::vector<std::string> cextensions, exclude;
    
    std::vector<std::string> cflags, lflags;
    
    Dependencies deps;
};

class TOMLReader {
public:
    TOMLReader(std::filesystem::path path, bool member);
public:
    inline TOMLData &get_data() { return m_Data; }
private:
    TOMLData m_Data;
};