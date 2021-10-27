#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Ticker.h>
#include <WiFiManager.h>

#include <Config.h>
#include <Debug.h>
#include <ObjectModelParser.h>
#include <Neopixels.h>
#include <Structure.h>

ESP8266WebServer server(80);

#define HOSTNAME_LEN 32
#define SERIAL_BUFFER_SIZE 384
char hostname[HOSTNAME_LEN] = "rrf-neopixel";

#ifdef DEBUGGING
WiFiManager wm(Serial1);
#else
WiFiManager wm;
#endif

WiFiManagerParameter custom_hostname("hostname",
 "Choose a hostname for this NeoPixel controller", hostname, HOSTNAME_LEN);
Ticker m409_ticker;
Ticker neopixel_ticker;
ObjectModel object_model;

void save_wifi_config() {
  DEBUG("WiFi portal save");
  if (strncmp(hostname, custom_hostname.getValue(), HOSTNAME_LEN) != 0) {
    LittleFS.begin();
    strncpy(hostname, custom_hostname.getValue(), HOSTNAME_LEN);
    DEBUGF("Saving new hostname: %s\n", hostname);
    File f = LittleFS.open("/hostname", "w");
    if (f) {
      f.print(hostname);
      f.close();
    }
    LittleFS.end();
  }
  delay(500);
  ESP.reset();
}

void send_m409() {
  Serial.println("M409 F\"d99f\"");
}

void handleConfigUpload() {
  HTTPUpload& upload = server.upload();
  static File file;
  switch (upload.status) {
    case UPLOAD_FILE_START:
      LittleFS.begin();
      file = LittleFS.open("/config.json.tmp", "w");
      break;
    case UPLOAD_FILE_WRITE:
      if (file)
        file.write(upload.buf, upload.currentSize);
      break;
    case UPLOAD_FILE_ABORTED:
    case UPLOAD_FILE_END:
      if (file) {
        file.close();
        if (LittleFS.rename("/config.json.tmp", "/config.json")) {
          server.send(200, "text/plain", "Upload OK\n");
        } else {
          server.send(500, "text/plain", "Rename failed\n");
        }
      } else {
        server.send(500, "text/plain", upload.status == UPLOAD_FILE_ABORTED
          ? "upload aborted" : "failed to create file");
      }
      DEBUGF("config upload size: %d\n", upload.totalSize);
      file = LittleFS.open("/config.json", "r");
      if (file) {
        config = parseConfig(file);
        file.close();
        init_neopixels();
      }
      LittleFS.end();
  }
}

void setup() {
  bool config_loaded = false;

  Serial.begin(57600);
  #ifdef DEBUGGING
  Serial1.begin(115200);
  Serial1.setDebugOutput(true);
  #else
  Serial1.begin(57600);
  #endif
  LittleFS.begin();

  File f = LittleFS.open("/hostname", "r");
  if (f) {
    strncpy(hostname, f.readStringUntil('\n').c_str(), HOSTNAME_LEN);
    DEBUGF("Loaded hostname configuration: %s\n", hostname);
    f.close();
  }
  f = LittleFS.open("/config.json", "r");
  if (f) {
    DEBUG("Loading configuration");
    config_loaded = true;
    config = parseConfig(f);
    f.close();
  }
  LittleFS.end();

  wm.setClass("invert");
  const char* menu[] = {"wifi", "wifinoscan", "update", "restart" };
  wm.setMenu(menu, 4);
  wm.addParameter(&custom_hostname);
  wm.setSaveConfigCallback(save_wifi_config);

  if (!config_loaded)
    wm.autoConnect(hostname);

  server.on("/wificonfig", []() {
    server.send(200, "text/plain", "Starting WiFi configurator\n");
    if (!wm.getWebPortalActive()) {
      server.close();
      wm.startWebPortal();
    }
  });
  server.on("/config.json", HTTP_GET, []() {
    LittleFS.begin();
    File configfile = LittleFS.open("/config.json", "r");
    if (configfile) {
      server.setContentLength(configfile.size());
      server.send(200, "application/json", "");
      server.sendContent(configfile);
    } else {
      server.send(404, "text/plain", "Config Not Found");
    }
    LittleFS.end();
  });

  server.on("/config.json", HTTP_POST, []() { server.send(100, "application/json", "{}\n"); }, handleConfigUpload);

  ArduinoOTA.setHostname(hostname);

  DEBUG("Launched");
  init_neopixels();

  m409_ticker.attach_ms_scheduled(1000, send_m409);
  neopixel_ticker.attach_ms_scheduled(200, render_neopixels);

  if (config_loaded && WiFi.SSID() != "") {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WiFi.SSID(), WiFi.psk());
  }
  if (WiFi.SSID() == "") {
    wm.setConfigPortalBlocking(false);
    wm.startConfigPortal(hostname);
  }
}

void loop() {
  static ObjectModel model;
  static char read_buffer[SERIAL_BUFFER_SIZE];
  static bool wifi_connected = false;
  static uint32_t wifi_down_time = millis();
  static bool config_portal_running = false;

  if (!wifi_connected && WiFi.status() == WL_CONNECTED) {
    if (MDNS.begin(hostname)) {
      MDNS.addService("http", "tcp", 80);
    }
    ArduinoOTA.begin();
    server.begin();
  }

  if (wifi_connected && WiFi.status() != WL_CONNECTED) {
    wifi_down_time = millis();
    MDNS.end();
    server.close();
  }

  wifi_connected = WiFi.status() == WL_CONNECTED;

  if (wifi_connected) {
    server.handleClient();
    MDNS.update();
    ArduinoOTA.handle();
  } else {
    if (!wm.getConfigPortalActive() && millis() - wifi_down_time > 15000) {
      DEBUG("Launching WiFi config portal");
      config_portal_running = true;
      WiFi.disconnect();
      wifi_station_disconnect();
      delay(200);
      WiFi.mode(WIFI_AP);
      wm.setConfigPortalBlocking(false);
      wm.startConfigPortal(hostname);
      if (WiFi.SSID() != "")
        wm.setConfigPortalTimeout(300);
    }
    render_connecting();
  }

  int available;
  while ((available = Serial.available()) > 0) {
    int read = Serial.read(read_buffer, min(SERIAL_BUFFER_SIZE, available));
    for (int i = 0; i < read; ++i) {
      parseObjectModel(read_buffer[i], &model);
    }
  }
  if (model.ready) {
    object_model = model;
    model.ready = false;
  }

  // wm.process changes isActive state, make sure it happens last to ensure consistency
  if (wm.getWebPortalActive() || wm.getConfigPortalActive()) {
    wm.process();
  } else if (config_portal_running) {
    config_portal_running = false;
    wifi_down_time = millis();
    DEBUG("WiFi config portal exited");
    if (!wifi_connected && WiFi.SSID() != "") {
      DEBUG("Attempting wifi reconnect");
      WiFi.mode(WIFI_STA);
      delay(200);
      WiFi.begin(WiFi.SSID(), WiFi.psk());
    }
  }
}