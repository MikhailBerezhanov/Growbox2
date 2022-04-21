# Growbox2
### Automated indoor grow planting system

([Learn more about this project](https://habr.com/ru/post/521414/))


Growbox2 is an indoor system that takes care of your plants. It helps with:

* Watering
* Lighting
* Climate control

It supports WEB-UI to configure and check all function.

![](https://hsto.org/webt/ln/6h/32/ln6h322xymy-xibcup8ywngpuoe.gif)

### How does it work

The system is based on 3 boards: 

* __Khadas VIM1__ - host MPU
* __FLC-01__ - self-made fitolamp controller
* __SensorsController__ - some kind of adapter from sensors-data to I2C bus 

Connection is the following:  

![](https://hsto.org/r/w1560/webt/9x/np/w6/9xnpw6grm87cg6pladlyx3_4asm.jpeg)


Host board runs __Ubuntu18.04 Linux__ and provides WEB-server that is available via Ethernet or WIFI connection.  
WEB interface is based on AJAX technology, so any sensor data is updated by request. You can use your __Browser__ to configure and watch the box system parameters. 

### IP address settings

There are different options in Growbox IP address configuration:

* __WIFI connection__ - by default system runs in WIFI Access Point (AP) mode, so using your smartphone you can
  1. find wifi-net called _Growbox_, 
  2. connect it, 
  3. see UI from mobile browser.  
That allows you to perform further configuration. 

*Technically WIFI mode could be configured to Client mode for wireless connection with your router, but this feature not implemented yet in _Settings_ menu.

* __LAN connection__ - by default system has dynamic IP (DHCP is on), so you'll need to
  1. check what address device got in your router menu,
  2. open this IP in Browser to get access to system UI   
Afterwards, LAN IP address can be changed to static in _Settings_ menu.


### Software

UI backend part is the CGI-application that runs on NGINX webserver.  

For _Debian \ Ubuntu_ backend dependencies installed as:  
`sudo apt install gcc g++ make nginx sqlite3 libconfig-dev libfcgi-dev libsqlite3-dev isc-dhcp-server hostapd`


Frontend part can be found in `UI/data` subfolder and has all dependencies inside to let system work without Internet connection.

Optionally application can be installed on target by calling `UI/install.sh` script. That will add __systemd__ service that can be controlled by _systemctl_ commands:

``` sh
# Run backend and check it's status
systemctl start gbweb
systemctl status gbweb
# Stop backend
systemctl stop gbweb
```



