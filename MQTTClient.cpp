#include "MQTTClient.h"

// Constructor with initialization list
MQTTClient::MQTTClient(const char* server, const int port) : client(espClient), mqtt_server(server), mqtt_port(port) {
  // Constructor body (if needed, it can be left empty)
}

void MQTTClient::begin() {
  client.setServer(mqtt_server, mqtt_port);
}

void MQTTClient::connect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      Serial.println("Terhubung ke broker MQTT");
    } else {
      Serial.print("Gagal terhubung ke broker MQTT, status: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void MQTTClient::subscribe(const char* topic) {
  client.subscribe(topic);
}

void MQTTClient::publish(const char* topic, const char* message) {
  client.publish(topic, message);
}

void MQTTClient::loop() {
  client.loop();
}

void MQTTClient::setCallback(MQTT_CALLBACK_SIGNATURE) {
  client.setCallback(callback);
}

bool MQTTClient::connected() {
  return client.connected();
}

