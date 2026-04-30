#pragma once

#include <map>
#include <string>
#include <vector>

namespace easy_config {

struct PathContext {
  std::string base;
  std::string date;
  std::string datetime;
  std::string year;
  std::string month;
  std::string day;
  std::string time;
  std::string hour;
  std::string minute;
  std::string second;
  std::string profile;
  std::string scene_collection;
  std::string scene;
  std::string tag;
};

struct PathResolveResult {
  bool ok = false;
  std::string path;
  std::vector<std::string> errors;
};

std::string trim(std::string value);
std::string sanitize_path_segment(const std::string &segment);
PathContext normalize_context(PathContext context);
std::map<std::string, std::string> variables_from_context(const PathContext &context);
PathResolveResult resolve_path_template(const std::string &path_template,
                                        PathContext context);

} // namespace easy_config
