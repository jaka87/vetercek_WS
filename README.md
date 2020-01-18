# Vetercek.com weather station
This project was created in order to provide cheap alternative to expensive autonomous weather stations. The goal is to create robust weather station that would require (almost) no maintenance. As mentioned in the title the data would be send to **vetercek.com** but with some modification other services could be included. Keep in mind that most attention is dedicated to measure wind since is used mostly by windsurfers, kiters and other wind enthusiast. In general we tried to keep the cost as low as possible on the other hand in some areas like for example anemometer we went with durable and well calibrated instead of cheap.


## Connecting options
+ [GSM 2G](vetercek_2G)
+ [WIFI](vetercek_wifi)
+ NB-IoT - in progress

### RJ11 PCB socket
[RJ11 ](https://www.ebay.com/itm/10pcs-set-RJ11-RJ12-6P6C-Computer-Internet-Network-PCB-Jack-Socket-ATAU/272983583460?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649) - there are few different variants with different pin number. This is the correct one.
Also aveliable as part number [5523](http://en.glgnet.biz/productsdetail/productId=97.html). Connection should look like this on PCB  
![RJ11](vetercek_2G/img/rj11.png)  

### Field test
2.6.2019 we manage to set up our first weather station located in Sv. Ivan, Croatia. It was quite the challenge since the station is located in the sea about 30-40m from the shore.  
![station location](vetercek_2G/img/st1.jpg)  
  
11.1.2020 we manage to set up new improved version of the station to lighthouse in Isola.  
![izola](vetercek_2G/img/svetilnik.jpg)  

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)
