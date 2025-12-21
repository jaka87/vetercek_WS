#!/bin/sh
var1=$1

if [ "$var1" == "1" ]; then
# Source file path
	INO_PATH="/home/jaka87/Arduino/vetercek_SIM7070/vetercek_SIM7070.ino"
TEMP_FILE="/home/jaka87/Arduino/vetercek_SIM7070/vetercek_temp.txt"
TEMP_FILE2="/home/jaka87/Arduino/vetercek_SIM7070/temp.txt"
# Step 1: Create a backup of the .ino file
cp "$INO_PATH" "$TEMP_FILE"


echo "Enter DEVICE_ID (default 1):"
read DEVICE_ID
DEVICE_ID=${DEVICE_ID:-1}
sed -i "s|#define DEVICE_ID .*|#define DEVICE_ID $DEVICE_ID|" "$INO_PATH"


# Step 2: Make changes to the original .ino file
echo "Do you want to enable DEBUG? (y/n)"
read ENABLE_DEBUG
if [[ "$ENABLE_DEBUG" == "y" ]]; then
    sed -i 's|//\(#define DEBUG\)|\1|' "$INO_PATH"
	echo "Do you want to enable DEBUG_MEASURE? (y/n)"
	read ENABLE_DEBUG_MEASURE
	if [[ "$ENABLE_DEBUG_MEASURE" == "y" ]]; then
		sed -i 's|//\(#define DEBUG_MEASURE\)|\1|' "$INO_PATH"
	else
		sed -i 's|^\(#define DEBUG_MEASURE\)|//\1|' "$INO_PATH"
	fi
    
else
    sed -i 's|^\(#define DEBUG\)|//\1|' "$INO_PATH"
fi



echo "Enable ultrasonic anemometer? (y/n)"
read ENABLE_ULTRASONIC

if [[ "$ENABLE_ULTRASONIC" == "y" ]]; then
    sed -i 's|//\(#define UZ_Anemometer\)|\1|' "$INO_PATH"


    echo "Enable toggle_UZ_power? (y/n)"
    read TOGGLE_UZ
    if [[ "$TOGGLE_UZ" == "y" ]]; then
        sed -i 's|//\(#define toggle_UZ_power\)|\1|' "$INO_PATH"
    else
        sed -i 's|^\(#define toggle_UZ_power\)|//\1|' "$INO_PATH"
    fi

else
    sed -i 's|^\(#define UZ_Anemometer\)|//\1|' "$INO_PATH"

fi




echo "Do you want to enable HUMIDITY? (y/n)"
read ENABLE_HUMIDITY

if [[ "$ENABLE_HUMIDITY" == "n" ]]; then
    # Comment out the HUMIDITY line
    sed -i 's|^\(#define HUMIDITY\)|//\1|' "$INO_PATH"
fi


echo "Do you want to enable TMP TMPDS18B20? (y/n)"
read TMPDS18B20
if [[ "$TMPDS18B20" == "y" ]]; then
    sed -i 's|//\(#define TMPDS18B20\)|\1|' "$INO_PATH"
else
    sed -i 's|^\(#define TMPDS18B20\)|//\1|' "$INO_PATH"
fi

echo "Do you want to enable RAIN? (y/n)"
read RAIN
if [[ "$RAIN" == "y" ]]; then
	sed -i 's/enableRain=0;/enableRain=1;/g' "$INO_PATH"
fi


echo "Do you want to enable PRESSURE? (y/n)"
read BMP
if [[ "$BMP" == "y" ]]; then
    sed -i 's|//\(#define BMP\)|\1|' "$INO_PATH"
    

    echo "Enter the value for sea level (in m):"
    read SEA
	SEA=${SEA:-0}
	sed -i "s|sea_level_m=.*|sea_level_m=$SEA\;|" "$INO_PATH"

else
    sed -i 's|^\(#define BMP\)|//\1|' "$INO_PATH"
fi


echo "Set operator (1 SLO - 2 HR - 3 ITA - 4 HUN - 5 AU - 6 GER - 7 FR - 8 NED - 9 POR - 10 GRE - 11 SPAIN)"  
read NETWORK_OPERATORS
NETWORK_OPERATORS=${NETWORK_OPERATORS:-1}
sed -i "s|#define NETWORK_OPERATORS .*|#define NETWORK_OPERATORS $NETWORK_OPERATORS|" "$INO_PATH"



# Compile the sketch from the temporary directory
/usr/share/arduino/arduino-builder -compile \
    -logger=machine \
    -hardware /usr/share/arduino/hardware \
    -hardware /home/jaka87/.arduino15/packages \
    -tools /usr/share/arduino/tools-builder \
    -tools /home/jaka87/.arduino15/packages \
    -libraries /home/jaka87/Arduino/libraries \
    -fqbn=MiniCore:avr:328:bootloader=uart0,eeprom=keep,variant=modelPB,BOD=disabled,LTO=Os,clock=8MHz_external \
    -ide-version=10819 \
    -build-path /home/jaka87/Arduino/temp/ \
    -warnings=none \
    -build-cache /tmp/arduino_cache_754522 \
    -prefs=build.warn_data_percentage=75 \
    -prefs=runtime.tools.arduinoOTA.path=/home/jaka87/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 \
    -prefs=runtime.tools.arduinoOTA-1.3.0.path=/home/jaka87/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 \
    -prefs=runtime.tools.avrdude.path=/home/jaka87/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino18 \
    -prefs=runtime.tools.avrdude-6.3.0-arduino18.path=/home/jaka87/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino18 \
    -prefs=runtime.tools.avr-gcc.path=/home/jaka87/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 \
    -prefs=runtime.tools.avr-gcc-7.3.0-atmel3.6.1-arduino7.path=/home/jaka87/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 \
    -verbose "$INO_PATH"


# Step 3: Perform compilation (optional, if needed)
# /path/to/arduino-builder <build commands>

# Step 4: Restore the original .ino file
cp "$INO_PATH" "$TEMP_FILE2"
rm "$INO_PATH"
mv "$TEMP_FILE" "$INO_PATH"

echo "Original .ino file restored."



elif [ "$var1" == "3" ]; then
	//bin/avrdude -C//etc/avrdude.conf -v -V -patmega328pb -e -cusbtiny -Uflash:w:/home/jaka87/Arduino/temp/vetercek_SIM7070.ino.hex:i lfuse:w:0xDF:m efuse:w:0xF1:m hfuse:w:DA:m lock:w:0xFF:m 


fi


