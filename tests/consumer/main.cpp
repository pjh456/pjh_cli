#include <pjh_cli.hpp>

int main(int argc, char **argv)
{
    pjh::cli::App app("consumer", "0.0.0", "install/export smoke test");
    auto r = app.parse(argc, argv);
    return r.is_ok() ? 0 : 1;
}
