echo "Available USB sticks:"
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT | grep "disk"
echo ""
echo "Select ISO to flash:"
echo "  1) nanoos.iso"
echo "  2) nanoos-debug.iso"
read -p "Enter choice [1 or 2]: " ISO_CHOICE

case $ISO_CHOICE in
    2)
        ISO_PATH="build/nanoos-debug.iso"
        ;;
    *)
        ISO_PATH="build/nanoos.iso"
        ;;
esac

read -p "Enter the device name of the USB stick (e.g., sdb): " DEVICE_NAME
USB_DEVICE="/dev/$DEVICE_NAME"

if [ -b "$USB_DEVICE" ]; then
    echo "Flashing $ISO_PATH to $USB_DEVICE..."
    sudo dd if="$ISO_PATH" of="$USB_DEVICE" bs=4M status=progress conv=fsync
    echo "Done."
else
    echo "Error: Device $USB_DEVICE not found."
    exit 1
fi
