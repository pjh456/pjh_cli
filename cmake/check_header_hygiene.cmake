# check_header_hygiene.cmake — installed public-header include hygiene guard.
#
# Run in CMake script mode (no configure, no compiler, no network):
#   cmake -DPJH_CLI_SOURCE_DIR=<repo-root> -P cmake/check_header_hygiene.cmake
#
# Static, compiler-free: a public header that names one of the curated std
# symbols below must include its canonical owner header directly (transitive
# providers do not count).  It pins the recurring "dependency-dishonest public
# header" defect class (missing <cstddef>/<utility>/<functional>/<string>/...),
# not complete IWYU; the curated table is deliberately closed and the allow
# table is empty by design.
#
# Preconditions / known gaps: comments are stripped before scanning, so a
# string literal containing `//` or `/* */` would truncate that line's code;
# no include/** header currently does this.  If one appears, list it in
# _hy_allow.  Documented non-rules: `std::hash` (<string_view> declares the
# relevant specialisation), generic `std::ranges` algorithms (owned by
# <algorithm>), and `std::istream`/`std::ostream` (declaration context may
# legitimately use <iosfwd>, so the two cannot be distinguished textually).
cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED PJH_CLI_SOURCE_DIR)
    message(FATAL_ERROR "usage: cmake -DPJH_CLI_SOURCE_DIR=<repo-root> -P cmake/check_header_hygiene.cmake")
endif()
set(_root "${PJH_CLI_SOURCE_DIR}")

file(GLOB_RECURSE _files RELATIVE "${_root}" "${_root}/include/*.hpp")
list(SORT _files)
if(NOT _files)
    message(FATAL_ERROR "header hygiene: no include/**/*.hpp found under ${_root}")
endif()

# Parallel lists; entry i of _hy_patterns pairs with entry i of _hy_owners.
# Every std:: pattern MUST carry a trailing ([^A-Za-z0-9_]|$) boundary; the bare
# size_t pattern MUST carry a leading (^|[^:A-Za-z0-9_]) boundary.
set(_hy_patterns
    "(^|[^:A-Za-z0-9_])size_t([^A-Za-z0-9_]|$)"
    "std::size_t([^A-Za-z0-9_]|$)"
    "std::move([^_A-Za-z0-9]|$)"   "std::forward([^_A-Za-z0-9]|$)"
    "std::pair([^_A-Za-z0-9]|$)"   "std::swap([^_A-Za-z0-9]|$)"
    "std::string([^_A-Za-z0-9]|$)" "std::string_view([^_A-Za-z0-9]|$)"
    "std::deque([^_A-Za-z0-9]|$)"  "std::vector([^_A-Za-z0-9]|$)"
    "std::array([^_A-Za-z0-9]|$)"
    "std::unique_ptr([^_A-Za-z0-9]|$)"  "std::shared_ptr([^_A-Za-z0-9]|$)"
    "std::make_unique([^_A-Za-z0-9]|$)" "std::make_shared([^_A-Za-z0-9]|$)"
    "std::equal_to([^_A-Za-z0-9]|$)"    "std::function([^_A-Za-z0-9]|$)"
    "std::ostringstream([^_A-Za-z0-9]|$)" "std::istringstream([^_A-Za-z0-9]|$)"
    "std::unordered_map([^_A-Za-z0-9]|$)" "std::unordered_set([^_A-Za-z0-9]|$)"
    "std::variant([^_A-Za-z0-9]|$)" "std::monostate([^_A-Za-z0-9]|$)"
    "std::optional([^_A-Za-z0-9]|$)" "std::tuple([^_A-Za-z0-9]|$)"
    "std::format([^_A-Za-z0-9]|$)"  "std::filesystem([^_A-Za-z0-9]|$)"
    "std::same_as([^_A-Za-z0-9]|$)" "std::derived_from([^_A-Za-z0-9]|$)"
    "std::convertible_to([^_A-Za-z0-9]|$)" "std::integral([^_A-Za-z0-9]|$)"
    "std::floating_point([^_A-Za-z0-9]|$)" "std::constructible_from([^_A-Za-z0-9]|$)"
    "std::span([^_A-Za-z0-9]|$)"
    "std::cout([^_A-Za-z0-9]|$)" "std::cin([^_A-Za-z0-9]|$)" "std::cerr([^_A-Za-z0-9]|$)"
    "std::logic_error([^_A-Za-z0-9]|$)" "std::runtime_error([^_A-Za-z0-9]|$)"
    "std::invalid_argument([^_A-Za-z0-9]|$)"
    "std::int32_t([^_A-Za-z0-9]|$)" "std::int64_t([^_A-Za-z0-9]|$)"
    "std::uint32_t([^_A-Za-z0-9]|$)" "std::uint64_t([^_A-Za-z0-9]|$)"
    "std::enable_if([^_A-Za-z0-9]|$)" "std::is_same([^_A-Za-z0-9]|$)"
    "std::remove_cvref([^_A-Za-z0-9]|$)" "std::decay([^_A-Za-z0-9]|$)"
    "std::max([^_A-Za-z0-9]|$)" "std::min([^_A-Za-z0-9]|$)"
    "std::chrono([^_A-Za-z0-9]|$)" "std::atomic([^_A-Za-z0-9]|$)"
    "std::thread([^_A-Za-z0-9]|$)")
set(_hy_owners
    "cstddef" "cstddef"
    "utility" "utility" "utility" "utility"
    "string" "string_view"
    "deque" "vector" "array"
    "memory" "memory" "memory" "memory"
    "functional" "functional"
    "sstream" "sstream"
    "unordered_map" "unordered_set"
    "variant" "variant" "optional" "tuple"
    "format" "filesystem"
    "concepts" "concepts" "concepts" "concepts" "concepts" "concepts"
    "span"
    "iostream" "iostream" "iostream"
    "stdexcept" "stdexcept" "stdexcept"
    "cstdint" "cstdint" "cstdint" "cstdint"
    "type_traits" "type_traits" "type_traits" "type_traits"
    "algorithm" "algorithm"
    "chrono" "atomic" "thread")

# Documented exceptions keyed "<repo-relative-file>|<owner>"; empty by design
# (std::hash is deliberately NOT a rule: <string_view> declares its specialisation).
set(_hy_allow "")

set(_violations "")
foreach(_file IN LISTS _files)
    file(READ "${_root}/${_file}" _raw)
    string(REGEX REPLACE "/\\*([^*]|\\*[^/])*\\*/" "" _code "${_raw}")
    string(REGEX REPLACE "//[^\n]*" "" _code "${_code}")

    file(STRINGS "${_root}/${_file}" _inc_lines
         REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"]")
    set(_inc "")
    foreach(_l IN LISTS _inc_lines)
        if(_l MATCHES "#[ \t]*include[ \t]*[<\"]([^>\"]+)[>\"]")
            list(APPEND _inc "${CMAKE_MATCH_1}")
        endif()
    endforeach()

    list(LENGTH _hy_patterns _n)
    math(EXPR _last "${_n} - 1")
    foreach(_i RANGE ${_last})
        list(GET _hy_patterns ${_i} _pat)
        list(GET _hy_owners ${_i} _owner)
        string(REGEX MATCH "${_pat}" _hit "${_code}")
        if(_hit)
            list(FIND _inc "${_owner}" _has)
            if(_has EQUAL -1)
                list(FIND _hy_allow "${_file}|${_owner}" _ok)
                if(_ok EQUAL -1)
                    list(APPEND _violations
                         "${_file}: names ${_pat} but does not include <${_owner}>")
                endif()
            endif()
        endif()
    endforeach()
endforeach()

if(_violations)
    list(LENGTH _violations _count)
    foreach(_v IN LISTS _violations)
        message("header hygiene: ${_v}")
    endforeach()
    message(FATAL_ERROR
            "header hygiene failed: ${_count} public header(s) rely on transitive "
            "standard-library includes; add the direct include(s) above")
endif()
list(LENGTH _files _file_count)
message(STATUS "header hygiene: OK (${_file_count} files scanned)")
