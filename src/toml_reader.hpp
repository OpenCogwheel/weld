#pragma once

#include "dependencies.hpp"
#include <filesystem>
#include <string>
#include <vector>

struct TOMLCommand {
    int stage;
    std::vector<std::string> cmds;
};

struct TOMLData {
    // Workspace stuff
    bool is_workspace = false;
    std::vector<std::string> members;
    
    // Build stuff
    std::vector<TOMLCommand> build_commands;

    // Project stuff
    std::string project_name, project_type, project_path;
    std::string src_dir, out_dir, include_dir;
    std::string toolset;

    std::vector<std::string> cextensions, exclude;

    std::vector<std::string> cflags, lflags;

    Dependencies deps;

    // Install stuff
    bool can_install = false;
    std::string binary_install_path;
};

class TOMLReader {
public:
    TOMLReader(std::filesystem::path path);
public:
    inline TOMLData &get_data() { return m_Data; }
private:
    TOMLData m_Data;
};
