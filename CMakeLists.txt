cmake_minimum_required(VERSION 3.20)
project(pjh_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

find_package(pjh_result 0.1.0 CONFIG QUIET)
if(NOT TARGET pjh_result)
    # pjh_result has no release tags yet; this SHA is the 0.1.0 dependency
    # (upstream project version 0.1.0). Bump deliberately; see README.
    FetchContent_Declare(pjh_result
        GIT_REPOSITORY https://github.com/pjh456/pjh_result.git
        GIT_TAG 4e2d37fa5a84ca9c70f3919c1fd7ce21ea4f22c5)
    FetchContent_MakeAvailable(pjh_result)
endif()

add_library(pjh_cli)
add_library(pjh::cli ALIAS pjh_cli)
set_target_properties(pjh_cli PROPERTIES EXPORT_NAME cli)
target_sources(pjh_cli
    PRIVATE
    src/command.cpp
    src/app.cpp
    src/parser.cpp
    src/matched_path_resolver.cpp
    src/option_consumer.cpp
    src/subcommand_resolver.cpp
    src/parse_finalizer.cpp
    src/value_writer.cpp
    src/help.cpp
    src/matcher.cpp
    src/console.cpp
    src/console_output.cpp
    src/console/query.cpp
    src/console/help_navigator.cpp
    src/console/in_memory_history.cpp
    src/console/ring_buffer_history.cpp
    src/console/line_editor.cpp
    src/console/tty_terminal.cpp
    src/hint.cpp
)
target_include_directories(pjh_cli
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(pjh_cli PUBLIC pjh_result)


option(PJH_CLI_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

option(PJH_CLI_BUILD_EXAMPLES "Build example programs" OFF)

if(PJH_CLI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PJH_CLI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

set(pjh_cli_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/pjh_cli)

install(TARGETS pjh_cli EXPORT pjh_cliTargets)
install(EXPORT pjh_cliTargets
    FILE pjh_cliTargets.cmake
    NAMESPACE pjh::
    DESTINATION ${pjh_cli_INSTALL_CMAKEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(cmake/pjh_cliConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/pjh_cliConfig.cmake
    INSTALL_DESTINATION ${pjh_cli_INSTALL_CMAKEDIR})
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/pjh_cliConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/pjh_cliConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/pjh_cliConfigVersion.cmake
    DESTINATION ${pjh_cli_INSTALL_CMAKEDIR})
