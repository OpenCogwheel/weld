#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <filesystem>

class Commands {
public:
    template<typename ...Args>
    static inline void run(Args && ...args) {
        std::string commands = join_args(std::forward<Args>(args)...);
        std::system(commands.c_str());
    }
private:
    template <typename Iterator>
    static std::string join_range(Iterator begin, Iterator end) {
        std::ostringstream oss;
        for (Iterator it = begin; it != end; ++it) {
            if (it != begin) oss << " ";
            oss << *it;
        }
        return oss.str();
    }
    
    template <typename... Args>
    static std::string join_args(Args&&... args) {
        std::ostringstream oss;
        bool first = true;
    
        (void)std::initializer_list<int>{(
            [&]() {
                if constexpr (std::is_same_v<std::decay_t<Args>, std::vector<std::string>>) {
                    if (!first) oss << " ";
                    oss << join_range(args.begin(), args.end());
                } else if constexpr (std::is_same_v<std::decay_t<Args>, std::vector<std::filesystem::path>>) {
                    if (!first) oss << " ";
                    oss << join_range(args.begin(), args.end());
                } else {
                    if (!first) oss << " ";
                    oss << std::forward<Args>(args);
                }
                first = false;
            }(), 0)...};
    
        return oss.str();
    }
};