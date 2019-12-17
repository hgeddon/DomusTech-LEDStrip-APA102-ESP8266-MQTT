[![Build Status](https://travis-ci.org/BenoitAnastay/DomusTech-LEDStrip-APA102-ESP8266-MQTT.svg?branch=master)](https://travis-ci.org/BenoitAnastay/DomusTech-LEDStrip-APA102-ESP8266-MQTT)
[![Build Status](https://github.com/BenoitAnastay/DomusTech-LEDStrip-APA102-ESP8266-MQTT/workflows/Github%20Arduino%20Library%20CI/badge.svg)](https://github.com/BenoitAnastay/DomusTech-LEDStrip-APA102-ESP8266-MQTT/actions)
# DomusTech APA102 LEDs driver for ESP8266
NOTE To be able to build it you need this version of FastLED [BenoitAnastay/FastLED](https://github.com/BenoitAnastay/FastLED) until the main repository add the SPI support for ESP8266


#### Fork information
The main goal of this fork is to update that code for the last revision of Home Assistant and make it work with APA102

Added SPI Support, check [ESP8266 SPI #936](https://github.com/FastLED/FastLED/pull/936)

Added a visualizer effect that work by receiving RAW pixel from UDP socket, those packets are sent by a python code

Added web configurator, when no setted up the ESP8266 create a Wifi AP and host a configuration web page, when the builtin led start to
flash rapidly you can connect to the network (default name DomusTech-LED) an go to [192.168.4.1](http://192.168.4.1) to set up all variables, you can reset those settings by pushing the flash builtin button of the NodeMCU (GPIO0, D3)
#####  Copyright
Main ESP8266 source code : [BRUH Automation](https://github.com/bruhautomation)

Added effects            : [Scott (Fma965)](https://github.com/Fma965)

UDP Visualizer           : [Scott Lawson](https://github.com/scottlawsonbc)

Conbtinious Integration  : [Adafruit](https://github.com/adafruit)

FastLED SPI for ESP8266  : [Benoit Anastay](https://github.com/BenoitAnastay)

FastLED Animation Library : [FastLED](https://github.com/FastLED)

Webserver for wifi setup : [Christopher Strider Cook](https://github.com/chriscook8)

Arduino Client for MQTT  : [Nick O'Leary](https://github.com/knolleary)

Arduino JSON library     : [Benoît Blanchon](https://github.com/bblanchon)

Arduino SDK              : [Arduino](https://github.com/arduino)

ESP8266 core for Arduino : [ESP8266 Community Forum](https://github.com/esp8266)

#### Supported Features Include
- RGB Color Selection
- Brightness 
- Fade
- Transitions
- Effects with Animation Speed
- Over-the-Air (OTA) Upload from the ArduinoIDE!
- Music Visualizer (Various effects)
- DrZZs Effects
- bkpsu Effects
- Webserver configurator 
Some of the effects incorporate the currrently selected color (sinelon, confetti, juggle, etc) while other effects use pre-defined colors. You can also select custom transition speeds between colors. The transition variable in Home Assistant (HA) also functions to control the animation speed of the currently running animation. The input_slider and automation in the HA configuration example allow you to easily set a transition speed from HA's user interface without needing to use the Services tool. 

The default speed for the effects is hard coded and is set when the light is first turned on. When changing between effects, the previously used transition speed will take over. If the effects don't look great, play around with the slider to adjust the transition speed (AKA the effect's animation speed). 

#### OTA Uploading
This code also supports remote uploading to the ESP8266 using Arduino's OTA library. To utilize this, you'll need to first upload the sketch using the traditional USB method. However, if you need to update your code after that, your WIFI-connected ESP chip should show up as an option under Tools -> Port -> Porch at your.ip.address.xxx. More information on OTA uploading can be found [here](http://esp8266.github.io/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html). Note: You cannot access the serial monitor over WIFI at this point.  

#### Parts List
- APA102 led strip
- NodeMCU
- 5V Power Supply (I'm personaly using computer ATX, but I do not recommand)
- Header Wires
- JST SM 4PIN (Commonly used on APA102 strips)


#### Wiring Diagram
![alt text](https://github.com/BenoitAnastay/ESP-MQTT-JSON-Digital-LEDs/raw/master/Diagram.png "Wiring Diagram")


#### Home Assistant Configuration
Copy past the [`Home Assistant/config/packages`](https://github.com/BenoitAnastay/DomusTech-LEDStrip-APA102-ESP8266-MQTT/tree/master/Home%20Assistant/config/packages) directory into Home Assistant `config` directory
```
{"entity_id":"light.stairs_strip",
"brightness":150,
"color_name":"blue",
"transition":"5"
}
```

#### Python UDP Visualizer
First thing first, don't forget to modify the `python-visualizer/config.py` file acordingly to your configuration
##### Windows 
Install dependencies using [anaconda package manager](https://www.anaconda.com/distribution/)

After installing Anaconda, run cmd.exe and execute
```
conda install numpy scipy pyqtgraph pyaudio
```

The you can run `python-visualizer/visualization.py`
##### Ubuntu
```
apt-get install python3 python3-pip
pip install numpy scipy pyqtgraph pyaudio
```

The you can run `python-visualizer/visualization.py`