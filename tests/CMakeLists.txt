set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/doctest ${CMAKE_BINARY_DIR}/thirdparty/doctest EXCLUDE_FROM_ALL)

set(PJH_CLI_TEST_SOURCES
    test_main.cpp
    test_fixed_string.cpp
    test_converter.cpp
    test_command.cpp
    test_matcher.cpp
    parser/test_options.cpp
    parser/test_flags.cpp
    parser/test_positional.cpp
    parser/test_subcommands.cpp
    parser/test_dash.cpp
    parser/test_errors.cpp
    console/test_basics.cpp
    console/test_process_line_help.cpp
    console/test_process_line_parse.cpp
    console/test_free_functions.cpp
)

add_executable(pjh_cli_tests ${PJH_CLI_TEST_SOURCES})
target_link_libraries(pjh_cli_tests PRIVATE pjh_cli doctest)

include(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/doctest/scripts/cmake/doctest.cmake)
doctest_discover_tests(pjh_cli_tests)
