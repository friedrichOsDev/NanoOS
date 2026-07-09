rm -rf screen.log
screen -L -Logfile screen.log /dev/tty.usbserial-1420 115200
clear
cat screen.log
