rm -f screen.log
screen -L -Logfile screen.log /dev/ttyNanoOS 115200
clear
bat screen.log
