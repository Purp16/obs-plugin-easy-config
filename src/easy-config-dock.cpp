#include "easy-config-dock.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QFontMetrics>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStyle>
#include <QToolTip>
#include <QVBoxLayout>

#include <obs-module.h>

#include <algorithm>
#include <cmath>
#include <functional>

namespace easy_config {
namespace {

constexpr int kControlSpacing = 6;
constexpr int kFormMargin = 8;

class FlowLayout : public QLayout {
public:
  explicit FlowLayout(QWidget *parent = nullptr, int margin = 0,
                      int horizontalSpacing = kControlSpacing,
                      int verticalSpacing = kControlSpacing)
    : QLayout(parent), horizontalSpacing_(horizontalSpacing),
      verticalSpacing_(verticalSpacing)
  {
    setContentsMargins(margin, margin, margin, margin);
  }

  ~FlowLayout() override
  {
    QLayoutItem *item = nullptr;
    while ((item = takeAt(0)) != nullptr)
      delete item;
  }

  void addItem(QLayoutItem *item) override { items_.push_back(item); }
  int count() const override { return static_cast<int>(items_.size()); }

  QLayoutItem *itemAt(int index) const override
  {
    return index >= 0 && index < count() ? items_[index] : nullptr;
  }

  QLayoutItem *takeAt(int index) override
  {
    if (index < 0 || index >= count())
      return nullptr;
    QLayoutItem *item = items_[index];
    items_.erase(items_.begin() + index);
    return item;
  }

  Qt::Orientations expandingDirections() const override { return {}; }
  bool hasHeightForWidth() const override { return true; }

  int heightForWidth(int width) const override
  {
    return doLayout(QRect(0, 0, width, 0), true);
  }

  QSize sizeHint() const override { return minimumSize(); }

  QSize minimumSize() const override
  {
    QSize size;
    for (const QLayoutItem *item : items_)
      size = size.expandedTo(item->minimumSize());

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(),
                  margins.top() + margins.bottom());
    return size;
  }

  void setGeometry(const QRect &rect) override
  {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
  }

private:
  int doLayout(const QRect &rect, bool testOnly) const
  {
    const QMargins margins = contentsMargins();
    const QRect effective = rect.adjusted(margins.left(), margins.top(),
                                          -margins.right(), -margins.bottom());
    int x = effective.x();
    int y = effective.y();
    int lineHeight = 0;

    for (QLayoutItem *item : items_) {
      const QSize hint = item->sizeHint();
      const int nextX = x + hint.width() + horizontalSpacing_;
      if (nextX - horizontalSpacing_ > effective.right() && lineHeight > 0) {
        x = effective.x();
        y += lineHeight + verticalSpacing_;
        lineHeight = 0;
      }

      if (!testOnly)
        item->setGeometry(QRect(QPoint(x, y), hint));

      x += hint.width() + horizontalSpacing_;
      lineHeight = std::max(lineHeight, hint.height());
    }

    return y + lineHeight - rect.y() + margins.bottom();
  }

  std::vector<QLayoutItem *> items_;
  int horizontalSpacing_ = kControlSpacing;
  int verticalSpacing_ = kControlSpacing;
};

class ResponsivePairLayout : public QLayout {
public:
  explicit ResponsivePairLayout(QWidget *parent = nullptr, int margin = 0,
                                int spacing = kControlSpacing)
    : QLayout(parent), spacing_(spacing)
  {
    setContentsMargins(margin, margin, margin, margin);
  }

  ~ResponsivePairLayout() override
  {
    QLayoutItem *item = nullptr;
    while ((item = takeAt(0)) != nullptr)
      delete item;
  }

  void addItem(QLayoutItem *item) override { items_.push_back(item); }
  int count() const override { return static_cast<int>(items_.size()); }

  QLayoutItem *itemAt(int index) const override
  {
    return index >= 0 && index < count() ? items_[index] : nullptr;
  }

  QLayoutItem *takeAt(int index) override
  {
    if (index < 0 || index >= count())
      return nullptr;
    QLayoutItem *item = items_[index];
    items_.erase(items_.begin() + index);
    return item;
  }

  Qt::Orientations expandingDirections() const override { return Qt::Horizontal; }
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }
  QSize sizeHint() const override { return minimumSize(); }

  QSize minimumSize() const override
  {
    QSize size;
    for (const QLayoutItem *item : visibleItems())
      size = size.expandedTo(item->minimumSize());

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(),
                  margins.top() + margins.bottom());
    return size;
  }

  void setGeometry(const QRect &rect) override
  {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
  }

private:
  std::vector<QLayoutItem *> visibleItems() const
  {
    std::vector<QLayoutItem *> visible;
    for (QLayoutItem *item : items_) {
      if (!item->widget() || item->widget()->isVisible())
        visible.push_back(item);
    }
    return visible;
  }

  int doLayout(const QRect &rect, bool testOnly) const
  {
    const auto items = visibleItems();
    const QMargins margins = contentsMargins();
    const QRect effective = rect.adjusted(margins.left(), margins.top(),
                                          -margins.right(), -margins.bottom());
    if (items.empty())
      return margins.top() + margins.bottom();

    int widestMinimum = 0;
    for (const QLayoutItem *item : items)
      widestMinimum = std::max(widestMinimum, item->minimumSize().width());

    const bool twoColumns =
      items.size() > 1 && effective.width() >= widestMinimum * 2 + spacing_;
    const int columns = twoColumns ? 2 : 1;
    const int columnWidth =
      columns == 2 ? (effective.width() - spacing_) / 2 : effective.width();
    int y = effective.y();

    for (std::size_t i = 0; i < items.size(); i += columns) {
      int rowHeight = 0;
      for (int column = 0; column < columns && i + column < items.size(); ++column) {
        QLayoutItem *item = items[i + column];
        const int height = item->hasHeightForWidth()
          ? item->heightForWidth(columnWidth)
          : item->sizeHint().height();
        rowHeight = std::max(rowHeight, height);
      }

      if (!testOnly) {
        for (int column = 0; column < columns && i + column < items.size(); ++column) {
          QLayoutItem *item = items[i + column];
          const int x = effective.x() + column * (columnWidth + spacing_);
          item->setGeometry(QRect(x, y, columnWidth, rowHeight));
        }
      }

      y += rowHeight + spacing_;
    }

    return y - spacing_ - rect.y() + margins.bottom();
  }

  std::vector<QLayoutItem *> items_;
  int spacing_ = kControlSpacing;
};

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

QString resolutionText(const ResolutionPreset &preset)
{
  return QString::fromUtf8(preset.label.c_str(), -1);
}

QString fpsText(const FpsPreset &preset)
{
  return QString::fromUtf8(format_fps_value(preset.fps).c_str(), -1);
}

void makeCompactButton(QPushButton *button)
{
  const int textWidth = QFontMetrics(button->font()).horizontalAdvance(button->text());
  const int width = std::max(textWidth + 30, 44);
  button->setStyleSheet(QLatin1String("padding-left: 6px; padding-right: 6px;"));
  button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  button->setMinimumWidth(0);
  button->setFixedWidth(width);
}

QString indexedName(const char *prefix, int index)
{
  return QString::fromLatin1(prefix) + QString::number(index);
}

QWidget *makeLabeledControl(const QString &label, QWidget *control, QWidget *parent)
{
  auto *container = new QWidget(parent);
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(2);

  auto *title = new QLabel(label, container);
  title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  layout->addWidget(title);
  layout->addWidget(control);
  return container;
}

QWidget *makePairSection(QWidget *first, QWidget *second, QWidget *parent)
{
  auto *section = new QWidget(parent);
  section->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  auto *layout = new ResponsivePairLayout(section);
  layout->addWidget(first);
  layout->addWidget(second);
  section->setLayout(layout);
  return section;
}

void clearLayout(QLayout *layout)
{
  while (auto *item = layout->takeAt(0)) {
    if (auto *widget = item->widget())
      widget->deleteLater();
    delete item;
  }
}

bool editResolutionPresetList(QWidget *parent, std::vector<ResolutionPreset> &presets)
{
  QDialog dialog(parent);
  dialog.setWindowTitle(trText("EditResolutionPresets"));

  auto *root = new QVBoxLayout(&dialog);
  auto *form = new QGridLayout();
  form->setHorizontalSpacing(kControlSpacing);
  form->setVerticalSpacing(kControlSpacing);
  root->addLayout(form);

  std::vector<QLineEdit *> labels;
  std::vector<QSpinBox *> primaryValues;
  std::vector<QSpinBox *> secondaryValues;

  auto syncRows = [&]() {
    std::vector<ResolutionPreset> synced;
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
      ResolutionPreset preset;
      preset.label = toStdStringCompat(labels[i]->text().trimmed());
      preset.width = primaryValues[i]->value();
      preset.height = secondaryValues[i]->value();
      synced.push_back(preset);
    }
    if (!synced.empty())
      presets = synced;
  };

  std::function<void()> rebuildRows;
  rebuildRows = [&]() {
    while (auto *item = form->takeAt(0)) {
      if (auto *widget = item->widget())
        widget->deleteLater();
      delete item;
    }

    labels.clear();
    primaryValues.clear();
    secondaryValues.clear();

    form->addWidget(new QLabel(trText("PresetLabel")), 0, 0);
    form->addWidget(new QLabel(trText("Width")), 0, 1);
    form->addWidget(new QLabel(trText("Height")), 0, 2);
    form->addWidget(new QLabel(QString()), 0, 3, 1, 3);

    for (int i = 0; i < static_cast<int>(presets.size()); ++i) {
      const auto &preset = presets[i];
      auto *label = new QLineEdit(QString::fromUtf8(preset.label.c_str(), -1));
      auto *width = new QSpinBox();
      width->setRange(1, 16384);
      width->setValue(preset.width > 0 ? preset.width : 1920);
      width->setMinimumWidth(84);
      auto *height = new QSpinBox();
      height->setRange(1, 16384);
      height->setValue(preset.height > 0 ? preset.height : 1080);
      height->setMinimumWidth(84);
      auto *up = new QPushButton(trText("MovePresetUp"));
      auto *down = new QPushButton(trText("MovePresetDown"));
      auto *remove = new QPushButton(trText("RemovePreset"));

      form->addWidget(label, i + 1, 0);
      form->addWidget(width, i + 1, 1);
      form->addWidget(height, i + 1, 2);
      form->addWidget(up, i + 1, 3);
      form->addWidget(down, i + 1, 4);
      form->addWidget(remove, i + 1, 5);
      labels.push_back(label);
      primaryValues.push_back(width);
      secondaryValues.push_back(height);

      up->setEnabled(i > 0);
      down->setEnabled(i + 1 < static_cast<int>(presets.size()));
      QObject::connect(up, &QPushButton::clicked, &dialog, [&, i]() {
        syncRows();
        if (i <= 0)
          return;
        std::swap(presets[i - 1], presets[i]);
        rebuildRows();
      });
      QObject::connect(down, &QPushButton::clicked, &dialog, [&, i]() {
        syncRows();
        if (i + 1 >= static_cast<int>(presets.size()))
          return;
        std::swap(presets[i], presets[i + 1]);
        rebuildRows();
      });

      QObject::connect(remove, &QPushButton::clicked, &dialog, [&, i]() {
        syncRows();
        if (presets.size() <= 1)
          return;
        presets.erase(presets.begin() + i);
        rebuildRows();
      });
    }
  };

  if (presets.empty())
    presets = PluginConfig().resolutionPresets;
  rebuildRows();

  auto *editButtons = new QHBoxLayout();
  auto *addButton = new QPushButton(trText("AddPreset"), &dialog);
  editButtons->addWidget(addButton);
  editButtons->addStretch(1);
  root->addLayout(editButtons);

  QObject::connect(addButton, &QPushButton::clicked, &dialog, [&]() {
    syncRows();
    presets.push_back({"1080p", 1920, 1080});
    rebuildRows();
  });

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                       Qt::Horizontal, &dialog);
  root->addWidget(buttons);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted)
    return false;

  presets.clear();
  for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
    if (!labels[i] || !primaryValues[i])
      continue;

    ResolutionPreset preset;
    preset.label = toStdStringCompat(labels[i]->text().trimmed());
    preset.width = primaryValues[i]->value();
    preset.height = secondaryValues[i] ? secondaryValues[i]->value() : 0;
    presets.push_back(preset);
  }

  return true;
}

bool editFpsPresetList(QWidget *parent, std::vector<FpsPreset> &presets)
{
  QDialog dialog(parent);
  dialog.setWindowTitle(trText("EditFpsPresets"));

  auto *root = new QVBoxLayout(&dialog);
  auto *form = new QGridLayout();
  form->setHorizontalSpacing(kControlSpacing);
  form->setVerticalSpacing(kControlSpacing);
  root->addLayout(form);

  std::vector<QLineEdit *> values;
  auto syncRows = [&]() {
    std::vector<FpsPreset> synced;
    for (auto *value : values) {
      bool ok = false;
      const double fpsValue = value->text().trimmed().toDouble(&ok);
      FpsPreset preset;
      preset.fps = ok ? fpsValue : 60.0;
      preset.label = format_fps_value(preset.fps);
      synced.push_back(preset);
    }
    if (!synced.empty())
      presets = synced;
  };

  std::function<void()> rebuildRows;
  rebuildRows = [&]() {
    while (auto *item = form->takeAt(0)) {
      if (auto *widget = item->widget())
        widget->deleteLater();
      delete item;
    }

    values.clear();
    form->addWidget(new QLabel(trText("FpsValue")), 0, 0);
    form->addWidget(new QLabel(QString()), 0, 1, 1, 3);

    for (int i = 0; i < static_cast<int>(presets.size()); ++i) {
      const auto &preset = presets[i];
      auto *fps = new QLineEdit(QString::fromUtf8(
        format_fps_value(preset.fps > 0.0 ? preset.fps : 60.0).c_str(), -1));
      auto *validator = new QDoubleValidator(1.0, 1000.0, 2, fps);
      validator->setNotation(QDoubleValidator::StandardNotation);
      fps->setValidator(validator);
      fps->setMinimumWidth(92);
      auto *up = new QPushButton(trText("MovePresetUp"));
      auto *down = new QPushButton(trText("MovePresetDown"));
      auto *remove = new QPushButton(trText("RemovePreset"));
      form->addWidget(fps, i + 1, 0);
      form->addWidget(up, i + 1, 1);
      form->addWidget(down, i + 1, 2);
      form->addWidget(remove, i + 1, 3);
      values.push_back(fps);

      up->setEnabled(i > 0);
      down->setEnabled(i + 1 < static_cast<int>(presets.size()));
      QObject::connect(up, &QPushButton::clicked, &dialog, [&, i]() {
        syncRows();
        if (i <= 0)
          return;
        std::swap(presets[i - 1], presets[i]);
        rebuildRows();
      });
      QObject::connect(down, &QPushButton::clicked, &dialog, [&, i]() {
        syncRows();
        if (i + 1 >= static_cast<int>(presets.size()))
          return;
        std::swap(presets[i], presets[i + 1]);
        rebuildRows();
      });

      QObject::connect(remove, &QPushButton::clicked, &dialog, [&, i]() {
        syncRows();
        if (presets.size() <= 1)
          return;
        presets.erase(presets.begin() + i);
        rebuildRows();
      });
    }
  };

  if (presets.empty())
    presets = PluginConfig().fpsPresets;
  rebuildRows();

  auto *editButtons = new QHBoxLayout();
  auto *addButton = new QPushButton(trText("AddPreset"), &dialog);
  editButtons->addWidget(addButton);
  editButtons->addStretch(1);
  root->addLayout(editButtons);

  QObject::connect(addButton, &QPushButton::clicked, &dialog, [&]() {
    syncRows();
    presets.push_back({"60", 60.0});
    rebuildRows();
  });

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                       Qt::Horizontal, &dialog);
  root->addWidget(buttons);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted)
    return false;

  presets.clear();
  for (auto *value : values) {
    bool ok = false;
    const double fpsValue = value->text().trimmed().toDouble(&ok);
    if (!ok)
      continue;

    FpsPreset preset;
    preset.fps = fpsValue;
    preset.label = format_fps_value(preset.fps);
    presets.push_back(preset);
  }

  return true;
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
  resolutionButtonLayout_ = new FlowLayout();
  fpsButtonLayout_ = new FlowLayout();
  replaySecondsSpin_ = new QSpinBox(this);
  replayMegabytesSpin_ = new QSpinBox(this);
  applyReplayButton_ = new QPushButton(trText("ApplyReplayBuffer"), this);
  previewLabel_ = new QLabel(this);
  settingsButton_ = new QPushButton(trText("SettingsMenuButton"), this);
  settingsMenu_ = new QMenu(settingsButton_);

  replaySecondsSpin_->setRange(1, 36000);
  replaySecondsSpin_->setSuffix(trText("SecondsSuffix"));
  replayMegabytesSpin_->setRange(1, 1048576);
  replayMegabytesSpin_->setSuffix(trText("MegabytesSuffix"));

  pathTemplateEdit_->setPlaceholderText(QLatin1String("{date}/{tag}"));
  manualTagEdit_->setPlaceholderText(trText("ManualTagPlaceholder"));

  previewLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  previewLabel_->setWordWrap(true);

  auto *browseButton = new QPushButton(trText("Browse"), this);
  auto *baseLayout = new QHBoxLayout();
  baseLayout->setContentsMargins(0, 0, 0, 0);
  baseLayout->setSpacing(kControlSpacing);
  baseLayout->addWidget(baseDirectoryEdit_, 1);
  baseLayout->addWidget(browseButton);

  auto *templateHelpButton = new QPushButton(QLatin1String("?"), this);
  templateHelpButton->setAccessibleName(trText("PathTemplateHelpTitle"));
  templateHelpButton->setFixedWidth(browseButton->sizeHint().height());
  templateHelpButton->setFocusPolicy(Qt::NoFocus);
  connect(templateHelpButton, &QPushButton::clicked, this, [templateHelpButton]() {
    QToolTip::showText(templateHelpButton->mapToGlobal(
                         QPoint(-260, -templateHelpButton->height())),
                       templateHelpText(), templateHelpButton);
  });

  auto *templateLayout = new QHBoxLayout();
  templateLayout->setContentsMargins(0, 0, 0, 0);
  templateLayout->setSpacing(kControlSpacing);
  templateLayout->addWidget(pathTemplateEdit_, 1);
  templateLayout->addWidget(templateHelpButton);

  auto *resolutionRow = new QWidget(this);
  resolutionRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  resolutionRow->setLayout(resolutionButtonLayout_);

  auto *fpsRow = new QWidget(this);
  fpsRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  fpsRow->setLayout(fpsButtonLayout_);

  auto *replayLayout = new FlowLayout();
  replayLayout->addWidget(replaySecondsSpin_);
  replayLayout->addWidget(replayMegabytesSpin_);
  replayLayout->addWidget(applyReplayButton_);

  auto *replayRow = new QWidget(this);
  replayRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  replayRow->setLayout(replayLayout);

  profileSceneSection_ = makePairSection(
    makeLabeledControl(trText("Profile"), profileCombo_, this),
    makeLabeledControl(trText("SceneCollection"), sceneCollectionCombo_, this),
    this);

  videoPresetSection_ = makePairSection(
    makeLabeledControl(trText("ResolutionPresets"), resolutionRow, this),
    makeLabeledControl(trText("FpsPresets"), fpsRow, this),
    this);

  replaySection_ = makeLabeledControl(trText("ReplayBuffer"), replayRow, this);

  auto *pathForm = new QFormLayout();
  pathForm->setContentsMargins(0, 0, 0, 0);
  pathForm->setVerticalSpacing(kControlSpacing);
  pathForm->setHorizontalSpacing(kControlSpacing);
  pathForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  pathForm->addRow(trText("BaseDirectory"), baseLayout);
  pathForm->addRow(trText("PathTemplate"), templateLayout);
  pathForm->addRow(trText("ManualTag"), manualTagEdit_);
  pathForm->addRow(QString(), enablePathAutomationCheck_);
  pathSection_ = new QWidget(this);
  pathSection_->setLayout(pathForm);

  auto *previewForm = new QFormLayout();
  previewForm->setContentsMargins(0, 0, 0, 0);
  previewForm->setVerticalSpacing(kControlSpacing);
  previewForm->setHorizontalSpacing(kControlSpacing);
  previewForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  previewForm->addRow(trText("Preview"), previewLabel_);
  previewSection_ = new QWidget(this);
  previewSection_->setLayout(previewForm);

  auto *content = new QWidget(this);
  auto *contentLayout = new QVBoxLayout(content);
  contentLayout->setContentsMargins(kFormMargin, kFormMargin, kFormMargin, kFormMargin);
  contentLayout->setSpacing(kControlSpacing);
  contentLayout->addWidget(profileSceneSection_);
  contentLayout->addWidget(videoPresetSection_);
  contentLayout->addWidget(replaySection_);
  contentLayout->addWidget(pathSection_);
  contentLayout->addWidget(previewSection_);
  contentLayout->addStretch(1);

  auto *scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidget(content);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(scroll);
  auto *bottomBar = new QHBoxLayout();
  bottomBar->setContentsMargins(kFormMargin, 0, kFormMargin, kFormMargin);
  bottomBar->addStretch(1);
  bottomBar->addWidget(settingsButton_);
  layout->addLayout(bottomBar);

  buildSettingsMenu();

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
  connect(applyReplayButton_, &QPushButton::clicked, this,
          &EasyConfigDock::applyReplayBufferSettings);
  connect(settingsButton_, &QPushButton::clicked, this, [this]() {
    settingsMenu_->popup(settingsButton_->mapToGlobal(
      QPoint(0, -settingsMenu_->sizeHint().height())));
  });

  connect(controller_, &ObsController::obsStateChanged, this, &EasyConfigDock::refreshObsState);
  connect(controller_, &ObsController::recordingStarting, this,
          &EasyConfigDock::applyBeforeRecording);
  connect(controller_, &ObsController::replayBufferStarting, this,
          &EasyConfigDock::applyBeforeRecording);
}

void EasyConfigDock::refreshObsState()
{
  refillCombo(sceneCollectionCombo_, controller_->sceneCollectionNames(),
              controller_->currentSceneCollectionName());
  refillCombo(profileCombo_, controller_->profileNames(), controller_->currentProfileName());
  refreshOutputControls();
  refreshReplayBufferSettings();
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

void EasyConfigDock::applyResolutionPreset()
{
  auto *button = qobject_cast<QPushButton *>(sender());
  if (!button)
    return;

  const int index = resolutionPresetIndex(button);
  if (index < 0 || index >= static_cast<int>(config_.resolutionPresets.size()))
    return;

  QString error;
  if (!controller_->setOutputResolution(config_.resolutionPresets[index], &error)) {
    setPreviewText(error, true);
    refreshOutputControls();
    return;
  }

  setPreviewText(trText("ResolutionApplied"));
  refreshOutputControls();
}

void EasyConfigDock::applyFpsPreset()
{
  auto *button = qobject_cast<QPushButton *>(sender());
  if (!button)
    return;

  const int index = fpsPresetIndex(button);
  if (index < 0 || index >= static_cast<int>(config_.fpsPresets.size()))
    return;

  QString error;
  if (!controller_->setFps(config_.fpsPresets[index], &error)) {
    setPreviewText(error, true);
    refreshOutputControls();
    return;
  }

  setPreviewText(trText("FpsApplied"));
  refreshOutputControls();
}

void EasyConfigDock::editResolutionPresets()
{
  auto presets = config_.resolutionPresets;
  const bool accepted = editResolutionPresetList(this, presets);

  if (!accepted)
    return;

  config_.resolutionPresets = normalize_resolution_presets(presets);
  refreshPresetButtons();
  saveCurrentConfig();
}

void EasyConfigDock::editFpsPresets()
{
  auto presets = config_.fpsPresets;
  const bool accepted = editFpsPresetList(this, presets);

  if (!accepted)
    return;

  config_.fpsPresets = normalize_fps_presets(presets);
  refreshPresetButtons();
  saveCurrentConfig();
}

void EasyConfigDock::applyReplayBufferSettings()
{
  ReplayBufferSettings settings;
  settings.seconds = replaySecondsSpin_->value();
  settings.megabytes = replayMegabytesSpin_->value();

  QString error;
  if (!controller_->setReplayBufferSettings(settings, &error)) {
    setPreviewText(error, true);
    return;
  }

  config_.lastReplayBufferSeconds = settings.seconds;
  config_.lastReplayBufferMegabytes = settings.megabytes;
  saveCurrentConfig();
  setPreviewText(trText("ReplayBufferApplied"));
}

void EasyConfigDock::updateSectionVisibility()
{
  if (profileSceneSection_)
    profileSceneSection_->setVisible(config_.showProfileSceneCollection);
  if (videoPresetSection_)
    videoPresetSection_->setVisible(config_.showVideoPresets);
  if (replaySection_)
    replaySection_->setVisible(config_.showReplayBuffer);
  if (pathSection_)
    pathSection_->setVisible(config_.showPathAutomation);
  if (previewSection_)
    previewSection_->setVisible(config_.showPreviewStatus);

  if (showProfileSceneAction_)
    showProfileSceneAction_->setChecked(config_.showProfileSceneCollection);
  if (showVideoPresetsAction_)
    showVideoPresetsAction_->setChecked(config_.showVideoPresets);
  if (showReplayBufferAction_)
    showReplayBufferAction_->setChecked(config_.showReplayBuffer);
  if (showPathAutomationAction_)
    showPathAutomationAction_->setChecked(config_.showPathAutomation);
  if (showPreviewStatusAction_)
    showPreviewStatusAction_->setChecked(config_.showPreviewStatus);
}

PluginConfig EasyConfigDock::configFromUi() const
{
  PluginConfig config = config_;
  config.baseDirectory = toStdStringCompat(baseDirectoryEdit_->text());
  config.pathTemplate = toStdStringCompat(pathTemplateEdit_->text());
  config.manualTag = toStdStringCompat(manualTagEdit_->text());
  config.autoApplyBeforeRecording = enablePathAutomationCheck_->isChecked();
  config.locale = "auto";
  config.lastReplayBufferSeconds = replaySecondsSpin_->value();
  config.lastReplayBufferMegabytes = replayMegabytesSpin_->value();
  config.showProfileSceneCollection = config_.showProfileSceneCollection;
  config.showVideoPresets = config_.showVideoPresets;
  config.showReplayBuffer = config_.showReplayBuffer;
  config.showPathAutomation = config_.showPathAutomation;
  config.showPreviewStatus = config_.showPreviewStatus;
  return config;
}

void EasyConfigDock::setUiFromConfig(const PluginConfig &config)
{
  config_ = config;
  QString baseDirectory = QString::fromUtf8(config.baseDirectory.c_str(), -1);
  if (baseDirectory.isEmpty())
    baseDirectory = controller_->currentRecordingDirectory();
  baseDirectoryEdit_->setText(baseDirectory);
  pathTemplateEdit_->setText(QString::fromUtf8(config.pathTemplate.c_str(), -1));
  manualTagEdit_->setText(QString::fromUtf8(config.manualTag.c_str(), -1));
  enablePathAutomationCheck_->setChecked(config.autoApplyBeforeRecording);
  replaySecondsSpin_->setValue(config.lastReplayBufferSeconds);
  replayMegabytesSpin_->setValue(config.lastReplayBufferMegabytes);
  refreshPresetButtons();
  updateSectionVisibility();
}

void EasyConfigDock::buildSettingsMenu()
{
  auto addVisibilityAction = [this](const char *textKey, bool PluginConfig::*field) {
    QAction *action = settingsMenu_->addAction(trText(textKey));
    action->setCheckable(true);
    connect(action, &QAction::toggled, this, [this, field](bool checked) {
      config_.*field = checked;
      updateSectionVisibility();
      saveCurrentConfig();
    });
    return action;
  };

  showProfileSceneAction_ =
    addVisibilityAction("ShowProfileSceneCollection", &PluginConfig::showProfileSceneCollection);
  showVideoPresetsAction_ =
    addVisibilityAction("ShowVideoPresets", &PluginConfig::showVideoPresets);
  showReplayBufferAction_ =
    addVisibilityAction("ShowReplayBuffer", &PluginConfig::showReplayBuffer);
  showPathAutomationAction_ =
    addVisibilityAction("ShowPathAutomation", &PluginConfig::showPathAutomation);
  showPreviewStatusAction_ =
    addVisibilityAction("ShowPreviewStatus", &PluginConfig::showPreviewStatus);

  settingsMenu_->addSeparator();
  connect(settingsMenu_->addAction(trText("EditResolutionPresets")), &QAction::triggered,
          this, &EasyConfigDock::editResolutionPresets);
  connect(settingsMenu_->addAction(trText("EditFpsPresets")), &QAction::triggered,
          this, &EasyConfigDock::editFpsPresets);
}

void EasyConfigDock::refreshPresetButtons()
{
  clearLayout(resolutionButtonLayout_);
  clearLayout(fpsButtonLayout_);
  resolutionButtons_.clear();
  fpsButtons_.clear();

  for (std::size_t i = 0; i < config_.resolutionPresets.size(); ++i) {
    auto *button = new QPushButton(resolutionText(config_.resolutionPresets[i]), this);
    button->setCheckable(true);
    makeCompactButton(button);
    connect(button, &QPushButton::clicked, this, &EasyConfigDock::applyResolutionPreset);
    resolutionButtons_.push_back(button);
    resolutionButtonLayout_->addWidget(button);
  }
  for (std::size_t i = 0; i < config_.fpsPresets.size(); ++i) {
    auto *button = new QPushButton(fpsText(config_.fpsPresets[i]), this);
    button->setCheckable(true);
    makeCompactButton(button);
    connect(button, &QPushButton::clicked, this, &EasyConfigDock::applyFpsPreset);
    fpsButtons_.push_back(button);
    fpsButtonLayout_->addWidget(button);
  }
  refreshOutputControls();
}

void EasyConfigDock::refreshOutputControls()
{
  const bool active = controller_->outputsActive();
  const auto resolution = controller_->currentOutputResolution();
  const double fps = controller_->currentFps();

  for (int i = 0; i < resolutionButtonLayout_->count(); ++i) {
    auto *button = qobject_cast<QPushButton *>(resolutionButtonLayout_->itemAt(i)->widget());
    if (!button)
      continue;
    const int index = resolutionPresetIndex(button);
    if (index < 0)
      continue;
    const auto &preset = config_.resolutionPresets[index];
    button->setEnabled(!active);
    button->setChecked(preset.width == resolution.width && preset.height == resolution.height);
  }
  for (int i = 0; i < fpsButtonLayout_->count(); ++i) {
    auto *button = qobject_cast<QPushButton *>(fpsButtonLayout_->itemAt(i)->widget());
    if (!button)
      continue;
    const int index = fpsPresetIndex(button);
    if (index < 0)
      continue;
    const auto &preset = config_.fpsPresets[index];
    button->setEnabled(!active);
    button->setChecked(std::abs(preset.fps - fps) < 0.005);
  }

  if (active)
    setPreviewText(trText("VideoSettingsLocked"));
}

int EasyConfigDock::resolutionPresetIndex(QObject *sender) const
{
  for (std::size_t i = 0; i < resolutionButtons_.size(); ++i) {
    if (resolutionButtons_[i] == sender)
      return static_cast<int>(i);
  }
  return -1;
}

int EasyConfigDock::fpsPresetIndex(QObject *sender) const
{
  for (std::size_t i = 0; i < fpsButtons_.size(); ++i) {
    if (fpsButtons_[i] == sender)
      return static_cast<int>(i);
  }
  return -1;
}

void EasyConfigDock::refreshReplayBufferSettings()
{
  const auto settings = controller_->currentReplayBufferSettings();
  if (settings.seconds > 0) {
    const QSignalBlocker blocker(replaySecondsSpin_);
    replaySecondsSpin_->setValue(settings.seconds);
  }
  if (settings.megabytes > 0) {
    const QSignalBlocker blocker(replayMegabytesSpin_);
    replayMegabytesSpin_->setValue(settings.megabytes);
  }
}

void EasyConfigDock::saveCurrentConfig()
{
  config_ = configFromUi();
  QString error;
  if (!controller_->saveConfig(config_, &error))
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
