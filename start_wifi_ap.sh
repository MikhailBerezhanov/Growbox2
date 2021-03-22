#!/bin/sh
set -e

# Reconfig wifi to get IP from /etc/network/interfacese
ifdown wlan0
ifup wlan0

# Enable DHCP server at wlan0
systemctl start isc-dhcp-server

# Start wifi AP service
systemctl start hostapd

exit 0