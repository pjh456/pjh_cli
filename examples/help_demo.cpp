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

    // ── 3. App::parse with --help (auto-detection) ──
    // When --help / -h is in argv, App::parse() / parse_fuzzy()
    // prints format_help() and calls std::exit(0) automatically.
    // Uncomment to see it in action (will exit after this line):
    // char *argv[] = { (char*)"demo", (char*)"--help" };
    // App("demo", "1.0", "Demo").parse(2, argv);

    std::cout << "=== 3. App::parse auto --help ===\n";
    std::cout << "  (runs when --help/-h in argv, prints then exit(0))\n";
    std::cout << "  Try: echo ${your_app} --help | xargs ./build/examples/help_example\n";
    return 0;
}
