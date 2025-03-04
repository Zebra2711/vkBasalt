#ifndef CONFIG_HPP_INCLUDED
#define CONFIG_HPP_INCLUDED

#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

namespace vkBasalt
{
    class Config
    {
    public:
        Config() noexcept;
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;
        Config(Config&&) noexcept = default;
        Config& operator=(Config&&) noexcept = default;
        ~Config() = default;

        template<typename T>
        [[nodiscard]] inline T getOption(const std::string& option, const T& defaultValue = {}) noexcept
        {
            T result = defaultValue;
            parseOption(option, result);
            return result;
        }

    private:
        std::unordered_map<std::string, std::string> options;

        void readConfigLine(std::string line) noexcept;
        void readConfigFile(std::ifstream& stream) noexcept;

        void parseOption(const std::string& option, int32_t& result) noexcept;
        void parseOption(const std::string& option, float& result) noexcept;
        void parseOption(const std::string& option, bool& result) noexcept;
        void parseOption(const std::string& option, std::string& result) noexcept;
        void parseOption(const std::string& option, std::vector<std::string>& result) noexcept;
    };
} // namespace vkBasalt

#endif // CONFIG_HPP_INCLUDED