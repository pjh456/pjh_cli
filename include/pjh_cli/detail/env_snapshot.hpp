#ifndef INCLUDE_PJH_CLI_DETAIL_ENV_SNAPSHOT_HPP
#define INCLUDE_PJH_CLI_DETAIL_ENV_SNAPSHOT_HPP

#include <memory>
#include <pjh_cli/detail/string_utils.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <cwchar>
#include <windows.h>
#else
extern "C" char **environ;
#endif

namespace pjh::cli::detail
{

    class EnvSnapshot
    {
        // Transparent key/equality: get() looks up with string_view directly,
        // so no temporary std::string key is built per lookup.
        std::unordered_map<
            std::string,
            std::string,
            transparent_string_hash,
            std::equal_to<void>>
            m_env;

    public:
        EnvSnapshot()
        {
#ifdef _WIN32
            std::unique_ptr<wchar_t, decltype(&FreeEnvironmentStringsW)> block{
                GetEnvironmentStringsW(), &FreeEnvironmentStringsW};
            if (!block)
                return;
            for (auto *env = block.get(); *env; env += std::wcslen(env) + 1)
            {
                std::wstring_view entry(env);
                auto eq = entry.find(L'=');
                if (eq != std::wstring_view::npos)
                    m_env.emplace(
                        to_utf8(entry.substr(0, eq)), to_utf8(entry.substr(eq + 1)));
            }
#else
            if (!environ)
                return;
            for (auto **env = environ; *env; ++env)
            {
                std::string_view entry(*env);
                auto eq = entry.find('=');
                if (eq != std::string_view::npos)
                    m_env.emplace(entry.substr(0, eq), entry.substr(eq + 1));
            }
#endif
        }

        /// @brief Look up @p name in the captured environment.
        ///
        /// Heterogeneous lookup: no temporary key is built, so the lookup
        /// allocates nothing and cannot throw.
        ///
        /// @param name Environment variable name.
        /// @return Pointer to the value, or nullptr if absent.
        const std::string *get(std::string_view name) const noexcept
        {
            auto it = m_env.find(name);
            if (it == m_env.end())
                return nullptr;
            return &it->second;
        }

    private:
#ifdef _WIN32
        static std::string to_utf8(std::wstring_view wsv)
        {
            if (wsv.empty())
                return {};
            int len = WideCharToMultiByte(
                CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()), nullptr, 0, nullptr,
                nullptr);
            if (len <= 0)
                return {};
            std::string result(static_cast<size_t>(len), '\0');
            WideCharToMultiByte(
                CP_UTF8, 0, wsv.data(), static_cast<int>(wsv.size()), result.data(), len,
                nullptr, nullptr);
            return result;
        }
#endif
    };

}  // namespace pjh::cli::detail

#endif
