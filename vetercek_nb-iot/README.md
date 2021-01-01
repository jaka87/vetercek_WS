# Vetercek.com weather station nb-iot
New PCB  
![PCB](img/pcb.jpg)  

## Features of the WS
+ Send wind speed and gusts to the server
+ Send water and air temperature to the server
+ Send WS battery state to the server
+ Remote adjustment of the time between updates (depending on sever response)
+ Remote adjustment of wind wane offset (depending on sever response)
+ Wind speed and direction are measured for 2 second every 10 seconds. It is a good compromise for accurate measurements and long battery life.
+ **New** switched from HTTP to MQTT protocol (with open socket MQTT data upload takes from 200-500ms compared to 2s with HTTP and also uses much less bandwidth)
+ **New** new GPRS modul that supports both nb-iot and also 2G (Thanks to [A1](https://www.a1.si/) and [Telekom Slovenije](https://www.telekom.si/) for providing me with nb-iot sim cards). With new module there is much faster data upload and lower battery consumption. Therefore I also enabled fast data upload that can be enabled when you need data upload every few secconds as opposed to few minutes.

## Required parts
+ Arduino pro mini 3.3V **3€**
+ Davis 6410 anemometer **160€**
+ Sim7000E gsm module **20€**
+ DS18B20 temperature sensor **3€**
+ DS18B20 water temperature sensor **4€**
+ TP4056 li-ion charger **1€**
+ 5 or 6V Monocrystalline Solar Power Panel **5-10€**
+ 3.6V li-ion batteries **10-15€**
+ Waterproof housing **5€**
+ GSM antena **3€**
+ Some minor electrical parts like resistors, capacitors, diodes... **5€**
+ RJ11 connector to PCB **1€**
+ 2 cable glands for anemometer cable, and temperature sensor **2€**

## Arduino pro mini
Arduino pro mini has different bootloader than Uno (older) that has watchdog bug that puts your arduino in endless loop. I solved this with uploading with programmer (skipping bootloader). If you wish to use bootloader i suggest Minicore. This skech uses a lot of flash memory. You might have to disable debugg to be able to upload it to your arduino.

## Scheme
![Scheme](img/scheme.png)  
Here is the [link](https://easyeda.com/jaka87/new-vetercek) to PCB design.  


## TO-DO
+ I will try to figure out how to get PSM to work - I theory this would enable very low power consumption of 6uA for SIM7000

## Libraries used in this project
All the libraries are uploaded in src folder. Previously I just post links since i was using libraries directly from the repositories, but now i had to make changes to some of them, mostly to delete some unused functions to decrease sketch size.


## Power consumption
In sleep 1-1.5mA  
Idle 4-5 mA  
Sending data 100-200mA  


## Thanks!
Thanks to all of you contributing to make this happen. Especially thanks to Tadej Tašner for drawing PCB and his advices regarding the hardware components. Also thanks to those people that took time and wrote libraries used in this project and therefore make the project easier to compile.

Contributing to this software is warmly welcomed. You can use it, change it, do what ever you want with it.

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)
