#include <doctest/doctest.h>

#include <optional>
#include <pjh_cli/app.hpp>
#include <pjh_cli/core/fixed_string.hpp>
#include <pjh_cli/parse/parser.hpp>
#include <string>

#include "option/test_helpers.hpp"
#include "parser/test_helpers.hpp"

using namespace pjh::cli;

TEST_CASE("App captures the environment at construction (snapshot semantics)")
{
    ScopedEnvVar guard{"PJH_CLI_TEST_ENV_TIMING", "before"};
    App app("test", "1.0", "timing");
    app.option<fixed_string("v")>("--v", "V").str().env(guard.name());

    // Mutate the live environment after construction: the App snapshot must
    // keep the value captured at construction time.
    test_env::write(guard.name(), std::optional<std::string>{"after"});

    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    CHECK(r.unwrap().get<std::string, fixed_string("v")>() == "before");
}

TEST_CASE("EnvSnapshot keeps a set-but-empty variable distinct from a missing one")
{
    ScopedEnvVar empty{"PJH_CLI_TEST_ENV_EMPTY", ""};
    ScopedEnvVar missing{"PJH_CLI_TEST_ENV_MISSING", std::nullopt};

    detail::EnvSnapshot snap;
    const auto *present = snap.get(empty.name());
    REQUIRE(present != nullptr);
    CHECK(present->empty());
    CHECK(snap.get(missing.name()) == nullptr);

    App app("test", "1.0", "empty vs missing");
    app.option<fixed_string("empty")>("--empty", "Empty").str().env(empty.name());
    app.option<fixed_string("missing")>("--missing", "Missing").str().env(missing.name());

    Argv argv{"test"};
    auto r = app.parse(argv.argc(), argv.argv());
    REQUIRE(r.is_ok());
    auto &ctx = r.unwrap();
    CHECK(ctx.has<fixed_string("empty")>());
    CHECK(ctx.get<std::string, fixed_string("empty")>().empty());
    CHECK_FALSE(ctx.has<fixed_string("missing")>());
}
