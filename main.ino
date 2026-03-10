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
RelayControl relays[] = {RelayControl(14), RelayControl(27), RelayControl(26), RelayControl(25)};
MQTTClient mqttClient(mqtt_server, mqtt_port);
SensorData sensorData;

bool relayState[4] = {false, false, false, false};

unsigned long previousMillisSendData = 0;
const long intervalSendData = 1000;  // Interval pengambilan dan pengiriman data

//Membuat objek LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4); 

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
float phThreshold;
float tdsThreshold;

// Membuat objek sensor TDS 
TDSSensor tdsSensor(35);  // Pin TDS, jumlah sampel, kalibrasi EC, koefisien temperatur
float tdsValue;

//Membuat objek untuk NTP
WiFiUDP udp;                      // Membuat objek UDP
NTPClient timeClient(udp, "pool.ntp.org", 7 * 3600, 3600000);  // Inisialisasi client NTP dengan zona waktu lokal (misalnya GMT+7 = 7 jam) Zona waktu Indonesia
unsigned long previousMillisTask = 0;
const long intervalTask = 60000;
bool hasRun = false;

void setup() {
  Serial.begin(115200);

  // Koneksi ke Wi-Fi
  connectToWiFi();
  
  // Koneksi MQTT
  connectToMQTT();

  // Subscribe ke topik perintah relay
  subscribeToRelayTopics();

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
  mqttClient.loop();
  unsigned long currentMillis = millis();

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

  //INTERVAL 60 Detik
  if (currentMillis - previousMillisTask >= intervalTask) {
    previousMillisTask += intervalTask;
  // Cek apakah jam adalah 6 pagi atau 4 sore dan lakukan pengecekan
    if (((currentHour == 7 && currentMinute == 0) || (currentHour == 16 && currentMinute == 0)) && !hasRun) {
      // Panggil fungsi pengecekan sesuai kebutuhan Anda
      scheduleTask();
      hasRun = true;
    }

    if (currentMinute != 0) {
      hasRun = false;
    }
    // Serial.print("Hour: ");
    // Serial.print(currentHour);
    // Serial.print(", Minute: ");
    // Serial.println(currentMinute);
  }

  // Mengambil data sensor setiap interval waktu
  if (currentMillis - previousMillisSendData >= intervalSendData) {
    previousMillisSendData += intervalSendData;

    // Mengambil data dari semua sensor
    phValue = getPhValue();
    tempC = getTemperature();
    tdsValue = getTdsValue();
    percentage = getWaterLevel();

    // Mengirimkan data sensor melalui MQTT
    sendSensorData(phValue, tempC, tdsValue, percentage);
  }

  updateLCD();
}

// Program Auto secara berkala pada waktu tertentu
void scheduleTask() {

  Serial.println("Schedule Start");

  //pengecekan PH
  if (phValue < phThreshold) {
    //menambahkan ph dengan menjalankan ph up
  
  }else if (phValue > phThreshold) {
    //mengurangi ph dengan menjalankan ph down

  }

  //Pengecekan TDS
  if (tdsValue < tdsThreshold) {
    //mennambahkan nutrisi dengan menjalankan ab mix

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
}

// Subscribe ke topik relay MQTT
void subscribeToRelayTopics() {
  for (int i = 1; i <= 4; i++) {
    String topic = "/control/pump" + String(i);
    mqttClient.subscribe(topic.c_str());
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
  lcd.setCursor(0, 0);  
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
}

// Fungsi untuk mengirimkan data target ke topik /control/target
void sendTargetData() {
  // Target Set Point pada saat inisiasi awal.
  StaticJsonDocument<200> doc;
  doc["targettds"] = 4.5;  // Target TDS
  doc["targetph"] = 4.0;   // Target pH
  doc["targettemperature"] = 3.7;  // Target suhu
  doc["targetwaterLevel"] = 6.9;   // Target level air

  // Serializing JSON to string
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // Mengirimkan data ke MQTT
  mqttClient.publish("/control/target", jsonBuffer);
}

// Callback MQTT untuk menerima perintah
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Pesan diterima dari topik: ");
  Serial.println(topic);
  Serial.print("Isi pesan: ");
  Serial.println(message);

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

