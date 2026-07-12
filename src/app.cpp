#include <pjh_cli/app.hpp>

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

} // namespace pjh::cli
