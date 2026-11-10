#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=../container_runtime.sh
source "$SCRIPT_DIR/../container_runtime.sh"

nanoos_run_make "$@"
