#include <iostream>
#include <pjh_cli.hpp>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("options", "1.0.0", "Option type examples");

    // Bool flag
    app.option<fixed_string("verbose")>("--verbose", 'v', "Enable verbose output")
        .boolean();

    // Valued option with short name and default
    app.option<fixed_string("port")>("--port", 'p', "Port number", 8080);

    // String with default
    app.option<fixed_string("host")>(
        "--host", 'H', "Host address", std::string("localhost"));

    // Integer with range validation
    app.option<fixed_string("level")>("--level", "Log level 0-4").integer().min(0).max(4);

    // Counting flag: -vvv → 3
    app.option<fixed_string("verbose2")>("--verbose2", 'V', "Verbosity level").count();

    // Required string option
    app.option<fixed_string("token")>("--token", "API token").str().required();

    auto r = app.parse(argc, argv);
    if (r.is_err())
    {
        std::cerr << r.unwrap_err().what() << "\n";
        return 1;
    }

    auto &ctx = r.unwrap();

    std::cout << "verbose: "
              << (ctx.has<fixed_string("verbose")>()
                      ? (ctx.get<bool, fixed_string("verbose")>() ? "true" : "false")
                      : "(not set)")
              << "\n"
              << "port:    " << ctx.get<int, fixed_string("port")>() << "\n"
              << "host:    " << ctx.get<std::string, fixed_string("host")>() << "\n"
              << "token:   " << ctx.get<std::string, fixed_string("token")>() << "\n";

    // Level with range —
    if (ctx.has<fixed_string("level")>())
        std::cout << "level:   " << ctx.get<int, fixed_string("level")>() << "\n";

    // Counting flag — use try_get (no throw)
    auto v = ctx.try_get<int, fixed_string("verbose2")>();
    std::cout << "verbose2: " << (v.is_some() ? std::to_string(v.unwrap()) : "0") << "\n";

    return 0;
}
