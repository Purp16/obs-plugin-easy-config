#include "easy-config-dock.hpp"
#include "obs-controller.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QStyle>
#include <QString>
#include <QToolButton>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-plugin-easy-config", "en-US")

namespace {

easy_config::ObsController *controller = nullptr;
easy_config::EasyConfigDock *dock = nullptr;

QString moduleText(const char *key)
{
  return QString::fromUtf8(obs_module_text(key), -1);
}

void applyCompactDockTitleBar(QMainWindow *mainWindow)
{
  auto *dockWidget = mainWindow->findChild<QDockWidget *>(QLatin1String("obs-plugin-easy-config"));
  if (!dockWidget)
    return;

  auto *titleBar = new QWidget(dockWidget);
  auto *layout = new QHBoxLayout(titleBar);
  layout->setContentsMargins(3, 0, 2, 0);
  layout->setSpacing(2);

  auto *title = new QLabel(moduleText("EasyConfig"), titleBar);
  QFont titleFont = title->font();
  const qreal pointSize = titleFont.pointSizeF() > 0 ? titleFont.pointSizeF() : 11.0;
  titleFont.setPointSizeF(std::max(8.0, pointSize - 2.0));
  titleFont.setBold(false);
  title->setFont(titleFont);
  title->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

  auto *closeButton = new QToolButton(titleBar);
  closeButton->setAutoRaise(true);
  closeButton->setIcon(titleBar->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
  closeButton->setIconSize(QSize(8, 8));
  closeButton->setFixedSize(14, 14);
  closeButton->setToolTip(moduleText("CloseDock"));
  QObject::connect(closeButton, &QToolButton::clicked, dockWidget, &QDockWidget::close);

  layout->addWidget(title, 1);
  layout->addWidget(closeButton, 0, Qt::AlignVCenter);
  titleBar->setFixedHeight(17);
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
  applyCompactDockTitleBar(mainWindow);
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
