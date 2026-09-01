cmake_minimum_required(VERSION 3.20)
project(pjh_cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT TARGET pjh_result)
    add_subdirectory(thirdparty/pjh_result EXCLUDE_FROM_ALL)
endif()

add_library(pjh_cli)
add_library(pjh::cli ALIAS pjh_cli)
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
