#include <iostream>
#include <pjh_cli/command/base_command.hpp>
#include <pjh_cli/command/command_builder.hpp>
#include <pjh_cli/command/leaf_command.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parser.hpp>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    LeafCommand app("archive", "Extra args and -- separator example");

    app.set_extra_args(ExtraArgsPolicy::Store);

    app.option<fixed_string("compress")>("--compress", 'z', "Compress archive").boolean();
    app.arg<std::string, 0>("output", "Archive file name").required();

    auto r = Parser::parse_command(app, argc, argv);
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
    // LeafCommand has no version; --version still short-circuits the parse.
    if (ctx.version_requested())
    {
        std::cout << ctx.version_text();
        return 0;
    }
    std::cout << "output: " << ctx.get<std::string, 0>() << "\n";

    auto extra = ctx.extra_args();
    if (!extra.empty())
    {
        std::cout << "input files:";
        for (const auto &f : extra) std::cout << " " << f;
        std::cout << "\n";
    }
    return 0;
}
