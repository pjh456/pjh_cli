#include <pjh_cli.hpp>
#include <iostream>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("calc", "1.0.0", "Calculator REPL example");

    // add <a> <b>
    auto &add = app.add_command("add", "Add two numbers");
    add.arg<int, 0>("a", "First number").required();
    add.arg<int, 1>("b", "Second number").required();
    add.action([](ParseContext &ctx) -> ParseResult<void>
    {
        auto a = ctx.get<int, 0>();
        auto b = ctx.get<int, 1>();
        std::cout << a << " + " << b << " = " << (a + b) << "\n";
        return ParseResult<void>::Ok();
    });

    // sub <a> <b>
    auto &sub = app.add_command("sub", "Subtract two numbers");
    sub.arg<int, 0>("a", "First number").required();
    sub.arg<int, 1>("b", "Second number").required();
    sub.action([](ParseContext &ctx) -> ParseResult<void>
    {
        auto a = ctx.get<int, 0>();
        auto b = ctx.get<int, 1>();
        std::cout << a << " - " << b << " = " << (a - b) << "\n";
        return ParseResult<void>::Ok();
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
        for (const auto &sub : app.subcommands())
            if (ctx.matched_path() == sub.name())
            {
                auto e = sub.execute(ctx);
                if (e.is_err())
                    std::cerr << e.unwrap_err().what() << "\n";
                return 0;
            }
        return 0;
    }

    // Interactive mode
    InteractiveConsole console(app, "calc>");
    console.run();
    return 0;
}
