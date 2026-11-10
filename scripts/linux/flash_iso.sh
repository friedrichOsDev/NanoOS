#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"

if ! command -v lsblk >/dev/null 2>&1; then
    echo "Error: lsblk is required. This script is for Linux." >&2
    exit 1
fi

echo "Available drives:"
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT
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

read -r -p "Enter the whole disk device (e.g. /dev/sdb or sdb): " DEVICE_NAME
if [[ "$DEVICE_NAME" != /dev/* ]]; then
    DEVICE_NAME="/dev/$DEVICE_NAME"
fi

if [[ ! -b "$DEVICE_NAME" ]]; then
    echo "Error: Block device not found: $DEVICE_NAME" >&2
    exit 1
fi

DEVICE_TYPE="$(lsblk -dn -o TYPE "$DEVICE_NAME")"
if [[ "$DEVICE_TYPE" != "disk" ]]; then
    echo "Error: Select a whole disk, not a partition or loop device." >&2
    exit 1
fi

if lsblk -nr -o MOUNTPOINT "$DEVICE_NAME" | grep -q '[^[:space:]]'; then
    echo "Error: One or more partitions on $DEVICE_NAME are mounted. Unmount them first." >&2
    exit 1
fi

echo
echo "WARNING: ALL DATA ON $DEVICE_NAME WILL BE DESTROYED."
read -r -p "Type YES to continue: " CONFIRM
if [[ "$CONFIRM" != "YES" ]]; then
    echo "Aborted."
    exit 1
fi

echo "Flashing $ISO_PATH to $DEVICE_NAME..."
sudo dd if="$ISO_PATH" of="$DEVICE_NAME" bs=4M status=progress conv=fsync
sync
echo "Done."
