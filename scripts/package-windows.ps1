param(
    [string]$BuildDir = "build-plugin",
    [string]$Version = "dev",
    [string]$Configuration = "RelWithDebInfo"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$DistDir = Join-Path $RepoRoot "dist"
$StageDir = Join-Path $RepoRoot "build/package/windows"
$InstallerScript = Join-Path $RepoRoot "packaging/windows/obs-plugin-easy-config.nsi"
$PackageName = "obs-plugin-easy-config-$Version-windows-x64"

Remove-Item -Recurse -Force $StageDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $StageDir, $DistDir | Out-Null

cmake --install (Join-Path $RepoRoot $BuildDir) --prefix $StageDir --config $Configuration

$PluginDll = Join-Path $StageDir "obs-plugins/64bit/obs-plugin-easy-config.dll"
if (!(Test-Path $PluginDll)) {
    throw "Missing staged Windows plugin binary: $PluginDll"
}

$ZipPath = Join-Path $DistDir "$PackageName.zip"
Remove-Item -Force $ZipPath -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $StageDir "*") -DestinationPath $ZipPath
Write-Output $ZipPath

$Makensis = Get-Command makensis -ErrorAction SilentlyContinue
if ($Makensis) {
    $ExePath = Join-Path $DistDir "$PackageName-installer.exe"
    & $Makensis.Source `
        "/DPRODUCT_VERSION=$Version" `
        "/DSTAGE_DIR=$StageDir" `
        "/DOUTPUT_FILE=$ExePath" `
        $InstallerScript
    Write-Output $ExePath
} else {
    Write-Warning "makensis was not found; skipped Windows installer creation."
}
