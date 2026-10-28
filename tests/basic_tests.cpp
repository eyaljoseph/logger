#include "logger/logger.hpp"
#include "logger/config.hpp"
#include "logger/tagged_logger.hpp"
#include <cassert>

using namespace logger;

int main() {
    Logger::init(Config{"config.json"});
    TaggedLogger t("mod");
    t.info(LOG_SRC, "value {}", 123);
    assert(true);
    return 0;
}
