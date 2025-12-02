#include <ESP8266WiFi.h>
#include <espnow.h>

// Adres MAC odbiornika (ESP32)
uint8_t receiverMac[] = {0x3c, 0x8a, 0x1f, 0xb0, 0x14, 0xd0};

// Struktura danych – identyczna jak w odbiorniku
typedef struct struct_message {
    int speed_us; // wartość PWM w mikrosekundach (1000–2000)
} struct_message;

struct_message myData;

String messageFromPC = "";
bool newDataFromPC = false;

// Callback po wysłaniu danych
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Status wysyłki: ");
  if (sendStatus == 0) {
    Serial.println("Dostarczono pomyślnie");
  } else {
    Serial.println("Błąd dostarczenia");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("Błąd inicjalizacji ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 0, NULL, 0);

  Serial.println("Nadajnik (ESP8266) gotowy.");
  Serial.println("Podaj wartość 1000–2000 i naciśnij Enter.");
}

void loop() {
  if (Serial.available() > 0) {
    messageFromPC = Serial.readStringUntil('\n');
    messageFromPC.trim();

    if (messageFromPC.length() > 0) {
      int value = messageFromPC.toInt(); // konwersja tekstu na liczbę

      // Zabezpieczenie zakresu
      if (value < 1000) value = 1000;
      if (value > 2000) value = 2000;

      myData.speed_us = value;

      Serial.print("Wysyłam przez ESP-NOW wartość: ");
      Serial.println(myData.speed_us);

      esp_now_send(receiverMac, (uint8_t *) &myData, sizeof(myData));
    }
  }
}
