# pjh_cli

C++20 CLI library with Rust-style error handling. Compile-time option keys, subcommand tree, fuzzy matching, and REPL mode.

## Requirements

- C++20 compiler
- CMake 3.20+
- `pjh_result` 0.1.0 or compatible — required, transitive (`PUBLIC`) dependency
- Network access on a clean configure (CMake fetches the pinned dependencies)

## Dependencies

`pjh_result` supplies `Result<T, E>` / `Option<T>` and is linked `PUBLIC`
(consumers inherit it). CMake resolves it in one of two ways:

1. **Installed** — if `find_package(pjh_result 0.1.0 CONFIG)` finds a
   compatible package (same major, version >= 0.1.0), it is used as-is.
2. **Fetched** — otherwise `FetchContent` clones
   `https://github.com/pjh456/pjh_result.git` and checks out the pinned commit
   `4e2d37fa5a84ca9c70f3919c1fd7ce21ea4f22c5` (upstream project version
   `0.1.0`). Upstream has no release tags yet, so a commit SHA is the only
   reproducible pin.

For an offline or byte-reproducible configure, install `pjh_result` first and
configure with `-DCMAKE_PREFIX_PATH=<prefix>` so path (1) is taken. The
installed package config also calls `find_dependency(pjh_result 0.1.0)`, so an
installed consumer must have a compatible `pjh_result` discoverable.

When tests are enabled (`PJH_CLI_BUILD_TESTS=ON`, the default at top level),
`doctest` `v2.5.0` is fetched the same way (`tests/CMakeLists.txt`).

## Usage

### Options (named)

```cpp
App app("myapp", "1.0.0", "Description");

// Flag (bool → no value consumed)
app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose output").boolean();

// Valued option with default
app.option<fixed_string("port")>("--port", 'p', "Port number", 8080);

// Required option
app.option<fixed_string("token")>("--token", "API token").str().required();

// Counting flag (-vvv → 3)
app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").count();

// Enum option with string mapping
enum class Color { red, green, blue };
app.option<fixed_string("color")>("--color", 'c', "Color")
    .enum_type<Color>()
    .mapping({{"red", Color::red}, {"green", Color::green}})
    .default_value(Color::red);

// Negatable flag (--no-xxx)
app.option<fixed_string("compress")>("--compress", "Compress output").boolean().negatable();

// Repeatable option (--opt a --opt b)
app.option<fixed_string("include")>("--include", 'I', "Include path").path().repeatable();

// Numeric option with range validation
app.option<fixed_string("level")>("--level", 'l', "Log level").integer().min(0).max(7);

// Env-var fallback (CLI > env > default)
app.option<fixed_string("host")>("--host", "Host").str().env("MYAPP_HOST").required();

// Option group (mutual exclusion / requirement)
app.group<fixed_string("verbose"), fixed_string("quiet")>().at_most_one();

auto r = app.parse(argc, argv);
if (r.is_err()) { /* r.unwrap_err().what() */ return 1; }

auto &ctx = r.unwrap();
if (ctx.help_requested())
{
    std::cout << ctx.help_text();
    return 0;
}
if (ctx.version_requested())
{
    std::cout << ctx.version_text();
    return 0;
}
int port = ctx.get<int, fixed_string("port")>();
auto color = ctx.get_enum<Color, fixed_string("color")>();
```

### Help and version

`parse()` / `parse_fuzzy()` never print help or version and never exit. When
`--help` / `-h` or `--version` is seen, parsing stops early and the returned `Ok`
context has `help_requested()` / `version_requested()` set (with pre-formatted
`help_text()` / `version_text()`).

`--help` / `-h` / `--version` are reserved and take precedence over user options.
Registering a long option named `help` or `version`, or a short option `-h`,
throws `LogicError` at construction. Use another short character (for example
`-H` for `--host`) or a different long name.

Dispatch on those predicates **before** reading any value: the meta-flag path
skips `ParseFinalizer`, so defaults and environment fallbacks are not applied and
`ctx.get()` may throw `LogicError`.

```cpp
auto &ctx = r.unwrap();
if (ctx.help_requested())
{
    std::cout << ctx.help_text();
    return 0;
}
if (ctx.version_requested())
{
    std::cout << ctx.version_text();
    return 0;
}
// safe to read values from here
```

The batch `--help` / `-h` text is rendered by `help_formatter()`, defaulting to
`HelpFormatter::format_help`. Inject a custom renderer (it must return a
non-empty string, because `help_requested()` is derived from `help_text()`):

```cpp
app.set_help_formatter([](const BaseCommand &cmd) {
    return my_render_help(cmd);
});
```

`Parser::parse_command(root, args, max_fuzzy_distance, help_fmt)` exposes the
same seam to direct `Parser` users. The REPL's `help` command keeps using its own
injected navigation formatter.

`format_help` annotates options with `(env: VAR)`, `(negatable)`, `(counting)`,
and `(repeatable)` in the Options table.

### Positional arguments

```cpp
LeafCommand cmd("copy", "Copy files");
cmd.arg<std::string, 0>("source", "Source file").required();
cmd.arg<std::string, 1>("dest", "Destination path").required();

// Access by compile-time index
auto src = ctx.get<std::string, 0>();
```

### Subcommands

```cpp
auto &server = app.add_branch("server", "Server management");
server.add_leaf("start", "Start server");
server.add_leaf("stop", "Stop server");

// Parse walks the tree; ctx.matched_path() returns "server start"
std::cout << ctx.matched_path();

// Structured path info
MatchedPath path = ctx.matched_path_info();
for (auto &name : path.commands) { /* "server", "start" */ }
```

### Ancestor options after descent

An option declared on a parent command (including the root `App`) is accepted
after a subcommand and readable from the leaf context: if the root declares
`--port`, then `app server --port 8080` sets it even though `server` does not
declare it.  Resolution walks the current command and its ancestors with the
nearest declaration winning; the value is stored in the declaring command's
context, so repeatable/counting ancestor options accumulate across descent
(`app --tag a server --tag b` yields `{a, b}`).  Siblings and descendants stay
invisible and unknown tokens still error.  Help and completion still list only
the current node's options.

### Actions

```cpp
auto &cmd = app.add_leaf("greet", "Say hello");
cmd.arg<std::string, 0>("name", "Who to greet").required();
cmd.action([](ParseContext &ctx) -> CliResult<void>
{
    std::cout << "Hello, " << ctx.get<std::string, 0>() << "\n";
    return CliResult<void>::Ok();
});
```

### REPL mode

```cpp
InteractiveConsole console(app, "> ");
console.run();
// Type ? to list subcommands, ?query to search,
// help / --help / -h for formatted help
```

When the input is an interactive TTY, `run()` reads keys in raw mode and Tab
completes the token under the cursor: subcommand names, option names, and option
**values** registered with `.completer(fn)`.  A unique candidate is appended
(with a trailing space); zero or several candidates print the candidate list and
a `HintBuilder` hint, then redraw the prompt.  Up/Down recall the previous/next
line from the injected `IHistory` (`InMemoryHistory` by default,
`RingBufferHistory` for bounded storage, `NoOpHistory`/`nullptr` to disable) and
restore the draft typed before the first Up when Down passes the newest entry.
Non-TTY input (pipes, files, injected test streams) keeps the line-based
`std::getline` path unchanged — history is still recorded but arrow keys cannot
navigate.  A custom `ITerminal` can be installed with `console.set_terminal(...)`.

### Fuzzy matching

```cpp
// auto corrects typos (Levenshtein distance ≤ 3)
app.parse_fuzzy(argc, argv);
```

A unique close match is auto-corrected transparently.  When two or more
candidates fall within distance 3, `parse_fuzzy()` reports
`Parse Error: ambiguous command '<input>', candidates: …` instead of guessing;
an exact or alias match still wins.

### Interactive hints

```cpp
// Build a hint string showing remaining options and args
auto hint = HintBuilder::format(app, partial_input);
// e.g. "[INT:port] <src> <dst>"
```

### Aliases

```cpp
app.add_leaf("list", "List items").alias("ls").alias("show");
// "ls" and "show" also match the list command
```

### Unmatched subcommands and extra arguments

`ExtraArgsPolicy::Ignore` is the default and applies to leaves and branches
without subcommands (POSIX-style trailing operands are silently discarded).
A branch that has subcommands reports an unmatched word as an unknown command
(`Parse Error: unknown command: 'instal'; did you mean: install`) instead of
silently succeeding.  In fuzzy mode, a word with several close matches is
reported as `Parse Error: ambiguous command '<input>', candidates: …` rather
than as an unknown command.  Call `set_extra_args(...)` on the branch —
including `Ignore` — to opt out explicitly.  Explicit policies are inherited by
subcommands.

A token whose first character after `-` is a digit or `.` (e.g. `-5`, `-3.14`)
is a positional/value token, not a short option; it never triggers subcommand
descent.  A registered short option for a digit or `.` is therefore unreachable
via that form.

## API Quick Reference

`#include <pjh_cli.hpp>`

| Expression | Purpose |
|---|---|
| `App(name, version, desc)` | Root command |
| `cmd.option<Key>(long, short?)... .integer()/boolean()/str()` | Named option |
| `cmd.option<Key>(long, short?, desc, default)` | Auto-dispatch option (T → type) |
| `.count()` | Counting flag (-vvv) |
| `.floating()` | Double-valued option |
| `.path()` | Filesystem path option |
| `.enum_type<E>().mapping({...})` | Enum option with string mapping |
| `.negatable()` | Support --no-xxx negation |
| `.repeatable()` | Accept multiple values (--opt a --opt b) |
| `.env("VAR")` | Environment variable fallback (CLI > env > default) |
| `.min(v)` / `.max(v)` | Numeric range validation |
| `.default_value(v)` | Manual default value |
| `cmd.arg<T, Index>(name, desc)` | Positional argument |
| `cmd.add_branch(name, desc)` | Child branch subcommand |
| `cmd.add_leaf(name, desc)` | Child leaf subcommand |
| `cmd.action(fn)` | Execute callback on match |
| `cmd.enabled(pred)` | Runtime enable/disable |
| `cmd.set_visibility(v)` | `Cli` / `Repl` / `Both` / `Hidden` |
| `cmd.set_extra_args(p)` | `Ignore` (default) / `Error` / `Store`; explicit setting opts out of the unknown-command check |
| `cmd.alias(name)` | Register an alias name |
| `.required()` | Mark option/arg required |
| `.completer(fn)` | Option-value completion callback, honored by the REPL Tab handler |
| `cmd.group<Keys...>().exactly_one()` | Option group: exactly one required |
| `cmd.group<Keys...>().at_most_one()` | Option group: zero or one |
| `cmd.group<Keys...>().at_least_one()` | Option group: at least one required |
| `app.parse(argc, argv)` | Batch parse |
| `app.parse_fuzzy(argc, argv)` | Batch parse with typo correction |
| `ctx.get<T, Key>()` | Get value (throws if absent) |
| `ctx.has<Key>()` | Check key exists |
| `ctx.try_get<T, Key>()` | Get → `Option<T>` (no throw) |
| `ctx.get_or<T, Key>(fallback)` | Get or return fallback (no throw) |
| `ctx.get_all<T, Key>()` | All values for repeatable option |
| `ctx.get_enum<E, Key>()` | Get enum value |
| `ctx.try_get_enum<E, Key>()` | Get enum → `Option<E>` |
| `ctx.get_or_enum<E, Key>(fallback)` | Get enum or fallback |
| `ctx.get_all_enum<E, Key>()` | All enum values for repeatable |
| `ctx.matched_path()` | Matched subcommand path, e.g. `"server start"` |
| `ctx.matched_path_info()` | MatchedPath{commands} struct |
| `ctx.matched_command()` | Deepest matched command pointer |
| `ctx.extra_args()` | Extra positional args (when policy is `Store`); parse-wide, includes tokens seen before a subcommand |
| `ctx.help_requested()` | True if --help / -h was passed; caller prints `help_text()` and exits |
| `ctx.help_text()` | Pre-formatted help string (non-empty when help requested) |
| `ctx.version_requested()` | True if --version was passed; caller prints `version_text()` and exits |
| `ctx.version_text()` | Pre-formatted version string (non-empty when version requested) |
| `HelpFormatter::format_help(cmd)` | Formatted help string |
| `HelpFormatter::format_usage(cmd)` | One-line usage string |
| `HelpFormatter::collect_help(cmd)` | Structured HelpInfo data |
| `HintBuilder::format(root, input)` | Interactive hint for partial input |
| `list_subcommands(cmd)` | Visible subcommand names |
| `complete(cmd, prefix)` | Tab completion candidates (strings) |
| `complete_candidates(cmd, prefix)` | Tab completion candidates (struct) |
| `complete_line(root, line, cursor)` | Completion for the token under the cursor, including `.completer` values |
| `complete_line_result(root, line, cursor)` | Same, plus the matched prefix length for inline/compact value insertion |
| `complete_value_candidates(opt, prefix)` | Option-value candidates from `.completer(fn)` |
| `InteractiveConsole(root, prompt)` | REPL console |
| `console.run()` / `console.stop()` | Start / stop REPL loop |
| `console.set_prompt(s)` | Override prompt string |
| `console.set_terminal(t)` | Install a custom `ITerminal` (nullptr = TTY detect / getline) |

### Key types

| Type | Description |
|---|---|
| `CliResult<T>` | `Result<T, CliError>` |
| `CliFailure` | `Failure<CliError>`, implicit conversion |
| `CliError` | Parse/execution error (`std::runtime_error`) |
| `LogicError` | Programming error (`std::logic_error`) |
| `fixed_string("...")` | Compile-time string for NTTP keys |
| `Visibility::Cli / Repl / Both / Hidden` | Visibility flags |
| `ExtraArgsPolicy::Ignore / Error / Store` | Extra positional arg handling; on a branch with subcommands the implicit `Ignore` default reports an unknown command |
| `GroupMode::ExactlyOne / AtMostOne / AtLeastOne` | Option group constraints |
| `HelpInfo / OptionInfo / ArgInfo / SubcommandInfo` | Structured help metadata |
| `UsageInfo / HelpDocument` | Pre-built help document |
| `HintContext / HintInfo / HintConfig` | Interactive hint structures |
| `SuggestionInfo / FuzzySuggestion` | Fuzzy match results |
| `CompletionCandidate` | Completion candidate struct |
| `KeyEvent / ITerminal / LineEditor` | Injectable raw-mode line editor (`console/line_editor.hpp`) |
| `MatchedPath` | Matched subcommand path struct |
| `VersionInfo` | Program version struct |

## Build

```cmake
cmake -B build
cmake --build build
```

CMake options:

| Option | Default | Description |
|---|---|---|
| `PJH_CLI_BUILD_TESTS` | `ON` (top-level) | Build tests with doctest |
| `PJH_CLI_BUILD_EXAMPLES` | `OFF` | Build example programs |

Include as a submodule:

```cmake
add_subdirectory(path/to/pjh_cli)
target_link_libraries(myapp PRIVATE pjh::cli)
```

Consume an installed package:

```cmake
find_package(pjh_cli CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE pjh::cli)
```

Both modes expose the same target name `pjh::cli`. The package config calls
`find_dependency(pjh_result 0.1.0)`, so an installed `pjh_result` must be discoverable
when `find_package(pjh_cli)` runs.

## Layering guard

`cmake/check_layering.cmake` mechanically enforces the layer DAG documented in
`codebase/ARCHITECTURE.md`: every `#include <pjh_cli/...>` in `include/pjh_cli/**`
and `src/**` must be a same-layer or downward edge. It checks direct `#include`
edges **and** their transitive closure, so a file that reaches a forbidden
subsystem through a chain of headers also fails. It scans include directives
only (no compiler, network, or extra tool), fails closed on unclassified files
and unresolved targets, and runs in well under a second:

```sh
cmake -DPJH_CLI_SOURCE_DIR="$PWD" -P cmake/check_layering.cmake
```

It is also registered as the `layering_guard` CTest test, so the standard test
command covers it:

```sh
ctest --test-dir build -R layering_guard --output-on-failure
```

The only cross-layer exceptions are the four value-storage carve-outs
`command|option -> parse/parse_context(_writer)` listed in `_allowed`. Those two
headers are also the only entries in `_transitive_ok`, so reaching any other
forbidden header through the include graph fails. A new source file must be
classified in the script's `_src_map`; a new subsystem needs a `_known_layers`
entry, a `_forbid_<layer>` row, and classifier coverage; a new intentional
exception is a single `layer|target` line in `_allowed` and must be documented
in the same change.
