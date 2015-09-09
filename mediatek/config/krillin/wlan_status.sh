#!/system/bin/sh 

wlan_prop=$(getprop wlan.driver.status)
wlan_addr_path=/sys/devices/platform/mt-wifi/net/wlan0/address

if [[ $wlan_prop == "ok" ]];then
    for i in 1 2 3 4 5; do
        sleep 1
        if [ ! -f $wlan_addr_path ]; then
            echo 1 > /dev/wmtWifi
        else
            break
        fi
    done
fi

if [[ $wlan_prop == "unloaded" ]];then
    for i in 1 2 3 4 5; do
        sleep 1
        if [ -f $wlan_addr_path ]; then
            echo 0 > /dev/wmtWifi
        else
            break
        fi
    done
fi
exit
