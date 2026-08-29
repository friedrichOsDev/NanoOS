rm -f screen.log
screen -L -Logfile screen.log /dev/ttyUSB0 115200
clear
bat screen.log
