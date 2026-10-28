# Logger
A lightweight and flexible C++ logging library with **tagged loggers**, **configurable sinks**, and **JSON/Text output formats**, built on top of [`fmt`](https://github.com/fmtlib/fmt) and [`nlohmann/json`](https://github.com/nlohmann/json).

---

## ✨ Features
- Simple configuration via JSON file
- Supports **console**, **syslog** and **file** sinks
- Supports **text** and **JSON** formatting
- Tagged loggers for module-specific logging
- Multiple log levels: `debug`, `info`, `notice`, `warning`, `error`, `critical`
- Customizable formatters with `Logger::instance().set_formatter(...)`

---

## 📦 Dependencies
Logger requires the following libraries:

# fmt
```bash
  sudo apt install libfmt-dev
```

# nlohmann_json
- [nlohmann/json](https://github.com/nlohmann/json)

Both can be installed via package managers (`apt`, `vcpkg`, `conan`) or manually.

---

## ⚙️ Building & Installing

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
sudo cmake --build build --target install -- -j$(nproc)
```

Syslog support is on by default on Unix. To build without it:

```bash
cmake -B build -S . -DLOGGERLIB_ENABLE_SYSLOG=OFF
```

A `syslog` sink in the config then falls back to the console sink.

### ✅ Tests

Tests are built by default. You can disable them with:

```bash
cmake -B build -S . -DLOGGER_BUILD_TESTS=OFF
```

To run tests:

```bash
cmake --build build -j
ctest --test-dir build
```

---

## 🔗 Using Logger in your project

In your `CMakeLists.txt`:

```cmake
find_package(logger REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE logger::logger)
```

Include the logger:

```cpp
#include <logger/logger.hpp>
```

---

## 📝 Configuration

Logger is initialized from a JSON config file that defines:
- `app_name` – name of your application
- `format` – output format (`text` or `json`) (`file` is a sink, not a formatter)
- `sink` – logging sink (`console`, `syslog`, or `file`)
- `file_path` – file path for the `file` sink (default: `logger.log`, resolved relative to the config file)

**Example config.json:**
```json
{
  "app_name": "MyApp",
  "format": "text",
  "sink": "file",
  "file_path": "test.txt"
}
```

### Multiple targets (sink + formatter pairs)

You can send different formats to different sinks by defining `targets`:

```json
{
  "app_name": "MyApp",
  "targets": [
    { "sink": "syslog", "format": "json" },
    { "sink": "console", "format": "text" }
  ]
}
```

Then initialize the logger using these targets:

```cpp
#include <logger/logger.hpp>

namespace lg = logger;

int main() {
    lg::Config cfg{"config.json"};
    std::vector<lg::Logger::Target> targets;
    targets.emplace_back(lg::Logger::Target{
        std::make_unique<lg::SyslogSink>(cfg.app_name),
        std::make_unique<lg::JsonFormatter>()
    });
    targets.emplace_back(lg::Logger::Target{
        std::make_unique<lg::ConsoleSink>(cfg.app_name),
        std::make_unique<lg::TextFormatter>()
    });
    lg::Logger::init(cfg, std::move(targets));
}
```

---

## 📝 Rsyslog and Logrotate installation

Run ```./install_logrotate.sh <your app's name>```

just make sure that this is the same name you put in the config.json file

---

## 🚀 Example Usage

```cpp
#include <logger/logger.hpp>

namespace lg = logger;

int main() {
    lg::Config cfg{"config.json"};
    lg::Logger::init(cfg);

    // Create a tagged logger
    lg::TaggedLogger var_name("tag");

    // Log messages
    var_name.info(LOG_SRC, "hi {}", 1);

    // Change formatter at runtime
    lg::Logger::instance().set_formatter(std::make_unique<lg::JsonFormatter>());
}
```

### Example Output (text mode):
```
Logger 10:54:14 [tag] INFO: hi 1, SOURCE: /home/.../main.cpp:8
```

### Example Output (JSON mode):
```json
{
  "app_name": "MyApp",
  "level": "INFO",
  "message": "hi 1",
  "tag": "tag",
  "time": "10:54:14"
}
```

---

## 📖 Log Functions
- `debug`
- `info`
- `notice`
- `warning`
- `error`
- `critical`
- `throw_error`

---
