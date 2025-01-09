    #pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cassert>
#include <fstream>

#include "toml_reader.hpp"
#include "command.hpp"
#include "threadpool.hpp"

std::vector<std::filesystem::path> get_args_with_extensions(const std::filesystem::path& dir, const std::vector<std::string>& extensions);

std::string find_exec_path(std::string name);

void exclude_files_and_folders(
    const std::filesystem::path &root_dir, 
    std::vector<std::filesystem::path> &files, 
    const std::vector<std::string> &exclude
);

void build_project_gnuc(TOMLData data);

void create_project(std::string toolset, std::string project_name);