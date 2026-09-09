#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
#include <pjh_cli/parse/matched_path_resolver.hpp>
#include <pjh_cli/parse/parse_context.hpp>
#include <vector>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("calc", "1.0.0", "Calculator REPL example");

    // add <a> <b> [--mode rounding] — Tab completes the --mode values
    auto &add = app.add_leaf("add", "Add two numbers");
    add.option<fixed_string("mode")>("--mode", 'm', "Rounding mode")
        .str()
        .completer(
            []() -> std::vector<std::string> { return {"floor", "ceil", "round"}; });
    add.arg<int, 0>("a", "First number").required();
    add.arg<int, 1>("b", "Second number").required();
    add.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            auto a = ctx.get<int, 0>();
            auto b = ctx.get<int, 1>();
            std::cout << a << " + " << b << " = " << (a + b) << "\n";
            return CliResult<void>::Ok();
        });

    // sub <a> <b>
    auto &sub = app.add_leaf("sub", "Subtract two numbers");
    sub.arg<int, 0>("a", "First number").required();
    sub.arg<int, 1>("b", "Second number").required();
    sub.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            auto a = ctx.get<int, 0>();
            auto b = ctx.get<int, 1>();
            std::cout << a << " - " << b << " = " << (a - b) << "\n";
            return CliResult<void>::Ok();
        });

    // Batch mode: execute and exit
    if (argc > 1)
    {
        auto r = app.parse_fuzzy(argc, argv);
        if (r.is_err())
        {
            std::cerr << r.unwrap_err().what() << "\n";
            return 1;
        }
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
        for (const auto &sub_ptr : app.subcommands())
            if (MatchedPathResolver::to_path_string(ctx.matched_command()) ==
                sub_ptr->name())
            {
                auto e = sub_ptr->execute(ctx);
                if (e.is_err())
                    std::cerr << e.unwrap_err().what() << "\n";
                return 0;
            }
        return 0;
    }

    // Interactive mode (Tab completes subcommands, options, and --mode values)
    InteractiveConsole console(app, "calc>");
    console.run();
    return 0;
}
