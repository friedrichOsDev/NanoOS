#!/bin/bash

echo "Available USB sticks:"
diskutil list external physical

read -r -p "Enter the device name of the USB stick (e.g., disk2): " DEVICE_NAME

if [ "$DEVICE_NAME" = "disk0" ]; then
    echo "Error: disk0 is usually your main system drive! Refusing to flash."
    exit 1
fi

USB_DEVICE="/dev/$DEVICE_NAME"
RAW_USB_DEVICE="/dev/r$DEVICE_NAME"

if [ -b "$USB_DEVICE" ]; then
    echo "Unmounting $USB_DEVICE so we can write to it..."
    diskutil unmountDisk "$USB_DEVICE"

    echo "Flashing build/nanoos.iso to $RAW_USB_DEVICE..."
    sudo dd if=build/nanoos.iso of="$RAW_USB_DEVICE" bs=4m status=progress

    echo "Syncing filesystem caches..."
    sync

    echo "Ejecting $USB_DEVICE..."
    diskutil eject "$USB_DEVICE"
    echo "Done."
else
    echo "Error: Device $USB_DEVICE not found."
    exit 1
fi