cmake_minimum_required(VERSION 3.20)
project(pjh_cli VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

find_package(pjh_result 0.2.0 CONFIG QUIET)
if(NOT TARGET pjh_result)
    # Pin to the v0.2.0 release tag (adds Context/Diagnostic). Bump deliberately; see README.
    FetchContent_Declare(pjh_result
        GIT_REPOSITORY https://github.com/pjh456/pjh_result.git
        GIT_TAG v0.2.0)
    FetchContent_MakeAvailable(pjh_result)
endif()

# ── pjh_platform: PRIVATE compile dependency; never leaks into a public header ──
# Resolution order:
#   1. a target already defined by the parent (vendored add_subdirectory),
#   2. an installed package (find_package CONFIG),
#   3. top-level only: FetchContent the pinned v0.2.0 tag.
# A subproject never fetches, so a wrong parent order fails here with an
# actionable message instead of a duplicate-target error later.
if(NOT TARGET pjh_platform AND NOT TARGET pjh::platform)
    find_package(pjh_platform 0.2.0 CONFIG QUIET)
endif()
if(NOT TARGET pjh_platform AND NOT TARGET pjh::platform)
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        FetchContent_Declare(pjh_platform
            GIT_REPOSITORY https://github.com/pjh456/pjh_platform.git
            GIT_TAG v0.2.0)
        FetchContent_MakeAvailable(pjh_platform)
    else()
        message(FATAL_ERROR
            "pjh_cli needs the pjh_platform target before configuration; "
            "add_subdirectory(<pjh_platform>) before add_subdirectory(pjh_cli) "
            "(see README 'Dependencies').")
    endif()
endif()

add_library(pjh_cli)
add_library(pjh::cli ALIAS pjh_cli)
set_target_properties(pjh_cli PROPERTIES EXPORT_NAME cli)
target_sources(pjh_cli
    PRIVATE
    src/command.cpp
    src/error.cpp
    src/app.cpp
    src/env.cpp
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
    src/console/file_history.cpp
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
# pjh_platform is resolved above, but a static library's PRIVATE dependency is
# still recorded in the install export as $<LINK_ONLY:pjh_platform>. Its own
# install(EXPORT) is skipped when it is a subproject (e.g. FetchContent), which
# would make the pjh_cli export reference a target that is not in any export
# set and break `generate`. Keep the link to the build interface only; the
# installed consumer re-adds it via pjh_cliConfig.cmake after
# find_dependency(pjh_platform 0.2.0).
target_link_libraries(pjh_cli PRIVATE $<BUILD_INTERFACE:pjh_platform>)


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
