#pragma once

#include "path-template.hpp"
#include "plugin-config.hpp"

#include <QObject>
#include <QString>
#include <QStringList>

namespace easy_config {

struct VideoResolution {
  int width = 0;
  int height = 0;
};

struct ReplayBufferSettings {
  int seconds = 0;
  int megabytes = 0;
};

class ObsController : public QObject {
  Q_OBJECT

public:
  explicit ObsController(QObject *parent = nullptr);
  ~ObsController() override;

  void markFrontendExiting();
  void markFrontendReady();
  void detachFrontendCallbacks();
  bool isFrontendReady() const;
  bool isFrontendExiting() const;

  QStringList sceneNames() const;
  QStringList sceneCollectionNames() const;
  QStringList profileNames() const;

  QString currentSceneName() const;
  QString currentSceneCollectionName() const;
  QString currentProfileName() const;
  QString currentRecordingDirectory() const;

  bool setCurrentScene(const QString &name, QString *error);
  bool setCurrentSceneCollection(const QString &name, QString *error);
  bool setCurrentProfile(const QString &name, QString *error);

  bool outputsActive() const;
  VideoResolution currentOutputResolution() const;
  double currentFps() const;
  ReplayBufferSettings currentReplayBufferSettings() const;
  bool setOutputResolution(const ResolutionPreset &preset, QString *error) const;
  bool setFps(const FpsPreset &preset, QString *error) const;
  bool setReplayBufferSettings(const ReplayBufferSettings &settings, QString *error) const;

  QString pluginConfigPath() const;
  PluginConfig loadConfig() const;
  bool saveConfig(const PluginConfig &config, QString *error) const;

  PathContext makePathContext(const PluginConfig &config) const;
  PathResolveResult previewRecordingPath(const PluginConfig &config) const;
  bool applyRecordingPath(const PluginConfig &config, QString *resolvedPath,
                          QString *error) const;

signals:
  void obsStateChanged();
  void recordingStarting();
  void replayBufferStarting();

private:
  bool frontendCallbacksAttached_ = false;
  bool frontendReady_ = false;
  bool frontendExiting_ = false;

};

} // namespace easy_config
