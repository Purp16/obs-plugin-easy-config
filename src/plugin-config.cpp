#include "plugin-config.hpp"

#include <fstream>
#include <filesystem>
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

  output << "{\n"
         << "  \"baseDirectory\": \"" << json_escape(config.baseDirectory) << "\",\n"
         << "  \"pathTemplate\": \"" << json_escape(config.pathTemplate) << "\",\n"
         << "  \"manualTag\": \"" << json_escape(config.manualTag) << "\",\n"
         << "  \"autoApplyBeforeRecording\": "
         << (config.autoApplyBeforeRecording ? "true" : "false") << ",\n"
         << "  \"locale\": \"" << json_escape(config.locale) << "\"\n"
         << "}\n";

  return true;
}

} // namespace easy_config
