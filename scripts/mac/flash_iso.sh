#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"

if ! command -v diskutil >/dev/null 2>&1; then
    echo "Error: diskutil is not available. This script is for macOS." >&2
    exit 1
fi

echo "Available external physical drives:"
diskutil list external physical
echo
echo "Select ISO to flash:"
echo "  1) nanoos.iso"
echo "  2) nanoos-debug.iso"
read -r -p "Enter choice [1 or 2]: " ISO_CHOICE

case "$ISO_CHOICE" in
    2) ISO_PATH="$REPO_ROOT/build/nanoos-debug.iso" ;;
    1|"") ISO_PATH="$REPO_ROOT/build/nanoos.iso" ;;
    *) echo "Error: Invalid ISO choice." >&2; exit 2 ;;
esac

if [[ ! -f "$ISO_PATH" ]]; then
    echo "Error: ISO not found: $ISO_PATH" >&2
    exit 1
fi

read -r -p "Enter the whole disk identifier (e.g. disk2, not disk2s1): " DEVICE_NAME
if [[ ! "$DEVICE_NAME" =~ ^disk[0-9]+$ ]]; then
    echo "Error: Invalid disk identifier. Use a whole disk such as disk2." >&2
    exit 1
fi

if [[ "$DEVICE_NAME" == "disk0" ]]; then
    echo "Error: Refusing to flash disk0." >&2
    exit 1
fi

USB_DEVICE="/dev/$DEVICE_NAME"
RAW_USB_DEVICE="/dev/r$DEVICE_NAME"

if [[ ! -b "$USB_DEVICE" ]]; then
    echo "Error: Block device not found: $USB_DEVICE" >&2
    exit 1
fi

if ! diskutil info "$USB_DEVICE" | grep -q "Whole: *Yes"; then
    echo "Error: $USB_DEVICE is not a whole physical disk." >&2
    exit 1
fi

echo
echo "WARNING: ALL DATA ON $USB_DEVICE WILL BE DESTROYED."
read -r -p "Type YES to continue: " CONFIRM
if [[ "$CONFIRM" != "YES" ]]; then
    echo "Aborted."
    exit 1
fi

diskutil unmountDisk "$USB_DEVICE"
echo "Flashing $ISO_PATH to $RAW_USB_DEVICE..."
sudo dd if="$ISO_PATH" of="$RAW_USB_DEVICE" bs=4m
sync
diskutil eject "$USB_DEVICE"
echo "Done."
