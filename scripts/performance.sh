echo "CPU0-3 :"
sudo cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies
sudo echo userspace > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
sudo echo 1800000 > /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed
echo "CPU0-3 set:"
sudo cat /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq

echo "CPU4-5:"
sudo cat /sys/devices/system/cpu/cpufreq/policy4/scaling_available_frequencies
sudo echo userspace > /sys/devices/system/cpu/cpufreq/policy4/scaling_governor
sudo echo 2400000 > /sys/devices/system/cpu/cpufreq/policy4/scaling_setspeed
echo "CPU4-5 set:"
sudo cat /sys/devices/system/cpu/cpufreq/policy4/cpuinfo_cur_freq

echo "CPU6-7:"
sudo cat /sys/devices/system/cpu/cpufreq/policy6/scaling_available_frequencies
sudo echo userspace > /sys/devices/system/cpu/cpufreq/policy6/scaling_governor
sudo echo 2400000 > /sys/devices/system/cpu/cpufreq/policy6/scaling_setspeed
echo "CPU6-7 set:"
sudo cat /sys/devices/system/cpu/cpufreq/policy6/cpuinfo_cur_freq

echo "NPU:"
sudo cat /sys/class/devfreq/fdab0000.npu/available_frequencies    
sudo echo userspace > /sys/class/devfreq/fdab0000.npu/governor
sudo echo 1000000000 > /sys/class/devfreq/fdab0000.npu/userspace/set_freq
echo "NPU set:"
sudo cat /sys/class/devfreq/fdab0000.npu/cur_freq