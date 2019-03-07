## Vetercek.com weather station
This project was created in order to provide cheap alternative to expensive autonomous weather stations. The goal is to create robust weather station that would require (almost) no maintenance. As mentioned in the title the data would be send to **vetercek.com** but with some modification other services could be included. Keep in mind that most attention is dedicated to measure wind since is used mostly by windsurfers, kiters and other wind enthusiast. In general we tried to keep the cost as low as possible on the other hand in some areas like for example anemometer we went with durable and well calibrated instead of cheap.

## Features of the WS
+ Send wind speed and gusts to the server
+ Send watter and air temperature to the server
+ Send WS battery state to the server
+ Remote adjustment of the time between updates (depending on sever response)
+ Remote adjustment of wind wane offset (depending on sever response)


## Required parts
+ Arduino pro mini 3.3V **3€**
+ Davis 6410 anemometer **150€**
+ Sim800l gsm module **4€**
+ DHT22 temperature sensor **3€**
+ DS18B20 water temperature sensor **4€**
+ TP4056 li-ion charger **1€**
+ 5 or 6V Monocrystalline Solar Power Panel **10€**
+ 3.6V li-ion batteries **10-15€**
+ Waterproof housing
+ Some minor electrical parts like resistors, capacitors, diodes...



## Scheme
[Scheme](scheme.png)


## TO-DO
+ create PCB and make larger test outside
+ allow for larger ints in order to have possibility for longer time between updates
+ support for wifi module
+ support for other web services
+ sending GPS cordinates to server (in case WS get stolen)

## Thanks!
Thanks to all of you contributing to make this happen. I will wrote contributors and their role in the project once it's up and running.

Contributing to this software is warmly welcomed. You can use it, change it, do what ever you want with it.

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)