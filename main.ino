#include "RelayControl.h"

RelayControl relays[] = {RelayControl(14),
                         RelayControl(27),
                         RelayControl(26),
                         RelayControl(25)};
const int numRelays = 4;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  for (int i = 0; i < numRelays; i++) {
    relays[i].begin();
  }

  Serial.println("Masukkan perintah untuk mengontrol relay \n(contoh: relay1 on atau relay1 off):");
}


void loop() {
  if (Serial.available() > 0) {
      // Membaca data dari Serial Monitor
      String command = Serial.readStringUntil('\n');  // Membaca perintah sampai newline

      // Menghapus karakter yang tidak perlu (newline, spasi, dll.)
      command.trim();

      // Memanggil fungsi untuk mengontrol relay berdasarkan perintah
      controlRelay(command);
    }
}

void controlRelay(String command) {
  // Menggunakan perulangan untuk mengontrol relay
  for (int i = 0; i < numRelays; i++) {
    String relayCommand = "relay" + String(i + 1);  // Membuat perintah seperti relay1, relay2, dll.

    if (command == relayCommand + " on") {
      relays[i].on();  // Nyalakan relay sesuai indeks
      Serial.println(relayCommand + " ON");
    }
    else if (command == relayCommand + " off") {
      relays[i].off();  // Matikan relay sesuai indeks
      Serial.println(relayCommand + " OFF");
    }
  }

  // Jika perintah tidak dikenali
  if (command.indexOf("relay") == -1) {
    Serial.println("Perintah tidak dikenali, coba lagi.");
  }
}
