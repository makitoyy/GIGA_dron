#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

// --- KONFIGURACJA ESC ---
Servo esc;
const int escPin = 5; // Pin, do którego podłączony jest ESC (GPIO 5)
const int minPulse = 1000;
const int maxPulse = 2000;

// --- KONFIGURACJA ESP-NOW ---
typedef struct struct_message {
    int speed_us; // Prędkość w mikrosekundach (1000 - 2000)
} struct_message;

struct_message myData;

// Callback – wywoływana, gdy nadejdą dane
void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));

    int receivedSpeed = myData.speed_us;

    // Zabezpieczenie zakresu
    if (receivedSpeed < minPulse) receivedSpeed = minPulse;
    if (receivedSpeed > maxPulse) receivedSpeed = maxPulse;

    // Wysłanie sygnału do silnika
    esc.writeMicroseconds(receivedSpeed);
}

void setup() {
    // --- 1. KONFIGURACJA ESC ---
    esc.setPeriodHertz(50); 
    esc.attach(escPin, minPulse, maxPulse);

    // Uzbrajanie ESC
    esc.writeMicroseconds(minPulse);
    delay(5000);

    // --- 2. KONFIGURACJA ESP-NOW ---
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        return; 
    }
    
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    // Sterowanie odbywa się w OnDataRecv
}
