#!/bin/sh
source /etc/profile

# First thing, set background color which will apply on next redraw
echo 0xFF222222 > /sys/module/sun8i_mixer/parameters/border_color
clear

/opt/winglet-gui/bin/winglet-gui
ret=$?

# Enable cursor after we close out of winglet gui
clear
printf '\033[?25h'

if [ $ret = 40 ]; then
    TERM=xterm-256color /usr/libexec/cboy_menu.py
elif [ $ret = 41 ]; then
    echo "Badge Shutting Down..."
    poweroff
elif [ $ret = 42 ]; then
    echo "Badge Rebooting..."
    reboot
elif [ $ret = 43 ]; then
    /sbin/getty -L tty1 0 linux
elif [ $ret = 44 ]; then
    echo -e "\033[1;93mTerminating UI for Hardware In Loop Dev\033[0m"
    echo "You can now deploy via Qt Creator"
    echo
    echo "NOTE: To exit, you must run the reboot command from ssh"
    echo "or tap the reset button on the back of the device."
    stty -echo
    while true; do
        cat /dev/stdin > /dev/null
    done
elif [ $ret = 45 ]; then
    python3 /usr/libexec/attest_device.py
fi

# Else, must have crashed, just reboot
