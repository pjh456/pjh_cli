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

    ParseResult<ParseContext>
    App::parse(
        int argc,
        char **argv)
    {
        std::vector<std::string_view> args;
        for (int a = 1; a < argc; a++)
            args.emplace_back(argv[a]);
        return parse_command(*this, args);
    }

    ParseResult<ParseContext>
    App::parse_fuzzy(
        int argc,
        char **argv)
    {
        std::vector<std::string_view> args;
        for (int a = 1; a < argc; a++)
            args.emplace_back(argv[a]);
        return parse_command_fuzzy(*this, args);
    }

} // namespace pjh::cli
