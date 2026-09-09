# Standalone install/export smoke test. This is a separate CMake project, not a
# subdirectory of tests/: it is configured only by the CI install/consume step
# (or manually), against an installed pjh_cli package.
cmake_minimum_required(VERSION 3.20)
project(pjh_cli_consumer LANGUAGES CXX)

find_package(pjh_cli CONFIG REQUIRED)

if(NOT TARGET pjh::cli)
    message(FATAL_ERROR "installed pjh_cli package did not export pjh::cli")
endif()
if(TARGET pjh_cli)
    message(FATAL_ERROR "un-namespaced target pjh_cli leaked from the package")
endif()

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE pjh::cli)
