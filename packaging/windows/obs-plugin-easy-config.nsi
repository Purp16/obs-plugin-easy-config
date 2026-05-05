Unicode true
ManifestDPIAware true
RequestExecutionLevel admin

!ifndef PRODUCT_VERSION
  !define PRODUCT_VERSION "dev"
!endif

!ifndef STAGE_DIR
  !error "STAGE_DIR is required"
!endif

!ifndef OUTPUT_FILE
  !define OUTPUT_FILE "obs-plugin-easy-config-${PRODUCT_VERSION}-windows-x64-installer.exe"
!endif

!define PRODUCT_NAME "OBS Plugin Easy Config"
!define PLUGIN_ID "obs-plugin-easy-config"

Name "${PRODUCT_NAME}"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\obs-studio"
InstallDirRegKey HKLM "Software\OBS Studio" ""
ShowInstDetails show
ShowUninstDetails show

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "Install"
  SetShellVarContext all
  SetOutPath "$INSTDIR"

  IfFileExists "$INSTDIR\bin\64bit\obs64.exe" obs_found obs_missing

obs_missing:
  MessageBox MB_ICONEXCLAMATION|MB_OK "OBS Studio was not found in the selected directory. Select the OBS Studio install directory that contains bin\64bit\obs64.exe."
  Abort

obs_found:
  DetailPrint "Installing to OBS Studio: $INSTDIR"

install_files:
  SetOutPath "$INSTDIR\obs-plugins\64bit"
  File "${STAGE_DIR}\obs-plugins\64bit\obs-plugin-easy-config.dll"

  SetOutPath "$INSTDIR\data\obs-plugins\${PLUGIN_ID}\locale"
  File /nonfatal "${STAGE_DIR}\data\obs-plugins\${PLUGIN_ID}\locale\*.ini"

  WriteUninstaller "$INSTDIR\uninstall-${PLUGIN_ID}.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\obs-plugins\64bit\obs-plugin-easy-config.dll"
  RMDir /r "$INSTDIR\data\obs-plugins\${PLUGIN_ID}"
  Delete "$INSTDIR\uninstall-${PLUGIN_ID}.exe"
SectionEnd
