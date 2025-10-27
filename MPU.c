
//autor Jan Michalec
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);  // SDA, SCL — zmień jeśli używasz innych pinów

    Serial.println("Inicjalizacja MPU6050...");
    mpu.initialize();

    // Sprawdź, czy czujnik odpowiada
    if (!mpu.testConnection()) {
        Serial.println("Błąd: nie wykryto MPU6050!");
        while (1);
    }
    Serial.println("MPU6050 połączony poprawnie.");
}

void loop() {
    int16_t ax, ay, az;  // dane akcelerometru
    int16_t gx, gy, gz;  // dane żyroskopu

    // odczyt surowych danych
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // przeliczenie na jednostki fizyczne
    float accelX = ax / 16384.0;  // ±2g zakres
    float accelY = ay / 16384.0;
    float accelZ = az / 16384.0;

    float gyroX = gx / 131.0;     // ±250°/s zakres
    float gyroY = gy / 131.0;
    float gyroZ = gz / 131.0;

    Serial.println("----- Dane MPU6050 -----");
    Serial.print("Akcelerometr [g]: X=");
    Serial.print(accelX); Serial.print("  Y=");
    Serial.print(accelY); Serial.print("  Z=");
    Serial.println(accelZ);

    Serial.print("Żyroskop [°/s]:   X=");
    Serial.print(gyroX); Serial.print("  Y=");
    Serial.print(gyroY); Serial.print("  Z=");
    Serial.println(gyroZ);

    Serial.println("-------------------------\n");
    delay(500);
}
