//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#include <Arduino.h>
#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include <map>

#define WIFI_SSID         "YOUR-WIFI-NAME"
#define WIFI_PASS         "YOUR-WIFI-PASSWORD"

#define APP_KEY           "YOUR-APP-KEY"
#define APP_SECRET        "YOUR-APP-SECRET"

// Enter the device IDs here
#define device_ID_1 "SWITCH_ID_NO_1_HERE"
#define device_ID_2 "SWITCH_ID_NO_2_HERE"
#define device_ID_3 "SWITCH_ID_NO_3_HERE"
#define device_ID_4 "SWITCH_ID_NO_4_HERE"

// Define GPIO connected with Relays and switches
#define RelayPin1 23   // D23
#define RelayPin2 22   // D22
#define RelayPin3 21   // D21
#define RelayPin4 19   // D19

#define SwitchPin1 13  // D13
#define SwitchPin2 12  // D12
#define SwitchPin3 14  // D14
#define SwitchPin4 27  // D27

#define wifiLed 2      // D2

// Comment the following line if you use toggle switches instead of tactile buttons
//#define TACTILE_BUTTON 1

#define BAUD_RATE 9600
#define DEBOUNCE_TIME 250

typedef struct {
  int relayPIN;
  int flipSwitchPIN;
} deviceConfig_t;

// Main configuration
std::map<String, deviceConfig_t> devices = {
  {device_ID_1, {RelayPin1, SwitchPin1}},
  {device_ID_2, {RelayPin2, SwitchPin2}},
  {device_ID_3, {RelayPin3, SwitchPin3}},
  {device_ID_4, {RelayPin4, SwitchPin4}}
};

typedef struct {
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;

// Setup Relays
void setupRelays() {
  for (auto &device : devices) {
    int relayPIN = device.second.relayPIN;
    pinMode(relayPIN, OUTPUT);
    digitalWrite(relayPIN, HIGH);
  }
}

// Setup Flip Switches
void setupFlipSwitches() {
  for (auto &device : devices) {

    flipSwitchConfig_t flipSwitchConfig;

    flipSwitchConfig.deviceId = device.first;
    flipSwitchConfig.lastFlipSwitchChange = 0;
    flipSwitchConfig.lastFlipSwitchState = true;

    int flipSwitchPIN = device.second.flipSwitchPIN;

    flipSwitches[flipSwitchPIN] = flipSwitchConfig;

    pinMode(flipSwitchPIN, INPUT_PULLUP);
  }
}

// Power state callback
bool onPowerState(const String &deviceId, bool &state) {

  Serial.printf("%s: %s\r\n",
                deviceId.c_str(),
                state ? "on" : "off");

  int relayPIN = devices[deviceId].relayPIN;

  digitalWrite(relayPIN, !state);

  return true;
}

// Handle manual switches
void handleFlipSwitches() {

  unsigned long actualMillis = millis();

  for (auto &flipSwitch : flipSwitches) {

    unsigned long lastFlipSwitchChange =
      flipSwitch.second.lastFlipSwitchChange;

    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {

      int flipSwitchPIN = flipSwitch.first;

      bool lastFlipSwitchState =
        flipSwitch.second.lastFlipSwitchState;

      bool flipSwitchState = digitalRead(flipSwitchPIN);

      if (flipSwitchState != lastFlipSwitchState) {

#ifdef TACTILE_BUTTON
        if (flipSwitchState) {
#endif

          flipSwitch.second.lastFlipSwitchChange = actualMillis;

          String deviceId = flipSwitch.second.deviceId;

          int relayPIN = devices[deviceId].relayPIN;

          bool newRelayState = !digitalRead(relayPIN);

          digitalWrite(relayPIN, newRelayState);

          SinricProSwitch &mySwitch =
            SinricPro[deviceId];

          mySwitch.sendPowerStateEvent(!newRelayState);

#ifdef TACTILE_BUTTON
        }
#endif

        flipSwitch.second.lastFlipSwitchState =
          flipSwitchState;
      }
    }
  }
}

// Setup WiFi
void setupWiFi() {

  Serial.printf("\r\n[WiFi]: Connecting");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }

  digitalWrite(wifiLed, HIGH);

  Serial.printf(
    "connected!\r\n[WiFi]: IP-Address is %s\r\n",
    WiFi.localIP().toString().c_str()
  );
}

// Setup SinricPro
void setupSinricPro() {

  for (auto &device : devices) {

    const char *deviceId = device.first.c_str();

    SinricProSwitch &mySwitch =
      SinricPro[deviceId];

    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);

  SinricPro.restoreDeviceStates(true);
}

// Setup
void setup() {

  Serial.begin(BAUD_RATE);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);

  setupRelays();
  setupFlipSwitches();
  setupWiFi();
  setupSinricPro();
}

// Main Loop
void loop() {

  SinricPro.handle();

  handleFlipSwitches();
}
