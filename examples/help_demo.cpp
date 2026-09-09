/// @file
/// Demo: show off all new help features.
/// Build: add_cli_example(help_demo) in examples/CMakeLists.txt

#include <iostream>
#include <pjh_cli.hpp>
#include <pjh_cli/format/help_formatter.hpp>

using namespace pjh::cli;

int main()
{
    // ── 1. Leaf: defaults + required/optional options + args ──
    std::cout << "=== 1. Leaf: defaults + required options ===\n";
    {
        LeafCommand root("copy", "Copy files with options");
        root.option<fixed_string("port")>("--port", 'p', "Port number", 8080);
        root.option<fixed_string("verbose")>("--verbose", 'v', "Verbose output")
            .boolean();
        root.option<fixed_string("timeout")>("--timeout", 't', "Timeout in seconds")
            .integer()
            .default_value(30)
            .required();
        root.arg<std::string, 0>("source", "Source file").required();
        root.arg<std::string, 1>("dest", "Destination");
        std::cout << "[usage] " << HelpFormatter::format_usage(root, "copy") << "\n\n";
        std::cout << HelpFormatter::format_help(root, "copy") << "\n";
    }

    // ── 2. Branch: subcommands ──
    std::cout << "=== 2. Branch + subcommands ===\n";
    {
        App app("pkg", "1.0", "Package manager");
        app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").count();
        auto &install = app.add_leaf("install", "Install a package");
        install.arg<std::string, 0>("name", "Package name").required();
        install.option<fixed_string("global")>("--global", 'g', "Install globally")
            .boolean()
            .default_value(true);
        auto &remove = app.add_leaf("remove", "Remove a package");
        remove.arg<std::string, 0>("name", "Package name").required();
        std::cout << HelpFormatter::format_help(app, "pkg") << "\n";
    }

    // ── 3. App::parse does not print or exit ──
    // parse() / parse_fuzzy() set ctx.help_requested() / ctx.version_requested()
    // on the returned Ok context.  The caller must dispatch before reading any
    // value: the meta-flag path skips ParseFinalizer, so defaults and env
    // fallbacks are not applied and ctx.get() may throw LogicError.
    //   auto &ctx = r.unwrap();
    //   if (ctx.help_requested()) { std::cout << ctx.help_text(); return 0; }
    //   if (ctx.version_requested()) { std::cout << ctx.version_text(); return 0; }

    std::cout << "=== 3. App::parse does not print or exit ===\n";
    std::cout << "  (caller dispatches on help_requested()/version_requested())\n";
    std::cout << "  Try: ./build/examples/help_example --help\n";
    return 0;
}
