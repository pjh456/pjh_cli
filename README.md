# pjh_cli

C++20 CLI library with Rust-style error handling. Compile-time option keys, subcommand tree, fuzzy matching, and REPL mode.

## Requirements

- C++20 compiler
- CMake 3.20+

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

auto r = app.parse(argc, argv);
if (r.is_err()) { /* r.unwrap_err().what() */ return 1; }

auto &ctx = r.unwrap();
int port = ctx.get<int, fixed_string("port")>();
```

### Positional arguments

```cpp
app.arg<std::string, 0>("source", "Source file").required();
app.arg<std::string, 1>("dest", "Destination path").required();

// Access by compile-time index
auto src = ctx.get<std::string, 0>();
```

### Subcommands

```cpp
auto &server = app.add_command("server", "Server management");
server.add_command("start", "Start server");
server.add_command("stop", "Stop server");

// Parse walks the tree; ctx.matched_path() returns "server start"
auto &ctx = r.unwrap();
std::cout << ctx.matched_path();
```

### Actions

```cpp
auto &cmd = app.add_command("greet", "Say hello");
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

### Fuzzy matching

```cpp
// auto corrects typos (Levenshtein distance ≤ 3)
app.parse_fuzzy(argc, argv);
```

## API Quick Reference

`#include <pjh_cli.hpp>`

| Expression | Purpose |
|---|---|
| `App(name, version, desc)` | Root command |
| `cmd.option<Key>(long, short?)... .integer()/boolean()/str()` | Named option |
| `cmd.option<Key>(long, short?, desc, default)` | Auto-dispatch option |
| `cmd.arg<T, Index>(name, desc)` | Positional argument |
| `cmd.add_command(name, desc)` | Child subcommand |
| `cmd.action(fn)` | Execute callback on match |
| `cmd.enabled(pred)` | Runtime enable/disable |
| `cmd.set_visibility(v)` | `Cli` / `Repl` / `Both` / `Hidden` |
| `cmd.set_extra_args(p)` | `Ignore` / `Error` / `Store` |
| `.required()` | Mark option/arg required |
| `.completer(fn)` | Tab completion callback |
| `app.parse(argc, argv)` | Batch parse |
| `app.parse_fuzzy(argc, argv)` | Batch parse with typo correction |
| `ctx.get<T, Key>()` | Get value (throws if absent) |
| `ctx.has<Key>()` | Check key exists |
| `ctx.try_get<T, Key>()` | Get → `Option<T>` (no throw) |
| `ctx.matched_path()` | Matched subcommand path, e.g. `"server start"` |
| `ctx.matched_command()` | Leaf command pointer |
| `ctx.extra_args()` | Extra positional args (when policy is `Store`) |
| `format_help(cmd)` | Formatted help string |
| `format_usage(cmd)` | One-line usage string |
| `list_subcommands(cmd)` | Visible subcommand names |
| `complete(cmd, prefix)` | Tab completion candidates |

### Key types

| Type | Description |
|---|---|
| `CliResult<T>` | `Result<T, CliError>` |
| `CliFailure` | `Failure<CliError>`, implicit conversion |
| `CliError` | Parse/execution error (`std::runtime_error`) |
| `LogicError` | Programming error (`std::logic_error`) |
| `fixed_string("...")` | Compile-time string for NTTP keys |
| `Visibility::Cli / Repl / Both / Hidden` | Visibility flags |
| `ExtraArgsPolicy::Ignore / Error / Store` | Extra positional arg handling |

## Build

```cmake
cmake -B build
cmake --build build
```

Include as a submodule:

```cmake
add_subdirectory(path/to/pjh_cli)
target_link_libraries(myapp pjh_cli)
```
