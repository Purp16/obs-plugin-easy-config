#pragma once

#include "obs-controller.hpp"
#include "plugin-config.hpp"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;

namespace easy_config {

class EasyConfigDock : public QWidget {
  Q_OBJECT

public:
  explicit EasyConfigDock(ObsController *controller, QWidget *parent = nullptr);

private slots:
  void refreshObsState();
  void browseBaseDirectory();
  void updatePreview();
  void applyBeforeRecording();

private:
  PluginConfig configFromUi() const;
  void setUiFromConfig(const PluginConfig &config);
  void saveCurrentConfig();
  void setPreviewText(const QString &message, bool error = false);
  void refillCombo(QComboBox *combo, const QStringList &items, const QString &current);

  ObsController *controller_ = nullptr;
  QComboBox *sceneCollectionCombo_ = nullptr;
  QComboBox *profileCombo_ = nullptr;
  QLineEdit *baseDirectoryEdit_ = nullptr;
  QLineEdit *pathTemplateEdit_ = nullptr;
  QLineEdit *manualTagEdit_ = nullptr;
  QCheckBox *enablePathAutomationCheck_ = nullptr;
  QLabel *previewLabel_ = nullptr;
};

} // namespace easy_config
