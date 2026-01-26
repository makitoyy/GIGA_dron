#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- KONFIGURACJA HARDWARE ---
Servo m1, m2, m3, m4;
Adafruit_MPU6050 mpu;

// Piny silników
#define PIN_FL 26 // Lewy Przód (CW)
#define PIN_FR 27 // Prawy Przód (CCW)
#define PIN_RL 14 // Lewy Tył (CCW)
#define PIN_RR 32 // Prawy Tył (CW)

// --- MANUALNE OFFSETY MOCY (Zostawiamy je jako bazę) ---
// Ustaw tu te wartości, które dobrałeś eksperymentalnie, żeby dron nie kręcił się na starcie
int offset_FL = 95-22; 
int offset_FR = 68-22; 
int offset_RL = 110-22; 
int offset_RR = 22-22; 

// --- PID STABILIZACJI (PITCH / ROLL) ---
float Kp = 0.6;   // Zmniejszone z 1.3 dla płynności
float Ki = 0; 
float Kd = 3;   // Zwiększone dla lepszego tłumienia drgań

// --- PID DLA OBRACANIA (YAW) - NOWOŚĆ! ---
// To sprawi, że dron będzie trzymał kierunek
float Kp_yaw = 2.5; 
float Kd_yaw = 0.5;

// --- ZMIENNE KALIBRACYJNE ---
float accelPitchOffset = 0; float accelRollOffset = 0;
float gyroPitchOffset = 0;  float gyroRollOffset = 0;
float gyroYawOffset = 0;    // Offset dla osi Z

// --- ZMIENNE SYSTEMOWE ---
double currentPitch = 0, currentRoll = 0;
long lastTime;

// Zmienne PID
float pid_p_pitch, pid_i_pitch, pid_d_pitch, last_error_pitch;
float pid_p_roll, pid_i_roll, pid_d_roll, last_error_roll;
float last_gyro_z = 0; // Do PID Yaw

// Dane z pilota
typedef struct struct_message {
  int throttle;
  float pitch_set;
  float roll_set;
  bool armed;
} struct_message;

struct_message incomingData;
bool isArmed = false;

void OnDataRecv(const esp_now_recv_info * info, const uint8_t *incomingDataArr, int len) {
  memcpy(&incomingData, incomingDataArr, sizeof(incomingData));
  isArmed = incomingData.armed;
}

void setup() {
  Serial.begin(115200);

  // 1. MPU6050
  Wire.begin(21, 22); 
  if (!mpu.begin()) { Serial.println("Błąd MPU!"); while (1); }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // 2. ESC
  m1.setPeriodHertz(50); m1.attach(PIN_FL, 1000, 2000);
  m2.setPeriodHertz(50); m2.attach(PIN_FR, 1000, 2000);
  m3.setPeriodHertz(50); m3.attach(PIN_RL, 1000, 2000);
  m4.setPeriodHertz(50); m4.attach(PIN_RR, 1000, 2000);

  m1.writeMicroseconds(1000); m2.writeMicroseconds(1000);
  m3.writeMicroseconds(1000); m4.writeMicroseconds(1000);

  // 3. ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(OnDataRecv);

  // --- KALIBRACJA ---
  Serial.println("POSTAW PŁASKO! Kalibracja za 3s...");
  delay(3000);
  Serial.println("Kalibruję...");

  float sumPitch = 0, sumRoll = 0;
  float sumGyroPitch = 0, sumGyroRoll = 0, sumGyroYaw = 0;
  int sampleCount = 1000;

  for (int i = 0; i < sampleCount; i++) {
     sensors_event_t a, g, temp;
     mpu.getEvent(&a, &g, &temp);

     // PITCH/ROLL (Poprawione znaki z poprzedniej lekcji)
     float p = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 57.3;
     float r = atan2(-a.acceleration.x, a.acceleration.z) * 57.3;

     sumPitch += p;
     sumRoll += r;
     
     // GYRO (Pitch/Roll - zamienione osie)
     sumGyroPitch += -g.gyro.y; 
     sumGyroRoll  += -g.gyro.x;
     
     // GYRO YAW (Oś Z - ta odpowiada za obrót bączka)
     sumGyroYaw += g.gyro.z;
     
     delay(2);
  }

  accelPitchOffset = sumPitch / sampleCount;
  accelRollOffset = sumRoll / sampleCount;
  gyroPitchOffset = sumGyroPitch / sampleCount;
  gyroRollOffset = sumGyroRoll / sampleCount;
  gyroYawOffset = sumGyroYaw / sampleCount; // Zapamiętujemy dryf osi Z

  currentPitch = accelPitchOffset;
  currentRoll = accelRollOffset;

  Serial.println("Gotowe!");
  lastTime = millis();
}

void loop() {
  if (!isArmed || incomingData.throttle < 1050) {
    m1.writeMicroseconds(1000); m2.writeMicroseconds(1000);
    m3.writeMicroseconds(1000); m4.writeMicroseconds(1000);
    
    // Reset PID
    pid_i_pitch = 0; last_error_pitch = 0;
    pid_i_roll = 0; last_error_roll = 0;
    
    // Reset pozycji
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    long currentTime = millis();
    lastTime = currentTime; 

    float accelPitch = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 57.3;
    float accelRoll = atan2(-a.acceleration.x, a.acceleration.z) * 57.3;
    currentPitch = accelPitch; currentRoll = accelRoll;
    return;
  }

  // --- 1. ODCZYT ---
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  long currentTime = millis();
  float dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;

  float accelPitch = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 57.3;
  float accelRoll = atan2(-a.acceleration.x, a.acceleration.z) * 57.3;

  float gyroRatePitch = (-g.gyro.y - gyroPitchOffset) * 57.3; 
  float gyroRateRoll  = (-g.gyro.x - gyroRollOffset) * 57.3; 
  
  // ODCZYT YAW (Prędkość obrotowa w stopniach na sekundę)
  float gyroRateYaw = (g.gyro.z - gyroYawOffset) * 57.3;

  // Filtr
  currentPitch = 0.98 * (currentPitch + gyroRatePitch * dt) + 0.02 * accelPitch;
  currentRoll = 0.98 * (currentRoll + gyroRateRoll * dt) + 0.02 * accelRoll;

  // --- 2. PID PITCH/ROLL ---
  float realPitch = currentPitch - accelPitchOffset;
  float realRoll = currentRoll - accelRollOffset;

  // Pitch
  float error_pitch = incomingData.pitch_set - realPitch;
  pid_p_pitch = Kp * error_pitch;
  pid_i_pitch += Ki * error_pitch;
  pid_i_pitch = constrain(pid_i_pitch, -100, 100); 
  pid_d_pitch = Kd * ((error_pitch - last_error_pitch) / dt);
  last_error_pitch = error_pitch;
  float PID_Pitch_Output = pid_p_pitch + pid_i_pitch + pid_d_pitch;

  // Roll
  float error_roll = incomingData.roll_set - realRoll;
  pid_p_roll = Kp * error_roll;
  pid_i_roll += Ki * error_roll;
  pid_i_roll = constrain(pid_i_roll, -100, 100);
  pid_d_roll = Kd * ((error_roll - last_error_roll) / dt);
  last_error_roll = error_roll;
  float PID_Roll_Output = pid_p_roll + pid_i_roll + pid_d_roll;

  // --- 3. PID YAW (NOWOŚĆ) ---
  // Cel: Chcemy, żeby prędkość obrotowa wynosiła 0 (czyli dron ma stać prosto)
  // error = 0 - gyroRateYaw;
  // poniewaz pilot nie ma sterowania Yaw, zakladamy ze zawsze chcemy 0.
  
  float error_yaw = 0 - gyroRateYaw; 
  // Prosty regulator PD dla Yaw (I nie jest konieczne do samej stabilizacji)
  float PID_Yaw_Output = (Kp_yaw * error_yaw) + (Kd_yaw * (error_yaw - (0 - last_gyro_z))); 
  last_gyro_z = gyroRateYaw;
  
  // Ogranicz Yaw, żeby nie zagłuszył wznoszenia
  PID_Yaw_Output = constrain(PID_Yaw_Output, -100, 100);


  // --- 4. MIKSER (Z YAW) ---
  // Pitch i Roll ograniczamy do 400
  PID_Pitch_Output = constrain(PID_Pitch_Output, -400, 400);
  PID_Roll_Output = constrain(PID_Roll_Output, -400, 400);

  int thr = incomingData.throttle;

  // Logika miksera dla układu X (Quad X):
  // Yaw działa na zasadzie przyspieszania silników po przekątnej
  // Jeśli dron kręci się w prawo (CW), musimy przyspieszyć silniki CCW (FR, RL)
  
  int pwm1 = thr + PID_Pitch_Output + PID_Roll_Output - PID_Yaw_Output + offset_FL; // FL (CW)
  int pwm2 = thr + PID_Pitch_Output - PID_Roll_Output + PID_Yaw_Output + offset_FR; // FR (CCW)
  int pwm3 = thr - PID_Pitch_Output + PID_Roll_Output + PID_Yaw_Output + offset_RL; // RL (CCW)
  int pwm4 = thr - PID_Pitch_Output - PID_Roll_Output - PID_Yaw_Output + offset_RR; // RR (CW)

  m1.writeMicroseconds(constrain(pwm1, 1000, 2000));
  m2.writeMicroseconds(constrain(pwm2, 1000, 2000));
  m3.writeMicroseconds(constrain(pwm3, 1000, 2000));
  m4.writeMicroseconds(constrain(pwm4, 1000, 2000));
}
