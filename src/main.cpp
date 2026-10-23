#include <Arduino.h>
#include <ArduinoJson.h>
#include "Wifimqtt.h"

#define led 2
unsigned long previousMillis = 0;
unsigned long dhtReadInterval = 5000; // Read DHT11 every 5 seconds

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n=== ESP32 Temperature & Humidity Monitor (DHT11) Starting ===");

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  // Initialize DHT11 sensor
  dht.begin();
  Serial.println("DHT11 sensor initialized!");

  Serial.println("Initializing WiFi and MQTT...");
  connectAP();

  client.setServer(mqtt_server, 1884);
  client.setCallback(callback);

  Serial.println("Setup complete!\n");
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectAP();
  }

  if (!client.connected())
  {
    Serial.println("MQTT disconnected! Reconnecting...");
    reconnect();
  }

  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= dhtReadInterval)
  {
    previousMillis = currentMillis;

    if (client.connected())
    {
      sendDHTvalues();
    }
    else
    {
      Serial.println("Not connected to MQTT - cannot publish");
    }
  }
}