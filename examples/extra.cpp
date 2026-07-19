#include <iostream>
#include <pjh_cli.hpp>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("archive", "1.0.0", "Extra args and -- separator example");

    app.set_extra_args(ExtraArgsPolicy::Store);

    app.option<bool, fixed_string("compress")>("--compress", 'z', "Compress archive");
    app.arg<std::string, 0>("output", "Archive file name").required();

    auto r = app.parse(argc, argv);
    if (r.is_err())
    {
        std::cerr << r.unwrap_err().what() << "\n";
        return 1;
    }

    auto &ctx = r.unwrap();
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
