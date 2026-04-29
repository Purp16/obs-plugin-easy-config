#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "plugin-config.hpp"
#include "path-template.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>

using easy_config::FpsPreset;
using easy_config::load_plugin_config;
using easy_config::normalize_fps_presets;
using easy_config::normalize_resolution_presets;
using easy_config::PathContext;
using easy_config::PluginConfig;
using easy_config::ResolutionPreset;
using easy_config::resolve_path_template;
using easy_config::save_plugin_config;
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

TEST_CASE("plugin config preserves next version presets and notification settings")
{
  const auto path = std::filesystem::temp_directory_path() / "easy-config-v2-test.json";
  PluginConfig config;
  config.baseDirectory = "/recordings";
  config.pathTemplate = "{date}/{scene}";
  config.manualTag = "demo";
  config.resolutionPresets = {
    {"HD", 1280, 720},
    {"Full HD", 1920, 1080},
  };
  config.fpsPresets = {
    {"29.97", 29.97},
    {"59.94", 59.94},
  };
  config.lastReplayBufferSeconds = 90;
  config.lastReplayBufferMegabytes = 2048;
  config.showProfileSceneCollection = false;
  config.showVideoPresets = true;
  config.showReplayBuffer = false;
  config.showPathAutomation = true;
  config.showPreviewStatus = false;

  REQUIRE(save_plugin_config(path.string(), config));
  const auto loaded = load_plugin_config(path.string());
  std::filesystem::remove(path);

  REQUIRE(loaded.resolutionPresets.size() == 2);
  CHECK(loaded.resolutionPresets[0].label == "HD");
  CHECK(loaded.resolutionPresets[0].width == 1280);
  CHECK(loaded.resolutionPresets[0].height == 720);
  REQUIRE(loaded.fpsPresets.size() == 2);
  CHECK(loaded.fpsPresets[1].label == "59.94");
  CHECK(std::abs(loaded.fpsPresets[1].fps - 59.94) < 0.001);
  CHECK(loaded.lastReplayBufferSeconds == 90);
  CHECK(loaded.lastReplayBufferMegabytes == 2048);
  CHECK(!loaded.showProfileSceneCollection);
  CHECK(loaded.showVideoPresets);
  CHECK(!loaded.showReplayBuffer);
  CHECK(loaded.showPathAutomation);
  CHECK(!loaded.showPreviewStatus);
}

TEST_CASE("preset normalization rejects invalid entries without capping visible presets")
{
  const auto resolutions = normalize_resolution_presets({
    {"bad", 0, 720},
    {"720p", 1280, 720},
    {"1080p", 1920, 1080},
    {"1440p", 2560, 1440},
    {"4K", 3840, 2160},
    {"8K", 7680, 4320},
  });

  REQUIRE(resolutions.size() == 5);
  CHECK(resolutions.front().label == "720p");
  CHECK(resolutions.back().label == "8K");

  const auto fps = normalize_fps_presets({
    {"bad", 0.0},
    {"custom", 29.97},
    {"60", 60.0},
    {"120", 120.0},
    {"144", 144.0},
    {"240", 240.0},
  });

  REQUIRE(fps.size() == 5);
  CHECK(fps.front().label == "29.97");
  CHECK(std::abs(fps.front().fps - 29.97) < 0.001);
  CHECK(std::abs(fps.back().fps - 240.0) < 0.001);
}

TEST_CASE("preset normalization keeps one valid preset and falls back when none remain")
{
  const auto resolutions = normalize_resolution_presets({
    {"bad", -1, 720},
    {"only", 1920, 1080},
  });
  const auto fps = normalize_fps_presets({
    {"bad", -1.0},
    {"only", 60.0},
  });

  REQUIRE(resolutions.size() == 1);
  CHECK(resolutions[0].label == "only");
  REQUIRE(fps.size() == 1);
  CHECK(std::abs(fps[0].fps - 60.0) < 0.001);

  CHECK(normalize_resolution_presets({{"bad", -1, 720}}) ==
        PluginConfig().resolutionPresets);
  CHECK(normalize_fps_presets({{"bad", -1.0}}) == PluginConfig().fpsPresets);
}
