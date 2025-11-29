#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h> 

// --- KONFIGURACJA ESC ---
Servo esc;
const int escPin = 5; // Pin, do którego podłączony jest ESC (GPIO 5)
const int minPulse = 1000;
const int maxPulse = 2000;

// --- KONFIGURACJA ESP-NOW ---
// Definicja struktury danych (MUSI być identyczna jak w nadajniku!)
typedef struct struct_message {
    int speed_us; // Prędkość w mikrosekundach (1000 - 2000)
} struct_message;

struct_message myData;

// Funkcja callback - wywoływana, gdy nadejdą dane
void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingData, int len) {
    // Skopiuj otrzymane dane do naszej struktury
    memcpy(&myData, incomingData, sizeof(myData));

    int receivedSpeed = myData.speed_us;
    
    // Zabezpieczenie zakresu 
    if (receivedSpeed < minPulse) {
      receivedSpeed = minPulse;
    } else if (receivedSpeed > maxPulse) {
      receivedSpeed = maxPulse;
    }

    // Wysłanie sygnału do silnika
    esc.writeMicroseconds(receivedSpeed);
}
  
void setup() {
    // --- 1. KONFIGURACJA ESC ---
    // Ustawienie częstotliwości PWM na 50 Hz (standard ESC)
    esc.setPeriodHertz(50); 
    esc.attach(escPin, minPulse, maxPulse);

    // UZBRAJANIE ESC
    esc.writeMicroseconds(minPulse);
    delay(5000); // 5 sekund na uzbrojenie
    

    // --- 2. KONFIGURACJA ESP-NOW ---
    // Ustawienie trybu stacji dla ESP-NOW
    WiFi.mode(WIFI_STA);

    // Inicjalizacja ESP-NOW
    if (esp_now_init() != ESP_OK) {
        // W przypadku błędu inicjalizacji, opuszczamy funkcję setup
        return; 
    }
    
    // Rejestracja funkcji, która obsłuży odebrane dane
    esp_now_register_recv_cb(OnDataRecv);
}
  
void loop() {
    // Całe sterowanie odbywa się w tle, w przerwaniu OnDataRecv.
}