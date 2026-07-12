cmake_minimum_required(VERSION 3.20)
project(pjh_cli LANGUAGES CXX)

add_subdirectory(third_party/pjh_result EXCLUDE_FROM_ALL)

add_library(pjh_cli)
target_sources(pjh_cli
    PRIVATE
    src/command.cpp
    src/app.cpp
    src/help.cpp
)
target_include_directories(pjh_cli
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(pjh_cli PUBLIC pjh_result)
target_compile_features(pjh_cli PUBLIC cxx_std_20)

option(PJH_CLI_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

if(PJH_CLI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
