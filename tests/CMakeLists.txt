set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/doctest ${CMAKE_BINARY_DIR}/thirdparty/doctest EXCLUDE_FROM_ALL)

set(PJH_CLI_TEST_SOURCES
    test_main.cpp
    test_fixed_string.cpp
    test_converter.cpp
    test_command.cpp
    test_matcher.cpp
    test_parse_context.cpp
    option/test_bool_option.cpp
    option/test_count_option.cpp
    option/test_env_option.cpp
    option/test_float_option.cpp
    option/test_int_option.cpp
    option/test_path_option.cpp
    option/test_str_option.cpp
    option/test_enum_option.cpp
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
    test_error.cpp
    test_string_utils.cpp
    test_command_utils.cpp
    test_hint.cpp
    test_parse_concurrent.cpp
)

add_executable(pjh_cli_tests ${PJH_CLI_TEST_SOURCES})
target_link_libraries(pjh_cli_tests PRIVATE pjh_cli doctest)
find_package(Threads REQUIRED)
target_link_libraries(pjh_cli_tests PRIVATE Threads::Threads)

include(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/doctest/scripts/cmake/doctest.cmake)
doctest_discover_tests(pjh_cli_tests)
