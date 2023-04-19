#!/bin/sh
var1=$1

if [ "$var1" == "1" ]; then
	/usr/share/arduino/arduino-builder -compile -logger=machine -hardware /usr/share/arduino/hardware -hardware /home/jaka87/.arduino15/packages -hardware /home/jaka87/Arduino/hardware -tools /usr/share/arduino/tools-builder -tools /home/jaka87/.arduino15/packages -libraries /home/jaka87/Arduino/libraries -fqbn=MiniCore:avr:328:bootloader=uart0,eeprom=keep,variant=modelPB,BOD=disabled,LTO=Os,clock=8MHz_external -ide-version=10819 -build-path /home/jaka87/Arduino/temp/ -warnings=none -build-cache /tmp/arduino_cache_754522 -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.arduinoOTA.path=/home/jaka87/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 -prefs=runtime.tools.arduinoOTA-1.3.0.path=/home/jaka87/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 -prefs=runtime.tools.avrdude.path=/home/jaka87/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino18 -prefs=runtime.tools.avrdude-6.3.0-arduino18.path=/home/jaka87/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino18 -prefs=runtime.tools.avr-gcc.path=/home/jaka87/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 -prefs=runtime.tools.avr-gcc-7.3.0-atmel3.6.1-arduino7.path=/home/jaka87/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 -verbose /home/jaka87/Arduino/vetercek_nb-iot-HW/vetercek_nb-iot-HW.ino
elif [ "$var1" == "3" ]; then
	//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:/home/jaka87/Arduino/temp/vetercek_nb-iot-HW.ino.hex:i lfuse:w:0xDF:m efuse:w:0xFD:m hfuse:w:DA:m lock:w:0xFF:m 


elif [[ $var1 == "4" ]]; then
	//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:/home/jaka87/Arduino/vetercek_nb-iot-HW/tiny7070.ino.hex:i lfuse:w:0xDF:m efuse:w:0xFD:m hfuse:w:DA:m lock:w:0xFF:m 

elif [[ $var1 == "5" ]]; then
	//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:/home/jaka87/Arduino/vetercek_nb-iot-HW/pwr.ino.hex:i lfuse:w:0xDF:m efuse:w:0xFD:m hfuse:w:DA:m lock:w:0xFF:m 

else
printf "
What to do
1. compile PB\n2. compile P\n3. upload PB\n4. upload P\n"

read -n1 input
if [[ $input == "1" ]]; then
	/usr/share/arduino/arduino-builder -compile -logger=machine -hardware /usr/share/arduino/hardware -hardware /home/jaka87/.arduino15/packages -hardware /home/jaka87/Arduino/hardware -tools /usr/share/arduino/tools-builder -tools /home/jaka87/.arduino15/packages -libraries /home/jaka87/Arduino/libraries -fqbn=MiniCore:avr:328:bootloader=uart0,eeprom=keep,variant=modelPB,BOD=disabled,LTO=Os,clock=8MHz_external -ide-version=10819 -build-path /home/jaka87/Arduino/temp/ -warnings=none -build-cache /tmp/arduino_cache_754522 -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.arduinoOTA.path=/home/jaka87/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 -prefs=runtime.tools.arduinoOTA-1.3.0.path=/home/jaka87/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 -prefs=runtime.tools.avrdude.path=/home/jaka87/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino18 -prefs=runtime.tools.avrdude-6.3.0-arduino18.path=/home/jaka87/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino18 -prefs=runtime.tools.avr-gcc.path=/home/jaka87/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 -prefs=runtime.tools.avr-gcc-7.3.0-atmel3.6.1-arduino7.path=/home/jaka87/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 -verbose /home/jaka87/Arduino/vetercek_nb-iot-HW/vetercek_nb-iot-HW.ino

elif [[ $input == "2" ]]; then
/usr/share/arduino/arduino-builder -compile -logger=machine -hardware /usr/share/arduino/hardware -hardware /home/jaka87/.arduino15/packages -hardware /home/jaka87/Arduino/hardware -tools /usr/share/arduino/tools-builder -tools /home/jaka87/.arduino15/packages -libraries /home/jaka87/Arduino/libraries -fqbn=MiniCore:avr:328:bootloader=uart0,eeprom=keep,variant=modelP,BOD=disabled,LTO=Os,clock=8MHz_external -ide-version=10819 -build-path /home/jaka87/Arduino/temp/ -warnings=none -build-cache /tmp/arduino_cache_754522 -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.arduinoOTA.path=/home/jaka87/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 -prefs=runtime.tools.arduinoOTA-1.3.0.path=/home/jaka87/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 -prefs=runtime.tools.avrdude.path=/home/jaka87/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino18 -prefs=runtime.tools.avrdude-6.3.0-arduino18.path=/home/jaka87/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino18 -prefs=runtime.tools.avr-gcc.path=/home/jaka87/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 -prefs=runtime.tools.avr-gcc-7.3.0-atmel3.6.1-arduino7.path=/home/jaka87/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 -verbose /home/jaka87/Arduino/vetercek_nb-iot-HW/vetercek_nb-iot-HW.ino
elif [[ $input == "3" ]]; then
	//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:/home/jaka87/Arduino/temp/vetercek_nb-iot-HW.ino.hex:i lfuse:w:0xDF:m efuse:w:0xFD:m hfuse:w:DA:m lock:w:0xFF:m 
	

else
	//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328p -cusbtiny -Uflash:w:/home/jaka87/Arduino/temp/vetercek_nb-iot-HW.ino.hex:i lfuse:w:0xDF:m efuse:w:0xFD:m hfuse:w:DA:m lock:w:0xFF:m 

fi


#printf "
#Select anemometer\n
#1. Ultrasonic anemometer\n2. Davis anemometer\n"

#uzsleep=""

#read -n1 input
#if [[ $input == "1" ]]; then
        #anemometer=" -DUZ_Anemometer"
#printf "
#UZ sleep\n
#1. Yes\n2. No\n"
#read -n1 input2
        #if [[ $input2 == "1" ]]; then
        	#uzsleep=" -DUZsleepChange"
        #fi
#else
        #anemometer=""
#fi


#printf "
#Select PCB\n
#1. 0.5\n2. older\n"
#read -n1 input
#if [[ $input == "2" ]]; then
    #pcb=" -DOLDPCB"
#else
    #pcb=""
#fi

#printf "
#Select Debug\n
#1. NO\n2. YES\n"
#read -n1 input
#if [[ $input == "2" ]]; then
    #debug=" -DDEBUG"
#else
    #debug=""
#fi


#printf "
#Select PRESSURE\n
#1. NO\n2. YES\n"
#read -n1 input
#if [[ $input == "2" ]]; then
    #bmp=" -DBMP"
#else
    #bmp=""
#fi

#stringg=$anemometer$debug$pcb$bmp$uzsleep$gsm

#echo $stringg

##arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 --build-property build.extra_flags="$stringg" --build-path=/home/jaka87/Arduino/temp/  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 
##arduino-cli compile --fqbn MiniCore:avr:328 --build-property build.extra_flags="$stringg" --build-path=/home/jaka87/Arduino/temp/  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 
#arduino-cli compile --fqbn=MiniCore:avr:328:bootloader=uart0,eeprom=keep,variant=modelP,BOD=disabled,LTO=Os,clock=8MHz_external --build-property build.extra_flags="$stringg"   --build-path=/home/jaka87/Arduino/temp/  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 

#printf "
#Compile over\n
#1. upload with programmer\n2. upload (serial usb0)\n3. upload (serial usb1)\n"
#read -n1 input
#if [[ $input == "1" ]]; then

#//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328p -cusbtiny -Uflash:w:/home/jaka87/Arduino/temp/vetercek_nb-iot.ino.hex:i lfuse:w:0xDF:m efuse:w:0xFD:m hfuse:w:DA:m lock:w:0xFF:m 

#elif [[ $input == "2" ]]; then
#arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 --build-property build.extra_flags="$stringg"  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 
#arduino-cli upload --port /dev/ttyUSB0   /home/jaka87/Arduino/vetercek_nb-iot/  --fqbn arduino:avr:pro:cpu=8MHzatmega328
#else
#arduino-cli compile --fqbn arduino:avr:pro:cpu=8MHzatmega328 --build-property build.extra_flags="$stringg"  /home/jaka87/Arduino/vetercek_nb-iot/vetercek_nb-iot.ino 
#arduino-cli upload --port /dev/ttyUSB1   /home/jaka87/Arduino/vetercek_nb-iot/  --fqbn arduino:avr:pro:cpu=8MHzatmega328
#fi


read

fi
