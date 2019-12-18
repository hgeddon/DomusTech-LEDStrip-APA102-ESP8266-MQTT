/*
  .______   .______    __    __   __    __          ___      __    __  .___________.  ______   .___  ___.      ___   .___________. __    ______   .__   __.
  |   _  \  |   _  \  |  |  |  | |  |  |  |        /   \    |  |  |  | |           | /  __  \  |   \/   |     /   \  |           ||  |  /  __  \  |  \ |  |
  |  |_)  | |  |_)  | |  |  |  | |  |__|  |       /  ^  \   |  |  |  | `---|  |----`|  |  |  | |  \  /  |    /  ^  \ `---|  |----`|  | |  |  |  | |   \|  |
  |   _  <  |      /  |  |  |  | |   __   |      /  /_\  \  |  |  |  |     |  |     |  |  |  | |  |\/|  |   /  /_\  \    |  |     |  | |  |  |  | |  . `  |
  |  |_)  | |  |\  \-.|  `--'  | |  |  |  |     /  _____  \ |  `--'  |     |  |     |  `--'  | |  |  |  |  /  _____  \   |  |     |  | |  `--'  | |  |\   |
  |______/  | _| `.__| \______/  |__|  |__|    /__/     \__\ \______/      |__|      \______/  |__|  |__| /__/     \__\  |__|     |__|  \______/  |__| \__|

  Thanks much to @corbanmailloux for providing a great framework for implementing flash/fade with HomeAssistant https://github.com/corbanmailloux/esp-mqtt-rgb-led

  To use this code you will need the following dependancies:

  - Support for the ESP8266 boards.
        - You can add it to the board manager by going to File -> Preference and pasting http://arduino.esp8266.com/stable/package_esp8266com_index.json into the Additional Board Managers URL field.
        - Next, download the ESP8266 dependancies by going to Tools -> Board -> Board Manager and searching for ESP8266 and installing it.

  - You will also need to download the follow libraries by going to Sketch -> Include Libraries -> Manage Libraries
      - FastLED
      - PubSubClient
      - ArduinoJSON
*/
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define ESP8266_SPI
#define FASTLED_ALL_PINS_HARDWARE_SPI
#define MQTT_MAX_PACKET_SIZE 2048
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "FastLED.h"
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

struct {
  char ssid[32] = "";
  char password[64] = "";
  char name[32] = "";
  char OTApassword[64] = "";
  char mqtt_server[32] = "";
  char mqtt_username[32] = "";
  char mqtt_password[64] = "";
  char effect[32] = "";
  int  numleds = 0;
  int technology = 0;
} config;
int getEEPROM() {
  EEPROM.begin(sizeof (config)); //Initialasing EEPROM
  EEPROM.get(0, config);
  return 0;
}
int EEPROMres = getEEPROM();
String technology;
/************ WIFI and MQTT Information (CHANGE THESE FOR YOUR SETUP) ******************/
const int mqtt_port = 1883;
//Visualizer
#define BUFFER_LEN 2880
unsigned int localPort = 7777;
char packetBuffer[BUFFER_LEN];

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);
int statusCode;
String st;
String content;

/**************************** FOR OTA **************************************************/
int OTAport = 8266;

const char* on_cmd = "ON";
const char* off_cmd = "OFF";
const char* effect = "solid";
const char* effectList[] = {"Visualizer", "Christmas", "Music - L2R", "Music - Middle", "Music - Fma965", "bpm", "candy cane", "confetti", "cyclon rainbow", "dots", "fire", "glitter", "juggle", "lightning", "noise", "police all", "police one", "rainbow", "rainbow with glitter", "ripple", "twinkle", "sinelon", "sine hue", "full hue", "breathe", "hue breathe", "Christmas bounce", "christmas alternate", "random stars", "St Patty", "Valentine", "Turkey Day", "Thanksgiving", "USA", "Independence", "Halloween", "Go Lions", "Hail", "Touchdown", "Punkin", "Lovey Day", "Holly Jolly"};
const int effectListc = sizeof(effectList) / sizeof(effectList[0]);
int effect_id = 1; //Skip visualiser
String effectString = "solid";
String oldeffectString = "solid";

/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(10);

/*********************************** FastLED Defintions ********************************/
#define DATA_PIN    7
#define CLOCK_PIN 5
#define COLOR_ORDER BGR

byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;

byte red = 255;
byte green = 255;
byte blue = 255;
byte brightness = 50;

/**************************** MUSIC VISUALIZER **************************************************/
#define UPDATES_PER_SECOND 100

#define MUSIC_SENSITIVITY 4
//To set a fixed MUSIC_SENSITIVITY you must un comment the line "int audio_input = analogRead(audio) * MUSIC_SENSITIVITY;" near the bottom.

// AUDIO INPUT SETUP
int audio = A0;

// STANDARD VISUALIZER VARIABLES
int loop_max = 0;
int k = 255; // COLOR WHEEL POSITION
int wheel_speed = 3; // COLOR WHEEL SPEED
int decay = 0; // HOW MANY MS BEFORE ONE LIGHT DECAY
int decay_check = 0;
long pre_react = 0; // NEW SPIKE CONVERSION
long react = 0; // NUMBER OF LEDs BEING LIT
long post_react = 0; // OLD SPIKE CONVERSION

///////////////DrZzs Palettes for custom BPM effects//////////////////////////
///////////////Add any custom palettes here//////////////////////////////////

// Gradient palette "bhw2_thanks_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw2/tn/bhw2_thanks.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw2_thanks_gp ) {
  0,   9,  5,  1,
  48,  25,  9,  1,
  76, 137, 27,  1,
  96,  98, 42,  1,
  124, 144, 79,  1,
  153,  98, 42,  1,
  178, 137, 27,  1,
  211,  23,  9,  1,
  255,   9,  5,  1
};

// Gradient palette "bhw2_redrosey_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw2/tn/bhw2_redrosey.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw2_redrosey_gp ) {
  0, 103,  1, 10,
  33, 109,  1, 12,
  76, 159,  5, 48,
  119, 175, 55, 103,
  127, 175, 55, 103,
  178, 159,  5, 48,
  221, 109,  1, 12,
  255, 103,  1, 10
};

// Gradient palette "bluered_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/h5/tn/bluered.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 12 bytes of program space.

DEFINE_GRADIENT_PALETTE( bluered_gp ) {
  0,   0,  0, 255,
  127, 255, 255, 255,
  255, 255,  0,  0
};

// Gradient palette "bhw2_xmas_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw2/tn/bhw2_xmas.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 48 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw2_xmas_gp ) {
  0,   0, 12,  0,
  40,   0, 55,  0,
  66,   1, 117,  2,
  77,   1, 84,  1,
  81,   0, 55,  0,
  119,   0, 12,  0,
  153,  42,  0,  0,
  181, 121,  0,  0,
  204, 255, 12,  8,
  224, 121,  0,  0,
  244,  42,  0,  0,
  255,  42,  0,  0
};

DEFINE_GRADIENT_PALETTE( bhw2_xmas_gp_b ) {
  0,   0, 12,  0,
  40,   0, 55,  0,
  119,   0, 12,  0,
  153,  42,  0,  0,
  181, 121,  0,  0,
  255,  42,  0,  0
};
// Gradient palette "bhw2_xc_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw2/tn/bhw2_xc.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw2_xc_gp ) {
  0,   4,  2,  9,
  58,  16,  0, 47,
  122,  24,  0, 16,
  158, 144,  9,  1,
  183, 179, 45,  1,
  219, 220, 114,  2,
  255, 234, 237,  1
};

// Gradient palette "bhw1_04_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_04.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw1_04_gp ) {
  0, 229, 227,  1,
  15, 227, 101,  3,
  142,  40,  1, 80,
  198,  17,  1, 79,
  255,   0,  0, 45
};

// Gradient palette "bhw4_051_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw4/tn/bhw4_051.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

// Gradient palette "fs2006_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/cl/tn/fs2006.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 56 bytes of program space.

DEFINE_GRADIENT_PALETTE( fs2006_gp ) {
  0,   0, 49,  5,
  34,   0, 49,  5,
  34,  79, 168, 66,
  62,  79, 168, 66,
  62, 252, 168, 92,
  103, 252, 168, 92,
  103, 234, 81, 29,
  143, 234, 81, 29,
  143, 222, 30,  1,
  184, 222, 30,  1,
  184,  90, 13,  1,
  238,  90, 13,  1,
  238, 210,  1,  1,
  255, 210,  1,  1
};


DEFINE_GRADIENT_PALETTE( bhw4_051_gp ) {
  0,   1,  1,  4,
  28,  16, 24, 77,
  66,  35, 87, 160,
  101, 125, 187, 205,
  127, 255, 233, 13,
  145, 125, 187, 205,
  193,  28, 70, 144,
  224,  14, 19, 62,
  255,   1,  1,  4
};

// Gradient palette "blue_g2_5_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/go2/webtwo/tn/blue-g2-5.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( blue_g2_5_gp ) {
  0,   2,  6, 63,
  127,   2,  9, 67,
  255,   255, 255, 115,
  255,   255, 255, 0
};

// Gradient palette "bhw3_41_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw3/tn/bhw3_41.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw3_41_gp ) {
  0,   0,  0, 45,
  71,   7, 12, 255,
  76,  75, 91, 255,
  76, 255, 255, 255,
  81, 255, 255, 255,
  178, 255, 255, 255,
  179, 255, 55, 45,
  196, 255,  0,  0,
  255,  42,  0,  0
};

DEFINE_GRADIENT_PALETTE( test_gp ) {
  0,  255,  0,  0, // Red
  // 32,  171, 85,  0, // Orange
  // 64,  171,171,  0, // Yellow
  // 96,    0,255,  0, // Green
  //128,    0,171, 85, // Aqua
  160,    0,  0, 255, // Blue
  //192,   85,  0,171, // Purple
  //224,  171,  0, 85, // Pink
  //255,  255,  0,  0};// and back to Red
};

// Gradient palette "bhw2_greenman_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw2/tn/bhw2_greenman.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 12 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw2_greenman_gp ) {
  0,   1, 22,  1,
  130,   1, 168,  2,
  255,   1, 22,  1
};

// Gradient palette "PSU_gp"
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 12 bytes of program space.

DEFINE_GRADIENT_PALETTE( PSU_gp ) {
  0,   4, 30, 66,
  127,  30, 64, 124,
  255, 255, 255, 255
};

// Gradient palette "Orange_to_Purple_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/icons/tn/Orange-to-Purple.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 12 bytes of program space.

DEFINE_GRADIENT_PALETTE( Orange_to_Purple_gp ) {
  0, 208, 50,  1,
  127, 146, 27, 45,
  255,  97, 12, 178
};

/* END PALETTE DEFINITIONS */

/******************************** GLOBALS for fade/flash *******************************/
bool stateOn = false;
bool startFade = false;
bool onbeforeflash = false;
unsigned long lastLoop = 0;
int transitionTime = 0;
int delayMultiplier = 1;
int effectSpeed = 0;
bool inFade = false;
int loopCount = 0;
int stepR, stepG, stepB;
int redVal, grnVal, bluVal;

bool flash = false;
bool startFlash = false;
int flashLength = 0;
unsigned long flashStartTime = 0;
byte flashRed = red;
byte flashGreen = green;
byte flashBlue = blue;
byte flashBrightness = brightness;

/********************************** GLOBALS for EFFECTS ******************************/
//RAINBOW
uint8_t thishue = 0;                                          // Starting hue value.
uint8_t deltahue = 10;

//CANDYCANE
CRGBPalette16 currentPalettestriped; //for Candy Cane
CRGBPalette16 hailPalettestriped; //for Hail
CRGBPalette16 ThxPalettestriped; //for Thanksgiving
CRGBPalette16 HalloweenPalettestriped; //for Halloween
CRGBPalette16 HJPalettestriped; //for Holly Jolly
CRGBPalette16 IndPalettestriped; //for Independence
CRGBPalette16 gPal; //for fire

//NOISE
static uint16_t dist;         // A random number for our noise generator.
uint16_t scale = 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint8_t maxChanges = 48;      // Value for blending between palettes.
CRGBPalette16 targetPalette(OceanColors_p);
CRGBPalette16 currentPalette(CRGB::Black);

//TWINKLE
#define DENSITY     80
int twinklecounter = 0;

//RIPPLE
uint8_t colour;                                               // Ripple colour is randomized.
int center = 0;                                               // Center of the current ripple.
int step = -1;                                                // -1 is the initializing step.
uint8_t myfade = 255;                                         // Starting brightness.
#define maxsteps 16                                           // Case statement wouldn't allow a variable.
uint8_t bgcol = 0;                                            // Background colour rotates.
int thisdelay = 20;                                           // Standard delay value.

//DOTS
uint8_t   count =   0;                                        // Count up to 255 and then reverts to 0
uint8_t fadeval = 224;                                        // Trail behind the LED's. Lower => faster fade.
uint8_t bpm = 30;

//LIGHTNING
uint8_t frequency = 50;                                       // controls the interval between strikes
uint8_t flashes = 8;                                          //the upper limit of flashes per strike
unsigned int dimmer = 1;
uint8_t ledstart;                                             // Starting location of a flash
uint8_t ledlen;
int lightningcounter = 0;

//FUNKBOX
int idex = 0;                //-LED INDEX (0 to config.numleds-1
int TOP_INDEX = config.numleds / 2;
int thissat = 255;           //-FX LOOPS DELAY VAR

//////////////////add thishue__ for Police All custom effects here/////////////////////////////////////////////////////////
/////////////////use hsv Hue number for one color, for second color change "thishue__ + __" in the setEffect section//////

uint8_t thishuepolice = 0;
uint8_t thishuehail = 183;
uint8_t thishueLovey = 0;

int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % config.numleds;
  }
  return iN;
}

//FIRE
#define COOLING  55
#define SPARKING 120
bool gReverseDirection = false;

//BPM
uint8_t gHue = 0;

//CHRISTMAS
int toggle = 0;

//SINE HUE
int hue_index = 0;
int led_index = 0;

WiFiClient espClient;
PubSubClient client(espClient);
struct CRGB leds[576];
WiFiUDP port;

int LED_state = HIGH;
long buttonTimer = 0;
long longPressTime = 6000;

boolean buttonActive = false;
boolean longPressActive = false;
/********************************** START SETUP*****************************************/
void setup() {
  pinMode(0, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  int numleds = config.numleds;
  if (config.technology == 1) {
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, COLOR_ORDER, DATA_RATE_MHZ(12)>(leds, numleds);
    Serial.print("Setting APA102 ");
    technology = "APA102";
  }
  else {
    FastLED.addLeds<WS2811, 7, COLOR_ORDER>(leds, numleds);
    Serial.print("Setting WS2811 ");
    technology = "WS2811";
  }
  Serial.print(technology);
  Serial.println(config.numleds);


  setupStripedPalette( CRGB::Red, CRGB::Red, CRGB::White, CRGB::White); //for CANDY CANE
  setupThxPalette( CRGB::OrangeRed, CRGB::Olive, CRGB::Maroon, CRGB::Maroon); //for Thanksgiving
  setupHailPalette( CRGB::Blue, CRGB::Blue, CRGB::White, CRGB::White); //for HAIL
  setupHalloweenPalette( CRGB::DarkOrange, CRGB::DarkOrange, CRGB::Indigo, CRGB::Indigo); //for Halloween
  setupHJPalette( CRGB::Red, CRGB::Red, CRGB::Green, CRGB::Green); //for Holly Jolly
  setupIndPalette( CRGB::FireBrick, CRGB::Cornsilk, CRGB::MediumBlue, CRGB::MediumBlue); //for Independence

  gPal = HeatColors_p; //for FIRE

  setup_wifi();
  client.setServer(config.mqtt_server, mqtt_port);
  client.setCallback(callback);
  effectString = config.effect;
  Serial.println(effectString);
  //Visualizer
  port.begin(localPort);

  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(config.name);

  // No authentication by default
  ArduinoOTA.setPassword(config.OTApassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_BUILTIN, LOW);
}

//To include in all infinite loop that can lock the device
void handle_input() {
  int buttonState = digitalRead(0) ;

  if (buttonState != HIGH) {
    if (buttonActive == false) {
      buttonActive = true;
      buttonTimer = millis();
    }
    if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) {
      longPressActive = true;
      reset_config();
    }
  } else if (buttonActive == true) {
    if (longPressActive == true) {
      longPressActive = false;
    } else {
      register_homeassistant();
      if (effect_id > effectListc)
        effect_id = 1; //Skip Visualizer
      effectString = effectList[effect_id++];
      Serial.println(effectString);
      stateOn = true;
      sendState();
    }
    buttonActive = false;
  }
}

void led_flash() {
  LED_state = !LED_state;
  digitalWrite(LED_BUILTIN, LED_state);
}

/********************************** START SETUP WIFI*****************************************/
void setup_wifi() {
  delay(10);
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(config.ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid, config.password);
  if (config.ssid[0] == '\0')
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }
  else if (testWifi())
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }

  while (WiFi.status() != WL_CONNECTED) {
    if ((millis() % 100) == 0) {
      Serial.print(".");
      led_flash();
      delay(10);
    }
    server.handleClient();
    handle_input();
    effects();
  }
}

/*
  SAMPLE PAYLOAD:
  {
    "brightness": 120,
    "color": {
      "r": 255,
      "g": 100,
      "b": 100
    },
    "flash": 2,
    "transition": 5,
    "state": "ON"
  }
*/



/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    return;
  }

  if (stateOn) {

    realRed = map(red, 0, 255, 0, brightness);
    realGreen = map(green, 0, 255, 0, brightness);
    realBlue = map(blue, 0, 255, 0, brightness);
  }
  else {

    realRed = 0;
    realGreen = 0;
    realBlue = 0;
  }

  Serial.println(effect);

  startFade = true;
  inFade = false; // Kill the current fade

  sendState();
}



/********************************** START PROCESS JSON*****************************************/
bool processJson(char* message) {
  DynamicJsonDocument root(1024);

  auto error = deserializeJson(root, message);

  if (error) {
    Serial.println("deserializeJson() failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
      onbeforeflash = false;
    }
  }

  // If "flash" is included, treat RGB and brightness differently
  if (root.containsKey("flash")) {
    flashLength = (int)root["flash"] * 1000;

    oldeffectString = effectString;

    if (root.containsKey("brightness")) {
      flashBrightness = root["brightness"];
    }
    else {
      flashBrightness = brightness;
    }

    if (root.containsKey("color")) {
      flashRed = root["color"]["r"];
      flashGreen = root["color"]["g"];
      flashBlue = root["color"]["b"];
    }
    else {
      flashRed = red;
      flashGreen = green;
      flashBlue = blue;
    }

    if (root.containsKey("effect")) {
      effect = root["effect"];
      effectString = effect;
      twinklecounter = 0; //manage twinklecounter
    }

    if (root.containsKey("transition")) {
      transitionTime = root["transition"];
    }
    if (root.containsKey("white_value")) {
      transitionTime = map(root["white_value"], 0, 255, 1, 150);
    }
    else if ( effectString == "solid") {
      transitionTime = 0;
    }

    flashRed = map(flashRed, 0, 255, 0, flashBrightness);
    flashGreen = map(flashGreen, 0, 255, 0, flashBrightness);
    flashBlue = map(flashBlue, 0, 255, 0, flashBrightness);

    flash = true;
    startFlash = true;
  }
  else { // Not flashing
    flash = false;

    if (stateOn) {   //if the light is turned on and the light isn't flashing
      onbeforeflash = true;
    }

    if (root.containsKey("color")) {
      red = root["color"]["r"];
      green = root["color"]["g"];
      blue = root["color"]["b"];
    }

    if (root.containsKey("color_temp")) {
      //temp comes in as mireds, need to convert to kelvin then to RGB
      int color_temp = root["color_temp"];
      unsigned int kelvin  = 1000000 / color_temp;

      temp2rgb(kelvin);

    }

    if (root.containsKey("brightness")) {
      brightness = root["brightness"];
    }

    if (root.containsKey("effect")) {
      effect = root["effect"];
      effectString = effect;
      twinklecounter = 0; //manage twinklecounter
      if (strlen(root["effect"]) < 32)
        strcpy(config.effect, root["effect"]);
      else
        strcpy(config.effect, "error");
      int q = sizeof(config) - sizeof(config.effect) - sizeof(config.technology)- sizeof(config.numleds);
      Serial.println("writing eeprom effect:");
      for (int i = 0; i < sizeof(config.effect); ++i)
      {
        EEPROM.write(q + i, config.effect[i]);
        Serial.print("Wrote: ");
        Serial.println(config.effect[i]);
      }
      noInterrupts();
      EEPROM.commit();
      interrupts();
    }

    if (root.containsKey("transition")) {
      transitionTime = root["transition"];
    }
    if (root.containsKey("white_value")) {
      transitionTime = map(root["white_value"], 0, 255, 1, 150);
    }
    else if ( effectString == "solid") {
      transitionTime = 0;
    }

  }

  return true;
}



/********************************** START SEND STATE*****************************************/
void sendState() {
  DynamicJsonDocument root(1024);

  root["state"] = (stateOn) ? on_cmd : off_cmd;
  JsonObject color = root.createNestedObject("color");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;

  root["brightness"] = brightness;
  root["effect"] = effectString.c_str();
  root["white_value"] = map(transitionTime, 1, 150, 0, 255);


  char buffer[measureJson(root) + 1];
  serializeJson(root, buffer, measureJson(root) + 1);
  char* endpoint    = new char[sizeof("domustech/") + strlen(config.name)];
  sprintf(endpoint, "domustech/%s", config.name);
  client.publish(endpoint, buffer, true);
}

/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    handle_input();
    led_flash();
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(config.name, config.mqtt_username, config.mqtt_password)) {
      Serial.println("connected");
      char* endpoint    = new char[sizeof("domustech/") + strlen(config.name) + strlen("/set")];
      sprintf(endpoint, "domustech/%s/set", config.name);
      client.subscribe(endpoint);
      setColor(0, 0, 0);
      sendState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    register_homeassistant();
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

/********************************** START Set Color*****************************************/
void setColor(int inR, int inG, int inB) {
  for (int i = 0; i < config.numleds; i++) {
    leds[i].red   = inR;
    leds[i].green = inG;
    leds[i].blue  = inB;
  }

  FastLED.show();

  Serial.println("Setting LEDs:");
  Serial.print("r: ");
  Serial.print(inR);
  Serial.print(", g: ");
  Serial.print(inG);
  Serial.print(", b: ");
  Serial.println(inB);
}

/********************************** START MAIN LOOP*****************************************/
void loop() {
  handle_input();
  if (!client.connected()) {
    reconnect();
  }
  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print("WIFI Disconnected. Attempting reconnection.");
    setup_wifi();
    return;
  }

  client.loop();
  digitalWrite(LED_BUILTIN, HIGH); //Tempory there because the LED stay on even when setup is end
  // Read data over socket
  int packetSize = port.parsePacket();
  // If packets have been received, interpret the command
  if (packetSize) {
    if (effectString != "Visualizer" || !stateOn) {
      effectString = "Visualizer";
      stateOn = true;
      setColor(0, 0, 0);
      sendState();
    }
    int len = port.read(packetBuffer, BUFFER_LEN);
    for (int i = 0; i < len; i += 5) {
      packetBuffer[len] = 0;
      int N = ((packetBuffer[i] << 8) + packetBuffer[i + 1]);
      leds[N] = CRGB((uint8_t)packetBuffer[i + 2], (uint8_t)packetBuffer[i + 3], (uint8_t)packetBuffer[i + 4]);
    }
    FastLED.show();
  } else {
    effects();
  }
}
void effects() {

  ArduinoOTA.handle();

  /////////////////////////////////////////
  //////Music Visualizer//////////////
  ///////////////////////////////////////

  // Left to Right
  if (effectString == "Music - L2R") {
    visualize_music(1);
  }
  // Middle Out
  if (effectString == "Music - Middle") {
    visualize_music(2);
  }
  // Custom for Fma965
  if (effectString == "Music - Fma965") {
    visualize_music(3);
  }
  // Out to Middle
  if (effectString == "Music - LR2M") {
    visualize_music(4);
  }

  /////////////////////////////////////////
  //////DrZzs custom effects//////////////
  ///////////////////////////////////////

  if (effectString == "Christmas bounce") {                                  // colored stripes pulsing in Shades of GREEN and RED
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = bhw2_xmas_gp;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < config.numleds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Christmas") {                                  // colored stripes pulsing in Shades of GREEN and RED
    static uint8_t startIndex = 0;
    startIndex = startIndex + floor(transitionTime / 10);
    CRGBPalette16 palette = bhw2_xmas_gp_b;
    fill_palette( leds, config.numleds, startIndex, 3, palette, 255, LINEARBLEND);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "St Patty") {                                  // colored stripes pulsing in Shades of GREEN
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = bhw2_greenman_gp;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < config.numleds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Valentine") {                                  // colored stripes pulsing in Shades of PINK and RED
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = bhw2_redrosey_gp;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < config.numleds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Turkey Day") {                                  // colored stripes pulsing in Shades of Brown and ORANGE
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = bhw2_thanks_gp;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < config.numleds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Thanksgiving") {                                  // colored stripes pulsing in Shades of Red and ORANGE and Green
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* higher = faster motion */

    fill_palette( leds, config.numleds,
                  startIndex, 16, /* higher = narrower stripes */
                  ThxPalettestriped, 255, LINEARBLEND);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "USA") {                                  // colored stripes pulsing in Shades of Red White & Blue
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = bhw3_41_gp;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < config.numleds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Independence") {                        // colored stripes of Red White & Blue
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* higher = faster motion */

    fill_palette( leds, config.numleds,
                  startIndex, 16, /* higher = narrower stripes */
                  IndPalettestriped, 255, LINEARBLEND);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }


  if (effectString == "Halloween") {                                  // colored stripes pulsing in Shades of Purple and Orange
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = Orange_to_Purple_gp;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < config.numleds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Go Lions") {                                  // colored stripes pulsing in Shades of <strike>Maize and</strike> Blue & White (FTFY DrZZZ :-P)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PSU_gp;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < config.numleds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Hail") {
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* higher = faster motion */

    fill_palette( leds, config.numleds,
                  startIndex, 16, /* higher = narrower stripes */
                  hailPalettestriped, 255, LINEARBLEND);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Touchdown") {                 //<strike>Maize and</strike> Blue & White with POLICE ALL animation
    idex++;
    if (idex >= config.numleds) {
      idex = 0;
    }
    int idexY = idex;
    int idexB = antipodal_index(idexY);
    int thathue = ( thishuehail + 64) % 255;
    leds[idexY] = CRGB::Blue; //CHSV(thishuehail, thissat, 255);
    leds[idexB] = CRGB::White; //CHSV(thathue, thissat, 255);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Punkin") {
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* higher = faster motion */

    fill_palette( leds, config.numleds,
                  startIndex, 16, /* higher = narrower stripes */
                  HalloweenPalettestriped, 255, LINEARBLEND);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Lovey Day") {                 //Valentine's Day colors (TWO COLOR SOLID)
    idex++;
    if (idex >= config.numleds) {
      idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    int thathue = (thishueLovey + 244) % 255;
    leds[idexR] = CHSV(thishueLovey, thissat, 255);
    leds[idexB] = CHSV(thathue, thissat, 255);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  if (effectString == "Holly Jolly") {
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* higher = faster motion */

    fill_palette( leds, config.numleds,
                  startIndex, 16, /* higher = narrower stripes */
                  HJPalettestriped, 255, LINEARBLEND);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  /////////////////End DrZzs effects/////////////
  ///////////////////////////////////////////////

  ////////Place your custom effects below////////////




  /////////////end custom effects////////////////

  ///////////////////////////////////////////////
  /////////fastLED & Bruh effects///////////////
  /////////////////////////////////////////////

  //EFFECT BPM
  if (effectString == "bpm") {
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for ( int i = 0; i < config.numleds; i++) { //9948
      leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT Candy Cane
  if (effectString == "candy cane") {
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* higher = faster motion */
    fill_palette( leds, config.numleds,
                  startIndex, 16, /* higher = narrower stripes */
                  currentPalettestriped, 255, LINEARBLEND);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 0;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT CONFETTI
  if (effectString == "confetti" ) {
    fadeToBlackBy( leds, config.numleds, 25);
    int pos = random16(config.numleds);
    leds[pos] += CRGB(realRed + random8(64), realGreen, realBlue);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT CYCLON RAINBOW
  if (effectString == "cyclon rainbow") {                    //Single Dot Down
    static uint8_t hue = 0;
    // First slide the led in one direction
    for (int i = 0; i < config.numleds; i++) {
      // Set the i'th led to red
      leds[i] = CHSV(hue++, 255, 255);
      // Show the leds
      delayMultiplier = 1;
      showleds();
      // now that we've shown the leds, reset the i'th led to black
      // leds[i] = CRGB::Black;
      fadeall();
      // Wait a little bit before we loop around and do it again
      delay(10);
    }
    for (int i = (config.numleds) - 1; i >= 0; i--) {
      // Set the i'th led to red
      leds[i] = CHSV(hue++, 255, 255);
      // Show the leds
      delayMultiplier = 1;
      showleds();
      // now that we've shown the leds, reset the i'th led to black
      // leds[i] = CRGB::Black;
      fadeall();
      // Wait a little bit before we loop around and do it again
      delay(10);
    }
  }


  //EFFECT DOTS
  if (effectString == "dots") {
    uint8_t inner = beatsin8(bpm, config.numleds / 4, config.numleds / 4 * 3);
    uint8_t outer = beatsin8(bpm, 0, config.numleds - 1);
    uint8_t middle = beatsin8(bpm, config.numleds / 3, config.numleds / 3 * 2);
    leds[middle] = CRGB::Purple;
    leds[inner] = CRGB::Blue;
    leds[outer] = CRGB::Aqua;
    nscale8(leds, config.numleds, fadeval);

    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT FIRE
  if (effectString == "fire") {
    Fire2012WithPalette();
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 150;
    }
    delayMultiplier = 2;
    showleds();
  }

  random16_add_entropy( random8());


  //EFFECT Glitter
  if (effectString == "glitter") {
    fadeToBlackBy( leds, config.numleds, 20);
    addGlitterColor(80, realRed, realGreen, realBlue);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT JUGGLE
  if (effectString == "juggle" ) {                           // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(leds, config.numleds, 20);
    for (int i = 0; i < 8; i++) {
      leds[beatsin16(i + 7, 0, config.numleds - 1  )] |= CRGB(realRed, realGreen, realBlue);
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 130;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT LIGHTNING
  if (effectString == "lightning") {
    twinklecounter = twinklecounter + 1;                     //Resets strip if previous animation was running
    if (twinklecounter < 2) {
      FastLED.clear();
      FastLED.show();
    }
    ledstart = random16(config.numleds);           // Determine starting location of flash
    ledlen = random16(config.numleds - ledstart);  // Determine length of flash (not to go beyond config.numleds-1)
    for (int flashCounter = 0; flashCounter < random8(3, flashes); flashCounter++) {
      if (flashCounter == 0) dimmer = 5;    // the brightness of the leader is scaled down by a factor of 5
      else dimmer = random8(1, 3);          // return strokes are brighter than the leader
      fill_solid(leds + ledstart, ledlen, CHSV(255, 0, 255 / dimmer));
      showleds();    // Show a section of LED's
      delay(random8(4, 10));                // each flash only lasts 4-10 milliseconds
      fill_solid(leds + ledstart, ledlen, CHSV(255, 0, 0)); // Clear the section of LED's
      showleds();
      if (flashCounter == 0) delay (130);   // longer delay until next flash after the leader
      delay(50 + random8(100));             // shorter delay between strokes
    }
    delay(random8(frequency) * 100);        // delay between strikes
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 0;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT POLICE ALL
  if (effectString == "police all") {                 //POLICE LIGHTS (TWO COLOR SOLID)
    idex++;
    if (idex >= config.numleds) {
      idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    int thathue = (thishuepolice + 160) % 255;
    leds[idexR] = CHSV(thishuepolice, thissat, 255);
    leds[idexB] = CHSV(thathue, thissat, 255);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }

  //EFFECT POLICE ONE
  if (effectString == "police one") {
    idex++;
    if (idex >= config.numleds) {
      idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    int thathue = (thishuepolice + 160) % 255;
    for (int i = 0; i < config.numleds; i++ ) {
      if (i == idexR) {
        leds[i] = CHSV(thishuepolice, thissat, 255);
      }
      else if (i == idexB) {
        leds[i] = CHSV(thathue, thissat, 255);
      }
      else {
        leds[i] = CHSV(0, 0, 0);
      }
    }
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT RAINBOW
  if (effectString == "rainbow") {
    // FastLED's built-in rainbow generator
    static uint8_t starthue = 0;    thishue++;
    fill_rainbow(leds, config.numleds, thishue, deltahue);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 130;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT RAINBOW WITH GLITTER
  if (effectString == "rainbow with glitter") {               // FastLED's built-in rainbow generator with Glitter
    static uint8_t starthue = 0;
    thishue++;
    fill_rainbow(leds, config.numleds, thishue, deltahue);
    addGlitter(80);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 130;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT SINELON
  if (effectString == "sinelon") {
    fadeToBlackBy( leds, config.numleds, 20);
    int pos = beatsin16(13, 0, config.numleds - 1);
    leds[pos] += CRGB(realRed, realGreen, realBlue);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 150;
    }
    delayMultiplier = 1;
    showleds();
  }


  //EFFECT TWINKLE
  if (effectString == "twinkle") {
    twinklecounter = twinklecounter + 1;
    if (twinklecounter < 2) {                               //Resets strip if previous animation was running
      FastLED.clear();
      FastLED.show();
    }
    const CRGB lightcolor(8, 7, 1);
    for ( int i = 0; i < config.numleds; i++) {
      if ( !leds[i]) continue; // skip black pixels
      if ( leds[i].r & 1) { // is red odd?
        leds[i] -= lightcolor; // darken if red is odd
      } else {
        leds[i] += lightcolor; // brighten if red is even
      }
    }
    if ( random8() < DENSITY) {
      int j = random16(config.numleds);
      if ( !leds[j] ) leds[j] = lightcolor;
    }

    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 0;
    }
    delayMultiplier = 1;
    showleds();
  }

  //EFFECT CHRISTMAS ALTERNATE
  if (effectString == "christmas alternate") {
    for (int i = 0; i < config.numleds; i++) {
      if ((toggle + i) % 2 == 0) {
        leds[i] = CRGB::Crimson;
      }
      else {
        leds[i] = CRGB::DarkGreen;
      }
    }
    toggle = (toggle + 1) % 2;
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 130;
    }
    delayMultiplier = 30;
    showleds();
    fadeall();
    //delay(200);
  }

  //EFFECT RANDOM STARS
  if (effectString == "random stars") {
    fadeUsingColor( leds, config.numleds, CRGB::Blue);
    int pos = random16(config.numleds);
    leds[pos] += CRGB(realRed + random8(64), realGreen, realBlue);
    addGlitter(80);
    if (transitionTime == 0 or transitionTime == NULL) {
      transitionTime = 30;
    }
    delayMultiplier = 6;
    //delay(60);
    showleds();

  }

  //EFFECT "Sine Hue"
  if (effectString == "sine hue") {
    static uint8_t hue_index = 0;
    static uint8_t led_index = 0;
    if (led_index >= config.numleds) {  //Start off at 0 if the led_index was incremented past the segment size in some other effect
      led_index = 0;
    }
    for (int i = 0; i < config.numleds; i = i + 1)
    {
      leds[i] = CHSV(hue_index, 255, 255 - int(abs(sin(float(i + led_index) / config.numleds * 2 * 3.14159) * 255)));
    }

    led_index++, hue_index++;

    if (hue_index >= 255) {
      hue_index = 0;
    }
    delayMultiplier = 2;
    showleds();
  }


  //EFFECT "Full Hue"
  if (effectString == "full hue") {
    static uint8_t hue_index = 0;
    fill_solid(leds, config.numleds, CHSV(hue_index, 255, 255));
    hue_index++;

    if (hue_index >= 255) {
      hue_index = 0;
    }
    delayMultiplier = 2;
    showleds();
  }


  //EFFECT "Breathe"
  if (effectString == "breathe") {
    static bool toggle;
    static uint8_t brightness_index = 0;
    fill_solid(leds, config.numleds, CHSV(thishue, 255, brightness_index));
    if (brightness_index >= 255) {
      toggle = 0;
    }
    else if (brightness_index <= 0)
    {
      toggle = 1;
    }

    if (toggle)
    {
      brightness_index++;
    }
    else
    {
      brightness_index--;
    }

    delayMultiplier = 2;
    showleds();
  }


  //EFFECT "Hue Breathe"
  if (effectString == "hue breathe") {
    static uint8_t hue_index = 0;
    static bool toggle = 1;
    static uint8_t brightness_index = 0;
    fill_solid(leds, config.numleds, CHSV(hue_index, 255, brightness_index));
    if (brightness_index >= 255) {
      toggle = 0;
      hue_index = hue_index + 10;
    }
    else if (brightness_index <= 0)
    {
      toggle = 1;
      hue_index = hue_index + 10;
    }

    if (toggle)
    {
      brightness_index++;
    }
    else
    {
      brightness_index--;
    }

    if (hue_index >= 255) {
      hue_index = 0;
    }

    delayMultiplier = 2;
    showleds();
  }

  EVERY_N_MILLISECONDS(10) {
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);  // FOR NOISE ANIMATIon
    {
      gHue++;
    }

    //EFFECT NOISE
    if (effectString == "noise") {
      for (int i = 0; i < config.numleds; i++) {                                     // Just onE loop to fill up the LED array as all of the pixels change.
        uint8_t index = inoise8(i * scale, dist + i * scale) % 255;            // Get a value from the noise function. I'm using both x and y axis.
        leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);   // With that value, look up the 8 bit colour palette value and assign it to the current LED.
      }
      dist += beatsin8(10, 1, 4);                                              // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
      // In some sketches, I've used millis() instead of an incremented counter. Works a treat.
      if (transitionTime == 0 or transitionTime == NULL) {
        transitionTime = 0;
      }
      delayMultiplier = 1;
      showleds();
    }

    //EFFECT RIPPLE
    if (effectString == "ripple") {
      for (int i = 0; i < config.numleds; i++) leds[i] = CHSV(bgcol++, 255, 15);  // Rotate background colour.
      switch (step) {
        case -1:                                                          // Initialize ripple variables.
          center = random(config.numleds);
          colour = random8();
          step = 0;
          break;
        case 0:
          leds[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
          step ++;
          break;
        case maxsteps:                                                    // At the end of the ripples.
          step = -1;
          break;
        default:                                                             // Middle of the ripples.
          leds[(center + step + config.numleds) % config.numleds] += CHSV(colour, 255, myfade / step * 2);   // Simple wrap from Marc Miller
          leds[(center - step + config.numleds) % config.numleds] += CHSV(colour, 255, myfade / step * 2);
          step ++;                                                         // Next step.
          break;
      }
      if (transitionTime == 0 or transitionTime == NULL) {
        transitionTime = 30;
      }
      delayMultiplier = 1;
      showleds();
    }
  }

  EVERY_N_SECONDS(5) {
    targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 192, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)));
  }

  //FLASH AND FADE SUPPORT
  if (flash) {
    if (startFlash) {
      startFlash = false;
      flashStartTime = millis();
    }

    if ((millis() - flashStartTime) <= flashLength) {
      if ((millis() - flashStartTime) % 1000 <= 500) {
        setColor(flashRed, flashGreen, flashBlue);
      }
      else {
        setColor(0, 0, 0);
        // If you'd prefer the flashing to happen "on top of"
        // the current color, uncomment the next line.
        // setColor(realRed, realGreen, realBlue);
      }
    }
    else {
      flash = false;
      effectString = oldeffectString;
      if (onbeforeflash) { //keeps light off after flash if light was originally off
        setColor(realRed, realGreen, realBlue);
      }
      else {
        stateOn = false;
        setColor(0, 0, 0);
        sendState();
      }
    }
  }

  if (startFade && effectString == "solid") {
    // If we don't want to fade, skip it.
    if (transitionTime == 0) {
      setColor(realRed, realGreen, realBlue);

      redVal = realRed;
      grnVal = realGreen;
      bluVal = realBlue;

      startFade = false;
    }
    else {
      loopCount = 0;
      stepR = calculateStep(redVal, realRed);
      stepG = calculateStep(grnVal, realGreen);
      stepB = calculateStep(bluVal, realBlue);

      inFade = true;
    }
  }

  if (inFade) {
    startFade = false;
    unsigned long now = millis();
    if (now - lastLoop > transitionTime) {
      if (loopCount <= 1020) {
        lastLoop = now;

        redVal = calculateVal(stepR, redVal, loopCount);
        grnVal = calculateVal(stepG, grnVal, loopCount);
        bluVal = calculateVal(stepB, bluVal, loopCount);

        if (effectString == "solid") {
          setColor(redVal, grnVal, bluVal); // Write current values to LED pins
        }
        loopCount++;
      }
      else {
        inFade = false;
      }
    }
  }
}

/**************************** START TRANSITION FADER *****************************************/
// From https://www.arduino.cc/en/Tutorial/ColorCrossfader
/* BELOW THIS LINE IS THE MATH -- YOU SHOULDN'T NEED TO CHANGE THIS FOR THE BASICS
  The program works like this:
  Imagine a crossfade that moves the red LED from 0-10,
    the green from 0-5, and the blue from 10 to 7, in
    ten steps.
    We'd want to count the 10 steps and increase or
    decrease color values in evenly stepped increments.
    Imagine a + indicates raising a value by 1, and a -
    equals lowering it. Our 10 step fade would look like:
    1 2 3 4 5 6 7 8 9 10
  R + + + + + + + + + +
  G   +   +   +   +   +
  B     -     -     -
  The red rises from 0 to 10 in ten steps, the green from
  0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
  In the real program, the color percentages are converted to
  0-255 values, and there are 1020 steps (255*4).
  To figure out how big a step there should be between one up- or
  down-tick of one of the LED values, we call calculateStep(),
  which calculates the absolute gap between the start and end values,
  and then divides that gap by 1020 to determine the size of the step
  between adjustments in the value.
*/
int calculateStep(int prevValue, int endValue) {
  int step = endValue - prevValue; // What's the overall gap?
  if (step) {                      // If its non-zero,
    step = 1020 / step;          //   divide by 1020
  }

  return step;
}
/* The next function is calculateVal. When the loop value, i,
   reaches the step size appropriate for one of the
   colors, it increases or decreases the value of that color by 1.
   (R, G, and B are each calculated separately.)
*/
int calculateVal(int step, int val, int i) {
  if ((step) && i % step == 0) { // If step is non-zero and its time to change a value,
    if (step > 0) {              //   increment the value if step is positive...
      val += 1;
    }
    else if (step < 0) {         //   ...or decrement it if step is negative
      val -= 1;
    }
  }

  // Defensive driving: make sure val stays in the range 0-255
  if (val > 255) {
    val = 255;
  }
  else if (val < 0) {
    val = 0;
  }

  return val;
}

////////////////////////place setup__Palette and __Palettestriped custom functions here - for Candy Cane effects /////////////////
///////You can use up to 4 colors and change the pattern of A's AB's B's and BA's as you like//////////////


/**************************** START STRIPLED PALETTE *****************************************/
void setupStripedPalette( CRGB A, CRGB AB, CRGB B, CRGB BA) {
  currentPalettestriped = CRGBPalette16(
                            A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                            //    A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                          );
}

void setupHailPalette( CRGB A, CRGB AB, CRGB B, CRGB BA)
{
  hailPalettestriped = CRGBPalette16(
                         A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                       );
}

void setupHJPalette( CRGB A, CRGB AB, CRGB B, CRGB BA)
{
  HJPalettestriped = CRGBPalette16(
                       A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                     );
}

void setupIndPalette( CRGB A, CRGB AB, CRGB B, CRGB BA)
{
  IndPalettestriped = CRGBPalette16(
                        A, A, A, A, A, AB, AB, AB, AB, AB, B, B, B, B, B, B
                      );
}

void setupThxPalette( CRGB A, CRGB AB, CRGB B, CRGB BA)
{
  ThxPalettestriped = CRGBPalette16(
                        A, A, A, A, A, A, A, AB, AB, AB, B, B, B, B, B, B
                      );
}

void setupHalloweenPalette( CRGB A, CRGB AB, CRGB B, CRGB BA)
{
  HalloweenPalettestriped = CRGBPalette16(
                              A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                            );
}

////////////////////////////////////////////////////////


/********************************** START FADE************************************************/
void fadeall() {
  for (int i = 0; i < config.numleds; i++) {
    leds[i].nscale8(250);  //for CYCLon
  }
}

/********************************** START FIRE **********************************************/
void Fire2012WithPalette()
{
  // Array of temperature readings at each simulation cell
  static byte heat[576];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < config.numleds; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / config.numleds) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = config.numleds - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < config.numleds; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( gPal, colorindex);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (config.numleds - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}

/********************************** START ADD GLITTER *********************************************/
void addGlitter( fract8 chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(config.numleds) ] += CRGB::White;
  }
}

/********************************** START ADD GLITTER COLOR ****************************************/
void addGlitterColor( fract8 chanceOfGlitter, int red, int green, int blue)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(config.numleds) ] += CRGB(red, green, blue);
  }
}

/********************************** START SHOW LEDS ***********************************************/
void showleds() {

  delay(1);

  if (stateOn) {
    FastLED.setBrightness(brightness);  //EXECUTE EFFECT COLOR
    FastLED.show();
    if (transitionTime > 0 && transitionTime < 250) {  //Sets animation speed based on receieved value
      FastLED.delay(transitionTime / 10 * delayMultiplier); //1000 / transitionTime);
      //delay(10*transitionTime);
    }
  }
  else if (startFade) {
    setColor(0, 0, 0);
    startFade = false;
  }
}
void temp2rgb(unsigned int kelvin) {
  int tmp_internal = kelvin / 100.0;

  // red
  if (tmp_internal <= 66) {
    red = 255;
  } else {
    float tmp_red = 329.698727446 * pow(tmp_internal - 60, -0.1332047592);
    if (tmp_red < 0) {
      red = 0;
    } else if (tmp_red > 255) {
      red = 255;
    } else {
      red = tmp_red;
    }
  }

  // green
  if (tmp_internal <= 66) {
    float tmp_green = 99.4708025861 * log(tmp_internal) - 161.1195681661;
    if (tmp_green < 0) {
      green = 0;
    } else if (tmp_green > 255) {
      green = 255;
    } else {
      green = tmp_green;
    }
  } else {
    float tmp_green = 288.1221695283 * pow(tmp_internal - 60, -0.0755148492);
    if (tmp_green < 0) {
      green = 0;
    } else if (tmp_green > 255) {
      green = 255;
    } else {
      green = tmp_green;
    }
  }

  // blue
  if (tmp_internal >= 66) {
    blue = 255;
  } else if (tmp_internal <= 19) {
    blue = 0;
  } else {
    float tmp_blue = 138.5177312231 * log(tmp_internal - 10) - 305.0447927307;
    if (tmp_blue < 0) {
      blue = 0;
    } else if (tmp_blue > 255) {
      blue = 255;
    } else {
      blue = tmp_blue;
    }
  }
}

/**************************** MUSIC VISUALIZER **************************************************/
// https://github.com/the-red-team/Arduino-FastLED-Music-Visualizer/blob/master/music_visualizer.ino
// Modified by Fma965

CRGB Scroll(int pos) {
  CRGB color (0, 0, 0);
  if (pos < 85) {
    color.g = 0;
    color.r = ((float)pos / 85.0f) * 255.0f;
    color.b = 255 - color.r;
  } else if (pos < 170) {
    color.g = ((float)(pos - 85) / 85.0f) * 255.0f;
    color.r = 255 - color.g;
    color.b = 0;
  } else if (pos < 256) {
    color.b = ((float)(pos - 170) / 85.0f) * 255.0f;
    color.g = 255 - color.b;
    color.r = 1;
  }
  return color;
}

void visualize_music(int LEDDirection)
{
  //int audio_input = analogRead(audio) * MUSIC_SENSITIVITY;
  int audio_input = analogRead(audio) * map(transitionTime, 1, 150, 2, 7);
  if (audio_input > 0)
  {
    if (LEDDirection == 1) {
      pre_react = ((long)config.numleds * (long)audio_input) / 1023L; // TRANSLATE AUDIO LEVEL TO NUMBER OF LEDs
    } else if (LEDDirection == 2 || LEDDirection == 4) {
      pre_react = ((long)config.numleds / 2 * (long)audio_input) / 1023L; // TRANSLATE AUDIO LEVEL TO NUMBER OF LEDs
    } else if (LEDDirection == 3) {
      pre_react = ((long)config.numleds / 4 * (long)audio_input) / 1023L; // TRANSLATE AUDIO LEVEL TO NUMBER OF LEDs
    }

    if (pre_react > react) // ONLY ADJUST LEVEL OF LED IF LEVEL HIGHER THAN CURRENT LEVEL
      react = pre_react;
  }
  if (LEDDirection == 1) {
    RainbowL2R(); // Left to Right
  } else if (LEDDirection == 2) {
    RainbowMiddleOut(); //Middle Out
  } else if (LEDDirection == 3) {
    RainbowFma965(); //Custom setup for Fma965
  } else if (LEDDirection == 4) {
    RainbowOutMiddle(); //Out to Middle
  }

  k = k - wheel_speed; // SPEED OF COLOR WHEEL
  if (k < 0) // RESET COLOR WHEEL
    k = 255;

  // REMOVE LEDs
  decay_check++;
  if (decay_check > decay)
  {
    decay_check = 0;
    if (react > 0)
      react--;
  }
  if (transitionTime <= 50) {
    delay(25);
  } else {
    delay(transitionTime / 2);
  }
}

// https://github.com/NeverPlayLegit/Rainbow-Fader-FastLED/blob/master/rainbow.ino
void RainbowL2R()
{
  for (int i = config.numleds - 1; i >= 0; i--) {
    if (i < react)
      leds[i] = Scroll((i * 256 / 50 + k) % 256);
    else
      leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

void RainbowMiddleOut()
{
  for (int i = 0; i < config.numleds / 2; i++) {
    if (i < react) {
      leds[config.numleds / 2 + i] = Scroll((i * 256 / config.numleds + k) % 256);
      leds[config.numleds / 2 - i - 1] = Scroll((i * 256 / config.numleds + k) % 256);
    }
    else {
      leds[config.numleds / 2 + i] = CRGB(0, 0, 0);
      leds[config.numleds / 2 - i - 1] = CRGB(0, 0, 0);
    }
  }
  FastLED.show();
}

void RainbowOutMiddle()
{
  for (int i = 0; i < config.numleds / 2 + 1; i++) {
    if (i < react) {
      leds[0 + i] = Scroll((i * 256 / config.numleds + k) % 256);
      leds[config.numleds - i] = Scroll((i * 256 / config.numleds + k) % 256);
    } else {
      leds[0 + i] = CRGB(0, 0, 0);
      leds[config.numleds - i] = CRGB(0, 0, 0);
    }
  }
  FastLED.show();
}

void RainbowFma965()
{
  for (int i = 0; i < 21; i++) {
    if (i < react) {
      leds[42 - i - 1] = Scroll((i * 256 / config.numleds + k) % 256);
      leds[42 + i] = Scroll((i * 256 / config.numleds + k) % 256);
    } else {
      leds[42 - i - 1] = CRGB(0, 0, 0);
      leds[42 + i] = CRGB(0, 0, 0);
    }
  }
  for (int i = 0; i < 12; i++) {
    if (i < react) {
      leds[10 + i] = Scroll((i * 256 / config.numleds + k) % 256);
      leds[10 - i - 1] = Scroll((i * 256 / config.numleds + k) % 256);
    } else {
      leds[10 + i] = CRGB(0, 0, 0);
      leds[10 - i - 1] = CRGB(0, 0, 0);
    }
  }
  FastLED.show();
}

//WIFI configurator
bool testWifi(void)
{
  int c = 0;
  unsigned long start_time = millis();
  Serial.println("Waiting for Wifi to connect");
  while ( millis() - start_time < 60000 ) {
    handle_input();
    effects();
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    if ((millis() % 1000) == 0) {
      Serial.print("*");
      led_flash();
      delay(10);
    }
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("DomusTech-LED" + String(ESP.getChipId()), "");
  Serial.println("softap");

  digitalWrite(LED_BUILTIN, LOW);
  launchWeb();
  Serial.println("over");
}

void createWebServer()
{
  {
    server.on("/", []() {

      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += ipStr;
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><br><label>Wifi password: </label><input name='pass' length=64><br><label>Strip name: </label><input name='sensorname' length=32><br><label>OTA password: </label><input name='otapass' length=64><br><label>MQTT Server: </label><input name='mqttserver' length=32><br><label>MQTT user: </label><input name='mqttuser' length=32><br><label>MQTT password: </label><input name='mqttpass' length=64><br><label>Number of leds: </label><input type='number' name='numleds' min='1' max='576'><br><label>Leds technology: </label><input list='technology' name='technology'><datalist id='technology'><option value='1'>APA102</option><option value='2'>WS2811</option></datalist><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      String sensorname = server.arg("sensorname");
      String otapass = server.arg("otapass");
      String mqttserver = server.arg("mqttserver");
      String mqttuser = server.arg("mqttuser");
      String mqttpass = server.arg("mqttpass");
      String numleds = server.arg("numleds");
      String technology = server.arg("technology");
      if (qsid.length() > 0 && qpass.length() > 0 && sensorname.length() > 0 && otapass.length() > 0 && mqttserver.length() > 0 && mqttuser.length() > 0 && mqttpass.length() > 0 && numleds.length() > 0 && technology.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < sizeof(config); ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
        int q = 0;
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(q + i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        q += sizeof(config.ssid);
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(q + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        q += sizeof(config.password);
        Serial.println("writing eeprom sensorname:");
        for (int i = 0; i < sensorname.length(); ++i)
        {
          EEPROM.write(q + i, sensorname[i]);
          Serial.print("Wrote: ");
          Serial.println(sensorname[i]);
        }
        q += sizeof(config.name);
        Serial.println("writing eeprom otapass:");
        for (int i = 0; i < otapass.length(); ++i)
        {
          EEPROM.write(q + i, otapass[i]);
          Serial.print("Wrote: ");
          Serial.println(otapass[i]);
        }
        q += sizeof(config.OTApassword);
        Serial.println("writing eeprom mqttserver:");
        for (int i = 0; i < mqttserver.length(); ++i)
        {
          EEPROM.write(q + i, mqttserver[i]);
          Serial.print("Wrote: ");
          Serial.println(mqttserver[i]);
        }
        q += sizeof(config.mqtt_server);
        Serial.println("writing eeprom mqttuser:");
        for (int i = 0; i < mqttuser.length(); ++i)
        {
          EEPROM.write(q + i, mqttuser[i]);
          Serial.print("Wrote: ");
          Serial.println(mqttuser[i]);
        }
        q += sizeof(config.mqtt_username);
        Serial.println("writing eeprom mqttpass:");
        for (int i = 0; i < mqttpass.length(); ++i)
        {
          EEPROM.write(q + i, mqttpass[i]);
          Serial.print("Wrote: ");
          Serial.println(mqttpass[i]);
        }
        q += sizeof(config.mqtt_password);
        q += sizeof(config.effect);
        Serial.println("writing eeprom numleds:");
        int a = numleds.toInt() & 0xFF;
        int b = numleds.toInt() >> 8;
        EEPROM.write(q, a);
        EEPROM.write(q + 1, b);
        Serial.print("Wrote: ");
        Serial.println(numleds);
        q += sizeof(config.numleds);
        Serial.println("writing eeprom technology:");

        EEPROM.write(q, technology.toInt());
        Serial.print("Wrote: ");
        Serial.println(technology);

        q += sizeof(config.technology);
        Serial.println(q);
        noInterrupts();
        EEPROM.commit();
        interrupts();

        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(statusCode, "application/json", content);
        delay(1000); //Avoid reset before sending state
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(statusCode, "application/json", content);
      }

    });
  }
}

void reset_config() {
  for (int i = 0; i < sizeof(config); ++i) {
    EEPROM.write(i, 0);
  }
  noInterrupts();
  EEPROM.commit();
  interrupts();
  ESP.reset();
}

void register_homeassistant() {
  for (int i = 0; i < 6; ++i) {
    led_flash();
    delay(100);
  }
  char* endpoint_state    = new char[sizeof("domustech/") + strlen(config.name)];
  sprintf(endpoint_state, "domustech/%s", config.name);
  char* endpoint    = new char[sizeof("domustech/") + strlen(config.name) + strlen("/set")];
  sprintf(endpoint, "domustech/%s/set", config.name);
  DynamicJsonDocument root(1024);
  DynamicJsonDocument effect_list(1024);
  for (int i = 0; i < effectListc; ++i) {
    effect_list.add(effectList[i]);
  }
  root["name"] = config.name;
  root["platform"] = "mqtt_json";
  root["state_topic"] = endpoint_state;
  root["command_topic"] = endpoint;
  root["brightness"] = true;
  root["rgb"] = true;
  root["white_value"] = true;
  root["color_temp"] = false;
  root["effect"] = true;
  root["effect_list"] = effect_list;
  serializeJson(root, Serial);
  char buffer[measureJson(root) + 1];
  serializeJson(root, buffer, measureJson(root) + 1);
  char* endpoint_config    = new char[sizeof("homeassistant/light/") + strlen(config.name) + strlen("/config")];
  sprintf(endpoint_config, "homeassistant/light/%s/config", config.name);
  client.publish(endpoint_config, buffer, true);
}
