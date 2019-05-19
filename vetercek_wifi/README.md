## Vetercek.com weather station WIFI
Wind is measured every 2 seconds, results are sent to server every 5min

## Required parts
+ Wemos D1 mini  **5€**
+ Davis 6410 anemometer **150€**
+ DS18B20 water temperature sensor **4€**
+ Waterproof housing
+ Some minor electrical parts like resistors, capacitors, diodes...
  + R1 = 4.7KΩ resistor
  + R2 = 1.8KΩ resistor
  + 1 JCT XH 3p connector for temperature sensor
  + RJ11 connector to PCB
  + 2 cable glands for anemometer cable, and temperature sensor
  + PG11 cable gland with tyvek foil coverig the exit functioning as an air vent


## Scheme
![Scheme](img/scheme.png)  
Here is the [link](https://easyeda.com/jaka87/wemos-d1) to PCB design.  

## TO-DO
+  test propperly
+ add option to enter vane offset and API key when selecting wifi network
+ reset settings with a push of a button

## Libraries used in this project
+ [ESP8266](https://github.com/esp8266/Arduino) - core library for ESP8266 chip
+ [WiFi Manager](https://github.com/tzapu/WiFiManager) - creates a temporary wifi network to which you connect and enter your wifi credentials
+ [OneWire](https://github.com/PaulStoffregen/OneWire) - manipulate DS18B20 sensor
+ [Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library) - also manipulate DS18B20 sensor
+ [ArduinoJson](https://github.com/bblanchon/ArduinoJson) - parse JSON data

## Thanks!
Thanks to all of you contributing to make this happen. 
Contributing to this software is warmly welcomed. You can use it, change it, do what ever you want with it.

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)
