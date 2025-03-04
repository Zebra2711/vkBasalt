#include "config.hpp"
#include "config_paths.hpp"
#include <array>
#include <cstring>
#include <cstdint>

namespace vkBasalt
{
    Config::Config() noexcept
    {
        const char* const configPaths[] = {
            std::getenv("VKBASALT_CONFIG_FILE"),
            "vkBasalt.conf",
            std::getenv("XDG_CONFIG_HOME"),
            std::getenv("XDG_DATA_HOME"),
            std::getenv("HOME"),
            SYSCONFDIR "/vkBasalt.conf",
            SYSCONFDIR "/vkBasalt/vkBasalt.conf",
            DATADIR "/vkBasalt/vkBasalt.conf"
        };

        std::array<char, 512> pathBuffer{};
        
        // Try custom config first
        if (configPaths[0]) {
            std::ifstream configFile(configPaths[0]);
            if (configFile.good()) {
                //Logger::info("config file: " + std::string(configPaths[0]));
                readConfigFile(configFile);
                return;
            }
        }

        // Try XDG config paths
        if (configPaths[2]) {
            std::snprintf(pathBuffer.data(), pathBuffer.size(), "%s/vkBasalt/vkBasalt.conf", configPaths[2]);
            std::ifstream configFile(pathBuffer.data());
            if (configFile.good()) {
                //Logger::info("config file: " + std::string(pathBuffer.data()));
                readConfigFile(configFile);
                return;
            }
        }

        // Try XDG data home
        if (configPaths[3]) {
            std::snprintf(pathBuffer.data(), pathBuffer.size(), "%s/vkBasalt/vkBasalt.conf", configPaths[3]);
        } else if (configPaths[4]) {
            std::snprintf(pathBuffer.data(), pathBuffer.size(), "%s/.local/share/vkBasalt/vkBasalt.conf", configPaths[4]);
        }
        
        std::ifstream configFile(pathBuffer.data());
        if (configFile.good()) {
            //Logger::info("config file: " + std::string(pathBuffer.data()));
            readConfigFile(configFile);
            return;
        }

        // Try system paths
        for (size_t i = 5; i < 8; ++i) {
            std::ifstream sysConfigFile(configPaths[i]);
            if (sysConfigFile.good()) {
                //Logger::info("config file: " + std::string(configPaths[i]));
                readConfigFile(sysConfigFile);
                return;
            }
        }

        //Logger::err("no good config file");
    }

    void Config::readConfigFile(std::ifstream& stream) noexcept
    {
        std::string line;
        line.reserve(256); // Reasonable line size

        while (std::getline(stream, line))
        {
            if (!line.empty()) {
                readConfigLine(line);
            }
        }
    }

    void Config::readConfigLine(std::string line) noexcept
    {
        if (line.empty() || line[0] == '#') return;

        std::string key;
        std::string value;
        key.reserve(32);
        value.reserve(128);

        size_t pos = 0;
        const size_t len = line.length();

        // Skip leading whitespace
        while (pos < len && (line[pos] == ' ' || line[pos] == '\t')) pos++;
        
        // Read key
        while (pos < len && line[pos] != '=' && line[pos] != ' ' && line[pos] != '\t') {
            key += line[pos++];
        }

        // Find equals sign
        while (pos < len && line[pos] != '=') pos++;
        if (pos >= len) return;
        pos++; // Skip equals

        // Skip whitespace after equals
        while (pos < len && (line[pos] == ' ' || line[pos] == '\t')) pos++;

        // Read value
        bool inQuotes = false;
        for (; pos < len; pos++) {
            char c = line[pos];
            if (c == '"') {
                inQuotes = !inQuotes;
                continue;
            }
            if (!inQuotes && c == '#') break;
            if (!inQuotes && (c == ' ' || c == '\t')) {
                if (value.empty() || value.back() != ' ') value += ' ';
            } else {
                value += c;
            }
        }

        // Trim trailing whitespace from value
        while (!value.empty() && value.back() == ' ') {
            value.pop_back();
        }

        if (!key.empty() && !value.empty()) {
            options[key] = std::move(value);
        }
    }

    void Config::parseOption(const std::string& option, int32_t& result) noexcept
    {
        auto found = options.find(option);
        if (found != options.end()) {
            char* end;
            long val = std::strtol(found->second.c_str(), &end, 10);
            if (*end == '\0' && val >= INT32_MIN && val <= INT32_MAX) {
                result = static_cast<int32_t>(val);
            } else {
                //Logger::warn("invalid int32_t value for: " + option);
            }
        }
    }

    void Config::parseOption(const std::string& option, float& result) noexcept
    {
        auto found = options.find(option);
        if (found != options.end()) {
            char* end;
            float val = std::strtof(found->second.c_str(), &end);
            if (*end == '\0' || (*end == 'f' && end[1] == '\0')) {
                result = val;
            } else {
                //Logger::warn("invalid float value for: " + option);
            }
        }
    }

    void Config::parseOption(const std::string& option, bool& result) noexcept
    {
        auto found = options.find(option);
        if (found != options.end()) {
            const std::string& val = found->second;
            if (val == "1" || val == "true" || val == "True") {
                result = true;
            } else if (val == "0" || val == "false" || val == "False") {
                result = false;
            } else {
                //Logger::warn("invalid bool value for: " + option);
            }
        }
    }

    void Config::parseOption(const std::string& option, std::string& result) noexcept
    {
        auto found = options.find(option);
        if (found != options.end())
        {
            result = found->second;
        }
    }

    void Config::parseOption(const std::string& option, std::vector<std::string>& result) noexcept
    {
        auto found = options.find(option);
        if (found == options.end())
        {
            return;
        }

        const std::string& value = found->second;
        size_t start = value.find('{');
        size_t end = value.find('}');

        if (start == std::string::npos || end == std::string::npos || start >= end)
        {
            return;
        }

        result.clear();
        result.reserve(8);

        size_t pos = start + 1;
        while (pos < end)
        {
            // Skip whitespace
            while (pos < end && (value[pos] == ' ' || value[pos] == '\n' || 
                   value[pos] == '\t' || value[pos] == '\r'))
            {
                ++pos;
            }

            if (pos >= end) break;

            size_t nextComma = value.find(',', pos);
            if (nextComma == std::string::npos || nextComma > end)
            {
                nextComma = end;
            }

            // Find effect name end
            size_t effectEnd = nextComma;
            while (effectEnd > pos && (value[effectEnd-1] == ' ' || 
                   value[effectEnd-1] == '\n' || value[effectEnd-1] == '\t' || 
                   value[effectEnd-1] == '\r'))
            {
                --effectEnd;
            }

            if (effectEnd > pos)
            {
                result.emplace_back(value.substr(pos, effectEnd - pos));
            }

            pos = nextComma + 1;
        }
    }
} // namespace vkBasalt
