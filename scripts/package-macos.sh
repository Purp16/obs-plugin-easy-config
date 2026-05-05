#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build-plugin}"
version="${2:-dev}"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_dir="${repo_root}/dist"
stage_dir="${repo_root}/build/package/macos"
root_dir="${repo_root}/build/package/macos-root"
plugin_name="obs-plugin-easy-config.plugin"
package_name="obs-plugin-easy-config-${version}-macos"
install_location="/Library/Application Support/obs-studio/plugins"
bundle_id="${MACOS_PKG_IDENTIFIER:-com.github.obs-plugin-easy-config.pkg}"
pkg_version="${version#v}"
pkg_version="${pkg_version%%-*}"
if [[ ! "$pkg_version" =~ ^[0-9]+([.][0-9]+)*$ ]]; then
  pkg_version="0"
fi

rm -rf "$stage_dir" "$root_dir"
mkdir -p "$stage_dir" "$root_dir${install_location}" "$dist_dir"

cmake --install "${repo_root}/${build_dir}" --prefix "$stage_dir" --config RelWithDebInfo

if [[ ! -d "${stage_dir}/${plugin_name}" ]]; then
  echo "missing staged macOS plugin bundle" >&2
  exit 1
fi

cp -R "${stage_dir}/${plugin_name}" "$root_dir${install_location}/"

pkgbuild \
  --root "$root_dir" \
  --identifier "$bundle_id" \
  --version "$pkg_version" \
  --install-location "/" \
  "${dist_dir}/${package_name}.pkg"

tar -C "$stage_dir" -czf "${dist_dir}/${package_name}.tar.gz" "$plugin_name"

echo "${dist_dir}/${package_name}.pkg"
echo "${dist_dir}/${package_name}.tar.gz"
