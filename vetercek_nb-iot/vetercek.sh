#!/bin/sh


printf "
Select anemometer\n
1. Ultrasonic anemometer\n2. Davis anemometer\n"

read -n1 input
if [[ $input == "1" ]]; then
        anemometer=" -DUZ_Anemometer"
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

stringg=$anemometer$debug$pcb$bmp

echo $stringg


arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 --build-property build.extra_flags="$stringg" --build-path=/home/jaka87/Arduino/temp/  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 


//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328p -cusbtiny -Uflash:w:/home/jaka87/Arduino/temp/vetercek_nb-iot.ino.hex:i lfuse:w:0xDF:m efuse:w:0xFF:m hfuse:w:DA:m lock:w:0xFF:m 

read

