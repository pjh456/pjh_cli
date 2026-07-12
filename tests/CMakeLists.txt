add_executable(test_fixed_string test_fixed_string.cpp)
target_link_libraries(test_fixed_string PRIVATE pjh_cli)

add_executable(test_converter test_converter.cpp)
target_link_libraries(test_converter PRIVATE pjh_cli)

add_executable(test_command test_command.cpp)
target_link_libraries(test_command PRIVATE pjh_cli)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE pjh_cli)

add_executable(test_matcher test_matcher.cpp)
target_link_libraries(test_matcher PRIVATE pjh_cli)

add_test(NAME test_fixed_string COMMAND test_fixed_string)
add_test(NAME test_converter COMMAND test_converter)
add_test(NAME test_command COMMAND test_command)
add_test(NAME test_parser COMMAND test_parser)
add_test(NAME test_matcher COMMAND test_matcher)
