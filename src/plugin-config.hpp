#pragma once

#include <string>

namespace easy_config {

struct PluginConfig {
  std::string baseDirectory;
  std::string pathTemplate = "{date}/{tag}";
  std::string manualTag = "untagged";
  bool autoApplyBeforeRecording = true;
  std::string locale = "auto";
};

PluginConfig load_plugin_config(const std::string &path);
bool save_plugin_config(const std::string &path, const PluginConfig &config,
                        std::string *error = nullptr);

} // namespace easy_config
