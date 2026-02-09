#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "RelayControl.h"

// Pengaturan Wi-Fi
const char* ssid = "plerr.co";         // Ganti dengan nama Wi-Fi Anda
const char* password = "pleerr22";     // Ganti dengan password Wi-Fi Anda

// Pengaturan broker MQTT
const char* mqtt_server = "192.168.100.85"; // Ganti dengan alamat IP broker Mosquitto lokal
const int mqtt_port = 1883;               // Port broker MQTT

// Membuat objek relay
RelayControl relays[] = {RelayControl(14), RelayControl(27), RelayControl(26), RelayControl(25)};
const int numRelays = 4;

// Inisialisasi Wi-Fi dan MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Array untuk menyimpan status relay (ON/OFF)
bool relayState[numRelays] = {false, false, false, false};

unsigned long previousMillisSendData = 0;
const long intervalSendData = 1000;
unsigned long currentMillis = 0;

// Fungsi untuk menghubungkan ke Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke Wi-Fi ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Tunggu sampai ESP32 terhubung ke Wi-Fi
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 10000) {  // Timeout setelah 10 detik
      Serial.println("Gagal terhubung ke Wi-Fi");
      return;
    }
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

  // Menampilkan topik dan pesan yang diterima
  Serial.print("Pesan diterima dari topik: ");
  Serial.println(topic);
  Serial.print("Isi pesan: ");
  Serial.println(message);
  
  // Mengontrol relay sesuai perintah yang diterima
  controlRelay(topic, message == "true");  // Nyalakan atau matikan relay berdasarkan pesan
}

// Fungsi untuk menghubungkan ke broker MQTT
void reconnect() {
  // Loop sampai terhubung ke broker
  while (!client.connected()) {
    Serial.print("Mencoba untuk terhubung ke MQTT...\n");
    if (client.connect("ESP32Client")) {
      Serial.println("Terhubung ke broker MQTT");
      
      // Subscribe ke topik /control/pump1 hingga /control/pump4 untuk menerima perintah
      for (int i = 1; i <= 4; i++) {
        String topic = "/control/pump" + String(i);
        client.subscribe(topic.c_str());
        Serial.print("Berlangganan ke topik: ");
        Serial.println(topic);
      }
    } else {
      Serial.print("Gagal terhubung, status: ");
      Serial.print(client.state());
      delay(5000);  // Retry delay
    }
  }
}

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
  Serial.println("Masukkan perintah untuk mengontrol relay \n(contoh: relay1 true atau relay1 false):");
  
  // Subscribe ke topik status relay untuk menerima pembaruan status
  for (int i = 0; i < numRelays; i++) {
    String topic = "/control/pump" + String(i + 1);  // Topik seperti /control/pump1, /control/pump2, dll.
    client.subscribe(topic.c_str());  // Subscribe ke topik perintah relay
  }
}

void loop() {
  currentMillis = millis();  // Ambil waktu sekarang

  // Menjaga koneksi ke broker MQTT dan menerima pesan
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // Menjaga koneksi dengan broker MQTT

  // Cek apakah ada data yang tersedia di Serial Monitor
  if (Serial.available() > 0) {
    // Membaca data dari Serial Monitor
    String command = Serial.readStringUntil('\n');  // Membaca perintah sampai newline

    // Menghapus karakter yang tidak perlu (newline, spasi, dll.)
    command.trim();

    // Memanggil fungsi untuk mengontrol relay berdasarkan perintah
    controlRelay(command);
  }

  // Mengirimkan pembacaan sensor dan status relay dalam format JSON
  sendSensorData();
  
  // Tambahkan sedikit delay untuk memberikan waktu bagi proses lain dan menghindari reset watchdog
  // delay(1000); // Interval pengiriman data sensor (misalnya 1 detik)
  yield();   // Memberikan kesempatan untuk menjalankan tugas lainnya
}

// Fungsi untuk mengontrol relay berdasarkan perintah
void controlRelay(String command) {
  // Menggunakan perulangan untuk mengontrol relay
  for (int i = 0; i < numRelays; i++) {
    String relayCommand = "relay" + String(i + 1);  // Membuat perintah seperti relay1, relay2, dll.
    String topic = "/value/pump" + String(i + 1);  // Topik yang sesuai: /value/pump1, /value/pump2, dll.

    if (command == relayCommand + " true" && !relayState[i]) {
      relays[i].on();  // Nyalakan relay sesuai indeks
      relayState[i] = true;  // Update status relay
      Serial.println(relayCommand + " ON");
    }
    else if (command == relayCommand + " false" && relayState[i]) {
      relays[i].off();  // Matikan relay sesuai indeks
      relayState[i] = false;  // Update status relay
      Serial.println(relayCommand + " OFF");
    }
  }
  // Jika perintah tidak dikenali
  if (command.indexOf("relay") == -1) {
    Serial.println("Perintah tidak dikenali, coba lagi.");
  }
}

// Fungsi untuk mengontrol relay berdasarkan topik dan nilai boolean
void controlRelay(char* topic, bool state) {
  // Menampilkan topik yang diterima (untuk debugging)
  Serial.print("Menerima perintah dari topik: ");
  Serial.println(topic);

  // Mendapatkan nomor relay dari topik dengan cara yang lebih tepat
  String topicStr = String(topic);  // Mengubah char* menjadi String untuk pemrosesan lebih mudah
  int relayIndex = -1;

  // Mencari angka setelah "/value/pump"
  int pos = topicStr.indexOf("pump");
  if (pos != -1) {
    // Mengambil angka setelah "pump" yang ada di dalam topik
    relayIndex = topicStr.substring(pos + 4).toInt() - 1;  // Ambil angka setelah "pump" dan dikurangi 1
  }

  // Periksa apakah relayIndex valid (pastikan tidak keluar dari batas array)
  if (relayIndex >= 0 && relayIndex < numRelays) {
    // Mengontrol relay sesuai state yang diterima
    if (state && !relayState[relayIndex]) {
      relays[relayIndex].on();  // Nyalakan relay
      relayState[relayIndex] = true;  // Update status relay
      Serial.print("Relay ");
      Serial.print(relayIndex + 1);
      Serial.println(" ON");
    } else if (!state && relayState[relayIndex]) {
      relays[relayIndex].off();  // Matikan relay
      relayState[relayIndex] = false;  // Update status relay
      Serial.print("Relay ");
      Serial.print(relayIndex + 1);
      Serial.println(" OFF");
    }
  } else {
    // Menangani kasus jika relayIndex tidak valid
    Serial.print("Error: Relay index ");
    Serial.print(relayIndex);
    Serial.println(" tidak valid!");
  }
}

// Fungsi untuk mengirimkan data sensor dan status relay dalam format JSON
void sendSensorData() {
  if (currentMillis - previousMillisSendData >= intervalSendData) {
    // Membuat objek JSON untuk mengemas data
    StaticJsonDocument<200> doc;

    // Menyimpan data sensor ke dalam JSON
    doc["tds"] = 350.0;         // Contoh data sensor TDS
    doc["ph"] = 7.2;            // Contoh data sensor pH
    doc["temperature"] = 28.5;  // Contoh data sensor suhu
    doc["waterLevel"] = 50;     // Contoh data sensor level air

    // Menyimpan status relay ke dalam JSON
    JsonObject relayStatus = doc.createNestedObject("relayStatus");
    for (int i = 0; i < numRelays; i++) {
      relayStatus[String("relay" + String(i + 1))] = relayState[i] ? "true" : "false";
    }

    // Serialisasi JSON dan publish ke topik
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    const char* topic = "/sensor/data"; // Topik untuk mengirimkan data sensor
    client.publish(topic, jsonBuffer);  // Mengirim data sensor dalam format JSON

    // Menampilkan data JSON di Serial Monitor untuk debugging
    Serial.print("Data sensor: ");
    Serial.println(jsonBuffer);
   previousMillisSendData = currentMillis;
  }
}
