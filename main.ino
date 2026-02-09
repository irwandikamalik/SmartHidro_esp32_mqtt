#include <WiFi.h>
#include <PubSubClient.h>
#include "RelayControl.h"

// Pengaturan Wi-Fi
const char* ssid = "plerr.co";         // Ganti dengan nama Wi-Fi Anda
const char* password = "pleerr22"; // Ganti dengan password Wi-Fi Anda

// Pengaturan broker MQTT
const char* mqtt_server = "192.168.100.85"; // Ganti dengan alamat IP broker Mosquitto lokal
const int mqtt_port = 1883;               // Port broker MQTT

// Membuat objek relay
RelayControl relays[] = {RelayControl(14), RelayControl(27), RelayControl(26), RelayControl(25)};
const int numRelays = 4;

// Inisialisasi Wi-Fi dan MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Inisialisasi Serial Monitor
  Serial.begin(115200);
  
  // Koneksi ke Wi-Fi
  setup_wifi();

  // Inisialisasi client MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  // Inisialisasi semua relay menggunakan perulangan for
  for (int i = 0; i < numRelays; i++) {
    relays[i].begin();
  }

  // Memberikan informasi kepada pengguna melalui Serial Monitor
  Serial.println("Masukkan perintah untuk mengontrol relay \n(contoh: relay1 on atau relay1 off):");
  
  // Subscribe ke topik MQTT untuk menerima perintah
  client.subscribe("relay/control");
}

void loop() {
  // Menjaga koneksi ke broker MQTT dan menerima pesan
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Cek apakah ada data yang tersedia di Serial Monitor
  if (Serial.available() > 0) {
    // Membaca data dari Serial Monitor
    String command = Serial.readStringUntil('\n');  // Membaca perintah sampai newline

    // Menghapus karakter yang tidak perlu (newline, spasi, dll.)
    command.trim();

    // Memanggil fungsi untuk mengontrol relay berdasarkan perintah
    controlRelay(command);
  }
}

// Fungsi untuk menghubungkan ke Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke Wi-Fi ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Terhubung ke Wi-Fi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Fungsi untuk menangani callback MQTT
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // Memanggil fungsi untuk mengontrol relay berdasarkan pesan MQTT
  controlRelay(message);
}

// Fungsi untuk menghubungkan ke broker MQTT
void reconnect() {
  // Loop sampai terhubung ke broker
  while (!client.connected()) {
    Serial.print("Mencoba untuk terhubung ke MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Terhubung ke broker MQTT");
      client.subscribe("relay/control"); // Subscribe ke topik relay/control
    } else {
      Serial.print("Gagal terhubung, status: ");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

// Fungsi untuk mengontrol relay berdasarkan perintah
void controlRelay(String command) {
  // Menggunakan perulangan untuk mengontrol relay
  for (int i = 0; i < numRelays; i++) {
    String relayCommand = "relay" + String(i + 1);  // Membuat perintah seperti relay1, relay2, dll.

    if (command == relayCommand + " on") {
      relays[i].on();  // Nyalakan relay sesuai indeks
      Serial.println(relayCommand + " ON");
      // Kirim status ke broker MQTT
      client.publish("relay/status", (relayCommand + " ON").c_str());
    }
    else if (command == relayCommand + " off") {
      relays[i].off();  // Matikan relay sesuai indeks
      Serial.println(relayCommand + " OFF");
      // Kirim status ke broker MQTT
      client.publish("relay/status", (relayCommand + " OFF").c_str());
    }
  }

  // Jika perintah tidak dikenali
  if (command.indexOf("relay") == -1) {
    Serial.println("Perintah tidak dikenali, coba lagi.");
  }
}
