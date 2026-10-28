#include "logger/utils.hpp"
#include <array>
#include <chrono>
#include <ctime>
#include <iostream>

using namespace logger;

std::string logger::make_timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::array<char, 9> buf{};
    std::strftime(buf.data(), sizeof(buf), "%H:%M:%S", &tm);
    return std::string{buf.data()};
}

const std::map<std::string, SinkType>& logger::get_sink_type() {
    static const std::map<std::string, SinkType> mapping = {
        {"console", SinkType::CONSOLE},
        {"syslog", SinkType::SYSLOG},
        {"file", SinkType::FILE}
    };
    return mapping;
}

const std::map<std::string, FormatterType>& logger::get_formatter_type() {
    static const std::map<std::string, FormatterType> mapping = {
        {"text", FormatterType::TEXT},
        {"json", FormatterType::JSON}
    };
    return mapping;
}

SinkType logger::string_to_sink_type(const std::string& type) {
    const auto& mapping = get_sink_type();
    const auto it = mapping.find(type);
    if (it == mapping.end()) {
        std::cerr << "Unknown sink type '" << type << "'. Falling back to 'console'." << std::endl;
        return SinkType::CONSOLE;
    }
    return it->second;
}

FormatterType logger::string_to_formatter_type(const std::string& type) {
    const auto& mapping = get_formatter_type();
    const auto it = mapping.find(type);
    if (it == mapping.end()) {
        std::cerr << "Unknown formatter type '" << type << "'. Falling back to 'text'." << std::endl;
        return FormatterType::TEXT;
    }
    return it->second;
}
