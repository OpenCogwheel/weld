#include "weld.hpp"

char *shift(int &argc, char ***argv) {
    assert(argc > 0 && "argc <= 0");
    --argc;
    return *(*argv)++;
}

int main(int argc, char **argv) {
    char *program = shift(argc, &argv);
    
    if (argc < 1) {
        TOMLReader toml_reader(std::filesystem::current_path());
        
        if (toml_reader.get_data().toolset == "gcc" || toml_reader.get_data().toolset == "g++") {
            TOMLData data = toml_reader.get_data();
            build_project_gnuc(data);
        } else {
            std::cerr << "error: invalid toolset" << std::endl;
            exit(1);
        }
    } else {
        if (argc < 1) {
            std::cerr << "error: missing subcommand!" << std::endl;
        }
        
        char *subcommand = shift(argc, &argv);
        
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