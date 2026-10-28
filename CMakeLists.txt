cmake_minimum_required(VERSION 3.16)
project(logger LANGUAGES CXX)

option(LOGGERLIB_ENABLE_SYSLOG "Build with syslog support" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)
find_package(nlohmann_json QUIET)

include(FetchContent)

if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(
          nlohmann_json
          GIT_REPOSITORY "https://github.com/nlohmann/json"
          GIT_TAG v3.12.0
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(logger
        src/text_formatter.cpp
        src/json_formatter.cpp
        src/console_sink.cpp
        src/syslog_sink.cpp
        src/file_sink.cpp
        src/logger.cpp
        src/utils.cpp)

target_include_directories(logger
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(logger PUBLIC fmt::fmt nlohmann_json::nlohmann_json)

if(LOGGERLIB_ENABLE_SYSLOG)
  if(UNIX)
    target_compile_definitions(logger PUBLIC LOGGERLIB_HAS_SYSLOG=1)
  endif()
endif()

add_executable(logger_demo examples/demo_main.cpp)
target_link_libraries(logger_demo PRIVATE logger)

option(LOGGER_BUILD_TESTS "Build logger tests" ON)
include(CTest)
if(LOGGER_BUILD_TESTS)
  add_executable(logger_basic_tests tests/basic_tests.cpp)
  target_link_libraries(logger_basic_tests PRIVATE logger)
  add_test(NAME logger_basic_tests COMMAND logger_basic_tests)
  set_tests_properties(logger_basic_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

######################################################################################
###                               library installation                             ###
######################################################################################

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/logger DESTINATION include)

install(TARGETS logger
        EXPORT LoggerTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

install(EXPORT LoggerTargets
        FILE loggerConfig.cmake
        NAMESPACE logger::
        DESTINATION lib/cmake/logger
)
