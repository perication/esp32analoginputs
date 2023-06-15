#include "generated/andycontroller-1_menu.h"
#include <Preferences.h>
#include <ADS1X15.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "FRITZ!Powerline 1260E";
const char* password = "81831059739106230277";
const char* mqttServer = "I192.168.1.114";
const int mqttPort = 1883;
const char* mqttTopic = "voltaje";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);



Preferences preferences;
ADS1115 ADS(0x48);

const int RELAY_1_PIN = 26;
const int RELAY_2_PIN = 27;

const int BUZZER_PIN = 17;

// Floats for resistor values in divider (in ohms); to measure up to 15V
const float R1 = 15000.0;
const float R2 = 7500.0;

float lowbatlevel = 12.5;
float thresholdRange = 0.3;

bool relayActive = false;
unsigned long activationStartTime = 0;

float readVoltage(int pin) {
    int raw = ADS.readADC(pin);
    float voltage = ADS.toVoltage(raw);
    Serial.println("pin: " + String(pin) + " raw: " + String(raw) + " vol: " + String(voltage));
    voltage = voltage / (R2 / (R1 + R2));
    return voltage;
}

String createKeyForMenuItem(AnalogMenuItem menuItem) {
    Serial.println("m" + String(menuItem.getId()));
    return ("m" + String(menuItem.getId()));
}

void initDefaultPreferenceFloat(AnalogMenuItem menuItem, float defaultValue) {
    const char *keyString = createKeyForMenuItem(menuItem).c_str();
    if (!preferences.isKey(keyString)) {
        preferences.putFloat(keyString, defaultValue);
    }
    menuItem.setFromFloatingPointValue(preferences.getFloat(keyString));
}

void initMenuByPreferences() {
    initDefaultPreferenceFloat(menuThreshold, 13.7);
    initDefaultPreferenceFloat(menuTrim, 0.0);
    initDefaultPreferenceFloat(menuLowbatlevel, 12.5);
}

void setup() {
    
    
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
    

    Serial.println("Connected to WiFi");
    mqttClient.setServer(mqttServer, mqttPort);

  

    ADS.begin();
    Serial.println("ADS busy?: " + String(ADS.isBusy()) + " connected: " + String(ADS.isConnected()) + " ready: " + String(ADS.isReady()));

    setupMenu();
    renderer.turnOffResetLogic();

    preferences.begin("PereController", false);
    initMenuByPreferences();

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(RELAY_1_PIN, OUTPUT);
    pinMode(RELAY_2_PIN, OUTPUT);

    digitalWrite(RELAY_1_PIN, HIGH);
    digitalWrite(RELAY_2_PIN, HIGH);

    taskManager.scheduleFixedRate(1000, [] {
        int raw = ADS.readADC(1);
        float internalV = ADS.toVoltage(raw);
        menuInternal.setFromFloatingPointValue(internalV);

        float batteryV = readVoltage(0);
        float trim = menuTrim.getAsFloatingPointValue();
        float trimmedVoltage = batteryV + trim;
        menuBattery.setFromFloatingPointValue(trimmedVoltage);

        float thresholdV = menuThreshold.getAsFloatingPointValue();

        if (relayActive) {
            if (batteryV < lowbatlevel) {
                relayActive = false;
                digitalWrite(RELAY_1_PIN, HIGH);
                digitalWrite(RELAY_2_PIN, HIGH);
                activationStartTime = 0;
            }
        } else {
            if (trimmedVoltage > thresholdV - thresholdRange && trimmedVoltage < thresholdV + thresholdRange) {
                if (activationStartTime == 0) {
                    activationStartTime = millis();
                } else if (millis() - activationStartTime >= 60000) {
                    relayActive = true;
                    digitalWrite(RELAY_1_PIN, LOW);
                    digitalWrite(RELAY_2_PIN, LOW);
                }
            } else {
                activationStartTime = 0;
            }
        }

    }, TIME_MILLIS);
}

void loop() {
    taskManager.runLoop();
}

void updatePreferenceByMenuItemFloat(AnalogMenuItem menuItem) {
    preferences.putFloat(createKeyForMenuItem(menuItem).c_str(), menuItem.getAsFloatingPointValue());
}

void CALLBACK_FUNCTION onVoltageThresholdChange(int id) {
    updatePreferenceByMenuItemFloat(menuThreshold);
}

void CALLBACK_FUNCTION onTrimChanged(int id) {
    updatePreferenceByMenuItemFloat(menuTrim);
}


void CALLBACK_FUNCTION onlowbatlevelchanged(int id) {
    updatePreferenceByMenuItemFloat(menuLowbatlevel);
}
