#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include "index_html.h"

// WiFi credentials
const char* ssid = "corp-wifi";
const char* password = "1nf1ni8m@InF0r";

WebServer server(80);

// PCA9685 setup (default address 0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#define SERVO_FREQ 50 // Analog servos run at ~50 Hz

struct ServoConfig {
  uint8_t pin;   // PCA9685 port number (0-15)
  int minUs;     // Minimum PWM pulse width (e.g. 459) for 0 degrees
  int maxUs;     // Maximum PWM pulse width (e.g. 2600) for 180 degrees
  float offset;  // Calibration offset in degrees to align physical 0 with IK 0
  bool invert;   // If true, flips the direction (useful for right vs left legs)
};

struct LegConfig {
  ServoConfig coxa;
  ServoConfig femur;
  ServoConfig tibia;
};

// Array of legs for future expansion. Currently prototyping 1 leg (Front-Left)
LegConfig legs[1] = {
  { // Front-Left Leg
    {0, 459, 2520, 90.0, true}, // Coxa: pin 0
    {1, 459, 2640, 80.0, false}, // Femur: pin 1
    {2, 459, 2600, 0.0, false}  // Tibia: pin 2
  }
};
#define FL_LEG 0 // Index for Front-Left leg

// Global Coordinates
float targetX = 0.0;
float targetY = 47.0;
float targetZ = -90.0;

bool isGaitTest = false;
unsigned long lastGaitMs = 0;
int gaitStep = 0;

float currentX = 0.0;
float currentY = 47.0;
float currentZ = -90.0;

float moveSpeed = 3.0; // mm per 20ms step (adjustable)
unsigned long lastUpdateMs = 0;
const int updateIntervalMs = 20; // 50 Hz control loop

bool targetUpdated = true;

// OLED setup
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Leg Dimensions (Adjust these to match your robot in mm) ---
const float L_COXA = 47.0;  // Length of the shoulder joint sideways
const float L_FEMUR = 50.0; // Length of the upper leg
const float L_TIBIA = 58.0; // Length of the lower leg

// --- Inverse Kinematics Function ---
void calculateIK(float x, float y, float z, float &coxa_angle, float &femur_angle, float &tibia_angle) {
  
  // 1. Calculate Coxa Angle (Shoulder Roll)
  // Distance from hip to foot in the Y-Z plane
  float L_yz = sqrt(y * y + z * z);
  
  // Straight-line distance from the hip pitch joint to the foot 
  float L_p = sqrt(L_yz * L_yz - L_COXA * L_COXA);
  
  // Coxa angle formula
  float alpha_coxa = atan2(y, -z);
  float beta_coxa = atan2(L_COXA, L_p);
  coxa_angle = alpha_coxa - beta_coxa;

  // 2. Calculate Tibia Angle (Knee Pitch)
  // Diagonal distance from hip pitch joint to foot in 3D space
  float D_squared = x * x + L_p * L_p;
  float D = sqrt(D_squared);
  
  // Law of Cosines for the knee angle
  float cos_tibia = (D_squared - L_FEMUR * L_FEMUR - L_TIBIA * L_TIBIA) / (2.0 * L_FEMUR * L_TIBIA);
  
  // Constrain to prevent NaN math errors if the target is physically out of reach
  cos_tibia = constrain(cos_tibia, -1.0, 1.0); 
  
  // Front dog legs usually bend backwards (negative angle)
  tibia_angle = -acos(cos_tibia); 

  // 3. Calculate Femur Angle (Hip Pitch)
  float alpha_femur = atan2(x, L_p);
  
  // Law of Cosines for the hip angle inner triangle
  float cos_femur = (L_FEMUR * L_FEMUR + D_squared - L_TIBIA * L_TIBIA) / (2.0 * L_FEMUR * D);
  cos_femur = constrain(cos_femur, -1.0, 1.0);
  float beta_femur = acos(cos_femur);
  
  femur_angle = alpha_femur + beta_femur; 

  // 4. Convert all output radians to degrees for standard servos
  coxa_angle = coxa_angle * 180.0 / PI;
  femur_angle = femur_angle * 180.0 / PI;
  tibia_angle = tibia_angle * 180.0 / PI;
}

void setAngle(ServoConfig servo, float angle) {
  // Apply offset and inversion
  float finalAngle = angle + servo.offset;
  if (servo.invert) {
    finalAngle = 180.0f - finalAngle;
  }

  finalAngle = abs(finalAngle);

  // Constrain the incoming angle strictly between 0.0 and 180.0
  finalAngle = fmax(0.0f, fmin(180.0f, finalAngle));

  // Precise float mapping to microseconds
  int pulseUs = (int)(finalAngle * (servo.maxUs - servo.minUs) / 180.0f + servo.minUs);

  Serial.println("pulseUs: "); Serial.println(pulseUs);

  // Send precise timings to PCA9685
  pwm.writeMicroseconds(servo.pin, pulseUs);
}

void updateScreen(float c, float f, float t) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0,0);
  display.print("Target: ");
  display.print((int)targetX); display.print(",");
  display.print((int)targetY); display.print(",");
  display.println((int)targetZ);
  
  display.setCursor(0, 16);
  display.print("Coxa:  "); display.println(c);
  display.print("Femur: "); display.println(f);
  display.print("Tibia: "); display.println(t);
  
  display.setCursor(0, 48);
  display.print(WiFi.localIP());
  display.display();
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleSet() {
  if (server.hasArg("x")) targetX = server.arg("x").toFloat();
  if (server.hasArg("y")) targetY = server.arg("y").toFloat();
  if (server.hasArg("z")) targetZ = server.arg("z").toFloat();
  targetUpdated = true;
  server.send(200, "text/plain", "OK");
}

void handleGait() {
  if (server.hasArg("enable")) {
    isGaitTest = (server.arg("enable") == "true");
    if(isGaitTest) {
      gaitStep = 0;
      lastGaitMs = 0; // force immediate step
    }
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  // Initialize I2C with explicit pins
  Wire.begin(21, 22);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connecting to WiFi...");
  display.display();

  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize WebServer
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/gait", handleGait);
  server.begin();

  // Initialize PWM
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
}

void loop() {
  server.handleClient();

  // Gait state machine
  if (isGaitTest) {
    if (millis() - lastGaitMs > 500) {
      lastGaitMs = millis();
      if (gaitStep == 0) {
        targetX = 20; targetY = 47; targetZ = -90;
        gaitStep = 1;
      } else if (gaitStep == 1) {
        targetX = 0; targetY = 47; targetZ = -70;
        gaitStep = 2;
      } else {
        targetX = -20; targetY = 27; targetZ = -90;
        gaitStep = 0;
      }
      targetUpdated = true;
    }
  }

  if (targetUpdated) {
    // Calculate IK for target to update OLED display
    float tCoxa, tFemur, tTibia;
    calculateIK(targetX, targetY, targetZ, tCoxa, tFemur, tTibia);
    updateScreen(tCoxa, tFemur, tTibia);
    targetUpdated = false;
  }

  // Smooth Interpolation Loop
  unsigned long currentMs = millis();
  if (currentMs - lastUpdateMs >= updateIntervalMs) {
    lastUpdateMs = currentMs;

    float dx = targetX - currentX;
    float dy = targetY - currentY;
    float dz = targetZ - currentZ;
    float dist = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (dist > 0.01) {
      if (dist > moveSpeed) {
        currentX += (dx / dist) * moveSpeed;
        currentY += (dy / dist) * moveSpeed;
        currentZ += (dz / dist) * moveSpeed;
      } else {
        currentX = targetX;
        currentY = targetY;
        currentZ = targetZ;
      }

      // Calculate IK for current interpolated position
      float coxaAngle, femurAngle, tibiaAngle;
      calculateIK(currentX, currentY, currentZ, coxaAngle, femurAngle, tibiaAngle);

      Serial.print("Coxa: "); Serial.print(coxaAngle); 
      Serial.print(" Femur: "); Serial.print(femurAngle); 
      Serial.print(" Tibia: "); Serial.println(tibiaAngle); 

      // Write to PCA9685
      setAngle(legs[FL_LEG].coxa, coxaAngle);
      setAngle(legs[FL_LEG].femur, femurAngle);
      setAngle(legs[FL_LEG].tibia, tibiaAngle);
    }
  }
}