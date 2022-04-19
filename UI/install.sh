#!/bin/sh
set -e

TARGET_DIR="/opt/growbox/"

CONFIGS_DIR="./configs"

echo -e "\e[1m[1] Configuring dependencies..\e[0m"
apt install htop members gcc g++ make nginx curl sqlite3 libconfig-dev libfcgi-dev libsqlite3-dev isc-dhcp-server hostapd

echo -e "\e[1m[2] Configuring nginx..\e[0m"
cp "$CONFIGS_DIR"/growbox-nginx.conf /etc/nginx/conf.d/growbox.conf
cp "$CONFIGS_DIR"/nginx-default /etc/nginx/sites-enabled/default

if [ -d /home/mik/ ]; then 
	groupadd growbox
	usermod -aG growbox www-data
	usermod -aG growbox mik
	members growbox
	chgrp -R growbox ./data
	chmod -R g+rw ./data
fi

echo -e "\e[1m[3] Making executable..\e[0m"
make

echo -e "\e[1m[4] Creating work directory in '$TARGET_DIR'..\e[0m"
mkdir "$TARGET_DIR"
mkdir "$TARGET_DIR"data/
cp ./gbweb "$TARGET_DIR"
cp -r ./data/ "$TARGET_DIR"
cp "$CONFIGS_DIR"/gbweb.conf.khadas "$TARGET_DIR"gbweb.conf

echo -e "\e[1m[5] Configuring services..\e[0m"
cp "$CONFIGS_DIR"/gbweb.service /etc/systemd/system/
cp "$CONFIGS_DIR"/hostapd.conf /etc/hostapd/
cp /etc/default/hostapd /etc/default/hostapd.bak
cp "$CONFIGS_DIR"/hostapd.default /etc/default/hostapd
cp /etc/default/isc-dhcp-server /etc/default/isc-dhcp-server.bak
cp "$CONFIGS_DIR"/isc-dhcp-server /etc/default/isc-dhcp-server
cp /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf.bak
cp "$CONFIGS_DIR"/dhcpd.conf /etc/dhcp/dhcpd.conf
cp /etc/network/interfaces /etc/network/interfaces.bak
cp "$CONFIGS_DIR"/interfaces /etc/network/interfaces

echo -e "\e[1m[6] Starting services..\e[0m"
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