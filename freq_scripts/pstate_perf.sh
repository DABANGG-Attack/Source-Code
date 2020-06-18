sudo -s <<EOF
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy1/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy2/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy3/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy4/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy5/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy6/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy7/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy8/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy9/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy10/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy11/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy12/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy13/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy14/scaling_governor
echo -n "performance" > /sys/devices/system/cpu/cpufreq/policy15/scaling_governor
echo "performance governor enabled"
EOF

