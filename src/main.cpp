#include "command.hpp"
#include "toml_reader.hpp"
#include "weld.hpp"
#include <cstdlib>
#include <filesystem>

void build_project(TOMLData data) {
    if (data.toolset == "gcc" || data.toolset == "g++") {
        build_project_gnuc(data);
    } else {
        std::cerr << "error: invalid toolset in " + data.project_name << std::endl;
        exit(1);
    }
}

void build_workspace(TOMLData data) {
    build_workspace_gnuc(data);
}

char *shift(int &argc, char ***argv) {
    assert(argc > 0 && "argc <= 0");
    --argc;
    return *(*argv)++;
}

int main(int argc, char **argv) {
    char *program = shift(argc, &argv);
    
    if (argc < 1) {
        TOMLReader toml_reader(std::filesystem::current_path());
        if (toml_reader.get_data().is_workspace) {
            TOMLData data = toml_reader.get_data();
            build_workspace(data);
        } else {
            build_project(toml_reader.get_data());
        }
    } else {
        if (argc < 1) {
            std::cerr << "error: missing subcommand!" << std::endl;
        }
        
        char *subcommand = shift(argc, &argv);
        
        if (std::string(subcommand) == "install") {
            TOMLReader toml_reader(std::filesystem::current_path());
            TOMLData data = toml_reader.get_data();
            
            if (data.can_install) {
                if (toml_reader.get_data().is_workspace) {
                    std::cout << "error: install for workspaces isn't supported yet!" << std::endl;
                    exit(1);
                } else {
                    build_project(data);
                }
                
                #ifdef __linux__
                    Commands::run(
                        "sudo", "-k", find_exec_path("cp"), "-i",
                        data.project_path + "/" + data.out_dir + "/" + data.project_name,
                        data.binary_install_path
                    );
                #endif
            } else {
                std::cerr << "error: this project doesnt support installation!" << std::endl;
                exit(1);
            }
        }
        
        if (std::string(subcommand) == "new") {
            std::string toolset = "gcc";
            
            if (argc < 1) {
                std::cerr << "error: missing toolset!" << std::endl;
            }
            
            char *project_name = shift(argc, &argv);
            
            while (argc > 0) {
                char *flag = shift(argc, &argv);
                
                if (std::string(flag) == "--toolset") {
                    toolset = shift(argc, &argv);
                } else {
                    std::cerr << "error: invalid flag `" << flag << "`" << std::endl;
                }
            }
            
            create_project(toolset, project_name);
        }
    }
}