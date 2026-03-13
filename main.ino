#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>  // Library untuk mempermudah pembuatan JSON
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "RelayControl.h"
#include "MQTTClient.h"
#include "SensorData.h"
#include "DistanceSensor.h"
#include "phSensor.h"
#include "tdsSensor.h"

// Pengaturan Wi-Fi
const char* ssid = "plerr.co";
const char* password = "pleerr22";

// Pengaturan broker MQTT
const char* mqtt_server = "192.168.100.161";
const int mqtt_port = 1883;

// Membuat objek relay
RelayControl relays[] = { RelayControl(14), RelayControl(27), RelayControl(26), RelayControl(25) };
MQTTClient mqttClient(mqtt_server, mqtt_port);
SensorData sensorData;

bool relayState[4] = { false, false, false, false };

unsigned long timerSendData = 0;
const long intervalSendData = 1000;  // Interval pengambilan dan pengiriman data

//Membuat objek LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);
unsigned long timerLcd = 0;
const long intervalLcd = 1000;

// Membuat objek sensor jarak (level Air)
DistanceSensor distanceSensor(5, 18);  // (trigPin, echoPin)
int percentage;

// Membuat objek sensor temperature
#define ONE_WIRE_BUS 32
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float tempC;

// Membuat objek sensor PH
#define PH_PIN 34
phSensor myPH(PH_PIN);
float phValue;
float phThreshold = 6;
float tdsThreshold = 600;

// Membuat objek sensor TDS
TDSSensor tdsSensor(35);  // Pin TDS, jumlah sampel, kalibrasi EC, koefisien temperatur
float tdsValue;

//Membuat objek untuk NTP
WiFiUDP udp;                                                   // Membuat objek UDP
NTPClient timeClient(udp, "pool.ntp.org", 7 * 3600, 3600000);  // Inisialisasi client NTP dengan zona waktu lokal (misalnya GMT+7 = 7 jam) Zona waktu Indonesia
unsigned long timerTask = 0;
const long intervalTask = 60000;
bool hasRun = false;

//Konfigurasi Automatis Dosing
bool autoMode = true;

// State untuk Auto Dosing
enum DosingState { IDLE, CHECK_PH, PH_UP, PH_DOWN, CHECK_TDS, TDS_ADD, MIXING, FINISHED };
DosingState dosingState = IDLE;

int dosingCycle = 0;
const int maxDosingCycle = 5;
bool dosingPerformed = false;
unsigned long dosingTimer = 0;
const unsigned long dosingDelay = 2000; // 2 detik untuk tiap relay
const unsigned long mixingDelay = 60000; // 60 detik mixing

void setup() {
  Serial.begin(115200);

  // Koneksi ke Wi-Fi
  connectToWiFi();

  // Koneksi MQTT
  connectToMQTT();

  // Inisialisasi relay dan sensor
  initializeDevices();

  // pH Sensor
  myPH.begin();
  myPH.setCalibration(-5.70, 25.5);

  //tds sensor
  tdsSensor.begin();

  // Kirim data target ke topik /control/target
  sendTargetData();

  // Inisialisasi NTP
  timeClient.begin();
  timeClient.update();  // Mengupdate waktu pertama kali
}

void loop() {
  if (!mqttClient.connected()) {
    Serial.println("MQTT reconnecting...");
    connectToMQTT();
    subscribeToRelayTopics();
  }

  mqttClient.loop();
  unsigned long now = millis();

  // Update waktu dari server NTP setiap loop
  timeClient.update();

  // Ambil jam dan menit dari waktu NTP
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  // int currentSecond = timeClient.getSeconds();

  // Serial.print("Hour: ");
  // Serial.print(currentHour);
  // Serial.print(", Minute: ");
  // Serial.println(currentMinute);

  if (autoMode && ((currentHour == 7 && currentMinute == 0) || (currentHour == 16 && currentMinute == 0))) {
      UpdateDosing();
  }
  
  // Mengambil data sensor setiap interval waktu
  if (now - timerSendData >= intervalSendData) {
    timerSendData = now;

    // Mengambil data dari semua sensor
    tempC = getTemperature();
    phValue = getPhValue();
    tdsValue = getTdsValue();
    percentage = getWaterLevel();

    // Mengirimkan data sensor melalui MQTT
    sendSensorData(phValue, tempC, tdsValue, percentage);
  }

  if (now - timerLcd >= intervalLcd) {
    timerLcd = now;
    updateLCD();
  }
}

void UpdateDosing() {
  unsigned long now = millis();

  switch (dosingState) {
    case IDLE:
      dosingCycle = 0;
      dosingState = CHECK_PH;
      dosingPerformed = false;
      Serial.println("Start Dosing");
      break;

    case CHECK_PH:
      tempC = getTemperature();
      phValue = getPhValue();
      tdsValue = getTdsValue();
      Serial.print("Cycle: "); Serial.println(dosingCycle + 1);
      Serial.print("PH: "); Serial.println(phValue);
      Serial.print("TDS: "); Serial.println(tdsValue);

      dosingPerformed = false;

      if (phValue < phThreshold) {
        dosingState = PH_UP;
        dosingTimer = now;
        relays[0].on(); relayState[0] = true;
        Serial.println("PH UP ON");
      } else if (phValue > phThreshold) {
        dosingState = PH_DOWN;
        dosingTimer = now;
        relays[1].on(); relayState[1] = true;
        Serial.println("PH DOWN ON");
      } else {
        dosingState = CHECK_TDS;
      }
      break;

    case PH_UP:
      if (now - dosingTimer >= dosingDelay) {
        relays[0].off(); relayState[0] = false;
        Serial.println("PH UP OFF");
        dosingPerformed = true;
        dosingState = CHECK_TDS;
      }
      break;

    case PH_DOWN:
      if (now - dosingTimer >= dosingDelay) {
        relays[1].off(); relayState[1] = false;
        Serial.println("PH DOWN OFF");
        dosingPerformed = true;
        dosingState = CHECK_TDS;
      }
      break;

    case CHECK_TDS:
      if (tdsValue < tdsThreshold) {
        dosingState = TDS_ADD;
        dosingTimer = now;
        relays[2].on(); relays[3].on();
        relayState[2] = true; relayState[3] = true;
        Serial.println("TDS ADD ON");
      } else {
        dosingState = MIXING;
        dosingTimer = now;
      }
      break;

    case TDS_ADD:
      if (now - dosingTimer >= dosingDelay) {
        relays[2].off(); relays[3].off();
        relayState[2] = false; relayState[3] = false;
        Serial.println("TDS ADD OFF");
        dosingPerformed = true;
        dosingState = MIXING;
        dosingTimer = now;
      }
      break;

    case MIXING:
      if (!dosingPerformed) {
        Serial.println("Target Reached, skip mixing");
        dosingState = FINISHED;
      } else if (now - dosingTimer >= mixingDelay) {
        dosingCycle++;
        if (dosingCycle < maxDosingCycle) {
          dosingState = CHECK_PH;
        } else {
          dosingState = FINISHED;
        }
      }
      break;

    case FINISHED:
      Serial.println("Dosing Finished");
      dosingState = IDLE; // Reset untuk siklus berikutnya
      break;
  }
}

// Koneksi WiFi
void connectToWiFi() {
  Serial.println("Menghubungkan WiFi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Terhubung ke Wi-Fi");
}

// Koneksi MQTT
void connectToMQTT() {
  mqttClient.begin();
  mqttClient.connect();
  mqttClient.setCallback(mqtt_callback);
  subscribeToRelayTopics();
}

// Subscribe ke topik relay MQTT
void subscribeToRelayTopics() {
  for (int i = 1; i <= 4; i++) {
    String topic = "/control/pump" + String(i);
    mqttClient.subscribe(topic.c_str());
  }
  mqttClient.subscribe("/control/auto");
  mqttClient.subscribe("/control/reboot");
  mqttClient.subscribe("/control/threshold");
}

//publish Auto State
void publishAutoStatus() {

  if (autoMode) {
    mqttClient.publish("/state/auto", "true");
  } else {
    mqttClient.publish("/state/auto", "false");
  }
}

// Inisialisasi perangkat
void initializeDevices() {
  for (int i = 0; i < 4; i++) {
    relays[i].begin();
  }

  sensors.begin();
  distanceSensor.begin();
  Wire.begin();
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.clear();
}

// Fungsi untuk mengambil nilai pH
float getPhValue() {
  myPH.update();
  myPH.setTemperature(tempC);
  return roundf(myPH.getPH() * 100.0) / 100.0;
}

// Fungsi untuk mendapatkan suhu
float getTemperature() {
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  return roundf(tempC * 10.0) / 10.0;
}

// Fungsi untuk menghitung nilai TDS
float getTdsValue() {
  tdsSensor.setTemperature(tempC);  // kirim suhu dari DS18B20
  tdsSensor.update();               // update pembacaan

  return roundf(tdsSensor.getTDS() * 10.0) / 10.0;
}

// Fungsi untuk mendapatkan level air dari sensor jarak
int getWaterLevel() {
  long distance = distanceSensor.measureDistance();
  return distanceSensor.calculatePercentage(distance);
}

// Fungsi untuk mengirimkan data sensor ke MQTT
void sendSensorData(float phValue, float tempC, float tdsValue, int percentage) {
  sensorData.setRelayStatus(relayState);
  sensorData.setPh(phValue);
  sensorData.setTds(tdsValue);
  sensorData.setTemperature(tempC);
  sensorData.setWaterLevel(percentage);

  sensorData.sendData(mqttClient, "/sensor/data");
  publishAutoStatus();
}

// Fungsi untuk mengirimkan data target ke topik /control/target
void sendTargetData() {
  // Target Set Point pada saat inisiasi awal.
  StaticJsonDocument<200> doc;
  doc["tds"] = 600;  // Target TDS
  doc["ph"] = 6.0;   // Target pH

  // Serializing JSON to string
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // Mengirimkan data ke MQTT
  mqttClient.publish("/control/threshold", jsonBuffer);
}

// Callback MQTT untuk menerima perintah
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Serial.print("Pesan diterima dari topik: ");
  // Serial.println(topic);
  // Serial.print("Isi pesan: ");
  // Serial.println(message);

  //Auto Mode
  if (String(topic) == "/control/auto") {
    if (message == "true") {
      autoMode = true;
    } else {
      autoMode = false;
    }

    publishAutoStatus();

    Serial.print("Auto Mode: ");
    Serial.println(autoMode ? "ON" : "OFF");

    return;
  }

  //Reboot Devices
  if (String(topic) == "/control/reboot") {

    if (message == "true") {

      Serial.println("Reboot command received!");

      delay(1000);  // beri waktu serial print
      ESP.restart();
    }

    return;
  }

  //threshold Data
  if (String(topic) == "/control/threshold") {

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {

      if (doc.containsKey("ph")) {
        phThreshold = doc["ph"];
      }

      if (doc.containsKey("tds")) {
        tdsThreshold = doc["tds"];
      }

      // Serial.println("Threshold Updated");
      // Serial.print("PH Threshold: ");
      // Serial.println(phThreshold);

      // Serial.print("TDS Threshold: ");
      // Serial.println(tdsThreshold);
    } else {
      Serial.println("JSON parse failed");
    }

    return;
  }

  // Mengontrol relay sesuai perintah yang diterima
  controlRelay(topic, message == "true");
}

// Fungsi untuk mengontrol relay berdasarkan perintah MQTT
void controlRelay(char* topic, bool state) {
  int relayIndex = topic[strlen(topic) - 1] - '1';  // Mengambil angka terakhir dari topik
  if (relayIndex >= 0 && relayIndex < 4) {
    if (state) {
      relays[relayIndex].on();
      relayState[relayIndex] = true;
    } else {
      relays[relayIndex].off();
      relayState[relayIndex] = false;
    }
  }
}

// Fungsi untuk mengupdate status di LCD
void updateLCD() {
  lcd.setCursor(0, 0);  // Set kursor ke baris 1
  lcd.print("PH+:" + String(relayState[0] ? "ON " : "OFF"));
  lcd.setCursor(0, 1);  // Set kursor ke baris 2
  lcd.print("PH-:" + String(relayState[1] ? "ON " : "OFF"));
  lcd.setCursor(8, 0);  // Set kursor ke baris 3
  lcd.print("AB+:" + String(relayState[2] ? "ON " : "OFF"));
  lcd.setCursor(8, 1);  // Set kursor ke baris 4
  lcd.print("AB-:" + String(relayState[3] ? "ON " : "OFF"));

  lcd.setCursor(0, 2);
  lcd.print("Lvl:");
  lcd.print(percentage);
  lcd.print("%");
  lcd.print(" Tmp:");
  lcd.print(tempC, 1);
  lcd.print((char)223);
  lcd.print("C   ");

  lcd.setCursor(0, 3);
  lcd.print("ph:");
  lcd.print(phValue, 2);

  lcd.print(" tds:");
  lcd.print(tdsValue, 2);
}
