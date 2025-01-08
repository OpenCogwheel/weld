#include <filesystem>
#include <string>
#include <vector>


struct TOMLData {
    std::string project_name, project_type;
    std::string src_dir, out_dir;
    std::string toolset;
    
    std::vector<std::string> cextensions, exclude;
    
    std::vector<std::string> cflags, lflags;
};

class TOMLReader {
public:
    TOMLReader(std::filesystem::path path);
public:
    inline TOMLData &get_data() { return m_Data; }
private:
    TOMLData m_Data;
};