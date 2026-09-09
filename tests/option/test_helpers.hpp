#ifndef TESTS_OPTION_TEST_HELPERS_HPP
#define TESTS_OPTION_TEST_HELPERS_HPP

#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

namespace test_env
{
    /// @brief Read @p name from the process environment.
    /// @param name Environment variable name.
    /// @return The current value, or std::nullopt when absent.
    inline std::optional<std::string> read(const std::string &name)
    {
        if (const char *value = std::getenv(name.c_str()))
            return std::string(value);
        return std::nullopt;
    }

    /// @brief Force @p name to @p value in the process environment.
    /// @param name Environment variable name.
    /// @param value New value, or std::nullopt to remove the variable.
    /// @note Performs no allocation, so it is safe to call from a destructor.
    inline void write(
        const std::string &name, const std::optional<std::string> &value) noexcept
    {
#ifdef _WIN32
        ::_putenv_s(name.c_str(), value ? value->c_str() : "");
#else
        if (value)
            ::setenv(name.c_str(), value->c_str(), 1);
        else
            ::unsetenv(name.c_str());
#endif
    }
}  // namespace test_env

/// @brief RAII guard that sets (or removes) a process environment variable for
///        a test and restores the previous value on destruction.
///
/// Construct before the App / EnvSnapshot under test, because EnvSnapshot
/// captures the environment at construction.  Restoration runs on normal
/// return, on REQUIRE-failure unwinding, and on exceptions, so a case can never
/// leak a variable into another case in the same process.
class ScopedEnvVar
{
public:
    /// @param name  Environment variable name.
    /// @param value New value, or std::nullopt to remove the variable.
    ScopedEnvVar(std::string name, std::optional<std::string> value) :
        m_name(std::move(name)), m_previous(test_env::read(m_name))
    {
        test_env::write(m_name, value);
    }

    ~ScopedEnvVar() { test_env::write(m_name, m_previous); }

    ScopedEnvVar(const ScopedEnvVar &) = delete;
    ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

    const std::string &name() const noexcept { return m_name; }

private:
    std::string m_name;
    std::optional<std::string> m_previous;
};

#endif  // TESTS_OPTION_TEST_HELPERS_HPP
