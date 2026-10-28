#include "logger/formatters/text_formatter.hpp"
#include <sstream>

using namespace logger;

std::string TextFormatter::format(const Record& r) {
    std::ostringstream oss;
    oss << r.app_name << ' ' << r.timestamp << " [" << r.tag << "] "
        << to_string(r.level) << ": " << r.message
        << ", SOURCE: " << r.src.file << ':' << r.src.line;
    return oss.str();
}