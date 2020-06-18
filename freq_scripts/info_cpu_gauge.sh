#!/bin/bash

while true; do

clear
echo "Frequencies in KHz: "
cat /proc/cpuinfo | grep -E "MHz|processor"
sleep 0.5

done
