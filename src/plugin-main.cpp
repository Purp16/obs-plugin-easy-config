#include "easy-config-dock.hpp"
#include "obs-controller.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPointer>
#include <QString>
#include <QToolButton>
#include <QWidget>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-plugin-easy-config", "en-US")

namespace {

easy_config::ObsController *controller = nullptr;
QPointer<easy_config::EasyConfigDock> dock;
bool frontendExiting = false;
constexpr auto *kDockTitleBarObjectName = "obs-plugin-easy-config-titlebar";

QString moduleText(const char *key)
{
  return QString::fromUtf8(obs_module_text(key), -1);
}

QDockWidget *findDockWidget(QMainWindow *mainWindow)
{
  if (!mainWindow || !dock)
    return nullptr;

  for (QWidget *parent = dock->parentWidget(); parent; parent = parent->parentWidget()) {
    if (auto *dockWidget = qobject_cast<QDockWidget *>(parent))
      return dockWidget;
  }

  const auto dockWidgets = mainWindow->findChildren<QDockWidget *>();
  for (QDockWidget *dockWidget : dockWidgets) {
    if (dockWidget->widget() == dock ||
        dockWidget->findChild<easy_config::EasyConfigDock *>(
          QLatin1String("obs-plugin-easy-config")) == dock)
      return dockWidget;
  }

  return nullptr;
}

class DockTitleBar final : public QWidget {
public:
  explicit DockTitleBar(QDockWidget *dockWidget, const QString &title)
    : QWidget(dockWidget), dockWidget_(dockWidget)
  {
    setObjectName(QLatin1String(kDockTitleBarObjectName));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 0, 4, 0);
    layout->setSpacing(4);

    auto *titleLabel = new QLabel(title, this);
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    floatButton_ = new QToolButton(this);
    floatButton_->setObjectName(QLatin1String("obs-plugin-easy-config-floatbutton"));
    floatButton_->setAutoRaise(true);
    floatButton_->setFocusPolicy(Qt::NoFocus);
    floatButton_->setFixedSize(18, 18);

    layout->addWidget(titleLabel);
    layout->addWidget(floatButton_);

    QObject::connect(floatButton_, &QToolButton::clicked, this, [this]() {
      if (dockWidget_)
        dockWidget_->setFloating(!dockWidget_->isFloating());
    });
    QObject::connect(dockWidget, &QDockWidget::topLevelChanged, this,
                     [this]() { refreshFloatButton(); });

    refreshFloatButton();
  }

protected:
  void mousePressEvent(QMouseEvent *event) override { event->ignore(); }
  void mouseMoveEvent(QMouseEvent *event) override { event->ignore(); }
  void mouseReleaseEvent(QMouseEvent *event) override { event->ignore(); }
  void mouseDoubleClickEvent(QMouseEvent *event) override { event->ignore(); }

private:
  void refreshFloatButton()
  {
    if (!dockWidget_ || !floatButton_)
      return;

    const bool floating = dockWidget_->isFloating();
    floatButton_->setText(floating ? QString::fromLatin1("D") : QString::fromLatin1("F"));
    floatButton_->setToolTip(floating ? QObject::tr("Dock") : QObject::tr("Float"));
  }

  QPointer<QDockWidget> dockWidget_;
  QToolButton *floatButton_ = nullptr;
};

void installDockTitleBar(QMainWindow *mainWindow)
{
  QDockWidget *dockWidget = findDockWidget(mainWindow);
  if (!dockWidget)
    return;

  dockWidget->setFeatures(dockWidget->features() & ~QDockWidget::DockWidgetClosable);

  QWidget *currentTitleBar = dockWidget->titleBarWidget();
  if (currentTitleBar &&
      currentTitleBar->objectName() == QLatin1String(kDockTitleBarObjectName))
    return;

  dockWidget->setTitleBarWidget(new DockTitleBar(dockWidget, moduleText("EasyConfig")));
  dockWidget->updateGeometry();
  dockWidget->update();
}

} // namespace

const char *obs_module_description()
{
  return obs_module_text("ModuleDescription");
}

bool obs_module_load()
{
  frontendExiting = false;
  auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
  if (!mainWindow)
    return false;

  controller = new easy_config::ObsController();
  dock = new easy_config::EasyConfigDock(controller);
  dock->setObjectName(QLatin1String("obs-plugin-easy-config"));
  dock->setWindowTitle(moduleText("EasyConfig"));

  if (!obs_frontend_add_dock_by_id("obs-plugin-easy-config", obs_module_text("EasyConfig"),
                                   dock.data())) {
    delete dock;
    dock = nullptr;
    delete controller;
    controller = nullptr;
    return false;
  }
  installDockTitleBar(mainWindow);
  blog(LOG_INFO, "[obs-plugin-easy-config] loaded");
  return true;
}

void obs_module_unload()
{
  frontendExiting = frontendExiting || (controller && controller->isFrontendExiting());

  if (dock) {
    if (!frontendExiting)
      obs_frontend_remove_dock("obs-plugin-easy-config");
    dock = nullptr;
  }

  if (controller) {
    controller->markFrontendExiting();
    delete controller;
    controller = nullptr;
  }

  dock = nullptr;
  blog(LOG_INFO, "[obs-plugin-easy-config] unloaded");
}
