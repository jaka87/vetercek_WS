@ECHO OFF
ECHO Installation begin
avrdude.exe -Cavrdude.conf -v -V -patmega328pb -cusbtiny -Uflash:w:vetercek.hex:i lfuse:w:0xDF:m efuse:w:0xFD:m hfuse:w:DA:m lock:w:0xFF:m 


PAUSE
