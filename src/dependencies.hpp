#pragma once

#include <cstdlib>
#include <iostream>
#include <tuple>
#include <vector>

#include "toml.hpp"

class Dependencies {
public:
    void read(std::string path) {
        auto config = toml::parse(path + "/weld.toml");
        
        if (config.contains("dependencies")) {
            auto dependencies = config["dependencies"].as_table();
            
            for (auto &[key, value] : dependencies) {
                if (value.is_table()) {;
                    if (value.contains("path")) {
                        auto path = value.at("path");
                        
                        bool include = false;
                        if (value.contains("include")) {
                            include = value.at("include").as_boolean();
                        }
                        
                        if (path.is_string()) {
                            m_Dependencies.push_back(std::make_tuple(path.as_string(), include));
                        } else {
                            std::cerr << "error: path must be string!" << std::endl;
                            exit(1);
                        }
                    }
                }
            }
        }
    }
public:
    std::vector<std::tuple<std::string, bool>> m_Dependencies;
};