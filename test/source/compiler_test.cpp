#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Test test", "[library]")
{
  auto const name = "hello";
  REQUIRE(name == "hello");
}
