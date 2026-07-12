add_executable(test_tokenizer test_tokenizer.cpp)
target_link_libraries(test_tokenizer PRIVATE pjh_cli)

add_executable(test_converter test_converter.cpp)
target_link_libraries(test_converter PRIVATE pjh_cli)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE pjh_cli)

add_executable(test_help test_help.cpp)
target_link_libraries(test_help PRIVATE pjh_cli)

add_test(NAME test_tokenizer COMMAND test_tokenizer)
add_test(NAME test_converter COMMAND test_converter)
add_test(NAME test_parser COMMAND test_parser)
add_test(NAME test_help COMMAND test_help)
