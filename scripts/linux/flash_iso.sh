echo "Available USB sticks:"
lsblk -o NAME,SIZE,TYPE,MOUNTPOINT | grep "disk"
read -p "Enter the device name of the USB stick (e.g., sdb): "
USB_DEVICE="/dev/$REPLY"

if [ -b "$USB_DEVICE" ]; then
    echo "Flashing build/nanoos.iso to $USB_DEVICE..."
    sudo dd if=build/nanoos.iso of="$USB_DEVICE" bs=4M status=progress conv=fsync
    echo "Done."
else
    echo "Error: Device $USB_DEVICE not found."
    exit 1
fi
