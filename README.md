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

#### Parts List
- APA102 led strip
- NodeMCU
- 5V Power Supply (I'm personaly using computer ATX, but I do not recommand)
- Header Wires
- JST SM 4PIN (Commonly used on APA102 strips)


#### Wiring Diagram
![Diagram](https://github.com/BenoitAnastay/ESP-MQTT-JSON-Digital-LEDs/raw/master/Diagram.png "Wiring Diagram")


### Home Assistant Configuration
##### MQTT Discovery
1. Install the [Mosquitto add-on](/addons/mosquitto/) with the default configuration via 'Hass.io > ADD-ON STORE'. (Don't forget to start the add-on & verify that 'Start on boot' is enabled.)

2. Create a new user for MQTT via the `Configuration > Users (manage users)`. (Note: This name cannot be "homeassistant" or "addon")

3. Once back on-line, return to `Configuration > Integrations` and select configure next to `MQTT`.

```text
  Broker: YOUR_HASSIO_IP_ADDRESS
  Port: 1883
  Username: MQTT_USERNAME
  Password: MQTT_PASSWORD
  Enable discovery: CHECKED 
```

<img src="https://i.imgur.com/mgnVneT.png | width=100" width="30%" height="30%" />

3. ~~Download [ESP8266Flasher.exe](https://github.com/BenoitAnastay/DomusTech-LEDStrip-APA102-ESP8266-MQTT/releases/latest)~~

4. ~~Plug your ESP8266 and click on `Flash`. (then you can disconnect the ESP8266 and plug it on a power suply)~~

(Use this [fork of nodemcu-pyflasher](https://github.com/BenoitAnastay/nodemcu-pyflasher) with both binary files in the same directory or compile it with Arduino IDE and SPIFS support) 

5. When the led of the ESP8266 blink quickly use a computer or a smartphone to connect on the wifi. (it will be limited, no network)

6. Now go to [http://192.168.4.1](http://192.168.4.1) and enter WIFI information plus MQTT informations and a name (they are all required), click Submit and the ESP8266 will reboot.

You will now have a light entity in Home Assistant named uppun the name you have inserted exemple `light.stairs`.

If your entity appear nowhere it might be normaln try to edit your overview to add it.

One last thing, the white value is controlling the speed of the effects

###### OTA Uploading
When asked by the webconfigurator you can set a ota login to be able tu use it with Arduino IDE

##### Leds meaning
- Static : Initialising

- Blinking slowly : Connecting to Wifi

- Blinking fast : Pairing mode, had created a ad hoc wifi

##### Buttons usage
- [FLASH]Short press : cycle effects and Home Assistant publication 
- [FLASH]Long press (6sec) : Flush saved configuration
- RESET : Reboot the device
##### Manual
Copy past the [`Home Assistant/config/packages`](https://github.com/BenoitAnastay/DomusTech-LEDStrip-APA102-ESP8266-MQTT/tree/master/Home%20Assistant/config/packages) directory into Home Assistant `config` directory

Now edit your `config/packages/rgb_strip.yaml` file, replace stairs by anything you want

And add this to you HA `configuration.yaml` (Making HA reading `packages` dircetory)
```
homeassistant:
    packages: !include_dir_named packages
```
#### Basic Automation with Sensor
When using `light.turn_on` the strip turn on with the last runned effect

A basic automation with a sensor
```
automation: 
  - alias: Turn on strip
    trigger:
      platform: state
      entity_id: binary_sensor.stairs_sensor
      to: 'on'
    condition:
      - condition: state
        entity_id: light.strip_escalier
        state: 'off'
    action:
      - service: light.turn_on
        entity_id: light.strip_escalier
      - service: automation.turn_on
        entity_id: automation.turn_off_strip
  - alias: Turn off strip
    trigger:
      platform: state
      entity_id: light.strip_escalier
      to: 'off'
    action:
      - service: light.turn_off
        entity_id: light.strip_escalier
      - service: automation.turn_off
        entity_id: automation.turn_off_strip
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
apt-get install python3 python3-pip portaudio19-dev python-pyaudio
pip3 install numpy scipy pyqtgraph pyaudio --user
```

The you can run `python-visualizer/visualization.py`
#  Copyright
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

NodeMCU Flasher          :[Rui Huang](https://github.com/vowstar)
