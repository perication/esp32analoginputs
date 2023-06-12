#include "generated/pere_menu.h"
#include <Preferences.h>
#include <ADS1X15.h>

Preferences preferences;
ADS1115 ADS(0x48);

const int RELAY_1_PIN = 26;
const int RELAY_2_PIN = 27;

const int BUZZER_PIN = 17;

// Floats for resistor values in divider (in ohms); to measure up to 15V
const float R1 = 15000.0;
const float R2 = 7500.0;

float readVoltage(int pin) {
    int raw = ADS.readADC(pin);
    float voltage = ADS.toVoltage(raw);
    Serial.println("pin: " + String(pin) + " raw: " + String(raw) + " vol: " + String(voltage));
    voltage = voltage / (R2/(R1 + R2));
    return voltage;
}

String createKeyForMenuItem(AnalogMenuItem menuItem) {
    Serial.println("m" + String(menuItem.getId()));
    return ("m" + String(menuItem.getId()));
}

void initDefaultPreferenceFloat(AnalogMenuItem menuItem, float defaultValue) {
    const char *keyString = createKeyForMenuItem(menuItem).c_str();
    if(!preferences.isKey(keyString)) {
        preferences.putFloat(keyString, defaultValue);
    }
    menuItem.setFromFloatingPointValue(preferences.getFloat(keyString));
}

void initMenuByPreferences() {
    initDefaultPreferenceFloat(menuThreshold, 14.2);
    initDefaultPreferenceFloat(menuTrim, 0.0);
}

void setup() {
    Serial.begin(115200);

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
        if(trimmedVoltage > thresholdV) {
            digitalWrite(RELAY_1_PIN, LOW);
            digitalWrite(RELAY_2_PIN, LOW);
        } else {
            digitalWrite(RELAY_1_PIN, HIGH);
            digitalWrite(RELAY_2_PIN, HIGH);
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
