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
# It also computes the transitive closure of the file-level include graph and
# fails if any file reaches a header in a forbidden subsystem.  The graph has
# a command<->option umbrella cycle, so the closure is a fixed point, not a
# topological sort.
#
# It also rejects external platform includes from installed public headers:
# pjh_platform/<...> (and the raw OS headers listed below) may appear in src/**
# only.  include/** — including the installed detail/ headers — must stay free
# of them; the empty allow table is the single, deliberate escape hatch.
#
# Fail-closed policy: a file whose layer cannot be resolved, an include target
# whose subsystem cannot be resolved, and a reachable target that is not part of
# the scanned file set are all reported as violations, so a new source file or
# subsystem directory must be classified here explicitly.
#
# This file encodes the zero-exception layer DAG: there are no cross-layer
# exceptions.

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

# ── External includes forbidden in installed public headers (include/**) ─────
# Platform code belongs to src/**. Add a new external dependency prefix here
# deliberately; the allow table (empty on purpose) is the only escape hatch,
# keyed "<repo-relative-file>|<prefix-or-header>".
set(_public_ext_forbidden "pjh_platform")
set(_public_raw_forbidden
    "windows.h;conio.h;io.h;termios.h;poll.h;unistd.h;cwchar")
set(_public_ext_allow "")

# ── Explicit src/ layer map (new source files must be added here) ───────────
set(_src_map
    "src/app.cpp=app"
    "src/env.cpp=detail"
    "src/command.cpp=command"
    "src/error.cpp=core"
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
    "src/console/file_history.cpp=console"
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
set(_edges "")  # file|target edges for the transitive closure below
foreach(_file IN LISTS _files)
    _layer_of("${_file}" _layer)
    if(_layer STREQUAL "unknown")
        list(APPEND _violations
             "${_file}: unmapped file — classify it in cmake/check_layering.cmake")
        continue()
    endif()

    string(MAKE_C_IDENTIFIER "${_file}" _key)
    set("_direct_${_key}" "")

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
        if(_forbidden GREATER -1)
            list(APPEND _violations "${_file}: ${_layer} -> <pjh_cli/${_target}>")
        endif()

        # Record the edge (and the direct target) for the closure pass.
        if(_target STREQUAL "pjh_cli.hpp")
            set(_tgt "include/pjh_cli.hpp")
        else()
            set(_tgt "include/pjh_cli/${_target}")
        endif()
        list(APPEND _edges "${_file}|${_tgt}")
        list(APPEND "_direct_${_key}" "${_tgt}")
    endforeach()
endforeach()

# ── External-dependency boundary: pjh_platform et al. must stay in src/ ─────
foreach(_file IN LISTS _files)
    if(NOT _file MATCHES "^include/")
        continue()
    endif()
    file(STRINGS "${_root}/${_file}" _ext_lines
         REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"]")
    foreach(_ext_line IN LISTS _ext_lines)
        foreach(_prefix IN LISTS _public_ext_forbidden)
            if(_ext_line MATCHES "[<\"]${_prefix}(/|\\.hpp|>)")
                list(FIND _public_ext_allow "${_file}|${_prefix}" _ext_ok)
                if(_ext_ok EQUAL -1)
                    list(APPEND _violations
                         "${_file}: public header must not include <${_prefix}/...>, move platform code to src/")
                endif()
            endif()
        endforeach()
        foreach(_header IN LISTS _public_raw_forbidden)
            if(_ext_line MATCHES "[<\"]${_header}[>\"]")
                list(FIND _public_ext_allow "${_file}|${_header}" _ext_ok)
                if(_ext_ok EQUAL -1)
                    list(APPEND _violations
                         "${_file}: public header must not include <${_header}>")
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()

# ── Transitive closure of the include graph (fixed point; the graph has a
#    command<->option umbrella cycle, so no topological order exists). ───────
foreach(_file IN LISTS _files)
    string(MAKE_C_IDENTIFIER "${_file}" _key)
    set("_reach_${_key}" "${_direct_${_key}}")
endforeach()
set(_changed TRUE)
while(_changed)
    set(_changed FALSE)
    foreach(_edge IN LISTS _edges)
        string(REPLACE "|" ";" _parts "${_edge}")
        list(GET _parts 0 _f)
        list(GET _parts 1 _t)
        string(MAKE_C_IDENTIFIER "${_f}" _fk)
        string(MAKE_C_IDENTIFIER "${_t}" _tk)
        foreach(_tt IN LISTS "_reach_${_tk}")
            list(FIND "_reach_${_fk}" "${_tt}" _seen)
            if(_seen EQUAL -1)
                list(APPEND "_reach_${_fk}" "${_tt}")
                set(_changed TRUE)
            endif()
        endforeach()
    endforeach()
endwhile()

# ── Transitive check: reject any reachable forbidden header that is not a
#    direct edge (direct edges are reported above). ──────────────────────────
foreach(_file IN LISTS _files)
    _layer_of("${_file}" _layer)
    if(_layer STREQUAL "unknown")
        continue()  # already reported by the direct pass
    endif()
    string(MAKE_C_IDENTIFIER "${_file}" _key)
    foreach(_tgt IN LISTS "_reach_${_key}")
        list(FIND "_direct_${_key}" "${_tgt}" _is_direct)
        if(_is_direct GREATER -1)
            continue()  # direct edges are reported above
        endif()
        if(NOT "${_tgt}" IN_LIST _files)
            list(APPEND _violations
                 "${_file}: unresolved include target ${_tgt}")
            continue()
        endif()
        if(_tgt STREQUAL "include/pjh_cli.hpp")
            set(_target "pjh_cli.hpp")
        else()
            string(REGEX REPLACE "^include/pjh_cli/" "" _target "${_tgt}")
        endif()
        _subsystem_of("${_target}" _sub)
        list(FIND _forbid_${_layer} "${_sub}" _forbidden)
        if(_forbidden EQUAL -1)
            continue()
        endif()
        list(APPEND _violations
             "${_file}: ${_layer} transitively -> <pjh_cli/${_target}>")
    endforeach()
endforeach()

list(LENGTH _files _file_count)
if(_violations)
    list(LENGTH _violations _count)
    foreach(_v IN LISTS _violations)
        message("layering guard: ${_v}")
    endforeach()
    message(FATAL_ERROR
            "layering guard failed: ${_count} direct or transitive violation(s) "
            "across ${_file_count} file(s); "
            "see codebase/ARCHITECTURE.md 'Layer model'")
endif()
message(STATUS "layering guard: OK (${_file_count} files scanned, direct + transitive)")
