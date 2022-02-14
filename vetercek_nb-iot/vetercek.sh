#!/bin/sh

printf "
Select anemometer\n
1. Ultrasonic anemometer\n2. Davis anemometer\n"

uzsleep=""

read -n1 input
if [[ $input == "1" ]]; then
        anemometer=" -DUZ_Anemometer"
printf "
UZ sleep\n
1. Yes\n2. No\n"
read -n1 input2
        if [[ $input2 == "1" ]]; then
        	uzsleep=" -DUZsleepChange"
        fi
else
        anemometer=""
fi


printf "
Select PCB\n
1. 0.5\n2. older\n"
read -n1 input
if [[ $input == "2" ]]; then
    pcb=" -DOLDPCB"
else
    pcb=""
fi

printf "
Select Debug\n
1. NO\n2. YES\n"
read -n1 input
if [[ $input == "2" ]]; then
    debug=" -DDEBUG"
else
    debug=""
fi


printf "
Select PRESSURE\n
1. NO\n2. YES\n"
read -n1 input
if [[ $input == "2" ]]; then
    bmp=" -DBMP"
else
    bmp=""
fi

stringg=$anemometer$debug$pcb$bmp$uzsleep$gsm

echo $stringg

arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 --build-property build.extra_flags="$stringg" --build-path=/home/jaka87/Arduino/temp/  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 

printf "
Compile over\n
1. upload with programmer\n2. upload (serial usb0)\n3. upload (serial usb1)\n"
read -n1 input
if [[ $input == "1" ]]; then
arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 --build-property build.extra_flags="$stringg" --build-path=/home/jaka87/Arduino/temp/  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 

//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328p -cusbtiny -Uflash:w:/home/jaka87/Arduino/temp/vetercek_nb-iot.ino.hex:i lfuse:w:0xDF:m efuse:w:0xFF:m hfuse:w:DA:m lock:w:0xFF:m 

elif [[ $input == "2" ]]; then
arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 --build-property build.extra_flags="$stringg"  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 
arduino-cli upload --port /dev/ttyUSB0   /home/jaka87/Arduino/vetercek_nb-iot/  --fqbn arduino:avr:pro:cpu=8MHzatmega328
else
arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 --build-property build.extra_flags="$stringg"  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 
arduino-cli upload --port /dev/ttyUSB1   /home/jaka87/Arduino/vetercek_nb-iot/  --fqbn arduino:avr:pro:cpu=8MHzatmega328
fi


read

