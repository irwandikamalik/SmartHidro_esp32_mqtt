#ifndef MQTTClient_h
#define MQTTClient_h

#include <WiFi.h>
#include <PubSubClient.h>

class MQTTClient {
  private:
    WiFiClient espClient;
    PubSubClient client;
    const char* mqtt_server;
    const int mqtt_port;  // This needs to be initialized

  public:
    MQTTClient(const char* server, const int port);
    void begin();
    void connect();
    void subscribe(const char* topic);
    void publish(const char* topic, const char* message);
    void loop();
    void setCallback(MQTT_CALLBACK_SIGNATURE);
};

#endif
