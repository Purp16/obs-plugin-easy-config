#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "plugin-config.hpp"
#include "path-template.hpp"

#include <filesystem>
#include <fstream>

using easy_config::load_plugin_config;
using easy_config::PathContext;
using easy_config::resolve_path_template;
using easy_config::sanitize_path_segment;

TEST_CASE("resolves default date and tag template under base directory")
{
  PathContext context;
  context.base = "/Volumes/Capture";
  context.date = "2026-04-29";
  context.year = "2026";
  context.month = "04";
  context.day = "29";
  context.profile = "Main";
  context.scene_collection = "Games";
  context.scene = "Desktop";
  context.tag = "Valorant";

  const auto result = resolve_path_template("{date}/{tag}", context);
  REQUIRE(result.ok);
  CHECK(result.path == "/Volumes/Capture/2026-04-29/Valorant");
}

TEST_CASE("year month and day match the date parts")
{
  PathContext context;
  context.base = "/recordings";
  context.date = "2026-04-29";
  context.year = "2026";
  context.month = "04";
  context.day = "29";
  context.tag = "match";

  const auto result = resolve_path_template("{year}/{month}/{day}/{tag}", context);
  REQUIRE(result.ok);
  CHECK(result.path == "/recordings/2026/04/29/match");
}

TEST_CASE("empty tag falls back to untagged")
{
  PathContext context;
  context.base = "/recordings";
  context.date = "2026-04-29";

  const auto result = resolve_path_template("{date}/{tag}", context);
  REQUIRE(result.ok);
  CHECK(result.path == "/recordings/2026-04-29/untagged");
}

TEST_CASE("unknown variables are rejected")
{
  PathContext context;
  context.base = "/recordings";
  context.date = "2026-04-29";
  context.tag = "demo";

  const auto result = resolve_path_template("{game}/{tag}", context);
  CHECK(!result.ok);
  REQUIRE(!result.errors.empty());
  CHECK(result.errors.front() == "Unknown variable: {game}");
}

TEST_CASE("path segments are sanitized across platforms")
{
  CHECK(sanitize_path_segment("  bad:name/with*chars?  ") == "bad_name_with_chars_");
  CHECK(sanitize_path_segment("CON") == "CON_");
  CHECK(sanitize_path_segment("name.") == "name");
}

TEST_CASE("scene and profile names with separators stay inside generated path")
{
  PathContext context;
  context.base = "/recordings";
  context.date = "2026-04-29";
  context.profile = "Main:Profile";
  context.scene_collection = "FPS/Games";
  context.scene = "Game/Window:Main";
  context.tag = "";

  const auto result =
    resolve_path_template("{profile}/{scene_collection}/{scene}/{tag}", context);
  REQUIRE(result.ok);
  CHECK(result.path == "/recordings/Main_Profile/FPS_Games/Game_Window_Main/untagged");
}

TEST_CASE("base variable is rejected because the template is relative to the base directory")
{
  PathContext context;
  context.base = "/recordings";
  context.date = "2026-04-29";

  const auto result = resolve_path_template("{base}/{date}", context);
  CHECK(!result.ok);
  REQUIRE(!result.errors.empty());
  CHECK(result.errors.front() == "Unknown variable: {base}");
}

TEST_CASE("windows style base directory keeps windows separators")
{
  PathContext context;
  context.base = "D:\\Capture";
  context.date = "2026-04-29";
  context.tag = "match";

  const auto result = resolve_path_template("{date}/{tag}", context);
  REQUIRE(result.ok);
  CHECK(result.path == "D:\\Capture\\2026-04-29\\match");
}

TEST_CASE("legacy base variable config is migrated to a relative template")
{
  const auto path = std::filesystem::temp_directory_path() / "easy-config-legacy-test.json";
  {
    std::ofstream output(path, std::ios::trunc);
    output << "{\n"
           << "  \"baseDirectory\": \"/recordings\",\n"
           << "  \"pathTemplate\": \"{base}/{date}/{tag}\",\n"
           << "  \"manualTag\": \"demo\",\n"
           << "  \"autoApplyBeforeRecording\": true,\n"
           << "  \"locale\": \"auto\"\n"
           << "}\n";
  }

  const auto config = load_plugin_config(path.string());
  std::filesystem::remove(path);
  CHECK(config.pathTemplate == "{date}/{tag}");
}
