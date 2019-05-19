## Vetercek.com weather station
This project was created in order to provide cheap alternative to expensive autonomous weather stations. The goal is to create robust weather station that would require (almost) no maintenance. As mentioned in the title the data would be send to **vetercek.com** but with some modification other services could be included. Keep in mind that most attention is dedicated to measure wind since is used mostly by windsurfers, kiters and other wind enthusiast. In general we tried to keep the cost as low as possible on the other hand in some areas like for example anemometer we went with durable and well calibrated instead of cheap.

## Features of the WS
+ Send wind speed and gusts to the server
+ Send water and air temperature to the server
+ Send WS battery state to the server
+ Remote adjustment of the time between updates (depending on sever response)
+ Remote adjustment of wind wane offset (depending on sever response)
+ Wind speed and direction are measured for 2 second every 10 seconds. It is a good compromise for accurate measurements and long battery life.


## Connecting options
+ [GSM 2G](vetercek_2G)
+ [WIFI](vetercek_wifi)
+ NB-IoT - in progress

## RJ11 PCB socket
[RJ11 ](https://www.ebay.com/itm/10pcs-set-RJ11-RJ12-6P6C-Computer-Internet-Network-PCB-Jack-Socket-ATAU/272983583460?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - there are few different variants with different pin number. This is the correct one.
Also aveliable as part number [5523](http://en.glgnet.biz/productsdetail/productId=97.html). Connection should look like this on PCB  
![RJ11](vetercek_2G/img/rj11.png)  

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)
