# Vetercek.com weather station nb-iot
New PCB  
![PCB](img/pcb.jpg)  ![PCB back](img/pcb2.jpg)  

## Features of the WS
+ Send wind speed and gusts to the server
+ Send water and air temperature to the server
+ Send WS battery state to the server
+ Remote adjustment of the time between updates (depending on sever response)
+ Remote adjustment of wind wane offset (depending on sever response)
+ Wind speed and direction are measured for 2 second every 10 seconds (adjustable). It is a good compromise for accurate measurements and long battery life.
+ **New** switched from HTTP and MQTT to custom UDP protocol - for speed, reliability and better battery consumption 
+ **New** new GPRS modul that supports both nb-iot and also 2G (Thanks to [A1](https://www.a1.si/) and [Telekom Slovenije](https://www.telekom.si/) for providing me with nb-iot sim cards). With new module there is much faster data upload and lower battery consumption. Therefore I also enabled fast data upload that can be enabled when you need data upload every few secconds as opposed to few minutes.

## Required parts
+ Davis 6410 anemometer or ultrasonic HY-WDC2E with RS232 
+ PCB with SIM7000E module 
+ SIM card 
+ 5 or 6V Solar Power Panel 
+ 3.6V li-ion batteries
+ Waterproof housing
+ GSM antena
+ DS18B20 temperature sensor
+ Cable gland for anemometer cable, solar and temperature sensor

## Optional parts
+ Rain gauge (whichever uses reed switch in tipping bucket)
+ BMP280 pressure sensor

## Payload
UDP payload using this station is 24 byte for upload and 9 bytes for server response  
**sample upload:**  
list=[11,11,11,11,11,11,11,1,   1,77, 12,2, 14,5, 1,20,3, 0,1,0, 77,18,40,0.88,12,34]  
+ first 8 bytes are id of the station
+ byte 9 and 10 are for wind wane -  (sample 177)
+ byte 11 and 12 are wind speed (sample 12.2 KT)
+ byte 13 and 14 are for gusts (sample 14.5 KT)
+ byte 15-17 are for air temperature (sample +20.3 C)
+ byte 18-20 are the same as above exept for water temperature (sample -1.0 C) or rain data
+ byte 21 is battery percentage (sample 77%)
+ byte 22 is signal 
+ byte 23 is count of measurements
+ byte 24 is reset reason  
+ byte 25 is solar current  
+ byte 26,27 is pressure measurement  

list=[40, 1,2,30, 23, 1, 5, 1, 0]  
**sample response:**
+ byte 1 is measure count - after how many measurements do the next data upload (sample 40)
+ byte 2-4 is for wind wane offset - (sample 230)
+ byte 5 is wind measurement timer (on client side this number is muultiplied by 10) - (sample 2300)
+ byte 6 controls tmp sensor - 0 off; 1 only air, 2 only watter; 3 both
+ byte 7 controsl cuttof wind  - when avg wind is under that value data is send 2x of setted interval
+ byte 8 - 0 no sleep between wind measurements, 1 - 8s delay
+ byte 9 - 1 resets the station, other numbers change settings of rain/water sensor, solar measurement, pressure measurement...



## Arduino pro mini
Arduino pro mini has different bootloader than Uno (older) that has watchdog bug that puts your arduino in endless loop. I solved this with uploading with programmer (skipping bootloader). If you wish to use bootloader I suggest Minicore. 

## Scheme
![Scheme](img/scheme.png)  
Here is the [link](https://easyeda.com/jaka87/new-vetercek) to PCB design.  


## TO-DO
+ Currently there are no planned updates. WS seems stable and (almost) ready for production

## Libraries used in this project
All the libraries are uploaded in src folder. Most of them are stock libraries but i did quite some modification os SIM7000 library 


## Power consumption
In sleep 1-1.5mA  
Idle 4-5 mA (Davis anemometer) and 20-25mA (ultrasonic anemometer)   
Sending data 100-200mA  


## Thanks!
Thanks to all of you contributing to make this happen. Especially thanks to Tadej Tašner for drawing PCB and his advices regarding the hardware components. Also thanks to those people that took time and wrote libraries used in this project and therefore make the project easier to compile.

Contributing to this software is warmly welcomed. You can use it, change it, do what ever you want with it.

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)
