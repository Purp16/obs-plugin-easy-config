#pragma once

#include <vector>
#include <string>

namespace easy_config {

struct ResolutionPreset {
  std::string label;
  int width = 0;
  int height = 0;

  bool operator==(const ResolutionPreset &other) const
  {
    return label == other.label && width == other.width && height == other.height;
  }
};

struct FpsPreset {
  std::string label;
  double fps = 0.0;

  bool operator==(const FpsPreset &other) const
  {
    return label == other.label && fps == other.fps;
  }
};

struct PluginConfig {
  std::string baseDirectory;
  std::string pathTemplate = "{date}/{tag}";
  std::string manualTag = "untagged";
  bool autoApplyBeforeRecording = true;
  std::string locale = "auto";
  std::vector<ResolutionPreset> resolutionPresets = {
    {"720p", 1280, 720},
    {"1080p", 1920, 1080},
    {"1440p", 2560, 1440},
    {"4K", 3840, 2160},
  };
  std::vector<FpsPreset> fpsPresets = {
    {"30", 30},
    {"60", 60},
    {"120", 120},
  };
  int lastReplayBufferSeconds = 20;
  int lastReplayBufferMegabytes = 512;
  bool showProfile = true;
  bool showSceneCollection = true;
  bool showResolutionPresets = true;
  bool showFpsPresets = true;
  bool showReplayBuffer = true;
  bool showPathAutomation = true;
  bool showPreviewStatus = true;
};

std::vector<ResolutionPreset>
normalize_resolution_presets(const std::vector<ResolutionPreset> &presets);
std::vector<FpsPreset>
normalize_fps_presets(const std::vector<FpsPreset> &presets);
std::string format_fps_value(double fps);

PluginConfig load_plugin_config(const std::string &path);
bool save_plugin_config(const std::string &path, const PluginConfig &config,
                        std::string *error = nullptr);

} // namespace easy_config
