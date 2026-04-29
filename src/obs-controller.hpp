#pragma once

#include "path-template.hpp"
#include "plugin-config.hpp"

#include <QObject>
#include <QString>
#include <QStringList>

namespace easy_config {

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

private:
  bool frontendCallbacksAttached_ = false;
  bool frontendReady_ = false;
  bool frontendExiting_ = false;

};

} // namespace easy_config
