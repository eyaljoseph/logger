#pragma once
#include "sinks/sink.hpp"
#include "formatters/formatter.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>

namespace logger {
    /**
     * @brief Configuration class for logger settings
     *
     * The Config class manages all logger configuration parameters that can be loaded
     * from a JSON file. It provides default values when the configuration file is
     * missing or invalid, ensuring the logger always has valid settings.
     *
     * Configuration options include:
     * - Sink type (console, file, etc.)
     * - Formatter type (text, JSON, etc.)
     * - Application name for log identification
     *
     * The class automatically handles file I/O errors and provides sensible defaults
     * when configuration values are missing or invalid.
     */
    class Config {
    public:
        struct TargetConfig {
            SinkType sink{SinkType::CONSOLE};
            FormatterType format{FormatterType::TEXT};
            std::string file_path{"logger.log"};
        };
        /**
         * @brief Logger sink type enumeration
         *
         * Defines the destination where log messages will be written.
         * Supported sinks include console output and file output.
         */
        SinkType sink{SinkType::CONSOLE};

        /**
         * @brief Logger formatter type enumeration
         *
         * Defines the format used for logging messages.
         * Supported formats include text and JSON formatting.
         */
        FormatterType format{FormatterType::TEXT};

        /**
         * @brief Application name for log identification
         *
         * Used to identify the source application in log entries.
         * This name appears in formatted log output to help distinguish
         * between different applications or services.
         */
        std::string app_name{"Logger"};
        std::string file_path{"logger.log"};
        std::vector<TargetConfig> targets;

        /**
         * @brief Construct a Config object from JSON file
         *
         * Loads configuration settings from a JSON file. If the file cannot be opened
         * or parsed, default values are used instead:
         * - Sink: Console output
         * - Format: Text formatting
         * - App name: "Logger"
         *
         * A value with the wrong type falls back to its default; the other values still apply.
         * Relative file paths are resolved against the config file's directory.
         *
         * @param json_path Path to the JSON configuration file
         */
        explicit Config(const std::string &json_path) {
            const std::filesystem::path config_dir = std::filesystem::path(json_path).parent_path();
            const auto resolve_path = [&config_dir](const std::string &raw_path) {
                std::string expanded_path = raw_path;
                if (!raw_path.empty() && raw_path[0] == '~') {
                    const char* home = std::getenv("HOME");
                    if (home != nullptr) {
                        if (raw_path.size() == 1) {
                            expanded_path = home;
                        } else if (raw_path[1] == '/') {
                            expanded_path = std::string(home) + raw_path.substr(1);
                        }
                    }
                }

                std::filesystem::path path{expanded_path};
                if (path.is_relative()) {
                    path = config_dir / path;
                }
                return path.lexically_normal().string();
            };

            const auto get_string = [](const nlohmann::json &obj, const char *key,
                                       const std::string &fallback) -> std::string {
                const auto it = obj.find(key);
                if (it == obj.end()) {
                    return fallback;
                }
                if (!it->is_string()) {
                    std::cerr << "Logger config value '" << key << "' must be a string. Using '"
                              << fallback << "'." << std::endl;
                    return fallback;
                }
                return it->get<std::string>();
            };

            std::ifstream file(json_path);
            if (!file.is_open()) {
                std::cerr << "Failed to open logger config file: " << json_path << std::endl;
                std::cout << "Set's logger to default settings: (sink = console, format = text, app_name = Logger)" <<
                        std::endl;
                return;
            }

            const nlohmann::json j = nlohmann::json::parse(file, nullptr, false);
            if (j.is_discarded() || !j.is_object()) {
                std::cerr << "Invalid logger config file: " << json_path
                          << ". Using default settings (sink = console, format = text, app_name = Logger)"
                          << std::endl;
                return;
            }

            sink = string_to_sink_type(get_string(j, "sink", "console"));
            format = string_to_formatter_type(get_string(j, "format", "text"));
            app_name = get_string(j, "app_name", "Logger");
            file_path = resolve_path(get_string(j, "file_path", "logger.log"));

            const auto targets_it = j.find("targets");
            if (targets_it != j.end() && targets_it->is_array()) {
                for (const auto &target : *targets_it) {
                    if (!target.is_object()) {
                        continue;
                    }
                    TargetConfig cfg;
                    cfg.sink = string_to_sink_type(get_string(target, "sink", "console"));
                    cfg.format = string_to_formatter_type(get_string(target, "format", "text"));
                    cfg.file_path = resolve_path(get_string(target, "file_path", "logger.log"));
                    targets.emplace_back(std::move(cfg));
                }
            }
        }
    };
} // namespace logger
