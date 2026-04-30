#include "plugin-config.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>

namespace easy_config {
namespace {

std::string read_file(const std::string &path)
{
  std::ifstream input(path);
  if (!input)
    return {};

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::string json_escape(const std::string &value)
{
  std::string escaped;
  escaped.reserve(value.size());

  for (char c : value) {
    switch (c) {
    case '\\':
      escaped += "\\\\";
      break;
    case '"':
      escaped += "\\\"";
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    case '\t':
      escaped += "\\t";
      break;
    default:
      escaped.push_back(c);
      break;
    }
  }

  return escaped;
}

std::string json_string_value(const std::string &json, const std::string &key,
                              const std::string &fallback)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos)
    return fallback;

  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos)
    return fallback;

  pos = json.find('"', pos + 1);
  if (pos == std::string::npos)
    return fallback;

  std::string value;
  bool escaped = false;
  for (++pos; pos < json.size(); ++pos) {
    const char c = json[pos];
    if (escaped) {
      switch (c) {
      case 'n':
        value.push_back('\n');
        break;
      case 'r':
        value.push_back('\r');
        break;
      case 't':
        value.push_back('\t');
        break;
      default:
        value.push_back(c);
        break;
      }
      escaped = false;
      continue;
    }

    if (c == '\\') {
      escaped = true;
      continue;
    }

    if (c == '"')
      return value;

    value.push_back(c);
  }

  return fallback;
}

bool json_bool_value(const std::string &json, const std::string &key, bool fallback)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos)
    return fallback;

  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos)
    return fallback;

  const auto first = json.find_first_not_of(" \t\r\n", pos + 1);
  if (first == std::string::npos)
    return fallback;

  if (json.compare(first, 4, "true") == 0)
    return true;
  if (json.compare(first, 5, "false") == 0)
    return false;

  return fallback;
}

std::optional<std::size_t> json_value_start(const std::string &json,
                                            const std::string &key)
{
  const std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos)
    return std::nullopt;

  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos)
    return std::nullopt;

  const auto first = json.find_first_not_of(" \t\r\n", pos + 1);
  if (first == std::string::npos)
    return std::nullopt;

  return first;
}

std::string json_scope(const std::string &json, std::size_t start, char open,
                       char close)
{
  if (start >= json.size() || json[start] != open)
    return {};

  int depth = 0;
  bool inString = false;
  bool escaped = false;
  for (std::size_t i = start; i < json.size(); ++i) {
    const char c = json[i];
    if (inString) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        inString = false;
      }
      continue;
    }

    if (c == '"') {
      inString = true;
      continue;
    }
    if (c == open) {
      ++depth;
      continue;
    }
    if (c == close) {
      --depth;
      if (depth == 0)
        return json.substr(start, i - start + 1);
    }
  }

  return {};
}

std::string json_array_value(const std::string &json, const std::string &key)
{
  const auto start = json_value_start(json, key);
  if (!start)
    return {};
  return json_scope(json, *start, '[', ']');
}

std::string json_object_value(const std::string &json, const std::string &key)
{
  const auto start = json_value_start(json, key);
  if (!start)
    return {};
  return json_scope(json, *start, '{', '}');
}

int json_int_value(const std::string &json, const std::string &key, int fallback)
{
  const auto start = json_value_start(json, key);
  if (!start)
    return fallback;

  const auto end = json.find_first_not_of("-0123456789", *start);
  const std::string value = json.substr(*start, end - *start);
  if (value.empty() || value == "-")
    return fallback;

  try {
    return std::stoi(value);
  } catch (...) {
    return fallback;
  }
}

double json_double_value(const std::string &json, const std::string &key,
                         double fallback)
{
  const auto start = json_value_start(json, key);
  if (!start)
    return fallback;

  const auto end = json.find_first_not_of("-0123456789.", *start);
  const std::string value = json.substr(*start, end - *start);
  if (value.empty() || value == "-" || value == ".")
    return fallback;

  try {
    return std::stod(value);
  } catch (...) {
    return fallback;
  }
}

std::vector<std::string> json_array_objects(const std::string &array)
{
  std::vector<std::string> objects;
  if (array.empty())
    return objects;

  for (std::size_t i = 0; i < array.size(); ++i) {
    if (array[i] != '{')
      continue;

    const std::string object = json_scope(array, i, '{', '}');
    if (object.empty())
      break;

    objects.push_back(object);
    i += object.size() - 1;
  }

  return objects;
}

std::string migrate_relative_template(std::string value)
{
  const std::string basePrefix = "{base}/";
  if (value == "{base}")
    return {};
  if (value.rfind(basePrefix, 0) == 0)
    return value.substr(basePrefix.size());
  return value;
}

} // namespace

std::string format_fps_value(double fps)
{
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << fps;
  std::string value = stream.str();
  while (value.size() > 1 && value.back() == '0')
    value.pop_back();
  if (!value.empty() && value.back() == '.')
    value.pop_back();
  return value;
}

std::vector<ResolutionPreset>
normalize_resolution_presets(const std::vector<ResolutionPreset> &presets)
{
  std::vector<ResolutionPreset> normalized;
  for (const auto &preset : presets) {
    if (preset.label.empty() || preset.width <= 0 || preset.height <= 0)
      continue;
    normalized.push_back(preset);
  }

  if (normalized.empty())
    return PluginConfig().resolutionPresets;
  return normalized;
}

std::vector<FpsPreset> normalize_fps_presets(const std::vector<FpsPreset> &presets)
{
  std::vector<FpsPreset> normalized;
  for (const auto &preset : presets) {
    if (preset.fps <= 0.0)
      continue;
    FpsPreset normalizedPreset = preset;
    normalizedPreset.label = format_fps_value(preset.fps);
    normalized.push_back(normalizedPreset);
  }

  if (normalized.empty())
    return PluginConfig().fpsPresets;
  return normalized;
}

PluginConfig load_plugin_config(const std::string &path)
{
  PluginConfig config;
  const std::string json = read_file(path);
  if (json.empty())
    return config;

  config.baseDirectory = json_string_value(json, "baseDirectory", config.baseDirectory);
  config.pathTemplate =
    migrate_relative_template(json_string_value(json, "pathTemplate", config.pathTemplate));
  config.manualTag = json_string_value(json, "manualTag", config.manualTag);
  config.autoApplyBeforeRecording =
    json_bool_value(json, "autoApplyBeforeRecording", config.autoApplyBeforeRecording);
  config.locale = json_string_value(json, "locale", config.locale);
  config.lastReplayBufferSeconds =
    json_int_value(json, "lastReplayBufferSeconds", config.lastReplayBufferSeconds);
  config.lastReplayBufferMegabytes =
    json_int_value(json, "lastReplayBufferMegabytes", config.lastReplayBufferMegabytes);
  config.showProfile = json_bool_value(
    json, "showProfile",
    json_bool_value(json, "showProfileSceneCollection", config.showProfile));
  config.showSceneCollection = json_bool_value(
    json, "showSceneCollection",
    json_bool_value(json, "showProfileSceneCollection", config.showSceneCollection));
  config.showResolutionPresets = json_bool_value(
    json, "showResolutionPresets",
    json_bool_value(json, "showVideoPresets", config.showResolutionPresets));
  config.showFpsPresets = json_bool_value(
    json, "showFpsPresets",
    json_bool_value(json, "showVideoPresets", config.showFpsPresets));
  config.showReplayBuffer =
    json_bool_value(json, "showReplayBuffer", config.showReplayBuffer);
  config.showPathAutomation =
    json_bool_value(json, "showPathAutomation", config.showPathAutomation);
  config.showPreviewStatus =
    json_bool_value(json, "showPreviewStatus", config.showPreviewStatus);

  std::vector<ResolutionPreset> resolutionPresets;
  for (const auto &object : json_array_objects(json_array_value(json, "resolutionPresets"))) {
    ResolutionPreset preset;
    preset.label = json_string_value(object, "label", {});
    preset.width = json_int_value(object, "width", 0);
    preset.height = json_int_value(object, "height", 0);
    resolutionPresets.push_back(preset);
  }
  if (!resolutionPresets.empty())
    config.resolutionPresets = normalize_resolution_presets(resolutionPresets);

  std::vector<FpsPreset> fpsPresets;
  for (const auto &object : json_array_objects(json_array_value(json, "fpsPresets"))) {
    FpsPreset preset;
    preset.label = json_string_value(object, "label", {});
    preset.fps = json_double_value(object, "fps", 0.0);
    fpsPresets.push_back(preset);
  }
  if (!fpsPresets.empty())
    config.fpsPresets = normalize_fps_presets(fpsPresets);

  return config;
}

bool save_plugin_config(const std::string &path, const PluginConfig &config,
                        std::string *error)
{
  std::error_code ec;
  const auto parent = std::filesystem::path(path).parent_path();
  if (!parent.empty())
    std::filesystem::create_directories(parent, ec);

  if (ec) {
    if (error)
      *error = "Unable to create plugin config directory: " + parent.string();
    return false;
  }

  std::ofstream output(path, std::ios::trunc);
  if (!output) {
    if (error)
      *error = "Unable to write plugin config: " + path;
    return false;
  }

  const auto resolutionPresets = normalize_resolution_presets(config.resolutionPresets);
  const auto fpsPresets = normalize_fps_presets(config.fpsPresets);

  output << "{\n"
         << "  \"baseDirectory\": \"" << json_escape(config.baseDirectory) << "\",\n"
         << "  \"pathTemplate\": \"" << json_escape(config.pathTemplate) << "\",\n"
         << "  \"manualTag\": \"" << json_escape(config.manualTag) << "\",\n"
         << "  \"autoApplyBeforeRecording\": "
         << (config.autoApplyBeforeRecording ? "true" : "false") << ",\n"
         << "  \"locale\": \"" << json_escape(config.locale) << "\",\n"
         << "  \"resolutionPresets\": [\n";

  for (std::size_t i = 0; i < resolutionPresets.size(); ++i) {
    const auto &preset = resolutionPresets[i];
    output << "    {\"label\": \"" << json_escape(preset.label)
           << "\", \"width\": " << preset.width
           << ", \"height\": " << preset.height << "}"
           << (i + 1 == resolutionPresets.size() ? "\n" : ",\n");
  }

  output << "  ],\n"
         << "  \"fpsPresets\": [\n";

  for (std::size_t i = 0; i < fpsPresets.size(); ++i) {
    const auto &preset = fpsPresets[i];
    output << "    {\"label\": \"" << json_escape(preset.label)
           << "\", \"fps\": " << format_fps_value(preset.fps) << "}"
           << (i + 1 == fpsPresets.size() ? "\n" : ",\n");
  }

  output << "  ],\n"
         << "  \"lastReplayBufferSeconds\": "
         << config.lastReplayBufferSeconds << ",\n"
         << "  \"lastReplayBufferMegabytes\": "
         << config.lastReplayBufferMegabytes << ",\n"
         << "  \"showProfile\": "
         << (config.showProfile ? "true" : "false") << ",\n"
         << "  \"showSceneCollection\": "
         << (config.showSceneCollection ? "true" : "false") << ",\n"
         << "  \"showResolutionPresets\": "
         << (config.showResolutionPresets ? "true" : "false") << ",\n"
         << "  \"showFpsPresets\": "
         << (config.showFpsPresets ? "true" : "false") << ",\n"
         << "  \"showReplayBuffer\": "
         << (config.showReplayBuffer ? "true" : "false") << ",\n"
         << "  \"showPathAutomation\": "
         << (config.showPathAutomation ? "true" : "false") << ",\n"
         << "  \"showPreviewStatus\": "
         << (config.showPreviewStatus ? "true" : "false") << "\n"
         << "}\n";

  return true;
}

} // namespace easy_config
