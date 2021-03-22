#!/bin/sh
set -e

TARGET_DIR="/opt/growbox/"

echo -e "\e[1m[1] Configuring dependencies..\e[0m"
apt install htop git members gcc g++ make nginx curl sqlite3 libconfig-dev libfcgi-dev libsqlite3-dev isc-dhcp-server hostapd

echo -e "\e[1m[2] Configuring git..\e[0m"
git config --global user.name "Mik"
git config --global user.email Berezhanov.M@gmail.com

echo -e "\e[1m[3] Configuring nginx..\e[0m"
cp ./growbox-nginx.conf /etc/nginx/conf.d/growbox.conf
cp ./nginx-default /etc/nginx/sites-enabled/default

if [ -d /home/mik/ ]; then 
	groupadd growbox
	usermod -aG growbox www-data
	usermod -aG growbox mik
	members growbox
	chgrp -R growbox ./data
	chmod -R g+rw ./data
fi

echo -e "\e[1m[4] Making executable..\e[0m"
make

echo -e "\e[1m[5] Creating work directory in '$TARGET_DIR'..\e[0m"
mkdir "$TARGET_DIR"
mkdir "$TARGET_DIR"data/
cp ./gbweb "$TARGET_DIR"
cp -r ./data/ "$TARGET_DIR"
cp ./gbweb.conf.khadas "$TARGET_DIR"gbweb.conf

echo -e "\e[1m[6] Configuring services..\e[0m"
cp ./gbweb.service /etc/systemd/system/
cp ./services_config/hostapd.conf /etc/hostapd/
cp /etc/default/hostapd /etc/default/hostapd.bak
cp ./services_config/hostapd.default /etc/default/hostapd
cp /etc/default/isc-dhcp-server /etc/default/isc-dhcp-server.bak
cp ./services_config/isc-dhcp-server /etc/default/isc-dhcp-server
cp /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf.bak
cp ./services_config/dhcpd.conf /etc/dhcp/dhcpd.conf
cp /etc/network/interfaces /etc/network/interfaces.bak
cp ./services_config/interfaces /etc/network/interfaces

echo -e "\e[1m[7] Starting services..\e[0m"
systemctl daemon-reload
systemctl reload nginx
systemctl enable gbweb
systemctl start gbweb
systemctl status gbweb

echo -e "\e[1m[8] Starting Wifi AP..\e[0m"
ifdown wlan0
ifup wlan0
systemctl start isc-dhcp-server
systemctl start hostapd