#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

AsyncWebServer server(80);

String State;

String processor(const String& var) {
  Serial.println(var);
  if (var == "STATE") {
    if (stateOn) {
      State = "ON";
    }
    else {
      State = "OFF";
    }
    Serial.print(State);
    return State;
  }
}

void webserver_setup() {

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route for root /config web page
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, " / config.html", String(), false, processor);
  });

  //Config POST
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest * request) {
    String reponse = "";
    int error = 0;
    int warning = 0;
    //Parameters
    String ssid = request->getParam("ssid", true)->value();
    String technology = request->getParam("technology", true)->value();
    String numleds = request->getParam("numleds", true)->value();
    if (technology != "1" && technology != "2") {
      reponse += "[ERROR]The technology must be set<br>";
      error = 1;
    }
    if (numleds == "" || technology == "0") {
      reponse += "[ERROR]The number of leds must be set<br>";
      error = 1;
    }
    if (ssid == "") {
      reponse += "[WARNING]There is no Wifi, will continue to work in Ad hoc mode<br>";
      warning = 1;
    }
    if (error) {
      request->send(400, "text / plain", reponse);
    } else if (warning) {
      request->send(200, "text / plain", reponse);
    } else
      request->send(200, "text / plain", "The led driver will now restart");
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, " / style.css", "text / css");
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest * request) {
    stateOn = true;
    sendState();
    request->send(SPIFFS, " / index.html", String(), false, processor);
  });

  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest * request) {
    stateOn = false;
    sendState();
    request->send(SPIFFS, " / index.html", String(), false, processor);
  });

  // Start server
  server.begin();
}
