#include <ESP32Servo.h>

Servo esc;
const int escPin = 5; // Twój pin

int currentSpeed = 1000; // Aktualna prędkość (startujemy od 0%)
const int step = 2;     // O ile zmieniać prędkość przy jednym kliknięciu (skok)

void setup() {
  Serial.begin(115200);
  
  // Konfiguracja ESC
  esc.setPeriodHertz(50); 
  esc.attach(escPin, 1000, 2000);

  // UZBRAJANIE
  Serial.println("Uzbrajanie... (3 sekundy)");
  esc.writeMicroseconds(1000);
  delay(3000);
  
  Serial.println("GOTOWE!");
  Serial.println("Klikaj 'w' aby przyspieszyc.");
  Serial.println("Klikaj 's' aby zwolnic.");
  Serial.println("Klikaj 'x' dla awaryjnego STOP.");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read(); // Czytamy jeden znak

    // Sterowanie
    if (command == 'w' || command == 'W') {
      currentSpeed = currentSpeed + step;
    } 
    else if (command == 's' || command == 'S') {
      currentSpeed = currentSpeed - step;
    }
    else if (command == 'x' || command == 'X') {
      currentSpeed = 1000; // Awaryjny stop
      Serial.println("!!! STOP !!!");
    }

    // Zabezpieczenie zakresu (żeby nie wyjść poza 1000-2000)
    if (currentSpeed > 2000) currentSpeed = 2000;
    if (currentSpeed < 1000) currentSpeed = 1000;

    // Wysłanie sygnału do silnika
    esc.writeMicroseconds(currentSpeed);

    // Wyświetlanie aktualnego stanu (tylko przy zmianie, dla czytelności)
    // Ignorujemy znaki nowej linii (enter), żeby nie śmiecić w konsoli
    if (command != '\n' && command != '\r') {
      Serial.print("Moc: ");
      Serial.print(currentSpeed);
      Serial.print(" us  (");
      // Przeliczenie na procenty dla wygody oka
      int percent = map(currentSpeed, 1000, 2000, 0, 100);
      Serial.print(percent);
      Serial.println("%)");
    }
  }
}