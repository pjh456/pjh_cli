#include <iostream>
#include <string_view>

#include <pjh_cli/app.hpp>
#include <pjh_cli/fixed_string.hpp>
#include <pjh_cli/help_formatter.hpp>
#include <pjh_cli/matcher.hpp>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("pkg", "1.0.0", "Package manager example with fuzzy matching");

    app.option<fixed_string("verbose")>("--verbose", 'v', "Verbose").count();

    auto &install = app.add_leaf("install", "Install a package");
    install.arg<std::string, 0>("name", "Package name").required();
    install.option<fixed_string("global")>("--global", 'g', "Install globally").boolean();

    auto &remove = app.add_leaf("remove", "Remove a package");
    remove.arg<std::string, 0>("name", "Package name").required();

    auto &search = app.add_leaf("search", "Search packages");
    search.arg<std::string, 0>("query", "Search query").required();

    if (argc < 2 || std::string_view(argv[1]) == "--help" ||
        std::string_view(argv[1]) == "-h")
    {
        std::cout << HelpFormatter::format_help(app, "pkg");
        return 0;
    }

    auto r = app.parse_fuzzy(argc, argv);
    if (r.is_err())
    {
        std::cout << HelpFormatter::format_usage(app, "pkg") << "\n";
        std::cout << "Try: pkg install <name>\n";
        std::cerr << r.unwrap_err().what() << "\n";
        return 1;
    }

    auto &ctx = r.unwrap();
    std::cout << "command: " << ctx.matched_path() << "\n";

    auto v = ctx.try_get<int, fixed_string("verbose")>();
    if (v.is_some())
        std::cout << "  verbose count: " << v.unwrap() << "\n";

    if (ctx.has<fixed_string("global")>())
        std::cout << "  --global\n";

    if (ctx.has<0>())
        std::cout << "  arg: " << ctx.get<std::string, 0>() << "\n";
    return 0;
}
