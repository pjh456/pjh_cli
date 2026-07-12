#include <pjh_cli/app.hpp>
#include <pjh_cli/parser.hpp>

namespace pjh::cli
{
    App::App(
        std::string name,
        std::string version,
        std::string description)
        : Command(
              std::move(name),
              std::move(description)),
          m_version(std::move(version))
    {
    }

    CliResult<ParseContext>
    App::parse(
        int argc,
        char **argv)
    {
        return parse_command(*this, argc, argv);
    }

    CliResult<ParseContext>
    App::parse_fuzzy(
        int argc,
        char **argv)
    {
        return parse_command(*this, argc, argv, 3);
    }

} // namespace pjh::cli
