#ifndef INCLUDE_PJH_CLI_DETAIL_ENV_SNAPSHOT_HPP
#define INCLUDE_PJH_CLI_DETAIL_ENV_SNAPSHOT_HPP

#include <string>
#include <string_view>
#include <unordered_map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <cwchar>
#include <windows.h>
#else
extern char **environ;
#endif

namespace pjh::cli::detail
{

    class EnvSnapshot
    {
        std::unordered_map<std::string, std::string> m_env;

    public:
        EnvSnapshot()
        {
#ifdef _WIN32
            auto *block = GetEnvironmentStringsW();
            if (!block)
                return;
            for (auto *env = block; *env; env += std::wcslen(env) + 1)
            {
                std::wstring_view entry(env);
                auto eq = entry.find(L'=');
                if (eq != std::wstring_view::npos)
                    m_env.emplace(
                        to_utf8(entry.substr(0, eq)), to_utf8(entry.substr(eq + 1)));
            }
            FreeEnvironmentStringsW(block);
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

        const std::string *get(std::string_view name) const noexcept
        {
            auto it = m_env.find(std::string(name));
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
