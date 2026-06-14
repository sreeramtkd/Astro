#!/bin/bash

# Ensure script is run as root
if [ "$EUID" -ne 0 ]; then 
  echo "Please run as root"
  exit
fi

# Function to handle errors
check_status() {
    if [ $? -ne 0 ]; then
        echo "----------------------------------------------------"
        echo "ERROR: The last command failed ($1)."
        echo "Please check the error logs above."
        echo "----------------------------------------------------"
        read -p "Do you want to continue despite this error? (y/N): " choice
        case "$choice" in 
          y|Y ) echo "Continuing...";;
          * ) echo "Aborting installation."; exit 1;;
        esac
    fi
}

echo "Starting PINS automated configuration..."

# 1. System Configuration
raspi-config nonint do_i2c 0
raspi-config nonint do_wifi_country IN
timedatectl set-timezone Asia/Kolkata
sudo timedatectl set-ntp true
check_status "System Configuration"

# 2. Update and Install Dependencies
apt-get update && apt-get upgrade -y
apt-get install -y gphoto2 util-linux-extra i2c-tools python3-pip
pip install sh --break-system-packages
check_status "Package Installation"

# 3. RTC Configuration
if ! grep -q "dtoverlay=i2c-rtc,ds3231" /boot/firmware/config.txt; then
    echo "dtoverlay=i2c-rtc,ds3231" >> /boot/firmware/config.txt
fi
check_status "RTC Config File Update"

# 4. RTC Sync Service
wget https://raw.githubusercontent.com/sreeramtkd/Astro/refs/heads/main/rtc_sync.py -O /usr/local/bin/rtc_sync.py
chmod +x /usr/local/bin/rtc_sync.py
check_status "RTC Sync Script Download"

cat <<EOF > /etc/systemd/system/rtc-sync.service
[Unit]
Description=Custom RTC and Internet Time Sync
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/bin/python3 /usr/local/bin/rtc_sync.py
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable rtc-sync.service
systemctl start rtc-sync.service
check_status "Systemd Service Setup"

# 5. Final Cleanup and Pre-Reboot Check
apt-get full-upgrade -y
apt-get autoremove -y
check_status "System Finalization"

echo "Installation complete and verified."
read -p "All steps completed. Ready to reboot? (Y/n): " confirm
if [[ $confirm == [nN] ]]; then
    echo "Reboot aborted. You can reboot manually later."
else
    echo "Rebooting now..."
    reboot
fi
