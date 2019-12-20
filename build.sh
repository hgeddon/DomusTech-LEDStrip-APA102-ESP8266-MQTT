arduino --board esp8266:esp8266:nodemcuv2:eesz=4M2M,xtal=160,wipe=none --save-prefs
arduino --verify --preserve-temp-files --pref build.path=./build ESP8266-APA102/ESP8266-APA102.ino
~/.arduino15/packages/esp8266/tools/mkspiffs/2.5.0-4-b40a506/mkspiffs -p 256 -b 8192 -s $((0x3FA000 - 0x300000)) -c ./ESP8266-APA102/data/ build/ESP8266-APA102.spiffs.bin
cp -f ./build/ESP8266-APA102.ino.bin ./flasher/Resources/Binaries/ESP8266_APA102.ino.bin
cp -f ./build/ESP8266-APA102.spiffs.bin ./flasher/Resources/Binaries/ESP8266_APA102.spiffs.bin