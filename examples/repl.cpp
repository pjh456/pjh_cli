#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/console.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/core/type.hpp>
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

    // Batch mode: one-shot parse + dispatch + execute + exit code
    if (argc > 1)
        return app.run_fuzzy(argc, argv);

    // Interactive mode (Tab completes subcommands, options, and --mode values)
    InteractiveConsole console(app, "calc>");
    console.run();
    return 0;
}
