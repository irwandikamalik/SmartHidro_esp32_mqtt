#include "SensorData.h"

SensorData::SensorData() {
  tds = 0.0;
  ph = 0.0;
  temperature = 0.0;
  waterLevel = 0.0;
  for (int i = 0; i < 4; i++) relayStatus[i] = false;
}

void SensorData::setTds(float value) {
  tds = value;
}

void SensorData::setPh(float value) {
  ph = value;
}

void SensorData::setTemperature(float value) {
  temperature = value;
}

void SensorData::setWaterLevel(float value) {
  waterLevel = value;
}

void SensorData::setRelayStatus(bool *status) {
  for (int i = 0; i < 4; i++) {
    relayStatus[i] = status[i];  // Menyimpan status relay dari main.ino
  }
}

void SensorData::sendData(MQTTClient &client, const char* topic) {
  StaticJsonDocument<512> doc;  // Membuat dokumen JSON dengan ukuran yang lebih besar

  // Menyimpan data sensor ke dalam JSON
  doc["tds"] = tds;
  doc["ph"] = ph;
  doc["temperature"] = temperature;
  doc["waterLevel"] = waterLevel;

  // Membuat objek nested "relayStatus" untuk menyimpan status relay
  JsonObject relayStatusObject = doc.createNestedObject("relayStatus");
  for (int i = 0; i < sizeof(relayStatus); i++) {
    relayStatusObject[String("relay" + String(i + 1))] = relayStatus[i] ? "true" : "false";  // Menyimpan status relay (ON/OFF)
  }

  // Serialisasi JSON menjadi buffer karakter
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // Publish JSON data ke MQTT broker pada topik yang sesuai
  client.publish(topic, jsonBuffer);
  // Serial.print("Data sensor dan relayStatus: ");
  // Serial.println(jsonBuffer);
}
