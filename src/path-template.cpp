#include "path-template.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

namespace easy_config {
namespace {

bool is_known_variable(const std::string &name)
{
  static const std::set<std::string> known = {
    "date", "year", "month", "day",
    "profile", "scene_collection", "scene", "tag",
  };
  return known.find(name) != known.end();
}

bool is_windows_reserved_name(std::string value)
{
  const auto dot = value.find('.');
  if (dot != std::string::npos)
    value = value.substr(0, dot);

  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });

  static const std::set<std::string> reserved = {
    "CON", "PRN", "AUX", "NUL",
    "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
    "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
  };

  return reserved.find(value) != reserved.end();
}

std::string collapse_separators(std::string value)
{
  std::string collapsed;
  bool last_was_separator = false;

  for (char c : value) {
    const bool separator = c == '/';
    if (separator && last_was_separator)
      continue;
    collapsed.push_back(c);
    last_was_separator = separator;
  }

  return collapsed;
}

std::vector<std::string> split_path(const std::string &value)
{
  std::vector<std::string> parts;
  std::stringstream stream(value);
  std::string part;

  while (std::getline(stream, part, '/'))
    parts.push_back(part);

  if (!value.empty() && value.back() == '/')
    parts.emplace_back();

  return parts;
}

std::string join_path(const std::vector<std::string> &parts, bool absolute)
{
  std::string joined = absolute ? "/" : "";

  for (const auto &part : parts) {
    if (part.empty())
      continue;
    if (!joined.empty() && joined.back() != '/')
      joined.push_back('/');
    joined += part;
  }

  return joined;
}

std::string strip_trailing_separators(std::string value)
{
  while (value.size() > 1 && (value.back() == '/' || value.back() == '\\'))
    value.pop_back();
  return value;
}

std::string join_base_and_relative_path(std::string base,
                                        std::string relative)
{
  base = strip_trailing_separators(std::move(base));
  while (!relative.empty() && relative.front() == '/')
    relative.erase(relative.begin());

  if (relative.empty())
    return base;

  if (base.empty())
    return relative;

  const char separator = base.find('\\') != std::string::npos ? '\\' : '/';
  if (separator == '\\')
    std::replace(relative.begin(), relative.end(), '/', '\\');
  return base + separator + relative;
}

} // namespace

std::string trim(std::string value)
{
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };

  value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char c) {
    return !is_space(static_cast<unsigned char>(c));
  }));

  value.erase(std::find_if(value.rbegin(), value.rend(), [&](char c) {
    return !is_space(static_cast<unsigned char>(c));
  }).base(), value.end());

  return value;
}

std::string sanitize_path_segment(const std::string &segment)
{
  std::string sanitized;
  sanitized.reserve(segment.size());

  for (const char c : trim(segment)) {
    const unsigned char uc = static_cast<unsigned char>(c);
    const bool control = uc < 32;
    const bool illegal = c == '<' || c == '>' || c == ':' || c == '"' ||
                         c == '/' || c == '\\' || c == '|' || c == '?' ||
                         c == '*';

    if (control || illegal) {
      if (sanitized.empty() || sanitized.back() != '_')
        sanitized.push_back('_');
      continue;
    }

    sanitized.push_back(c);
  }

  while (!sanitized.empty() && (sanitized.back() == '.' || sanitized.back() == ' '))
    sanitized.pop_back();

  if (sanitized.empty())
    sanitized = "untitled";

  if (is_windows_reserved_name(sanitized))
    sanitized += "_";

  return sanitized;
}

PathContext normalize_context(PathContext context)
{
  context.base = trim(context.base);
  context.date = trim(context.date);
  context.year = trim(context.year);
  context.month = trim(context.month);
  context.day = trim(context.day);
  context.profile = sanitize_path_segment(context.profile);
  context.scene_collection = sanitize_path_segment(context.scene_collection);
  context.scene = sanitize_path_segment(context.scene);
  context.tag = sanitize_path_segment(context.tag.empty() ? "untagged" : context.tag);
  return context;
}

std::map<std::string, std::string> variables_from_context(const PathContext &context)
{
  return {
    {"date", sanitize_path_segment(context.date)},
    {"year", sanitize_path_segment(context.year)},
    {"month", sanitize_path_segment(context.month)},
    {"day", sanitize_path_segment(context.day)},
    {"profile", sanitize_path_segment(context.profile)},
    {"scene_collection", sanitize_path_segment(context.scene_collection)},
    {"scene", sanitize_path_segment(context.scene)},
    {"tag", sanitize_path_segment(context.tag.empty() ? "untagged" : context.tag)},
  };
}

PathResolveResult resolve_path_template(const std::string &path_template,
                                        PathContext context)
{
  PathResolveResult result;
  context = normalize_context(std::move(context));
  const std::string trimmed_template = trim(path_template);

  if (context.base.empty())
    result.errors.emplace_back("Base directory is required.");

  if (trimmed_template.empty())
    result.errors.emplace_back("Path template is required.");

  const auto variables = variables_from_context(context);
  std::string expanded;

  for (std::size_t i = 0; i < trimmed_template.size();) {
    if (trimmed_template[i] != '{') {
      char c = trimmed_template[i++];
      expanded.push_back(c == '\\' ? '/' : c);
      continue;
    }

    const auto close = trimmed_template.find('}', i + 1);
    if (close == std::string::npos) {
      result.errors.emplace_back("Unclosed variable in path template.");
      break;
    }

    const std::string name = trimmed_template.substr(i + 1, close - i - 1);
    if (!is_known_variable(name)) {
      result.errors.emplace_back("Unknown variable: {" + name + "}");
      i = close + 1;
      continue;
    }

    expanded += variables.at(name);
    i = close + 1;
  }

  expanded = collapse_separators(expanded);
  const bool absolute = !expanded.empty() && expanded.front() == '/';
  std::vector<std::string> sanitized_parts;

  auto parts = split_path(expanded);
  for (std::size_t i = 0; i < parts.size(); ++i) {
    const auto &part = parts[i];
    if (part.empty())
      continue;
    if (i == 0 && part.size() == 2 && std::isalpha(static_cast<unsigned char>(part[0])) && part[1] == ':') {
      sanitized_parts.push_back(part);
      continue;
    }
    sanitized_parts.push_back(sanitize_path_segment(part));
  }

  result.path = join_path(sanitized_parts, absolute);
  result.path = join_base_and_relative_path(context.base, result.path);
  result.ok = result.errors.empty() && !result.path.empty();
  return result;
}

} // namespace easy_config
