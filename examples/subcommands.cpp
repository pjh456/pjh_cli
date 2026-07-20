#include <iostream>

#include "pjh_cli/app.hpp"
#include "pjh_cli/fixed_string.hpp"

using namespace pjh::cli;

int main(int argc, char **argv)
{
    App app("deploy", "1.0.0", "Subcommand tree example");

    // Parent-level option — must appear BEFORE the subcommand name
    app.option<fixed_string("dryrun")>("--dry-run", "Dry run (no-op)").boolean();

    // server start [--port N] [--daemon]
    auto &server = app.add_branch("server", "Server management");
    auto &server_start = server.add_leaf("start", "Start server");
    server_start.option<fixed_string("port")>("--port", 'p', "Port", 8080);
    server_start.option<fixed_string("daemon")>("--daemon", 'd', "Run as daemon")
        .boolean();

    server.add_leaf("stop", "Stop server");

    // config show [--format FMT]
    auto &config = app.add_branch("config", "Configuration commands");
    auto &config_show = config.add_leaf("show", "Show configuration");
    config_show.option<fixed_string("format")>(
        "--format", 'f', "Output format", std::string("yaml"));

    auto r = app.parse(argc, argv);
    if (r.is_err())
    {
        std::cerr << r.unwrap_err().what() << "\n";
        return 1;
    }

    auto &ctx = r.unwrap();
    std::cout << "path: " << ctx.matched_path();

    if (ctx.has<fixed_string("dryrun")>())
        std::cout << ", dry-run";

    if (ctx.has<fixed_string("port")>())
        std::cout << ", port: " << ctx.get<int, fixed_string("port")>();
    if (ctx.has<fixed_string("daemon")>())
        std::cout << ", daemon: " << ctx.get<bool, fixed_string("daemon")>();
    if (ctx.has<fixed_string("format")>())
        std::cout << ", format: " << ctx.get<std::string, fixed_string("format")>();
    std::cout << "\n";
    return 0;
}
