#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace doctest {

struct TestFailure : std::exception {
  explicit TestFailure(std::string message) : message_(std::move(message)) {}
  const char *what() const noexcept override { return message_.c_str(); }
  std::string message_;
};

using TestFunc = void (*)();

struct TestCase {
  const char *name;
  TestFunc func;
};

inline std::vector<TestCase> &registry()
{
  static std::vector<TestCase> tests;
  return tests;
}

struct Registrar {
  Registrar(const char *name, TestFunc func) { registry().push_back({name, func}); }
};

inline void check(bool value, const char *expr, const char *file, int line)
{
  if (value)
    return;

  std::ostringstream out;
  out << file << ':' << line << ": check failed: " << expr;
  throw TestFailure(out.str());
}

inline int run_tests()
{
  int failures = 0;
  for (const auto &test : registry()) {
    try {
      test.func();
      std::cout << "[pass] " << test.name << '\n';
    } catch (const std::exception &error) {
      ++failures;
      std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
    }
  }
  return failures == 0 ? 0 : 1;
}

} // namespace doctest

#define DOCTEST_JOIN_IMPL(a, b) a##b
#define DOCTEST_JOIN(a, b) DOCTEST_JOIN_IMPL(a, b)
#define TEST_CASE(name)                                                        \
  static void DOCTEST_JOIN(test_func_, __LINE__)();                            \
  static doctest::Registrar DOCTEST_JOIN(test_reg_, __LINE__)(name,            \
                                                              DOCTEST_JOIN(test_func_, __LINE__)); \
  static void DOCTEST_JOIN(test_func_, __LINE__)()
#define CHECK(expr) doctest::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define REQUIRE(expr) CHECK(expr)

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
int main()
{
  return doctest::run_tests();
}
#endif
