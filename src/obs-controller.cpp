#include "obs-controller.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/bmem.h>
#include <util/config-file.h>

#include <QDate>

#include <filesystem>

namespace easy_config {
namespace {

QString obsString(const char *value)
{
  return value ? QString::fromUtf8(value, -1) : QString();
}

QStringList stringListFromObs(char **values)
{
  QStringList list;
  if (!values)
    return list;

  for (char **value = values; *value; ++value)
    list << QString::fromUtf8(*value, -1);

  bfree(values);
  return list;
}

std::string toStdString(const QString &value)
{
  const QByteArray utf8 = value.toUtf8();
  return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString configPath()
{
  char *path = obs_module_config_path("easy-config.json");
  QString qpath = obsString(path);
  bfree(path);
  return qpath;
}

QString recordingConfigKey(config_t *config)
{
  const QString mode = obsString(config_get_string(config, "Output", "Mode"));
  if (mode.compare("Advanced", Qt::CaseInsensitive) != 0)
    return "SimpleOutput/FilePath";

  const QString recType = obsString(config_get_string(config, "AdvOut", "RecType"));
  if (recType.compare("FFmpeg", Qt::CaseInsensitive) == 0)
    return "AdvOut/FFFilePath";
  if (recType.compare("ffmpeg", Qt::CaseInsensitive) == 0)
    return "AdvOut/FFFilePath";

  return "AdvOut/RecFilePath";
}

bool setConfigPath(config_t *config, const QString &compoundKey,
                   const QString &path, QString *error)
{
  const auto parts = compoundKey.split('/');
  if (parts.size() != 2) {
    if (error)
      *error = "Internal error: invalid OBS recording path key.";
    return false;
  }

  config_set_string(config, parts[0].toUtf8().constData(),
                    parts[1].toUtf8().constData(), path.toUtf8().constData());
  return config_save_safe(config, "tmp", nullptr) == CONFIG_SUCCESS;
}

QString getConfigPath(config_t *config, const QString &compoundKey)
{
  const auto parts = compoundKey.split('/');
  if (parts.size() != 2)
    return {};

  return obsString(config_get_string(config, parts[0].toUtf8().constData(),
                                    parts[1].toUtf8().constData()));
}

void frontendEventThunk(enum obs_frontend_event event, void *data)
{
  auto *controller = static_cast<ObsController *>(data);
  if (!controller)
    return;

  switch (event) {
  case OBS_FRONTEND_EVENT_EXIT:
    controller->markFrontendExiting();
    controller->detachFrontendCallbacks();
    break;
  case OBS_FRONTEND_EVENT_FINISHED_LOADING:
    controller->markFrontendReady();
    emit controller->obsStateChanged();
    break;
  case OBS_FRONTEND_EVENT_SCENE_CHANGED:
  case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
  case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
    emit controller->obsStateChanged();
    break;
  case OBS_FRONTEND_EVENT_RECORDING_STARTING:
    emit controller->recordingStarting();
    break;
  default:
    break;
  }
}

} // namespace

ObsController::ObsController(QObject *parent) : QObject(parent)
{
  obs_frontend_add_event_callback(frontendEventThunk, this);
  frontendCallbacksAttached_ = true;
}

ObsController::~ObsController()
{
  if (!frontendExiting_)
    detachFrontendCallbacks();
}

void ObsController::markFrontendExiting()
{
  frontendExiting_ = true;
}

void ObsController::markFrontendReady()
{
  frontendReady_ = true;
}

void ObsController::detachFrontendCallbacks()
{
  if (!frontendCallbacksAttached_)
    return;

  obs_frontend_remove_event_callback(frontendEventThunk, this);
  frontendCallbacksAttached_ = false;
}

bool ObsController::isFrontendExiting() const
{
  return frontendExiting_;
}

bool ObsController::isFrontendReady() const
{
  return frontendReady_ && !frontendExiting_;
}

QStringList ObsController::sceneNames() const
{
  if (!isFrontendReady())
    return {};

  return stringListFromObs(obs_frontend_get_scene_names());
}

QStringList ObsController::sceneCollectionNames() const
{
  if (!isFrontendReady())
    return {};

  return stringListFromObs(obs_frontend_get_scene_collections());
}

QStringList ObsController::profileNames() const
{
  if (!isFrontendReady())
    return {};

  return stringListFromObs(obs_frontend_get_profiles());
}

QString ObsController::currentSceneName() const
{
  if (!isFrontendReady())
    return {};

  obs_source_t *scene = obs_frontend_get_current_scene();
  QString name;
  if (scene) {
    name = obsString(obs_source_get_name(scene));
    obs_source_release(scene);
  }
  return name;
}

QString ObsController::currentSceneCollectionName() const
{
  if (!isFrontendReady())
    return {};

  char *value = obs_frontend_get_current_scene_collection();
  const QString name = obsString(value);
  bfree(value);
  return name;
}

QString ObsController::currentProfileName() const
{
  if (!isFrontendReady())
    return {};

  char *value = obs_frontend_get_current_profile();
  const QString name = obsString(value);
  bfree(value);
  return name;
}

QString ObsController::currentRecordingDirectory() const
{
  if (!isFrontendReady())
    return {};

  config_t *profileConfig = obs_frontend_get_profile_config();
  if (!profileConfig)
    return {};

  return getConfigPath(profileConfig, recordingConfigKey(profileConfig));
}

bool ObsController::setCurrentScene(const QString &name, QString *error)
{
  obs_frontend_source_list scenes = {};
  obs_frontend_get_scenes(&scenes);

  for (size_t i = 0; i < scenes.sources.num; ++i) {
    obs_source_t *source = scenes.sources.array[i];
    if (name == obsString(obs_source_get_name(source))) {
      obs_frontend_set_current_scene(source);
      obs_frontend_source_list_free(&scenes);
      return true;
    }
  }

  obs_frontend_source_list_free(&scenes);
  if (error)
    *error = "Scene not found: " + name;
  return false;
}

bool ObsController::setCurrentSceneCollection(const QString &name, QString *error)
{
  if (name.isEmpty()) {
    if (error)
      *error = "Scene collection is required.";
    return false;
  }

  obs_frontend_set_current_scene_collection(name.toUtf8().constData());
  return true;
}

bool ObsController::setCurrentProfile(const QString &name, QString *error)
{
  if (name.isEmpty()) {
    if (error)
      *error = "Profile is required.";
    return false;
  }

  obs_frontend_set_current_profile(name.toUtf8().constData());
  return true;
}

QString ObsController::pluginConfigPath() const
{
  return configPath();
}

PluginConfig ObsController::loadConfig() const
{
  return load_plugin_config(toStdString(pluginConfigPath()));
}

bool ObsController::saveConfig(const PluginConfig &config, QString *error) const
{
  std::string saveError;
  const bool ok = save_plugin_config(toStdString(pluginConfigPath()), config, &saveError);
  if (!ok && error)
    *error = QString::fromUtf8(saveError.c_str(), -1);
  return ok;
}

PathContext ObsController::makePathContext(const PluginConfig &config) const
{
  const QDate today = QDate::currentDate();
  PathContext context;
  context.base = config.baseDirectory;
  context.date = toStdString(today.toString("yyyy-MM-dd"));
  context.year = toStdString(today.toString("yyyy"));
  context.month = toStdString(today.toString("MM"));
  context.day = toStdString(today.toString("dd"));
  context.profile = toStdString(currentProfileName());
  context.scene_collection = toStdString(currentSceneCollectionName());
  context.scene = toStdString(currentSceneName());
  context.tag = config.manualTag;
  return context;
}

PathResolveResult ObsController::previewRecordingPath(const PluginConfig &config) const
{
  return resolve_path_template(config.pathTemplate, makePathContext(config));
}

bool ObsController::applyRecordingPath(const PluginConfig &config, QString *resolvedPath,
                                       QString *error) const
{
  const auto result = previewRecordingPath(config);
  if (!result.ok) {
    if (error)
      *error = QString::fromUtf8(result.errors.empty() ? "Unable to resolve path." : result.errors.front().c_str(), -1);
    return false;
  }

  const QString path = QString::fromUtf8(result.path.c_str(), -1);
  std::error_code ec;
  if (!std::filesystem::create_directories(result.path, ec) &&
      !std::filesystem::exists(result.path)) {
    if (error)
      *error = "Unable to create directory: " + path;
    return false;
  }

  config_t *profileConfig = obs_frontend_get_profile_config();
  if (!profileConfig) {
    if (error)
      *error = "Unable to access OBS profile config.";
    return false;
  }

  const QString key = recordingConfigKey(profileConfig);
  if (!setConfigPath(profileConfig, key, path, error)) {
    if (error && error->isEmpty())
      *error = "Unable to save OBS recording path.";
    return false;
  }

  if (resolvedPath)
    *resolvedPath = path;
  return true;
}

} // namespace easy_config
