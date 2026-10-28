#include <logger/logger.hpp>
#include <filesystem>
#include <string>

namespace lg = logger;

int main(int argc, char** argv) {
    lg::Config cfg{"../config.multi.json"};
    lg::Logger::init(cfg);

    lg::TaggedLogger ingest("ingest");

    ingest.info(LOG_SRC, "ingest step {}", 1);
    // try {
    //     throw std::runtime_error("Blah");
    // }
    // catch(std::exception& e) {
    //     // THROW_ERROR("noder {}", 1);
    //     ingest.throw_error<std::runtime_error>(LOG_SRC, "noder check {}", 32);
    // }
    //
    // // lg::Logger::instance().set_formatter(std::make_unique<lg::JsonFormatter>());
    // ingest.notice(LOG_SRC, "now json format!");
    //
    // TAG_DEBUG(ingest ,"asdada {}", 1);
    // NOTICE("asdada {}", 1);

    return 0;
}
