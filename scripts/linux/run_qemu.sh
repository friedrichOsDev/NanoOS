#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"

QEMU_BIN="${QEMU_BIN:-qemu-system-x86_64}"
OVMF_CODE="${OVMF_CODE:-$REPO_ROOT/uefi/OVMF_CODE.4m.fd}"
OVMF_VARS="${OVMF_VARS:-$REPO_ROOT/uefi/my_OVMF_VARS.4m.fd}"
QEMU_LOG="${QEMU_LOG:-$REPO_ROOT/qemu.log}"
SERIAL_LOG="${SERIAL_LOG:-$REPO_ROOT/serial.log}"
ISO_IMAGE="$REPO_ROOT/build/nanoos.iso"
DEBUG_GDB=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)
            ISO_IMAGE="$REPO_ROOT/build/nanoos-debug.iso"
            shift
            ;;
        --gdb)
            ISO_IMAGE="$REPO_ROOT/build/nanoos-debug.iso"
            DEBUG_GDB=1
            shift
            ;;
        *)
            echo "Usage: $0 [--debug|--gdb]" >&2
            exit 2
            ;;
    esac
done

if ! command -v "$QEMU_BIN" >/dev/null 2>&1; then
    echo "Error: $QEMU_BIN was not found in PATH." >&2
    exit 1
fi

for required_file in "$OVMF_CODE" "$OVMF_VARS" "$ISO_IMAGE"; do
    if [[ ! -f "$required_file" ]]; then
        echo "Error: Required file not found: $required_file" >&2
        exit 1
    fi
done

DISPLAY_BACKEND="${NANOOS_QEMU_DISPLAY:-gtk,gl=off,zoom-to-fit=off}"

qemu_args=(
    -machine q35
    -drive "if=pflash,format=raw,readonly=on,file=$OVMF_CODE"
    -drive "if=pflash,format=raw,file=$OVMF_VARS"
    -smp 4
    -m 8G
    -vga virtio
    -display "$DISPLAY_BACKEND"
    -cdrom "$ISO_IMAGE"
    -d int,cpu_reset
    -D "$QEMU_LOG"
    -serial "file:$SERIAL_LOG"
)

if [[ "$DEBUG_GDB" -eq 1 ]]; then
    qemu_args+=(-S -s)
    echo "QEMU is paused. Connect with: gdb build/kernel.debug -ex 'target remote localhost:1234'"
fi

cd "$REPO_ROOT"
exec "$QEMU_BIN" "${qemu_args[@]}"
