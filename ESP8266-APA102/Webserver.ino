#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

AsyncWebServer server(80);

String State;
String Name;
String Effect_list;
String Hexcolor;

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
  if (var == "NAME") {
    Name = config.name;
    Serial.print(Name);
    return Name;
  }
  if (var == "EFFECT_LIST") {
    Effect_list = "";
    for (int i = 0; i < effectListc; ++i) {
      Effect_list += "<option value='";
      Effect_list += effectList[i];
      Effect_list += "'";
      if (effectString == effectList[i])
        Effect_list += " selected";
      Effect_list += ">";
      Effect_list += effectList[i];
      Effect_list += "</option>\r\n";
    }
    return Effect_list;
  }
  if (var == "HEXCOLOR") {
    realRed = map(red, 0, 255, 0, brightness);
    realGreen = map(green, 0, 255, 0, brightness);
    realBlue = map(blue, 0, 255, 0, brightness);
    Hexcolor = "";
    if (String(realRed, HEX).length() == 1)
      Hexcolor += "0" + String(realRed, HEX);
    else
      Hexcolor += String(realRed, HEX);
    if (String(realGreen, HEX).length() == 1)
      Hexcolor += "0" + String(realGreen, HEX);
    else
      Hexcolor += String(realGreen, HEX);
    if (String(realBlue, HEX).length() == 1)
      Hexcolor += "0" + String(realBlue, HEX);
    else
      Hexcolor += String(realBlue, HEX);
    return Hexcolor;
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
    if (config.numleds == 0)
      request->redirect("/config");
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route for root /config web page
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/config.html", String(), false, processor);
  });

  //Config POST
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest * request) {
    String reponse = "";
    int error = 0;
    int warning = 0;
    //Parameters
    String ssid = request->getParam("ssid", true)->value();
    String pass = request->getParam("pass", true)->value();
    String sensorname = request->getParam("sensorname", true)->value();
    String otapass = request->getParam("otapass", true)->value();
    String mqttserver = request->getParam("mqttserver", true)->value();
    String mqttuser = request->getParam("mqttuser", true)->value();
    String mqttpass = request->getParam("mqttpass", true)->value();
    String technology = request->getParam("technology", true)->value();
    String numleds = request->getParam("numleds", true)->value();
    if (technology != "1" && technology != "2") {
      reponse += "[ERROR]The technology must be set\r\n";
      error = 1;
    }
    if (numleds == "" || technology == "0") {
      reponse += "[ERROR]The number of leds must be set\r\n";
      error = 1;
    }
    if (ssid == "") {
      reponse += "[WARNING]There is no Wifi, will continue to work in Ad hoc mode\r\n";
      warning = 1;
    }
    if (mqttserver == "") {
      reponse += "[WARNING]There is no MQTT Server, will only work as stand alone\r\n";
      warning = 1;
    }
    if (error) {
      request->send(400, "text/plain", reponse);
    } else if (warning) {
      request->send(200, "text/plain", reponse);
    } else
      request->send(200, "text/plain", "The led driver will now restart");

    if (!error) {
      int q = 0;
      Serial.println("writing eeprom ssid:");
      for (int i = 0; i < ssid.length(); ++i)
      {
        EEPROM.write(q + i, ssid[i]);
        Serial.print("Wrote: ");
        Serial.println(ssid[i]);
      }
      q += sizeof(config.ssid);
      Serial.println("writing eeprom pass:");
      if (pass.length() > 0) {
        for (int i = 0; i < pass.length(); ++i)
        {
          EEPROM.write(q + i, pass[i]);
          Serial.print("Wrote: ");
          Serial.println(pass[i]);
        }
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
      delay(6000); //Avoid reset before sending state
      ESP.reset();
    }
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to load bootstrap.min.css file
  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bootstrap.min.css", "text/css");
  });

  // Route to load bootstrap.min.js file
  server.on("/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bootstrap.min.js", "text/css");
  });

  // Route to load jscolor.min.js file
  server.on("/jscolor.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/jscolor.min.js", "text/css");
  });

  // Route to set STATE to ON
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest * request) {
    stateOn = true;
    sendState();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to set STATE to OFF
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest * request) {
    stateOn = false;
    setColor(0, 0, 0);
    sendState();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to set EFFECT
  server.on("/effect", HTTP_GET, [](AsyncWebServerRequest * request) {
    effectString = request->getParam("effect")->value();
    stateOn = true;
    realRed = map(red, 0, 255, 0, brightness);
    realGreen = map(green, 0, 255, 0, brightness);
    realBlue = map(blue, 0, 255, 0, brightness);
    startFade = true;
    inFade = false;
    sendState();
    request->redirect("/");
  });

  // Route to set RGB
  server.on("/rgb", HTTP_GET, [](AsyncWebServerRequest * request) {
    String r = request->getParam("r")->value();
    String g = request->getParam("g")->value();
    String b = request->getParam("b")->value();
    brightness = r.toInt();
    if (g.toInt() > brightness)
      brightness = g.toInt();
    else if (b.toInt() > brightness)
      brightness = b.toInt();
    realRed = r.toInt();
    realGreen = g.toInt();
    realBlue = b.toInt();
    red = map(realRed, 0, brightness, 0, 255);
    green = map(realGreen, 0, brightness, 0, 255);
    blue = map(realBlue, 0, brightness, 0, 255);
    startFade = true;
    inFade = false;
    stateOn = true;
    sendState();
    request->redirect("/");
  });

  // Start server
  server.begin();
}
