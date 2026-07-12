#include <pjh_cli.hpp>
#include <iostream>

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("options", "1.0.0", "Option type examples");

    // Bool flag
    app.option<
        bool,
        fixed_string("verbose")>(
        "--verbose",
        'v',
        "Enable verbose output");

    // Valued option with short name
    app.option<
        int,
        fixed_string("port")>(
        "--port",
        'p',
        "Port number",
        8080);

    // String with default
    app.option<
        std::string,
        fixed_string("host")>(
        "--host",
        'H',
        "Host address",
        std::string("localhost"));

    // Required option
    app.option<
           std::string,
           fixed_string("token")>(
           "--token",
           "API token")
        .required();

    auto r = app.parse(argc, argv);
    if (r.is_err())
    {
        std::cerr
            << r.unwrap_err().what()
            << "\n";
        return 1;
    }

    auto &ctx = r.unwrap();
    std::cout << "verbose: "
              << (ctx.has<fixed_string("verbose")>()
                      ? (ctx.get<
                             bool,
                             fixed_string("verbose")>()
                             ? "true"
                             : "false")
                      : "(not set)")
              << "\n"
              << "port:    " << ctx.get<int, fixed_string("port")>() << "\n"
              << "host:    " << ctx.get<std::string, fixed_string("host")>() << "\n"
              << "token:   " << ctx.get<std::string, fixed_string("token")>() << "\n";
    return 0;
}
