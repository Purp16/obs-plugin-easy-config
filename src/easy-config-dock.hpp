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
class QResizeEvent;
class QScrollArea;
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
  void openAboutDialog();

protected:
  void resizeEvent(QResizeEvent *event) override;

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
  void setPreviewText(const QString &path, bool error = false);
  void setStatusText(const QString &message, bool error = false);
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
  QSpinBox *replaySecondsSpin_ = nullptr;
  QSpinBox *replayMegabytesSpin_ = nullptr;
  QPushButton *applyReplayButton_ = nullptr;
  QLabel *previewLabel_ = nullptr;
  QLabel *statusLabel_ = nullptr;
  QScrollArea *scrollArea_ = nullptr;
  QWidget *profileControl_ = nullptr;
  QWidget *sceneCollectionControl_ = nullptr;
  QWidget *resolutionControl_ = nullptr;
  QWidget *fpsControl_ = nullptr;
  QWidget *replayControl_ = nullptr;
  QWidget *baseDirectoryControl_ = nullptr;
  QWidget *pathTemplateControl_ = nullptr;
  QWidget *manualTagControl_ = nullptr;
  QWidget *previewControl_ = nullptr;
  QWidget *statusControl_ = nullptr;
  QWidget *profileSceneSection_ = nullptr;
  QWidget *videoPresetSection_ = nullptr;
  QWidget *replaySection_ = nullptr;
  QWidget *pathSection_ = nullptr;
  QWidget *contentWidget_ = nullptr;
  QPushButton *settingsButton_ = nullptr;
  QMenu *settingsMenu_ = nullptr;
  QAction *showProfileAction_ = nullptr;
  QAction *showSceneCollectionAction_ = nullptr;
  QAction *showResolutionPresetsAction_ = nullptr;
  QAction *showFpsPresetsAction_ = nullptr;
  QAction *showReplayBufferAction_ = nullptr;
  QAction *showPathAutomationAction_ = nullptr;
  QAction *showPreviewStatusAction_ = nullptr;
  std::vector<QPushButton *> resolutionButtons_;
  std::vector<QPushButton *> fpsButtons_;
  std::vector<QWidget *> wrapSections_;
};

} // namespace easy_config
