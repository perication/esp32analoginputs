#include <Preferences.h>
#include <ADS1X15.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "TP-Link_46E4";
const char* password = "77793003";
const char* mqttServer = "test.mosquitto.org";
const char* mqtt_username = "perication";
const char* mqtt_password = "public";
const int mqttPort = 1883;
const char* mqttTopic = "voltaje";
int retries;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

ADS1115 ADS(0x48);

const float R1 = 15000.0;
const float R2 = 7500.0;

float readVoltage(int pin) {
    int raw = ADS.readADC(pin);
    float voltage = ADS.toVoltage(raw);
    Serial.println("pin: " + String(pin) + " raw: " + String(raw) + " vol: " + String(voltage));
    
    // Ajuste del voltaje con las resistencias
    float adjustedVoltage = voltage / (R2 / (R1 + R2));
    
    return adjustedVoltage;
}


void setupWiFi() {
    int retries = 0;
    const int maxRetries = 30;

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        if (retries < maxRetries) {
            delay(1000);
            Serial.println("Conectando a WiFi...");
            retries++;
            Serial.println("Reintentando: " + String(retries));
        } else {
            Serial.println("Error al conectar a WiFi después de " + String(maxRetries) + " intentos.");
            break;
        }
    }
        if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Conexión WiFi establecida");
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("No se pudo establecer la conexión WiFi. Reiniciando Arduino.");
        ESP.restart();
    }
}

void setupMQTT() {
    mqttClient.setServer(mqttServer, mqttPort);

}

void reconnectMQTT() {
   while (!mqttClient.connected()) {
    Serial.println("Conectando al servidor MQTT...");
    if (mqttClient.connect("denky32")) { // Nombre único del cliente MQTT
      Serial.println("Conexión MQTT establecida");
    } else {
      Serial.print("Error al conectar al servidor MQTT. Estado: ");
      Serial.println(mqttClient.state());
      delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    ADS.begin();
    Serial.println("ADS busy?: " + String(ADS.isBusy()) + " connected: " + String(ADS.isConnected()) + " ready: " + String(ADS.isReady()));

    setupWiFi();
    setupMQTT();
}

void loop() {
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();

    float voltage = readVoltage(0);
    Serial.println("Voltage: " + String(voltage));


    // Envío del voltaje a través de MQTT
    char payload[10];
    sprintf(payload, "%.2f", voltage);
    mqttClient.publish(mqttTopic, payload);
    Serial.print("Payload: ");
    Serial.println(payload);

    delay(5000);
}
