#pragma once

#define LOG_SRC ::logger::SourceLocation{__FILE__, __LINE__, __func__}

namespace logger {

struct SourceLocation {
    const char* file {""};
    int         line {0};
    const char* func {""};
};

} // namespace logger
