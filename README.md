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


