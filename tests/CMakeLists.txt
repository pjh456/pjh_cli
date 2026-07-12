add_executable(test_fixed_string test_fixed_string.cpp)
target_link_libraries(test_fixed_string PRIVATE pjh_cli)

add_executable(test_converter test_converter.cpp)
target_link_libraries(test_converter PRIVATE pjh_cli)

add_test(NAME test_fixed_string COMMAND test_fixed_string)
add_test(NAME test_converter COMMAND test_converter)
