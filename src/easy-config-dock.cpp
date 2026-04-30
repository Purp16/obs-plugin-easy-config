#include "easy-config-dock.hpp"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QFontMetrics>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QSpinBox>
#include <QTabWidget>
#include <QStyle>
#include <QVBoxLayout>

#include <obs-module.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>

namespace easy_config {
namespace {

constexpr int kControlSpacing = 5;
constexpr int kFormMargin = 6;
constexpr int kTwoColumnBreakpoint = 220;

class PersistentCheckMenu : public QMenu {
public:
  explicit PersistentCheckMenu(QWidget *parent = nullptr) : QMenu(parent) {}

protected:
  void mouseReleaseEvent(QMouseEvent *event) override
  {
    QAction *action = actionAt(event->pos());
    if (action && action->isCheckable() && action->isEnabled()) {
      action->setChecked(!action->isChecked());
      event->accept();
      return;
    }

    QMenu::mouseReleaseEvent(event);
  }
};

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

class WrapSectionLayout : public QLayout {
public:
  explicit WrapSectionLayout(QWidget *parent = nullptr, int margin = 0,
                             int spacing = kControlSpacing,
                             int maxItemsPerRow = 2,
                             int minItemWidth = kTwoColumnBreakpoint)
    : QLayout(parent), spacing_(spacing), maxItemsPerRow_(maxItemsPerRow)
  {
    setContentsMargins(margin, margin, margin, margin);
    setMinItemWidth(minItemWidth);
  }

  ~WrapSectionLayout() override
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

  int minItemWidth() const { return minItemWidth_; }

  void setMinItemWidth(int width)
  {
    minItemWidth_ = std::max(120, width);
    invalidate();
  }

private:
  std::vector<QLayoutItem *> visibleItems() const
  {
    std::vector<QLayoutItem *> visible;
    for (QLayoutItem *item : items_) {
      if (!item->widget() || !item->widget()->isHidden())
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

    int y = effective.y();
    std::size_t rowStart = 0;

    while (rowStart < items.size()) {
      int preferredWidth = 0;
      int minimumWidth = 0;
      std::size_t rowEnd = rowStart;
      const bool allowTwoColumns =
        effective.width() >= minItemWidth_ * maxItemsPerRow_ + spacing_;

      for (; rowEnd < items.size(); ++rowEnd) {
        if (rowEnd > rowStart &&
            (!allowTwoColumns || static_cast<int>(rowEnd - rowStart) >= maxItemsPerRow_))
          break;

        const int itemPreferredWidth = items[rowEnd]->sizeHint().width();
        const int itemMinimumWidth = items[rowEnd]->minimumSize().width();
        const int preferredCandidate =
          preferredWidth + (rowEnd > rowStart ? spacing_ : 0) + itemPreferredWidth;
        const int minimumCandidate =
          minimumWidth + (rowEnd > rowStart ? spacing_ : 0) + itemMinimumWidth;
        if (rowEnd > rowStart && preferredCandidate > effective.width() &&
            minimumCandidate > effective.width())
          break;

        preferredWidth = preferredCandidate;
        minimumWidth = minimumCandidate;
      }

      if (rowEnd == rowStart)
        rowEnd = rowStart + 1;

      const int rowCount = static_cast<int>(rowEnd - rowStart);
      const int availableWidth =
        effective.width() - spacing_ * std::max(rowCount - 1, 0);
      const int columnWidth =
        rowCount > 0 ? availableWidth / rowCount : effective.width();
      int rowHeight = 0;

      for (std::size_t i = rowStart; i < rowEnd; ++i) {
        QLayoutItem *item = items[i];
        const int height = item->hasHeightForWidth()
          ? item->heightForWidth(columnWidth)
          : item->sizeHint().height();
        rowHeight = std::max(rowHeight, height);
      }

      if (!testOnly) {
        int x = effective.x();
        for (std::size_t i = rowStart; i < rowEnd; ++i) {
          QLayoutItem *item = items[i];
          item->setGeometry(QRect(x, y, columnWidth, rowHeight));
          x += columnWidth + spacing_;
        }
      }

      y += rowHeight + spacing_;
      rowStart = rowEnd;
    }

    return y - spacing_ - rect.y() + margins.bottom();
  }

  std::vector<QLayoutItem *> items_;
  int spacing_ = kControlSpacing;
  int maxItemsPerRow_ = 2;
  int minItemWidth_ = kTwoColumnBreakpoint;
};

class WrapSectionWidget : public QWidget {
public:
  explicit WrapSectionWidget(QWidget *parent = nullptr) : QWidget(parent)
  {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  }

  bool hasHeightForWidth() const override { return layout() && layout()->hasHeightForWidth(); }

  int heightForWidth(int width) const override
  {
    return layout() ? layout()->heightForWidth(width) : QWidget::heightForWidth(width);
  }

  QSize sizeHint() const override
  {
    if (!layout())
      return QWidget::sizeHint();
    QSize hint = layout()->sizeHint();
    const int width = std::max(this->width(), std::max(hint.width(), 1));
    return {width, layout()->heightForWidth(width)};
  }

  QSize minimumSizeHint() const override
  {
    if (!layout())
      return QWidget::minimumSizeHint();
    return {0, layout()->heightForWidth(std::max(this->width(), 1))};
  }

protected:
  void resizeEvent(QResizeEvent *event) override
  {
    QWidget::resizeEvent(event);
    updateMinimumHeightForWidth(event->size().width());
  }

  void showEvent(QShowEvent *event) override
  {
    QWidget::showEvent(event);
    updateMinimumHeightForWidth(width());
  }

private:
  void updateMinimumHeightForWidth(int width)
  {
    if (!layout())
      return;

    const int height = layout()->heightForWidth(std::max(width, 1));
    if (minimumHeight() != height)
      setMinimumHeight(height);
  }
};

class ResponsiveContentWidget : public QWidget {
public:
  explicit ResponsiveContentWidget(QWidget *parent = nullptr) : QWidget(parent)
  {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumSize(0, 0);
  }

  bool hasHeightForWidth() const override { return layout() && layout()->hasHeightForWidth(); }

  int heightForWidth(int width) const override
  {
    return layout() ? layout()->heightForWidth(width) : QWidget::heightForWidth(width);
  }

  QSize sizeHint() const override
  {
    if (!layout())
      return QWidget::sizeHint();
    const int width = std::max(this->width(), 1);
    return {width, layout()->heightForWidth(width)};
  }

  QSize minimumSizeHint() const override { return {0, 0}; }
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

struct TemplateVariableOption {
  const char *name;
  const char *labelKey;
};

std::vector<TemplateVariableOption> templateVariableOptions()
{
  return {
    {"date", "TemplateVarDate"},
    {"datetime", "TemplateVarDatetime"},
    {"year", "TemplateVarYear"},
    {"month", "TemplateVarMonth"},
    {"day", "TemplateVarDay"},
    {"time", "TemplateVarTime"},
    {"hour", "TemplateVarHour"},
    {"minute", "TemplateVarMinute"},
    {"second", "TemplateVarSecond"},
    {"profile", "TemplateVarProfile"},
    {"scene_collection", "TemplateVarSceneCollection"},
    {"scene", "TemplateVarScene"},
    {"tag", "TemplateVarTag"},
  };
}

QString templateToken(const QString &name)
{
  return QLatin1String("{") + name + QLatin1String("}");
}

QString templateVariableLabel(const TemplateVariableOption &option)
{
  return trText(option.labelKey) + QLatin1String("  ") +
         templateToken(QLatin1String(option.name));
}

QString templateFromList(QListWidget *list)
{
  QStringList tokens;
  for (int i = 0; i < list->count(); ++i) {
    const QString name = list->item(i)->data(Qt::UserRole).toString();
    if (!name.isEmpty())
      tokens.push_back(templateToken(name));
  }
  return tokens.join(QLatin1Char('/'));
}

std::vector<QString> parseTemplateVariableOrder(const QString &pathTemplate)
{
  std::map<QString, bool> known;
  for (const auto &option : templateVariableOptions())
    known[QLatin1String(option.name)] = true;

  std::vector<QString> result;
  int offset = 0;
  while (offset < pathTemplate.size()) {
    const int open = pathTemplate.indexOf(QLatin1Char('{'), offset);
    if (open < 0)
      break;
    const int close = pathTemplate.indexOf(QLatin1Char('}'), open + 1);
    if (close < 0)
      break;

    const QString name = pathTemplate.mid(open + 1, close - open - 1);
    if (known.find(name) != known.end())
      result.push_back(name);
    offset = close + 1;
  }

  if (result.empty()) {
    result.push_back(QLatin1String("date"));
    result.push_back(QLatin1String("tag"));
  }
  return result;
}

PathContext makeTemplateExampleContext(const QString &baseDirectory,
                                       const QString &profile,
                                       const QString &sceneCollection,
                                       const QString &scene,
                                       const QString &tag)
{
  PathContext context;
  context.base = toStdStringCompat(baseDirectory);
  context.date = "2026-04-30";
  context.datetime = "2026-04-30_21-35-18";
  context.year = "2026";
  context.month = "04";
  context.day = "30";
  context.time = "21-35-18";
  context.hour = "21";
  context.minute = "35";
  context.second = "18";
  context.profile = toStdStringCompat(profile.isEmpty() ? trText("Profile") : profile);
  context.scene_collection = toStdStringCompat(
    sceneCollection.isEmpty() ? trText("SceneCollection") : sceneCollection);
  context.scene = toStdStringCompat(scene.isEmpty() ? trText("Scene") : scene);
  context.tag = toStdStringCompat(tag.isEmpty() ? QLatin1String("clips") : tag);
  return context;
}

bool editPathTemplateDialog(QWidget *parent, const QString &baseDirectory,
                            const QString &profile, const QString &sceneCollection,
                            const QString &scene, const QString &tag,
                            QString *pathTemplate)
{
  QDialog dialog(parent);
  dialog.setWindowTitle(trText("EditPathTemplate"));
  dialog.resize(620, 460);

  auto *root = new QVBoxLayout(&dialog);
  auto *columns = new QGridLayout();
  columns->setHorizontalSpacing(kControlSpacing * 2);
  columns->setVerticalSpacing(kControlSpacing);
  root->addLayout(columns, 1);

  auto *availableLabel = new QLabel(trText("PathTemplateAvailableVariables"), &dialog);
  auto *selectedLabel = new QLabel(trText("PathTemplateSelectedVariables"), &dialog);
  auto *availableList = new QListWidget(&dialog);
  auto *selectedList = new QListWidget(&dialog);
  auto *actions = new QVBoxLayout();

  availableList->setSelectionMode(QAbstractItemView::SingleSelection);
  availableList->setDragEnabled(true);
  availableList->setDragDropMode(QAbstractItemView::DragOnly);
  selectedList->setSelectionMode(QAbstractItemView::SingleSelection);
  selectedList->setDragDropMode(QAbstractItemView::DragDrop);
  selectedList->setDefaultDropAction(Qt::MoveAction);
  selectedList->setDragEnabled(true);
  selectedList->setAcceptDrops(true);
  selectedList->setDropIndicatorShown(true);

  for (const auto &option : templateVariableOptions()) {
    auto *item = new QListWidgetItem(templateVariableLabel(option), availableList);
    item->setData(Qt::UserRole, QLatin1String(option.name));
  }

  auto addSelectedItem = [&](const QString &name) {
    const auto options = templateVariableOptions();
    auto it = std::find_if(options.begin(), options.end(),
                           [&](const TemplateVariableOption &option) {
                             return name == QLatin1String(option.name);
                           });
    if (it == options.end())
      return;
    auto *item = new QListWidgetItem(templateVariableLabel(*it), selectedList);
    item->setData(Qt::UserRole, name);
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  };

  for (const QString &name : parseTemplateVariableOrder(*pathTemplate))
    addSelectedItem(name);

  auto *addButton = new QPushButton(trText("AddTemplateVariable"), &dialog);
  auto *removeButton = new QPushButton(trText("RemovePreset"), &dialog);
  auto *upButton = new QPushButton(trText("MovePresetUp"), &dialog);
  auto *downButton = new QPushButton(trText("MovePresetDown"), &dialog);
  actions->addStretch(1);
  actions->addWidget(addButton);
  actions->addWidget(removeButton);
  actions->addSpacing(kControlSpacing);
  actions->addWidget(upButton);
  actions->addWidget(downButton);
  actions->addStretch(1);

  columns->addWidget(availableLabel, 0, 0);
  columns->addWidget(selectedLabel, 0, 2);
  columns->addWidget(availableList, 1, 0);
  columns->addLayout(actions, 1, 1);
  columns->addWidget(selectedList, 1, 2);
  columns->setColumnStretch(0, 1);
  columns->setColumnStretch(2, 1);

  auto *templatePreview = new QLineEdit(&dialog);
  templatePreview->setReadOnly(true);
  root->addWidget(templatePreview);

  auto *examplesLabel = new QLabel(trText("PathTemplateExamples"), &dialog);
  auto *examples = new QListWidget(&dialog);
  examples->setSelectionMode(QAbstractItemView::NoSelection);
  examples->setMaximumHeight(96);
  root->addWidget(examplesLabel);
  root->addWidget(examples);

  const QString exampleBase = baseDirectory.trimmed().isEmpty()
    ? QLatin1String("D:/Recordings")
    : baseDirectory.trimmed();
  const QString exampleTag = tag.trimmed().isEmpty() ? QLatin1String("clips") : tag.trimmed();

  auto updateExamples = [&]() {
    const QString currentTemplate = templateFromList(selectedList);
    templatePreview->setText(currentTemplate);
    examples->clear();

    const std::vector<PathContext> contexts = {
      makeTemplateExampleContext(exampleBase, profile, sceneCollection, scene, exampleTag),
      makeTemplateExampleContext(exampleBase, profile, sceneCollection, scene,
                                 QLatin1String("ranked")),
      makeTemplateExampleContext(exampleBase, profile, sceneCollection, scene,
                                 QLatin1String("test")),
    };

    for (const PathContext &context : contexts) {
      const auto result = resolve_path_template(toStdStringCompat(currentTemplate), context);
      examples->addItem(QString::fromUtf8(result.path.c_str(), -1));
    }
  };

  QObject::connect(addButton, &QPushButton::clicked, &dialog, [&]() {
    auto *item = availableList->currentItem();
    if (!item)
      return;
    addSelectedItem(item->data(Qt::UserRole).toString());
    selectedList->setCurrentRow(selectedList->count() - 1);
    updateExamples();
  });
  QObject::connect(availableList, &QListWidget::itemDoubleClicked, &dialog,
                   [&](QListWidgetItem *item) {
                     if (!item)
                       return;
                     addSelectedItem(item->data(Qt::UserRole).toString());
                     selectedList->setCurrentRow(selectedList->count() - 1);
                     updateExamples();
                   });
  QObject::connect(removeButton, &QPushButton::clicked, &dialog, [&]() {
    delete selectedList->takeItem(selectedList->currentRow());
    updateExamples();
  });
  QObject::connect(upButton, &QPushButton::clicked, &dialog, [&]() {
    const int row = selectedList->currentRow();
    if (row <= 0)
      return;
    auto *item = selectedList->takeItem(row);
    selectedList->insertItem(row - 1, item);
    selectedList->setCurrentRow(row - 1);
    updateExamples();
  });
  QObject::connect(downButton, &QPushButton::clicked, &dialog, [&]() {
    const int row = selectedList->currentRow();
    if (row < 0 || row + 1 >= selectedList->count())
      return;
    auto *item = selectedList->takeItem(row);
    selectedList->insertItem(row + 1, item);
    selectedList->setCurrentRow(row + 1);
    updateExamples();
  });
  QObject::connect(selectedList->model(), &QAbstractItemModel::rowsMoved,
                   &dialog, [&](const QModelIndex &, int, int, const QModelIndex &, int) {
                     updateExamples();
                   });
  QObject::connect(selectedList->model(), &QAbstractItemModel::rowsInserted,
                   &dialog, [&](const QModelIndex &, int, int) {
                     updateExamples();
                   });
  QObject::connect(selectedList->model(), &QAbstractItemModel::rowsRemoved,
                   &dialog, [&](const QModelIndex &, int, int) {
                     updateExamples();
                   });

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                       Qt::Horizontal, &dialog);
  root->addWidget(buttons);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
    if (selectedList->count() <= 0)
      return;
    dialog.accept();
  });
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (availableList->count() > 0)
    availableList->setCurrentRow(0);
  if (selectedList->count() > 0)
    selectedList->setCurrentRow(0);
  updateExamples();

  if (dialog.exec() != QDialog::Accepted)
    return false;

  const QString generatedTemplate = templateFromList(selectedList);
  if (generatedTemplate.isEmpty())
    return false;

  *pathTemplate = generatedTemplate;
  return true;
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

QWidget *makeInlineLabelControl(const QString &label, QWidget *control, QWidget *parent)
{
  auto *container = new QWidget(parent);
  auto *layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(kControlSpacing);

  auto *title = new QLabel(label, container);
  title->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
  layout->addWidget(title);
  layout->addWidget(control, 1);
  return container;
}

QWidget *makeLayoutWidget(QLayout *layout, QWidget *parent)
{
  auto *container = new QWidget(parent);
  container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  container->setLayout(layout);
  return container;
}

QWidget *makeWrapSection(const std::vector<QWidget *> &controls, QWidget *parent)
{
  auto *section = new WrapSectionWidget(parent);
  auto *layout = new WrapSectionLayout(section);
  for (QWidget *control : controls)
    layout->addWidget(control);
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

bool editVideoPresetLists(QWidget *parent, std::vector<ResolutionPreset> &resolutionPresets,
                          std::vector<FpsPreset> &fpsPresets)
{
  QDialog dialog(parent);
  dialog.setWindowTitle(trText("EditVideoPresets"));

  auto *root = new QVBoxLayout(&dialog);
  auto *tabs = new QTabWidget(&dialog);
  root->addWidget(tabs);

  auto *resolutionPage = new QWidget(tabs);
  auto *resolutionRoot = new QVBoxLayout(resolutionPage);
  auto *resolutionForm = new QGridLayout();
  resolutionForm->setHorizontalSpacing(kControlSpacing);
  resolutionForm->setVerticalSpacing(kControlSpacing);
  resolutionRoot->addLayout(resolutionForm);

  std::vector<QLineEdit *> resolutionLabels;
  std::vector<QSpinBox *> resolutionWidths;
  std::vector<QSpinBox *> resolutionHeights;

  auto syncResolutions = [&]() {
    std::vector<ResolutionPreset> synced;
    for (int i = 0; i < static_cast<int>(resolutionLabels.size()); ++i) {
      ResolutionPreset preset;
      preset.label = toStdStringCompat(resolutionLabels[i]->text().trimmed());
      preset.width = resolutionWidths[i]->value();
      preset.height = resolutionHeights[i]->value();
      synced.push_back(preset);
    }
    if (!synced.empty())
      resolutionPresets = synced;
  };

  std::function<void()> rebuildResolutions;
  rebuildResolutions = [&]() {
    clearLayout(resolutionForm);
    resolutionLabels.clear();
    resolutionWidths.clear();
    resolutionHeights.clear();

    resolutionForm->addWidget(new QLabel(trText("PresetLabel")), 0, 0);
    resolutionForm->addWidget(new QLabel(trText("Width")), 0, 1);
    resolutionForm->addWidget(new QLabel(trText("Height")), 0, 2);
    resolutionForm->addWidget(new QLabel(QString()), 0, 3, 1, 3);

    for (int i = 0; i < static_cast<int>(resolutionPresets.size()); ++i) {
      const auto &preset = resolutionPresets[i];
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

      resolutionForm->addWidget(label, i + 1, 0);
      resolutionForm->addWidget(width, i + 1, 1);
      resolutionForm->addWidget(height, i + 1, 2);
      resolutionForm->addWidget(up, i + 1, 3);
      resolutionForm->addWidget(down, i + 1, 4);
      resolutionForm->addWidget(remove, i + 1, 5);
      resolutionLabels.push_back(label);
      resolutionWidths.push_back(width);
      resolutionHeights.push_back(height);

      up->setEnabled(i > 0);
      down->setEnabled(i + 1 < static_cast<int>(resolutionPresets.size()));
      QObject::connect(up, &QPushButton::clicked, &dialog, [&, i]() {
        syncResolutions();
        if (i <= 0)
          return;
        std::swap(resolutionPresets[i - 1], resolutionPresets[i]);
        rebuildResolutions();
      });
      QObject::connect(down, &QPushButton::clicked, &dialog, [&, i]() {
        syncResolutions();
        if (i + 1 >= static_cast<int>(resolutionPresets.size()))
          return;
        std::swap(resolutionPresets[i], resolutionPresets[i + 1]);
        rebuildResolutions();
      });
      QObject::connect(remove, &QPushButton::clicked, &dialog, [&, i]() {
        syncResolutions();
        if (resolutionPresets.size() <= 1)
          return;
        resolutionPresets.erase(resolutionPresets.begin() + i);
        rebuildResolutions();
      });
    }
  };

  auto *addResolutionButton = new QPushButton(trText("AddPreset"), resolutionPage);
  auto *resolutionActions = new QHBoxLayout();
  resolutionActions->addWidget(addResolutionButton);
  resolutionActions->addStretch(1);
  resolutionRoot->addLayout(resolutionActions);
  QObject::connect(addResolutionButton, &QPushButton::clicked, &dialog, [&]() {
    syncResolutions();
    resolutionPresets.push_back({"1080p", 1920, 1080});
    rebuildResolutions();
  });

  auto *fpsPage = new QWidget(tabs);
  auto *fpsRoot = new QVBoxLayout(fpsPage);
  auto *fpsForm = new QGridLayout();
  fpsForm->setHorizontalSpacing(kControlSpacing);
  fpsForm->setVerticalSpacing(kControlSpacing);
  fpsRoot->addLayout(fpsForm);

  std::vector<QLineEdit *> fpsValues;
  auto syncFps = [&]() {
    std::vector<FpsPreset> synced;
    for (auto *value : fpsValues) {
      bool ok = false;
      const double fpsValue = value->text().trimmed().toDouble(&ok);
      FpsPreset preset;
      preset.fps = ok ? fpsValue : 60.0;
      preset.label = format_fps_value(preset.fps);
      synced.push_back(preset);
    }
    if (!synced.empty())
      fpsPresets = synced;
  };

  std::function<void()> rebuildFps;
  rebuildFps = [&]() {
    clearLayout(fpsForm);
    fpsValues.clear();

    fpsForm->addWidget(new QLabel(trText("FpsValue")), 0, 0);
    fpsForm->addWidget(new QLabel(QString()), 0, 1, 1, 3);

    for (int i = 0; i < static_cast<int>(fpsPresets.size()); ++i) {
      const auto &preset = fpsPresets[i];
      auto *fps = new QLineEdit(QString::fromUtf8(
        format_fps_value(preset.fps > 0.0 ? preset.fps : 60.0).c_str(), -1));
      auto *validator = new QDoubleValidator(1.0, 1000.0, 2, fps);
      validator->setNotation(QDoubleValidator::StandardNotation);
      fps->setValidator(validator);
      fps->setMinimumWidth(92);
      auto *up = new QPushButton(trText("MovePresetUp"));
      auto *down = new QPushButton(trText("MovePresetDown"));
      auto *remove = new QPushButton(trText("RemovePreset"));

      fpsForm->addWidget(fps, i + 1, 0);
      fpsForm->addWidget(up, i + 1, 1);
      fpsForm->addWidget(down, i + 1, 2);
      fpsForm->addWidget(remove, i + 1, 3);
      fpsValues.push_back(fps);

      up->setEnabled(i > 0);
      down->setEnabled(i + 1 < static_cast<int>(fpsPresets.size()));
      QObject::connect(up, &QPushButton::clicked, &dialog, [&, i]() {
        syncFps();
        if (i <= 0)
          return;
        std::swap(fpsPresets[i - 1], fpsPresets[i]);
        rebuildFps();
      });
      QObject::connect(down, &QPushButton::clicked, &dialog, [&, i]() {
        syncFps();
        if (i + 1 >= static_cast<int>(fpsPresets.size()))
          return;
        std::swap(fpsPresets[i], fpsPresets[i + 1]);
        rebuildFps();
      });
      QObject::connect(remove, &QPushButton::clicked, &dialog, [&, i]() {
        syncFps();
        if (fpsPresets.size() <= 1)
          return;
        fpsPresets.erase(fpsPresets.begin() + i);
        rebuildFps();
      });
    }
  };

  auto *addFpsButton = new QPushButton(trText("AddPreset"), fpsPage);
  auto *fpsActions = new QHBoxLayout();
  fpsActions->addWidget(addFpsButton);
  fpsActions->addStretch(1);
  fpsRoot->addLayout(fpsActions);
  QObject::connect(addFpsButton, &QPushButton::clicked, &dialog, [&]() {
    syncFps();
    fpsPresets.push_back({"60", 60.0});
    rebuildFps();
  });

  tabs->addTab(resolutionPage, trText("ResolutionPresets"));
  tabs->addTab(fpsPage, trText("FpsPresets"));

  if (resolutionPresets.empty())
    resolutionPresets = PluginConfig().resolutionPresets;
  if (fpsPresets.empty())
    fpsPresets = PluginConfig().fpsPresets;
  rebuildResolutions();
  rebuildFps();

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                       Qt::Horizontal, &dialog);
  root->addWidget(buttons);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted)
    return false;

  syncResolutions();
  syncFps();
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
  statusLabel_ = new QLabel(this);
  settingsButton_ = new QPushButton(trText("SettingsMenuButton"), this);
  settingsMenu_ = new PersistentCheckMenu(settingsButton_);
  settingsButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  settingsButton_->setFixedSize(52, 24);
  settingsButton_->setMinimumHeight(0);
  settingsButton_->raise();
  settingsButton_->setStyleSheet(QLatin1String(
    "padding: 0px 8px; min-height: 0px;"));

  replaySecondsSpin_->setRange(1, 36000);
  replaySecondsSpin_->setSuffix(trText("SecondsSuffix"));
  replayMegabytesSpin_->setRange(1, 1048576);
  replayMegabytesSpin_->setSuffix(trText("MegabytesSuffix"));
  replaySecondsSpin_->setFixedWidth(86);
  replayMegabytesSpin_->setFixedWidth(104);

  pathTemplateEdit_->setPlaceholderText(QLatin1String("{date}/{tag}"));
  manualTagEdit_->setPlaceholderText(trText("ManualTagPlaceholder"));

  previewLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  previewLabel_->setWordWrap(true);
  statusLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  statusLabel_->setWordWrap(true);

  auto *browseButton = new QPushButton(trText("Browse"), this);
  auto *baseLayout = new QHBoxLayout();
  baseLayout->setContentsMargins(0, 0, 0, 0);
  baseLayout->setSpacing(kControlSpacing);
  baseLayout->addWidget(baseDirectoryEdit_, 1);
  baseLayout->addWidget(browseButton);

  auto *templateEditButton = new QPushButton(trText("EditPathTemplateButton"), this);
  makeCompactButton(templateEditButton);

  auto *templateLayout = new QHBoxLayout();
  templateLayout->setContentsMargins(0, 0, 0, 0);
  templateLayout->setSpacing(kControlSpacing);
  templateLayout->addWidget(pathTemplateEdit_, 1);
  templateLayout->addWidget(templateEditButton);

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

  profileControl_ = makeLabeledControl(trText("Profile"), profileCombo_, this);
  sceneCollectionControl_ =
    makeLabeledControl(trText("SceneCollection"), sceneCollectionCombo_, this);
  profileSceneSection_ =
    makeWrapSection({profileControl_, sceneCollectionControl_}, this);

  resolutionControl_ = makeLabeledControl(trText("ResolutionPresets"), resolutionRow, this);
  fpsControl_ = makeLabeledControl(trText("FpsPresets"), fpsRow, this);
  videoPresetSection_ = makeWrapSection({resolutionControl_, fpsControl_}, this);

  replayControl_ = makeLabeledControl(trText("ReplayBuffer"), replayRow, this);

  baseDirectoryControl_ =
    makeLabeledControl(trText("BaseDirectory"), makeLayoutWidget(baseLayout, this), this);
  pathTemplateControl_ =
    makeLabeledControl(trText("PathTemplate"), makeLayoutWidget(templateLayout, this), this);
  manualTagControl_ = makeLabeledControl(trText("ManualTag"), manualTagEdit_, this);
  previewControl_ = makeInlineLabelControl(trText("Preview"), previewLabel_, this);
  statusControl_ = makeInlineLabelControl(trText("Status"), statusLabel_, this);

  auto *pathStatusLayout = new QVBoxLayout();
  pathStatusLayout->setContentsMargins(0, 0, 0, 0);
  pathStatusLayout->setSpacing(kControlSpacing);

  auto *pathPreviewLayout = new QHBoxLayout();
  pathPreviewLayout->setContentsMargins(0, 0, 0, 0);
  pathPreviewLayout->setSpacing(kControlSpacing * 2);
  pathPreviewLayout->addWidget(enablePathAutomationCheck_, 0);
  pathPreviewLayout->addWidget(previewControl_, 1);
  pathStatusLayout->addLayout(pathPreviewLayout);
  pathStatusLayout->addWidget(statusControl_, 0, Qt::AlignLeft);
  pathStatusRow_ = makeLayoutWidget(pathStatusLayout, this);

  replaySection_ = makeWrapSection({replayControl_, manualTagControl_}, this);
  pathSection_ = makeWrapSection({baseDirectoryControl_, pathTemplateControl_}, this);
  wrapSections_ = {profileSceneSection_, videoPresetSection_, replaySection_, pathSection_};

  contentWidget_ = new ResponsiveContentWidget(this);
  auto *contentLayout = new QVBoxLayout(contentWidget_);
  contentLayout->setContentsMargins(kFormMargin, kFormMargin, kFormMargin, kFormMargin);
  contentLayout->setSpacing(kControlSpacing);
  contentLayout->setAlignment(Qt::AlignTop);
  contentLayout->addWidget(profileSceneSection_);
  contentLayout->addWidget(videoPresetSection_);
  contentLayout->addWidget(replaySection_);
  contentLayout->addWidget(pathSection_);
  contentLayout->addWidget(pathStatusRow_);

  scrollArea_ = new QScrollArea(this);
  scrollArea_->setMinimumSize(0, 0);
  scrollArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  scrollArea_->setWidgetResizable(true);
  scrollArea_->setFrameShape(QFrame::NoFrame);
  scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scrollArea_->setWidget(contentWidget_);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(scrollArea_);
  settingsButton_->setParent(this);
  buildSettingsMenu();

  setUiFromConfig(controller_->loadConfig());
  refreshObsState();

  connect(sceneCollectionCombo_, &QComboBox::currentTextChanged, this, [this](const QString &value) {
    QString error;
    if (!controller_->setCurrentSceneCollection(value, &error))
      setStatusText(error, true);
    updatePreview();
  });
  connect(profileCombo_, &QComboBox::currentTextChanged, this, [this](const QString &value) {
    QString error;
    if (!controller_->setCurrentProfile(value, &error))
      setStatusText(error, true);
    updatePreview();
  });
  connect(browseButton, &QPushButton::clicked, this, &EasyConfigDock::browseBaseDirectory);
  connect(templateEditButton, &QPushButton::clicked, this, &EasyConfigDock::editPathTemplate);
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
    const QString previewPath = QString::fromUtf8(result.path.c_str(), -1);
    setPreviewText(previewPath);
    setStatusText(trText("Ready"));
    if (config.autoApplyBeforeRecording) {
      QString appliedPath;
      QString error;
      if (controller_->applyRecordingPath(config, &appliedPath, &error)) {
        if (!appliedPath.isEmpty())
          setPreviewText(appliedPath);
        setStatusText(trText("Applied"));
      } else {
        setStatusText(error, true);
      }
    }
  } else {
    if (!result.errors.empty())
      setStatusText(localizedError(result.errors.front()), true);
    else
      setStatusText(QString::fromUtf8(result.path.c_str(), -1), true);
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
    setStatusText(error, true);
    return;
  }

  setPreviewText(path);
  setStatusText(trText("Applied"));
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
    setStatusText(error, true);
    refreshOutputControls();
    return;
  }

  setStatusText(trText("ResolutionApplied"));
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
    setStatusText(error, true);
    refreshOutputControls();
    return;
  }

  setStatusText(trText("FpsApplied"));
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
    setStatusText(error, true);
    return;
  }

  config_.lastReplayBufferSeconds = settings.seconds;
  config_.lastReplayBufferMegabytes = settings.megabytes;
  saveCurrentConfig();
  setStatusText(trText("ReplayBufferApplied"));
}

void EasyConfigDock::editPathTemplate()
{
  QString pathTemplate = pathTemplateEdit_->text();
  const bool accepted = editPathTemplateDialog(
    this, baseDirectoryEdit_->text(), profileCombo_->currentText(),
    sceneCollectionCombo_->currentText(), controller_->currentSceneName(),
    manualTagEdit_->text(), &pathTemplate);

  if (!accepted)
    return;

  pathTemplateEdit_->setText(pathTemplate);
  updatePreview();
}

void EasyConfigDock::updateSectionVisibility()
{
  if (profileControl_)
    profileControl_->setVisible(config_.showProfile);
  if (sceneCollectionControl_)
    sceneCollectionControl_->setVisible(config_.showSceneCollection);
  if (profileSceneSection_)
    profileSceneSection_->setVisible(config_.showProfile || config_.showSceneCollection);
  if (resolutionControl_)
    resolutionControl_->setVisible(config_.showResolutionPresets);
  if (fpsControl_)
    fpsControl_->setVisible(config_.showFpsPresets);
  if (videoPresetSection_)
    videoPresetSection_->setVisible(config_.showResolutionPresets || config_.showFpsPresets);
  if (replaySection_)
    replaySection_->setVisible(config_.showReplayBuffer);
  if (baseDirectoryControl_)
    baseDirectoryControl_->setVisible(config_.showPathAutomation);
  if (pathTemplateControl_)
    pathTemplateControl_->setVisible(config_.showPathAutomation);
  if (manualTagControl_)
    manualTagControl_->setVisible(config_.showPathAutomation);
  if (previewControl_)
    previewControl_->setVisible(config_.showPathAutomation && config_.showPreviewStatus);
  if (statusControl_)
    statusControl_->setVisible(config_.showPreviewStatus);
  if (pathStatusRow_)
    pathStatusRow_->setVisible(config_.showPathAutomation || config_.showPreviewStatus);
  if (pathSection_)
    pathSection_->setVisible(config_.showPathAutomation);
  if (enablePathAutomationCheck_)
    enablePathAutomationCheck_->setVisible(config_.showPathAutomation);

  if (showProfileAction_)
    showProfileAction_->setChecked(config_.showProfile);
  if (showSceneCollectionAction_)
    showSceneCollectionAction_->setChecked(config_.showSceneCollection);
  if (showResolutionPresetsAction_)
    showResolutionPresetsAction_->setChecked(config_.showResolutionPresets);
  if (showFpsPresetsAction_)
    showFpsPresetsAction_->setChecked(config_.showFpsPresets);
  if (showReplayBufferAction_)
    showReplayBufferAction_->setChecked(config_.showReplayBuffer);
  if (showPathAutomationAction_)
    showPathAutomationAction_->setChecked(config_.showPathAutomation);
  if (showPreviewStatusAction_)
    showPreviewStatusAction_->setChecked(config_.showPreviewStatus);

  if (contentWidget_)
    contentWidget_->updateGeometry();
  if (scrollArea_)
    scrollArea_->updateGeometry();
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
  config.showProfile = config_.showProfile;
  config.showSceneCollection = config_.showSceneCollection;
  config.showResolutionPresets = config_.showResolutionPresets;
  config.showFpsPresets = config_.showFpsPresets;
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
    action->setChecked(config_.*field);
    connect(action, &QAction::toggled, this, [this, field](bool checked) {
      config_.*field = checked;
      updateSectionVisibility();
      saveCurrentConfig();
    });
    return action;
  };

  showProfileAction_ = addVisibilityAction("ShowProfile", &PluginConfig::showProfile);
  showSceneCollectionAction_ =
    addVisibilityAction("ShowSceneCollection", &PluginConfig::showSceneCollection);
  showResolutionPresetsAction_ =
    addVisibilityAction("ShowResolutionPresets", &PluginConfig::showResolutionPresets);
  showFpsPresetsAction_ = addVisibilityAction("ShowFpsPresets", &PluginConfig::showFpsPresets);
  showReplayBufferAction_ =
    addVisibilityAction("ShowReplayBuffer", &PluginConfig::showReplayBuffer);
  showPathAutomationAction_ =
    addVisibilityAction("ShowPathAutomation", &PluginConfig::showPathAutomation);
  showPreviewStatusAction_ =
    addVisibilityAction("ShowPreviewStatus", &PluginConfig::showPreviewStatus);

  settingsMenu_->addSeparator();
  connect(settingsMenu_->addAction(trText("EditVideoPresets")), &QAction::triggered,
          this, [this]() {
            auto resolutions = config_.resolutionPresets;
            auto fps = config_.fpsPresets;
            if (!editVideoPresetLists(this, resolutions, fps))
              return;
            config_.resolutionPresets = normalize_resolution_presets(resolutions);
            config_.fpsPresets = normalize_fps_presets(fps);
            refreshPresetButtons();
            saveCurrentConfig();
          });
  connect(settingsMenu_->addAction(trText("AboutPlugin")), &QAction::triggered,
          this, &EasyConfigDock::openAboutDialog);
}

void EasyConfigDock::openAboutDialog()
{
  QDialog dialog(this);
  dialog.setWindowTitle(trText("AboutPlugin"));
  auto *root = new QVBoxLayout(&dialog);
  auto *about = new QLabel(trText("AboutPluginText"), &dialog);
  about->setWordWrap(true);
  about->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
  root->addWidget(about);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
  root->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  dialog.exec();
}

void EasyConfigDock::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  if (!settingsButton_ || !scrollArea_)
    return;

  if (contentWidget_)
    contentWidget_->updateGeometry();
  for (QWidget *section : wrapSections_)
    section->updateGeometry();

  constexpr int margin = 6;
  const QRect viewportRect = scrollArea_->viewport()->geometry();
  settingsButton_->move(viewportRect.right() - settingsButton_->width() - margin,
                        viewportRect.bottom() - settingsButton_->height() - margin);
  settingsButton_->raise();
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
    setStatusText(trText("VideoSettingsLocked"));
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
    setStatusText(error, true);
}

void EasyConfigDock::setPreviewText(const QString &message, bool error)
{
  previewLabel_->setText(message);
  previewLabel_->setStyleSheet(error ? "color: #c62828;" : "color: palette(text);");
}

void EasyConfigDock::setStatusText(const QString &message, bool error)
{
  if (!statusLabel_)
    return;
  QString display = message.trimmed();
  while (display.endsWith(QLatin1Char('.')) || display.endsWith(QChar(0x3002)))
    display.chop(1);
  statusLabel_->setText(display);
  statusLabel_->setStyleSheet(error ? "color: #c62828;" : "color: palette(text);");
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
