#pragma once

#include "types.h"
#include <string>
#include <map>
#include <variant>
#include <mutex>
#include <optional>
#include <fstream>
#include <sstream>

namespace zigbee_mesh::core {

using ConfigValue = std::variant<
    std::string,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    double,
    bool
>;

struct ConfigSection {
    std::string name;
    std::map<std::string, ConfigValue> values;
};

class Config {
public:
    Config() = default;

    static Config& global() {
        static Config instance;
        return instance;
    }

    bool loadFromFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ifstream file(path);
        if (!file.is_open()) return false;

        std::string line;
        std::string current_section;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            if (line[0] == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
                continue;
            }

            auto eq_pos = line.find('=');
            if (eq_pos == std::string::npos) continue;

            std::string key = trim(line.substr(0, eq_pos));
            std::string value = trim(line.substr(eq_pos + 1));

            sections_[current_section][key] = parseValue(value);
        }
        file_path_ = path;
        return true;
    }

    bool saveToFile(const std::string& path = "") const {
        std::lock_guard<std::mutex> lock(mutex_);
        const std::string& target = path.empty() ? file_path_ : path;
        if (target.empty()) return false;

        std::ofstream file(target);
        if (!file.is_open()) return false;

        for (const auto& [section_name, section] : sections_) {
            if (!section_name.empty()) {
                file << "[" << section_name << "]\n";
            }
            for (const auto& [key, value] : section.values) {
                file << key << " = " << valueToString(value) << "\n";
            }
            file << "\n";
        }
        return true;
    }

    template <typename T>
    std::optional<T> get(const std::string& section, const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto sec_it = sections_.find(section);
        if (sec_it == sections_.end()) return std::nullopt;
        auto key_it = sec_it->second.find(key);
        if (key_it == sec_it->second.end()) return std::nullopt;

        if (auto* val = std::get_if<T>(&key_it->second)) {
            return *val;
        }
        return std::nullopt;
    }

    template <typename T>
    T get(const std::string& section, const std::string& key, T default_val) const {
        auto result = get<T>(section, key);
        return result.value_or(default_val);
    }

    template <typename T>
    void set(const std::string& section, const std::string& key, T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        sections_[section][key] = ConfigValue(std::move(value));
    }

    void merge(const Config& other) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [section_name, section] : other.sections_) {
            for (const auto& [key, value] : section.values) {
                sections_[section_name][key] = value;
            }
        }
    }

    bool hasSection(const std::string& section) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sections_.find(section) != sections_.end();
    }

    bool hasKey(const std::string& section, const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto sec_it = sections_.find(section);
        if (sec_it == sections_.end()) return false;
        return sec_it->second.find(key) != sec_it->second.end();
    }

    std::map<std::string, ConfigSection> getAllSections() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sections_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        sections_.clear();
    }

private:
    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\r\n\"");
        auto end = s.find_last_not_of(" \t\r\n\"");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }

    static ConfigValue parseValue(const std::string& str) {
        if (str == "true") return ConfigValue(true);
        if (str == "false") return ConfigValue(false);

        if (str.find('.') != std::string::npos) {
            try {
                return ConfigValue(std::stod(str));
            } catch (...) {}
        }

        try {
            return ConfigValue(static_cast<int64_t>(std::stoll(str)));
        } catch (...) {}

        return ConfigValue(str);
    }

    static std::string valueToString(const ConfigValue& val) {
        return std::visit([](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else {
                return std::to_string(v);
            }
        }, val);
    }

    mutable std::mutex mutex_;
    std::map<std::string, ConfigSection> sections_;
    std::string file_path_;
};

} // namespace zigbee_mesh::core
