#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 <owner/repo> <tag> <asset-name> <output-path>" >&2
  exit 64
fi

repo="$1"
tag="$2"
asset_name="$3"
output_path="$4"

mkdir -p "$(dirname "$output_path")"

python_bin="${PYTHON:-}"
if [[ -z "$python_bin" ]]; then
  if command -v python3 >/dev/null 2>&1; then
    python_bin="python3"
  elif command -v python >/dev/null 2>&1; then
    python_bin="python"
  else
    echo "python3 or python is required" >&2
    exit 69
  fi
fi

api_url="https://api.github.com/repos/${repo}/releases/tags/${tag}"
auth_args=()
if [[ -n "${GITHUB_TOKEN:-}" ]]; then
  auth_args=(-H "Authorization: Bearer ${GITHUB_TOKEN}")
fi

asset_url="$(
  curl -fsSL "${auth_args[@]}" "$api_url" |
    "$python_bin" -c '
import json
import sys

release = json.load(sys.stdin)
asset_name = sys.argv[1]
for asset in release.get("assets", []):
    if asset.get("name") == asset_name:
        print(asset["browser_download_url"])
        break
else:
    print(f"asset not found: {asset_name}", file=sys.stderr)
    sys.exit(1)
' "$asset_name"
)"

curl -fL "${auth_args[@]}" -o "$output_path" "$asset_url"
