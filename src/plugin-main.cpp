#include "easy-config-dock.hpp"
#include "obs-controller.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QAbstractButton>
#include <QDockWidget>
#include <QMainWindow>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QWidget>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-plugin-easy-config", "en-US")

namespace {

easy_config::ObsController *controller = nullptr;
QPointer<easy_config::EasyConfigDock> dock;
bool frontendExiting = false;

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
          QLatin1String("obs-plugin-easy-config")) == dock ||
        dockWidget->windowTitle() == moduleText("EasyConfig"))
      return dockWidget;
  }

  return nullptr;
}

bool isDockCloseButton(QAbstractButton *button)
{
  const QString objectName = button->objectName();
  if (objectName.contains(QLatin1String("close"), Qt::CaseInsensitive))
    return true;

  const QString closeText = moduleText("CloseDock");
  return button->toolTip() == closeText || button->accessibleName() == closeText ||
         button->text() == closeText;
}

void removeDockCloseButton(QMainWindow *mainWindow)
{
  QDockWidget *dockWidget = findDockWidget(mainWindow);
  if (!dockWidget)
    return;

  dockWidget->setFeatures(dockWidget->features() & ~QDockWidget::DockWidgetClosable);

  const auto buttons = dockWidget->findChildren<QAbstractButton *>();
  for (QAbstractButton *button : buttons) {
    if (!isDockCloseButton(button))
      continue;

    button->hide();
    button->setEnabled(false);
  }
}

void scheduleRemoveDockCloseButton(QMainWindow *mainWindow)
{
  removeDockCloseButton(mainWindow);

  QTimer::singleShot(0, mainWindow, [mainWindow]() { removeDockCloseButton(mainWindow); });
  QTimer::singleShot(250, mainWindow, [mainWindow]() { removeDockCloseButton(mainWindow); });
  QTimer::singleShot(1000, mainWindow, [mainWindow]() { removeDockCloseButton(mainWindow); });
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
  scheduleRemoveDockCloseButton(mainWindow);
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
