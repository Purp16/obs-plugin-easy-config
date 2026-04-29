#include "easy-config-dock.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyle>
#include <QToolTip>
#include <QVBoxLayout>

#include <obs-module.h>

namespace easy_config {
namespace {

constexpr int kControlSpacing = 6;

QString trText(const char *key)
{
  return QString::fromUtf8(obs_module_text(key), -1);
}

QString localizedError(const std::string &error)
{
  if (error == "Base directory is required.")
    return trText("ErrorBaseDirectoryRequired");
  if (error == "Path template is required.")
    return trText("ErrorPathTemplateRequired");
  if (error == "Unclosed variable in path template.")
    return trText("ErrorUnclosedVariable");

  const std::string unknownPrefix = "Unknown variable: ";
  if (error.rfind(unknownPrefix, 0) == 0) {
    QString message = trText("ErrorUnknownVariable");
    message.replace(QLatin1String("%1"),
                    QString::fromUtf8(error.substr(unknownPrefix.size()).c_str(), -1));
    return message;
  }

  return QString::fromUtf8(error.c_str(), -1);
}

std::string toStdStringCompat(const QString &value)
{
  const QByteArray utf8 = value.toUtf8();
  return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString templateHelpText()
{
  return trText("PathTemplateHelp");
}

void makeCompact(QWidget *widget, int height)
{
  widget->setFixedHeight(height);
  widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

} // namespace

EasyConfigDock::EasyConfigDock(ObsController *controller, QWidget *parent)
  : QWidget(parent), controller_(controller)
{
  sceneCollectionCombo_ = new QComboBox(this);
  profileCombo_ = new QComboBox(this);
  baseDirectoryEdit_ = new QLineEdit(this);
  pathTemplateEdit_ = new QLineEdit(this);
  manualTagEdit_ = new QLineEdit(this);
  enablePathAutomationCheck_ = new QCheckBox(trText("EnablePathAutomation"), this);
  previewLabel_ = new QLabel(this);

  pathTemplateEdit_->setPlaceholderText(QLatin1String("{date}/{tag}"));
  pathTemplateEdit_->setToolTip(templateHelpText());
  manualTagEdit_->setPlaceholderText(trText("ManualTagPlaceholder"));

  previewLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  previewLabel_->setWordWrap(true);

  auto *browseButton = new QPushButton(trText("Browse"), this);
  const int controlHeight = browseButton->sizeHint().height();
  makeCompact(sceneCollectionCombo_, controlHeight);
  makeCompact(profileCombo_, controlHeight);
  makeCompact(baseDirectoryEdit_, controlHeight);
  makeCompact(pathTemplateEdit_, controlHeight);
  makeCompact(manualTagEdit_, controlHeight);
  browseButton->setFixedHeight(controlHeight);
  enablePathAutomationCheck_->setFixedHeight(controlHeight);

  auto *baseLayout = new QHBoxLayout();
  baseLayout->setContentsMargins(0, 0, 0, 0);
  baseLayout->setSpacing(kControlSpacing);
  baseLayout->addWidget(baseDirectoryEdit_, 1);
  baseLayout->addWidget(browseButton);

  auto *templateHelpButton = new QPushButton(QLatin1String("?"), this);
  templateHelpButton->setToolTip(templateHelpText());
  templateHelpButton->setAccessibleName(trText("PathTemplateHelpTitle"));
  templateHelpButton->setFixedSize(controlHeight, controlHeight);
  templateHelpButton->setFocusPolicy(Qt::NoFocus);
  connect(templateHelpButton, &QPushButton::clicked, this, [templateHelpButton]() {
    QToolTip::showText(templateHelpButton->mapToGlobal(
                         QPoint(0, templateHelpButton->height())),
                       templateHelpText(), templateHelpButton);
  });

  auto *templateLayout = new QHBoxLayout();
  templateLayout->setContentsMargins(0, 0, 0, 0);
  templateLayout->setSpacing(kControlSpacing);
  templateLayout->addWidget(pathTemplateEdit_, 1);
  templateLayout->addWidget(templateHelpButton);

  auto *form = new QFormLayout();
  form->setContentsMargins(0, 0, 0, 0);
  form->setHorizontalSpacing(style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));
  form->setVerticalSpacing(kControlSpacing);
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form->addRow(trText("Profile"), profileCombo_);
  form->addRow(trText("SceneCollection"), sceneCollectionCombo_);
  form->addRow(trText("BaseDirectory"), baseLayout);
  form->addRow(trText("PathTemplate"), templateLayout);
  form->addRow(trText("ManualTag"), manualTagEdit_);
  form->addRow(trText("Preview"), previewLabel_);
  form->addRow(QString(), enablePathAutomationCheck_);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(kControlSpacing);
  layout->addLayout(form);
  layout->addStretch(1);

  setUiFromConfig(controller_->loadConfig());
  refreshObsState();

  connect(sceneCollectionCombo_, &QComboBox::currentTextChanged, this, [this](const QString &value) {
    QString error;
    if (!controller_->setCurrentSceneCollection(value, &error))
      setPreviewText(error, true);
    updatePreview();
  });
  connect(profileCombo_, &QComboBox::currentTextChanged, this, [this](const QString &value) {
    QString error;
    if (!controller_->setCurrentProfile(value, &error))
      setPreviewText(error, true);
    updatePreview();
  });
  connect(browseButton, &QPushButton::clicked, this, &EasyConfigDock::browseBaseDirectory);
  connect(baseDirectoryEdit_, &QLineEdit::textChanged, this, &EasyConfigDock::updatePreview);
  connect(pathTemplateEdit_, &QLineEdit::textChanged, this, &EasyConfigDock::updatePreview);
  connect(manualTagEdit_, &QLineEdit::textChanged, this, &EasyConfigDock::updatePreview);
  connect(enablePathAutomationCheck_, &QCheckBox::toggled, this, [this]() {
    updatePreview();
    saveCurrentConfig();
  });
  connect(controller_, &ObsController::obsStateChanged, this, &EasyConfigDock::refreshObsState);
  connect(controller_, &ObsController::recordingStarting, this, &EasyConfigDock::applyBeforeRecording);

  updatePreview();
}

void EasyConfigDock::refreshObsState()
{
  refillCombo(sceneCollectionCombo_, controller_->sceneCollectionNames(),
              controller_->currentSceneCollectionName());
  refillCombo(profileCombo_, controller_->profileNames(), controller_->currentProfileName());
  updatePreview();
}

void EasyConfigDock::browseBaseDirectory()
{
  const QString directory = QFileDialog::getExistingDirectory(
    this, trText("ChooseBaseDirectory"), baseDirectoryEdit_->text());
  if (!directory.isEmpty())
    baseDirectoryEdit_->setText(directory);
}

void EasyConfigDock::updatePreview()
{
  const PluginConfig config = configFromUi();
  const auto result = controller_->previewRecordingPath(config);
  if (result.ok) {
    setPreviewText(QString::fromUtf8(result.path.c_str(), -1));
  } else {
    if (!result.errors.empty())
      setPreviewText(localizedError(result.errors.front()), true);
    else
      setPreviewText(QString::fromUtf8(result.path.c_str(), -1), true);
  }
  saveCurrentConfig();
}

void EasyConfigDock::applyBeforeRecording()
{
  const PluginConfig config = configFromUi();
  if (!config.autoApplyBeforeRecording)
    return;

  QString path;
  QString error;
  if (!controller_->applyRecordingPath(config, &path, &error)) {
    setPreviewText(error, true);
    return;
  }

  setPreviewText(path);
}

PluginConfig EasyConfigDock::configFromUi() const
{
  PluginConfig config;
  config.baseDirectory = toStdStringCompat(baseDirectoryEdit_->text());
  config.pathTemplate = toStdStringCompat(pathTemplateEdit_->text());
  config.manualTag = toStdStringCompat(manualTagEdit_->text());
  config.autoApplyBeforeRecording = enablePathAutomationCheck_->isChecked();
  config.locale = "auto";
  return config;
}

void EasyConfigDock::setUiFromConfig(const PluginConfig &config)
{
  QString baseDirectory = QString::fromUtf8(config.baseDirectory.c_str(), -1);
  if (baseDirectory.isEmpty())
    baseDirectory = controller_->currentRecordingDirectory();
  baseDirectoryEdit_->setText(baseDirectory);
  pathTemplateEdit_->setText(QString::fromUtf8(config.pathTemplate.c_str(), -1));
  manualTagEdit_->setText(QString::fromUtf8(config.manualTag.c_str(), -1));
  enablePathAutomationCheck_->setChecked(config.autoApplyBeforeRecording);
}

void EasyConfigDock::saveCurrentConfig()
{
  QString error;
  if (!controller_->saveConfig(configFromUi(), &error))
    setPreviewText(error, true);
}

void EasyConfigDock::setPreviewText(const QString &message, bool error)
{
  previewLabel_->setText(message);
  previewLabel_->setStyleSheet(error ? "color: #c62828;" : "color: palette(text);");
}

void EasyConfigDock::refillCombo(QComboBox *combo, const QStringList &items,
                                 const QString &current)
{
  const QSignalBlocker blocker(combo);
  combo->clear();
  combo->addItems(items);
  const int index = combo->findText(current);
  if (index >= 0)
    combo->setCurrentIndex(index);
}

} // namespace easy_config
