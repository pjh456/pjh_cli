# check_layering.cmake — include-direction guard for the pjh_cli layer DAG.
#
# Run in CMake script mode (no configure, no compiler, no network):
#   cmake -DPJH_CLI_SOURCE_DIR=<repo-root> -P cmake/check_layering.cmake
#
# It scans every include/pjh_cli/**.hpp and src/**.cpp file for literal
# `#include <pjh_cli/...>` directives, maps the including file to a layer and
# the include target to a subsystem, and fails if the edge points against the
# documented layer DAG (codebase/ARCHITECTURE.md "Layer model") unless the exact
# `layer|target` pair is allow-listed below.
#
# Fail-closed policy: a file whose layer cannot be resolved and an include
# target whose subsystem cannot be resolved are both reported as violations, so
# a new source file or subsystem directory must be classified here explicitly.
#
# This file encodes the post-task-20 tree. The only cross-layer exceptions are
# the four command/option -> parse/parse_context(_writer) value-storage edges.

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PJH_CLI_SOURCE_DIR)
    message(FATAL_ERROR
            "usage: cmake -DPJH_CLI_SOURCE_DIR=<repo-root> -P cmake/check_layering.cmake")
endif()
set(_root "${PJH_CLI_SOURCE_DIR}")

# ── Known layer ids; anything else is fail-closed "unknown" ─────────────────
set(_known_layers core detail command option parse format console app)

# ── Forbidden target subsystems per including layer ─────────────────────────
set(_forbid_core    "command;option;parse;format;console;app;umbrella")
set(_forbid_detail  "command;option;parse;format;console;app;umbrella")
set(_forbid_command "parse;console;format;app;umbrella")
set(_forbid_option  "parse;console;format;app;umbrella")
set(_forbid_parse   "format;console;app;umbrella")
set(_forbid_format  "parse;console;app;umbrella")
set(_forbid_console "app;umbrella")
set(_forbid_app     "console;umbrella")
set(_forbid_umbrella "")

# ── Exact allow-list: the pre-existing value-storage carve-outs ─────────────
set(_allowed
    "command|parse/parse_context.hpp"
    "command|parse/parse_context_writer.hpp"
    "option|parse/parse_context.hpp"
    "option|parse/parse_context_writer.hpp")

# ── Explicit src/ layer map (new source files must be added here) ───────────
set(_src_map
    "src/app.cpp=app"
    "src/command.cpp=command"
    "src/parser.cpp=parse"
    "src/matched_path_resolver.cpp=parse"
    "src/option_consumer.cpp=parse"
    "src/subcommand_resolver.cpp=parse"
    "src/parse_finalizer.cpp=parse"
    "src/value_writer.cpp=parse"
    "src/help.cpp=format"
    "src/matcher.cpp=format"
    "src/hint.cpp=format"
    "src/console_output.cpp=format"
    "src/console.cpp=console"
    "src/console/query.cpp=console"
    "src/console/help_navigator.cpp=console"
    "src/console/in_memory_history.cpp=console"
    "src/console/ring_buffer_history.cpp=console"
    "src/console/line_editor.cpp=console"
    "src/console/tty_terminal.cpp=console")

# <pjh_cli/X> include target -> subsystem id.
function(_subsystem_of target out)
    if(target STREQUAL "pjh_cli.hpp")
        set(${out} "umbrella" PARENT_SCOPE)
        return()
    endif()
    foreach(_s core detail command option parse format console)
        if(target MATCHES "^${_s}/")
            set(${out} "${_s}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    foreach(_s core command option parse format console)
        if(target STREQUAL "${_s}.hpp")
            set(${out} "${_s}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    if(target STREQUAL "app.hpp")
        set(${out} "app" PARENT_SCOPE)
        return()
    endif()
    set(${out} "unknown" PARENT_SCOPE)
endfunction()

# Repo-relative file path -> layer id.
function(_layer_of file out)
    if(file STREQUAL "include/pjh_cli.hpp")
        set(${out} "umbrella" PARENT_SCOPE)
        return()
    endif()
    if(file MATCHES "^include/pjh_cli/([a-z_]+)/")
        list(FIND _known_layers "${CMAKE_MATCH_1}" _known)
        if(_known GREATER -1)
            set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
            return()
        endif()
    endif()
    if(file MATCHES "^include/pjh_cli/([a-z_]+)\\.hpp$")
        list(FIND _known_layers "${CMAKE_MATCH_1}" _known)
        if(_known GREATER -1)
            set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
            return()
        endif()
    endif()
    foreach(_entry IN LISTS _src_map)
        if(_entry MATCHES "^([^=]+)=(.*)$" AND file STREQUAL CMAKE_MATCH_1)
            set(${out} "${CMAKE_MATCH_2}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out} "unknown" PARENT_SCOPE)
endfunction()

# ── Scan ─────────────────────────────────────────────────────────────────────
file(GLOB_RECURSE _files RELATIVE "${_root}"
     "${_root}/include/*.hpp" "${_root}/src/*.cpp")
list(SORT _files)  # deterministic violation order

set(_violations "")
foreach(_file IN LISTS _files)
    _layer_of("${_file}" _layer)
    if(_layer STREQUAL "unknown")
        list(APPEND _violations
             "${_file}: unmapped file — classify it in cmake/check_layering.cmake")
        continue()
    endif()

    file(STRINGS "${_root}/${_file}" _lines
         REGEX "^[ \t]*#[ \t]*include[ \t]*<pjh_cli")
    foreach(_line IN LISTS _lines)
        if(_line MATCHES "<pjh_cli\\.hpp>")
            set(_target "pjh_cli.hpp")
        elseif(_line MATCHES "<pjh_cli/([^>]+)>")
            set(_target "${CMAKE_MATCH_1}")
        else()
            list(APPEND _violations "${_file}: unparsed pjh_cli include: ${_line}")
            continue()
        endif()

        _subsystem_of("${_target}" _sub)
        if(_sub STREQUAL "unknown")
            list(APPEND _violations
                 "${_file}: unclassified include target <pjh_cli/${_target}>")
            continue()
        endif()

        list(FIND _forbid_${_layer} "${_sub}" _forbidden)
        list(FIND _allowed "${_layer}|${_target}" _ok)
        if(_forbidden GREATER -1 AND _ok EQUAL -1)
            list(APPEND _violations "${_file}: ${_layer} -> <pjh_cli/${_target}>")
        endif()
    endforeach()
endforeach()

list(LENGTH _files _file_count)
if(_violations)
    list(LENGTH _violations _count)
    foreach(_v IN LISTS _violations)
        message("layering guard: ${_v}")
    endforeach()
    message(FATAL_ERROR
            "layering guard failed: ${_count} violation(s) across ${_file_count} file(s); "
            "see codebase/ARCHITECTURE.md 'Layer model'")
endif()
message(STATUS "layering guard: OK (${_file_count} files scanned)")
