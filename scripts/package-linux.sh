#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build-plugin}"
version="${2:-dev}"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dist_dir="${repo_root}/dist"
stage_dir="${repo_root}/build/package/linux"
package_name="obs-plugin-easy-config-${version}-linux-x86_64"

rm -rf "$stage_dir"
mkdir -p "$stage_dir" "$dist_dir"

cmake --install "${repo_root}/${build_dir}" --prefix "$stage_dir" --config RelWithDebInfo

if [[ ! -f "${stage_dir}/obs-plugin-easy-config/bin/64bit/obs-plugin-easy-config.so" ]]; then
  echo "missing staged Linux plugin binary" >&2
  exit 1
fi

tar -C "$stage_dir" -czf "${dist_dir}/${package_name}.tar.gz" obs-plugin-easy-config
echo "${dist_dir}/${package_name}.tar.gz"
