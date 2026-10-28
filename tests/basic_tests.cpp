#include "logger/logger.hpp"
#include "logger/config.hpp"
#include "logger/tagged_logger.hpp"
#include "logger/sinks/file_sink.hpp"
#include "logger/formatters/text_formatter.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace logger;
namespace fs = std::filesystem;

namespace {
int failures = 0;

// assert() compiles out under NDEBUG, so Release test builds would check nothing.
#define CHECK(cond)                                                                        \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: " #cond << '\n'; \
            ++failures;                                                                    \
        }                                                                                  \
    } while (0)

fs::path make_temp_dir(const std::string& name) {
    const auto dir = fs::temp_directory_path() / ("logger_tests_" + name);
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

fs::path write_file(const fs::path& path, const std::string& content) {
    std::ofstream(path) << content;
    return path;
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void check_config_falls_back(const std::string& name, const std::string& content) {
    const auto path = write_file(make_temp_dir(name) / "config.json", content);
    try {
        const Config cfg{path.string()};
        CHECK(cfg.sink == SinkType::CONSOLE);
        CHECK(cfg.format == FormatterType::TEXT);
        CHECK(cfg.app_name == "Logger");
    } catch (const std::exception& e) {
        std::cerr << name << ": Config threw: " << e.what() << '\n';
        ++failures;
    }
}

// Runs first: the singleton must not be created twice when several threads log before init().
void test_concurrent_first_use() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([] { TaggedLogger("first_use").info(LOG_SRC, "hello"); });
    }
    for (auto& t : threads) {
        t.join();
    }
}

void test_invalid_config_uses_defaults() {
    check_config_falls_back("invalid_json", "{ not json");
    check_config_falls_back("not_object", "[1, 2]");
    check_config_falls_back("wrong_types",
                            R"({"sink": 5, "format": [], "app_name": null, "file_path": 1})");
}

void test_wrong_typed_value_keeps_other_values() {
    const auto path = write_file(make_temp_dir("partial") / "config.json",
                                 R"({"app_name": "Partial", "sink": 5,
                                     "targets": [{"sink": "file", "format": 7}]})");
    try {
        const Config cfg{path.string()};
        CHECK(cfg.app_name == "Partial");
        CHECK(cfg.sink == SinkType::CONSOLE);
        CHECK(cfg.targets.size() == 1);
        CHECK(!cfg.targets.empty() && cfg.targets[0].sink == SinkType::FILE);
        CHECK(!cfg.targets.empty() && cfg.targets[0].format == FormatterType::TEXT);
    } catch (const std::exception& e) {
        std::cerr << "partial: Config threw: " << e.what() << '\n';
        ++failures;
    }
}

void test_relative_file_path_resolves_next_to_config() {
    const auto dir = make_temp_dir("relative_path");
    const auto path = write_file(dir / "config.json",
                                 R"({"file_path": "out.log",
                                     "targets": [{"sink": "file", "file_path": "logs/t.log"},
                                                 {"sink": "file", "file_path": "/abs/a.log"}]})");
    const Config cfg{path.string()};
    CHECK(fs::path(cfg.file_path) == (dir / "out.log").lexically_normal());
    CHECK(cfg.targets.size() == 2);
    if (cfg.targets.size() == 2) {
        CHECK(fs::path(cfg.targets[0].file_path) == (dir / "logs/t.log").lexically_normal());
        CHECK(cfg.targets[1].file_path == "/abs/a.log");
    }
}

void test_file_sink_writes_next_to_config() {
    const auto dir = make_temp_dir("file_sink");
    const auto path = write_file(dir / "config.json",
                                 R"({"app_name": "FileTest", "sink": "file", "file_path": "app.log"})");
    Logger::init(Config{path.string()});
    TaggedLogger("mod").info(LOG_SRC, "value {}", 123);

    const auto content = read_file(dir / "app.log");
    CHECK(content.find("FileTest") != std::string::npos);
    CHECK(content.find("[mod] INFO: value 123") != std::string::npos);
}

// init() used to destroy the logger that other threads were still writing through.
void test_reinit_while_logging() {
    const auto dir = make_temp_dir("reinit");
    const auto path = write_file(dir / "config.json", R"({"sink": "file", "file_path": "reinit.log"})");
    const Config cfg{path.string()};
    Logger::init(cfg);

    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&stop] {
            TaggedLogger worker("worker");
            while (!stop) {
                worker.info(LOG_SRC, "tick");
            }
        });
    }
    for (int i = 0; i < 200; ++i) {
        Logger::init(cfg);
    }
    stop = true;
    for (auto& t : threads) {
        t.join();
    }
    CHECK(read_file(dir / "reinit.log").find("[worker] INFO: tick") != std::string::npos);
}
}  // namespace

int main() {
    test_concurrent_first_use();
    test_invalid_config_uses_defaults();
    test_wrong_typed_value_keeps_other_values();
    test_relative_file_path_resolves_next_to_config();
    test_file_sink_writes_next_to_config();
    test_reinit_while_logging();

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All checks passed\n";
    return 0;
}
