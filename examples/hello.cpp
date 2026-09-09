#include <iostream>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("hello", "1.0.0", "Minimal greeting example");

    app.option<fixed_string("name")>("--name", 'n', "Who to greet", std::string("world"));

    app.action(
        [](ParseContext &ctx) -> CliResult<void>
        {
            std::cout << "Hello, " << ctx.get<std::string, fixed_string("name")>()
                      << "!\n";
            return CliResult<void>::Ok();
        });
    return app.run(argc, argv);
}
