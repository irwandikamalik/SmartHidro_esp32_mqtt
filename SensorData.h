#ifndef SensorData_h
#define SensorData_h

#include <ArduinoJson.h>
#include <PubSubClient.h>  // Pastikan Anda menambahkan ini
#include "MQTTClient.h"     // Menambahkan MQTTClient

class SensorData {
  private:
    float tds, ph, temperature, waterLevel;
    bool relayStatus[4];  // Array untuk menyimpan status relay
  
  public:
    SensorData();
    void setTds(float value);
    void setPh(float value);
    void setTemperature(float value);
    void setWaterLevel(float value);
    void setRelayStatus(bool *status);  // Fungsi untuk mengatur relay status
    void sendData(MQTTClient &client, const char* topic);  // Ubah ini untuk menerima MQTTClient
};

#endif
