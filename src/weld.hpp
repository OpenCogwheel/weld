    #pragma once

#include <cstdlib>
#include <string>
#include <vector>
#include <filesystem>
#include <cassert>

#include "toml_reader.hpp"

std::vector<std::filesystem::path> get_args_with_extensions(const std::filesystem::path& dir, const std::vector<std::string>& extensions);

std::string find_exec_path(std::string name);

void exclude_files_and_folders(
    const std::filesystem::path &root_dir, 
    std::vector<std::filesystem::path> &files, 
    const std::vector<std::string> &exclude
);

void build_project_gnuc(TOMLData data);
void build_workspace_gnuc(TOMLData data);

void create_project(std::string toolset, std::string project_name);