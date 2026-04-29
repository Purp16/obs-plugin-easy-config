#include "easy-config-dock.hpp"
#include "obs-controller.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QDockWidget>
#include <QMainWindow>
#include <QTimer>
#include <QString>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-plugin-easy-config", "en-US")

namespace {

easy_config::ObsController *controller = nullptr;
easy_config::EasyConfigDock *dock = nullptr;

QString moduleText(const char *key)
{
  return QString::fromUtf8(obs_module_text(key), -1);
}

void removeDockCloseButton(QMainWindow *mainWindow)
{
  auto *dockWidget = mainWindow->findChild<QDockWidget *>(QLatin1String("obs-plugin-easy-config"));
  if (!dockWidget)
    return;

  dockWidget->setFeatures(dockWidget->features() & ~QDockWidget::DockWidgetClosable);
}

} // namespace

const char *obs_module_description()
{
  return obs_module_text("ModuleDescription");
}

bool obs_module_load()
{
  auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
  if (!mainWindow)
    return false;

  controller = new easy_config::ObsController(mainWindow);
  dock = new easy_config::EasyConfigDock(controller, mainWindow);
  dock->setObjectName(QLatin1String("obs-plugin-easy-config"));
  dock->setWindowTitle(moduleText("EasyConfig"));

  obs_frontend_add_dock_by_id("obs-plugin-easy-config", obs_module_text("EasyConfig"), dock);
  removeDockCloseButton(mainWindow);
  QTimer::singleShot(0, mainWindow, [mainWindow]() {
    removeDockCloseButton(mainWindow);
  });
  blog(LOG_INFO, "[obs-plugin-easy-config] loaded");
  return true;
}

void obs_module_unload()
{
  obs_frontend_remove_dock("obs-plugin-easy-config");
  delete dock;
  dock = nullptr;
  delete controller;
  controller = nullptr;
  blog(LOG_INFO, "[obs-plugin-easy-config] unloaded");
}
