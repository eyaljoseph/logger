#include "logger/base_logger.hpp"
#include "logger/concepts.hpp"
#include "logger/general_logs.hpp"
#include "logger/sinks/console_sink.hpp"
#include "logger/sinks/syslog_sink.hpp"
#include "logger/sinks/file_sink.hpp"
#include "logger/formatters/text_formatter.hpp"
#include "logger/formatters/json_formatter.hpp"
#include <atomic>
#include <iostream>

using namespace logger;

namespace {
// The global logger lives for the whole process: init() swaps its targets instead of
// replacing the object, so references held by other threads stay valid.
std::mutex g_init_mutex;
std::atomic<bool> g_initialized{false};

Logger& global_logger() {
    static Logger instance{{}, "Logger"};
    return instance;
}

FormatterPtr make_formatter(const FormatterType type) {
    switch (type) {
        case FormatterType::JSON:
            return std::make_unique<JsonFormatter>();
        default:
            return std::make_unique<TextFormatter>();
    }
}

SinkPtr make_sink(const SinkType type, const std::string& app_name, const std::string& file_path) {
    switch (type) {
        case SinkType::SYSLOG:
#ifdef LOGGERLIB_HAS_SYSLOG
            return std::make_unique<SyslogSink>(app_name);
#else
            std::cerr << "Syslog support is not built in. Falling back to console sink." << std::endl;
            return std::make_unique<ConsoleSink>(app_name);
#endif
        case SinkType::FILE:
            return std::make_unique<FileSink>(file_path);
        default:
            return std::make_unique<ConsoleSink>(app_name);
    }
}

Logger::Target make_target(SinkType sink_type,
                           FormatterType formatter_type,
                           const std::string& app_name,
                           const std::string& file_path) {
    return Logger::Target{make_sink(sink_type, app_name, file_path), make_formatter(formatter_type)};
}

std::vector<Logger::Target> make_targets_from_config(const Config& cfg) {
    std::vector<Logger::Target> targets;
    if (!cfg.targets.empty()) {
        targets.reserve(cfg.targets.size());
        for (const auto& target : cfg.targets) {
            targets.emplace_back(make_target(target.sink, target.format, cfg.app_name, target.file_path));
        }
        return targets;
    }
    targets.emplace_back(make_target(cfg.sink, cfg.format, cfg.app_name, cfg.file_path));
    return targets;
}
}

Logger::Logger(std::vector<Target> targets, std::string app_name)
    : targets_(std::move(targets)), app_name_(std::move(app_name)) {}

void Logger::init(const Config& cfg) {
    init(cfg, make_targets_from_config(cfg));
}

void Logger::init(const Config& cfg, FormatterPtr formatter) {
    std::vector<Target> targets;
    targets.emplace_back(Target{make_sink(cfg.sink, cfg.app_name, cfg.file_path), std::move(formatter)});
    init(cfg, std::move(targets));
}

void Logger::init(const Config& cfg, SinkPtr sink) {
    std::vector<Target> targets;
    targets.emplace_back(Target{std::move(sink), make_formatter(cfg.format)});
    init(cfg, std::move(targets));
}

void Logger::init(const Config& cfg, SinkPtr sink, FormatterPtr formatter) {
    std::vector<Target> targets;
    targets.emplace_back(Target{std::move(sink), std::move(formatter)});
    init(cfg, std::move(targets));
}

void Logger::init(const Config& cfg, std::vector<Target> targets) {
    std::lock_guard lock(g_init_mutex);
    global_logger().reset(std::move(targets), cfg.app_name);
    g_initialized.store(true, std::memory_order_release);
}

Logger& Logger::instance() {
    Logger& global = global_logger();
    if (!g_initialized.load(std::memory_order_acquire)) {
        bool defaulted = false;
        {
            std::lock_guard lock(g_init_mutex);
            if (!g_initialized.load(std::memory_order_relaxed)) {
                std::vector<Target> targets;
                targets.emplace_back(make_target(SinkType::CONSOLE, FormatterType::TEXT, "Logger", "logger.log"));
                global.reset(std::move(targets), "Logger");
                g_initialized.store(true, std::memory_order_release);
                defaulted = true;
            }
        }
        // Logged outside the lock: WARNING calls back into instance().
        if (defaulted) {
            WARNING("Logger not initialized. Defaulting to console sink and text format.");
        }
    }
    return global;
}

void Logger::reset(std::vector<Target> targets, std::string app_name) {
    std::lock_guard<std::mutex> lock(m_);
    targets_ = std::move(targets);
    app_name_ = std::move(app_name);
}

void Logger::set_formatter(FormatterPtr fmt) {
    std::lock_guard<std::mutex> lock(m_);
    if (targets_.empty()) {
        return;
    }
    targets_.front().formatter = std::move(fmt);
}

void Logger::set_sink(SinkPtr sink) {
    std::lock_guard<std::mutex> lock(m_);
    if (targets_.empty()) {
        return;
    }
    targets_.front().sink = std::move(sink);
}

void Logger::set_targets(std::vector<Target> targets) {
    std::lock_guard<std::mutex> lock(m_);
    targets_ = std::move(targets);
}

void Logger::add_target(SinkPtr sink, FormatterPtr formatter) {
    std::lock_guard<std::mutex> lock(m_);
    targets_.push_back(Target{std::move(sink), std::move(formatter)});
}
