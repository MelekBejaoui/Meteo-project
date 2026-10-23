#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "credentials.h"

#define led 2
#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// MQTT
String clientID = "ESP32-";
const char *mqtt_server = "192.168.1.18";
const char *mqtt_user = "Melek";
const char *mqtt_password = "Melek2005";
WiFiClient espClient;
PubSubClient client(espClient);

void reconnect()
{
    int attempts = 0;
    while (!client.connected() && attempts < 10)
    {
        Serial.print("Attempting MQTT connection to: ");
        Serial.print(mqtt_server);
        Serial.print(":1884 with user: ");
        Serial.println(mqtt_user);

        clientID = "ESP32-";
        clientID += String(random(0xffff), HEX);
        Serial.print("Client ID: ");
        Serial.println(clientID);

        if (client.connect(clientID.c_str(), mqtt_user, mqtt_password))
        {
            Serial.println("✓ Connected to MQTT!");
            client.subscribe("fromNodeRED");
            client.subscribe("LedState");
            client.subscribe("JSONfromNodeRED");
            return;
        }
        else
        {
            int state = client.state();
            Serial.print("✗ Failed, code: ");
            Serial.println(state);

            switch (state)
            {
            case -4:
                Serial.println("MQTT_CONNECTION_TIMEOUT");
                break;
            case -3:
                Serial.println("MQTT_CONNECTION_LOST");
                break;
            case -2:
                Serial.println("MQTT_CONNECT_FAILED");
                break;
            case -1:
                Serial.println("MQTT_DISCONNECTED");
                break;
            case 0:
                Serial.println("Connected");
                break;
            case 1:
                Serial.println("Bad protocol version");
                break;
            case 2:
                Serial.println("Bad client id");
                break;
            case 3:
                Serial.println("Unavailable");
                break;
            case 4:
                Serial.println("Bad credentials");
                break;
            case 5:
                Serial.println("Unauthorized");
                break;
            }

            Serial.println("Retrying in 5 seconds...\n");
            delay(5000);
            attempts++;
        }
    }
    Serial.println("Failed to connect after multiple attempts");
}

void callback(char *topic, byte *message, unsigned int length)
{
    String messageTemp;
    for (int i = 0; i < length; i++)
    {
        messageTemp += (char)message[i];
    }

    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(" | Message: ");
    Serial.println(messageTemp);

    if (String(topic) == "fromNodeRED")
    {
        Serial.println("Received from Node-RED: " + messageTemp);
    }
    else if (String(topic) == "LedState")
    {
        if (messageTemp == "ON")
        {
            digitalWrite(led, HIGH);
            Serial.println("LED turned ON");
        }
        else if (messageTemp == "OFF")
        {
            digitalWrite(led, LOW);
            Serial.println("LED turned OFF");
        }
    }
    else if (String(topic) == "JSONfromNodeRED")
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, messageTemp);

        if (error)
        {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return;
        }

        const char *device = doc["device"];
        int threshold = doc["threshold"];

        Serial.println("=== JSON Data Received ===");
        Serial.print("Device: ");
        Serial.println(device);
        Serial.print("Threshold: ");
        Serial.println(threshold);
        Serial.println("========================\n");
    }
}

void connectAP()
{
    Serial.println("\nConnecting to WiFi...");
    Serial.print("SSID: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    byte cnt = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        cnt++;
        if (cnt > 30)
        {
            Serial.println("\nWiFi connection timeout! Restarting...");
            ESP.restart();
        }
    }

    Serial.println("\n✓ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void sendDHTvalues()
{
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature))
    {
        Serial.println("Failed to read from DHT11 sensor!");
        return;
    }

    JsonDocument doc;
    doc["device"] = "Temperature & Humidity Monitor";
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["timestamp"] = millis();

    doc.shrinkToFit();
    char buff[256];
    serializeJson(doc, buff);
    client.publish("DHT11/readings", buff);

    Serial.println("=== DHT11 Data Sent ===");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.println("=======================\n");
}