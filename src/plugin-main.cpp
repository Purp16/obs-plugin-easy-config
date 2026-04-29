#include "easy-config-dock.hpp"
#include "obs-controller.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QStyle>
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

void applyDockTitleBar(QMainWindow *mainWindow)
{
  auto *dockWidget = mainWindow->findChild<QDockWidget *>(QLatin1String("obs-plugin-easy-config"));
  if (!dockWidget)
    return;

  auto *titleBar = new QWidget(dockWidget);
  titleBar->setObjectName(QLatin1String("easy-config-titlebar"));
  titleBar->setFixedHeight(34);

  auto *layout = new QHBoxLayout(titleBar);
  layout->setContentsMargins(8, 0, 8, 0);
  layout->setSpacing(6);

  auto *icon = new QLabel(titleBar);
  icon->setPixmap(titleBar->style()->standardIcon(QStyle::SP_FileDialogDetailedView)
                    .pixmap(18, 18));
  icon->setFixedSize(20, 20);
  icon->setAlignment(Qt::AlignCenter);

  auto *title = new QLabel(moduleText("EasyConfig"), titleBar);
  title->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  layout->addWidget(icon);
  layout->addWidget(title, 1);
  dockWidget->setTitleBarWidget(titleBar);
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
  applyDockTitleBar(mainWindow);
  QTimer::singleShot(0, mainWindow, [mainWindow]() {
    applyDockTitleBar(mainWindow);
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
