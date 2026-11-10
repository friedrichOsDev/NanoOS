#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
SERIAL_DEVICE="${1:-${NANOOS_SERIAL_DEVICE:-/dev/tty.usbserial-1420}}"
LOG_FILE="${NANOOS_SERIAL_LOG:-$REPO_ROOT/screen.log}"

if ! command -v screen >/dev/null 2>&1; then
    echo "Error: GNU Screen is required (screen)." >&2
    exit 1
fi

if [[ ! -e "$SERIAL_DEVICE" ]]; then
    echo "Error: Serial device not found: $SERIAL_DEVICE" >&2
    echo "Pass the device as the first argument or set NANOOS_SERIAL_DEVICE." >&2
    exit 1
fi

echo "Opening $SERIAL_DEVICE at 115200 baud. Press Ctrl+A, then K to close Screen."
screen -L -Logfile "$LOG_FILE" "$SERIAL_DEVICE" 115200

if [[ -f "$LOG_FILE" ]]; then
    echo "Serial log: $LOG_FILE"
    cat "$LOG_FILE"
fi
