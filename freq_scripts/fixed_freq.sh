sudo -s <<EOF

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy1/scaling_min_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy2/scaling_min_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy3/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy4/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy5/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy6/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy7/scaling_min_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy9/scaling_min_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy10/scaling_min_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy11/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy12/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy13/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy14/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy15/scaling_min_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy1/scaling_max_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy2/scaling_max_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy3/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy5/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy6/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy7/scaling_max_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy9/scaling_max_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy10/scaling_max_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy11/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy12/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy13/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy14/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy15/scaling_max_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy8/scaling_min_freq

echo -n $1 > /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq
echo -n $1 > /sys/devices/system/cpu/cpufreq/policy8/scaling_max_freq

echo $1 "frequencies enabled"
EOF

