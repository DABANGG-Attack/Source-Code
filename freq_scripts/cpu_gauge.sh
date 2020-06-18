#!/bin/bash

while true; do

clear
echo "Frequencies in KHz: "
sudo cat /sys/devices/system/cpu/cpufreq/policy*/cpuinfo_cur_freq
sleep 0.5

done
