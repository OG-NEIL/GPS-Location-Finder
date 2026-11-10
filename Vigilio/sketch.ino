#include <Wire.h>
#include <TinyGPS++.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* SERVER_URL =
    "https://unspoiled-pastime-reassign.ngrok-free.dev/api/gps";

TinyGPSPlus gps;

HardwareSerial GPSserial(2);

#define GPS_RX 16
#define GPS_TX 17

void connectWiFi() {
    Serial.print("Connecting to WiFi");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
}

void sendGPSLocation(double latitude, double longitude) {

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return;
    }

    HTTPClient http;

    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"latitude\":" + String(latitude, 6) + ",";
    json += "\"longitude\":" + String(longitude, 6);
    json += "}";

    Serial.println("Sending GPS data:");
    Serial.println(json);

    int httpResponseCode = http.POST(json);

    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
        String response = http.getString();

        Serial.println("Server response:");
        Serial.println(response);
    }

    http.end();
}

void setup() {

    Serial.begin(115200);

    GPSserial.begin(
        9600,
        SERIAL_8N1,
        GPS_RX,
        GPS_TX
    );

    Serial.println("GPS Tracker Starting...");

    connectWiFi();
}

unsigned long lastSend = 0;

void loop() {

    while (GPSserial.available()) {
        gps.encode(GPSserial.read());
    }

    if (gps.location.isValid()) {

        if (millis() - lastSend >= 5000) {

            lastSend = millis();

            double latitude = gps.location.lat();
            double longitude = gps.location.lng();

            Serial.print("GPS Location: ");
            Serial.print(latitude, 6);
            Serial.print(", ");
            Serial.println(longitude, 6);

            sendGPSLocation(latitude, longitude);
        }
    }
}

// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =====================================================
// MPU6050
// =====================================================

Adafruit_MPU6050 mpu;

#define SDA_PIN 21
#define SCL_PIN 22

#define MOTION_THRESHOLD 2.0

// Fall thresholds
#define FREEFALL_THRESHOLD 3.0
#define IMPACT_THRESHOLD 25.0
#define ROTATION_THRESHOLD 150.0

#define FALL_CONFIRM_TIME 2000

bool possibleFall = false;
unsigned long fallStartTime = 0;

// =====================================================
// GPS
// =====================================================

TinyGPSPlus gps;

#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600

HardwareSerial GPSserial(2);

// =====================================================
// SOS + VIBRATION
// =====================================================

#define SOS_BUTTON 25
#define VIBRATION_PIN 2

bool lastButtonState = HIGH;

// =====================================================
// GEOFENCE
// =====================================================

const double GEOFENCE_LAT = 23.4664;
const double GEOFENCE_LON = 51.9101;

const double GEOFENCE_RADIUS = 100.0;

bool wasInsideGeofence = true;

// =====================================================
// BATTERY
// =====================================================

int batteryLevel = 100;

unsigned long lastBatteryUpdate = 0;

#define ACTIVE_DRAIN_TIME 5000
#define SLEEP_DRAIN_TIME 15000;

// =====================================================
// SLEEP / WAKE
// =====================================================

#define SLEEP_TIMEOUT 10000

unsigned long lastMovementTime = 0;

bool sleeping = false;

// =====================================================
// GENERAL
// =====================================================

bool moving = false;

double distanceFromHome = 0;

unsigned long lastDisplayUpdate = 0;


// =====================================================
// VIBRATION
// =====================================================

void vibrate(int times) {

  for (int i = 0; i < times; i++) {

    digitalWrite(VIBRATION_PIN, HIGH);

    delay(300);

    digitalWrite(VIBRATION_PIN, LOW);

    delay(200);
  }
}


// =====================================================
// SOS
// =====================================================

void triggerSOS(const char* reason) {

  Serial.println();
  Serial.println("================================");
  Serial.println("🚨 SOS ALERT");
  Serial.print("REASON: ");
  Serial.println(reason);
  Serial.println("================================");

  // Simulated vibration
  vibrate(3);

  Serial.println("SOS EVENT CREATED");
  Serial.println("Ready for server transmission");
}


// =====================================================
// READ GPS
// =====================================================

void readGPS() {

  while (GPSserial.available()) {

    char c = GPSserial.read();

    gps.encode(c);
  }
}


// =====================================================
// READ MPU6050
// =====================================================

void readMotion() {

  sensors_event_t acceleration;
  sensors_event_t gyro;
  sensors_event_t temperature;

  mpu.getEvent(
    &acceleration,
    &gyro,
    &temperature
  );

  float x = acceleration.acceleration.x;
  float y = acceleration.acceleration.y;
  float z = acceleration.acceleration.z;

  float totalAcceleration = sqrt(
    x * x +
    y * y +
    z * z
  );

  float movement = fabs(
    totalAcceleration - 9.81
  );

  float rotationX =
    gyro.gyro.x * 57.2958;

  float rotationY =
    gyro.gyro.y * 57.2958;

  float rotationZ =
    gyro.gyro.z * 57.2958;

  float totalRotation = sqrt(
    rotationX * rotationX +
    rotationY * rotationY +
    rotationZ * rotationZ
  );


  // ==========================================
  // MOTION
  // ==========================================

  if (movement >= MOTION_THRESHOLD) {

    moving = true;

    lastMovementTime = millis();

    // Wake device if sleeping

    if (sleeping) {

      sleeping = false;

      Serial.println();
      Serial.println("🟢 MOVEMENT DETECTED");
      Serial.println("DEVICE WAKING UP");
    }

  } else {

    moving = false;
  }


  // ==========================================
  // FALL DETECTION
  // ==========================================

  if (!possibleFall) {

    if (
      totalAcceleration < FREEFALL_THRESHOLD
    ) {

      possibleFall = true;

      fallStartTime = millis();

      Serial.println();
      Serial.println("⚠️ POSSIBLE FALL");
      Serial.println("FREE-FALL DETECTED");
    }

    else if (
      totalAcceleration > IMPACT_THRESHOLD &&
      totalRotation > ROTATION_THRESHOLD
    ) {

      possibleFall = true;

      fallStartTime = millis();

      Serial.println();
      Serial.println("⚠️ POSSIBLE FALL");
      Serial.println("IMPACT + ROTATION DETECTED");
    }
  }


  // ==========================================
  // FALL CONFIRMATION
  // ==========================================

  if (possibleFall) {

    if (
      millis() - fallStartTime >=
      FALL_CONFIRM_TIME
    ) {

      if (
        !moving &&
        totalRotation < 60.0
      ) {

        Serial.println();
        Serial.println("🚨 FALL CONFIRMED!");

        triggerSOS("FALL DETECTED");
      }

      else {

        Serial.println(
          "Fall cancelled - movement detected."
        );
      }

      possibleFall = false;
    }
  }
}


// =====================================================
// GEOFENCE
// =====================================================

void checkGeofence() {

  if (!gps.location.isValid()) {
    return;
  }

  double latitude = gps.location.lat();
  double longitude = gps.location.lng();

  distanceFromHome =
    TinyGPSPlus::distanceBetween(
      latitude,
      longitude,
      GEOFENCE_LAT,
      GEOFENCE_LON
    );


  if (
    distanceFromHome <=
    GEOFENCE_RADIUS
  ) {

    if (!wasInsideGeofence) {

      Serial.println();
      Serial.println(
        "🟢 DEVICE RETURNED TO SAFE ZONE"
      );

      wasInsideGeofence = true;
    }

  }

  else {

    if (wasInsideGeofence) {

      Serial.println();
      Serial.println(
        "⚠️ GEOFENCE BREACH!"
      );

      triggerSOS("GEOFENCE BREACH");

      wasInsideGeofence = false;
    }
  }
}


// =====================================================
// SOS BUTTON
// =====================================================

void checkSOSButton() {

  bool buttonState =
    digitalRead(SOS_BUTTON);

  if (
    buttonState == LOW &&
    lastButtonState == HIGH
  ) {

    triggerSOS("MANUAL SOS BUTTON");
  }

  lastButtonState = buttonState;
}


// =====================================================
// SLEEP / WAKE
// =====================================================

void checkSleep() {

  if (
    !sleeping &&
    millis() - lastMovementTime >=
    SLEEP_TIMEOUT
  ) {

    sleeping = true;

    Serial.println();
    Serial.println("😴 NO MOVEMENT");
    Serial.println("DEVICE ENTERING SLEEP MODE");
  }
}


// =====================================================
// BATTERY
// =====================================================

void updateBattery() {

  unsigned long currentTime =
    millis();

  unsigned long drainTime;

  if (sleeping) {

    drainTime =
      SLEEP_DRAIN_TIME;

  } else {

    drainTime =
      ACTIVE_DRAIN_TIME;
  }


  if (
    currentTime - lastBatteryUpdate >=
    drainTime
  ) {

    lastBatteryUpdate =
      currentTime;

    if (batteryLevel > 0) {

      batteryLevel--;

      Serial.print("Battery: ");
      Serial.print(batteryLevel);
      Serial.println("%");
    }
  }


  if (batteryLevel == 20) {

    Serial.println(
      "⚠️ LOW BATTERY"
    );
  }


  if (batteryLevel == 5) {

    Serial.println(
      "🚨 CRITICAL BATTERY"
    );
  }


  if (batteryLevel <= 0) {

    Serial.println(
      "🔴 BATTERY EMPTY"
    );
  }
}


// =====================================================
// OLED DISPLAY
// =====================================================

void updateDisplay() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  // Title

  display.setTextSize(1);

  display.setCursor(30, 0);

  display.println(
    "SMARTBAND"
  );


  // GPS

  display.setCursor(0, 12);

  display.print("GPS: ");

  if (gps.location.isValid()) {

    display.print("OK");

  } else {

    display.print("SEARCH");
  }


  // Motion

  display.setCursor(65, 12);

  display.print("M:");

  if (moving) {

    display.print("MOVE");

  } else {

    display.print("STILL");
  }


  // Battery

  display.setCursor(0, 24);

  display.print("BAT:");

  display.print(batteryLevel);

  display.print("%");


  // Sleep

  display.setCursor(65, 24);

  if (sleeping) {

    display.print("SLEEP");

  } else {

    display.print("ACTIVE");
  }


  // Latitude

  display.setCursor(0, 36);

  display.print("LAT:");

  if (gps.location.isValid()) {

    display.print(
      gps.location.lat(),
      4
    );

  } else {

    display.print("---");
  }


  // Longitude

  display.setCursor(0, 48);

  display.print("LON:");

  if (gps.location.isValid()) {

    display.print(
      gps.location.lng(),
      4
    );

  } else {

    display.print("---");
  }


  // Geofence

  display.setCursor(85, 48);

  if (
    gps.location.isValid() &&
    distanceFromHome <=
    GEOFENCE_RADIUS
  ) {

    display.print("SAFE");

  } else {

    display.print("OUT");
  }


  display.display();
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);


  // I2C

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  // GPS

  GPSserial.begin(
    GPS_BAUD,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );


  // Button

  pinMode(
    SOS_BUTTON,
    INPUT_PULLUP
  );


  // Vibration / LED

  pinMode(
    VIBRATION_PIN,
    OUTPUT
  );

  digitalWrite(
    VIBRATION_PIN,
    LOW
  );


  // MPU6050

  if (!mpu.begin()) {

    Serial.println(
      "MPU6050 NOT FOUND!"
    );

    while (1);
  }

  mpu.setAccelerometerRange(
    MPU6050_RANGE_8_G
  );

  mpu.setGyroRange(
    MPU6050_RANGE_500_DEG
  );


  // OLED

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {

    Serial.println(
      "OLED NOT FOUND!"
    );

    while (1);
  }


  // Initial state

  lastMovementTime =
    millis();

  lastBatteryUpdate =
    millis();


  Serial.println();
  Serial.println("==============================");
  Serial.println("     SMARTBAND SYSTEM");
  Serial.println("==============================");
  Serial.println("GPS: READY");
  Serial.println("MPU6050: READY");
  Serial.println("OLED: READY");
  Serial.println("SOS: READY");
  Serial.println("GEOFENCE: READY");
  Serial.println("SLEEP/WAKE: READY");
  Serial.println("BATTERY: READY");
  Serial.println("==============================");
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // Always keep GPS processing
  readGPS();


  // Safety functions
  checkSOSButton();

  readMotion();

  checkGeofence();


  // Power management
  checkSleep();

  updateBattery();


  // OLED
  if (
    millis() - lastDisplayUpdate >=
    500
  ) {

    lastDisplayUpdate =
      millis();

    updateDisplay();
  }


  delay(20);
}