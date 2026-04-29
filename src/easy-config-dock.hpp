#pragma once

#include "obs-controller.hpp"
#include "plugin-config.hpp"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QLabel;
class QLayout;
class QMenu;
class QPushButton;
class QSpinBox;
class QAction;

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
  void applyResolutionPreset();
  void applyFpsPreset();
  void editResolutionPresets();
  void editFpsPresets();
  void applyReplayBufferSettings();
  void updateSectionVisibility();

private:
  PluginConfig configFromUi() const;
  void setUiFromConfig(const PluginConfig &config);
  void buildSettingsMenu();
  void refreshPresetButtons();
  void refreshOutputControls();
  void refreshReplayBufferSettings();
  int resolutionPresetIndex(QObject *sender) const;
  int fpsPresetIndex(QObject *sender) const;
  void saveCurrentConfig();
  void setPreviewText(const QString &message, bool error = false);
  void refillCombo(QComboBox *combo, const QStringList &items, const QString &current);

  ObsController *controller_ = nullptr;
  PluginConfig config_;
  QComboBox *sceneCollectionCombo_ = nullptr;
  QComboBox *profileCombo_ = nullptr;
  QLineEdit *baseDirectoryEdit_ = nullptr;
  QLineEdit *pathTemplateEdit_ = nullptr;
  QLineEdit *manualTagEdit_ = nullptr;
  QCheckBox *enablePathAutomationCheck_ = nullptr;
  QLayout *resolutionButtonLayout_ = nullptr;
  QLayout *fpsButtonLayout_ = nullptr;
  QPushButton *editResolutionButton_ = nullptr;
  QPushButton *editFpsButton_ = nullptr;
  QSpinBox *replaySecondsSpin_ = nullptr;
  QSpinBox *replayMegabytesSpin_ = nullptr;
  QPushButton *applyReplayButton_ = nullptr;
  QLabel *previewLabel_ = nullptr;
  QWidget *profileSceneSection_ = nullptr;
  QWidget *videoPresetSection_ = nullptr;
  QWidget *replaySection_ = nullptr;
  QWidget *pathSection_ = nullptr;
  QWidget *previewSection_ = nullptr;
  QPushButton *settingsButton_ = nullptr;
  QMenu *settingsMenu_ = nullptr;
  QAction *showProfileSceneAction_ = nullptr;
  QAction *showVideoPresetsAction_ = nullptr;
  QAction *showReplayBufferAction_ = nullptr;
  QAction *showPathAutomationAction_ = nullptr;
  QAction *showPreviewStatusAction_ = nullptr;
  std::vector<QPushButton *> resolutionButtons_;
  std::vector<QPushButton *> fpsButtons_;
};

} // namespace easy_config
