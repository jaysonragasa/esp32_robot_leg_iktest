#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include "index_html.h"

// ==============================================================================
// 1. HARDWARE & NETWORK CONFIGURATION
// ==============================================================================

// WiFi settings
const char* ssid = "corp-wifi";
const char* password = "1nf1ni8m@InF0r";
WebServer server(80);

// PWM Motor Controller setup (default I2C address 0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
#define SERVO_FREQ 50 // Analog servos typically run at 50 updates per second (50 Hz)

// OLED display setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Leg Dimensions ---
// Adjust these measurements (in millimeters) to match your physical robot build.
const float L_COXA = 17.6;  // Length of the shoulder joint sideways
const float L_FEMUR = 49.3; // Length of the upper leg
const float L_TIBIA = 58.0; // Length of the lower leg
//const float L_TIBIA = 72.0; // Length of the lower leg

// ==============================================================================
// 2. DATA STRUCTURES (Structuring our variables)
// ==============================================================================

// How many motors is the leg actually using?
enum LegDegreesOfFreedom {
  DOF_1, // Femur only (Swing like a stick)
  DOF_2, // Femur + Tibia (Forward/Backward and Up/Down, no side-to-side)
  DOF_3  // Coxa + Femur + Tibia (Full 3D movement)
};
LegDegreesOfFreedom currentDOF = DOF_3; // Default to 3DOF

// How are the physical motors mounted?
enum LinkageType {
  LINKAGE_SERIAL,   // Tibia motor is mounted on the Femur (Knee angle is local)
  LINKAGE_PARALLEL  // Tibia motor is mounted on the Body (Knee angle is global/absolute)
};
LinkageType currentLinkage = LINKAGE_PARALLEL; // Default to standard Serial linkage


/*
 * ServoConfig holds the calibration data for a single physical motor.
 * - minUs / maxUs: The pulse lengths (in microseconds) that map to 0 and 180 degrees.
 * - offset: Adds or subtracts degrees to align the physical leg with the math.
 * - invert: Flips the motor direction (useful because left and right legs are mirrored).
 */
struct ServoConfig {
  uint8_t pin;   
  int minUs;     
  int maxUs;     
  float offset;  
  bool invert;   
};

/*
 * LegConfig groups three ServoConfigs into a single "Leg".
 * Every leg has a coxa (shoulder), femur (thigh), and tibia (calf).
 */
struct LegConfig {
  ServoConfig coxa;
  ServoConfig femur;
  ServoConfig tibia;
};

// We are prototyping just the Front-Left leg for now.
LegConfig legs[1] = {
  { 
    {0, 459, 2520, 90.0, true},  // Coxa on pin 0
    {1, 618, 2718, 86.0, false}, // Femur on pin 1
    {2, 464, 2590, 0.0, true}   // Tibia on pin 2

    // {0, 459, 2520, 90.0, true},  // Coxa on pin 0
    // {1, 459, 2530, 80.0, false}, // Femur on pin 1
    // {2, 459, 2580, 90.0, false}   // Tibia on pin 2
  }
};
#define FL_LEG 0 // A friendly name for index 0

// ==============================================================================
// 3. STATE VARIABLES (Tracking what the robot is currently doing)
// ==============================================================================

// Where we WANT the leg to go (Target Position)
float targetX = 0.0;
float targetY = 20.0;
float targetZ = -90.0;
bool targetUpdated = true; // True if we received a new target

// Where the leg CURRENTLY is (Current Position)
// We use this to slowly move towards the target (Interpolation)
float currentX = 0.0;
float currentY = 20.0;
float currentZ = -90.0;

// Variables for the smooth movement loop
float moveSpeed = 3.0; // Millimeters to move per step
unsigned long lastUpdateMs = 0;
const int updateIntervalMs = 20; // 20ms delay = 50 updates per second

// Variables for the automatic walking test (Gait Test)
bool isGaitTest = false;
unsigned long lastGaitMs = 0;
int gaitStep = 0;

// Variables for loop profiling
unsigned long lastHzUpdateMs = 0;
int loopCounter = 0;
float loopHz = 0.0;

// ==============================================================================
// 4. KINEMATICS (The Math Engine)
// ==============================================================================

/*
 * calculateIK converts an (x, y, z) 3D coordinate into angles for our three motors.
 * This is called Inverse Kinematics (IK). 
 */
void calculateIK(float x, float y, float z, float &coxa_angle, float &femur_angle, float &tibia_angle) {
  
  // Flip X axis so that Positive X means "Forward"
  x = -x;

  if (currentDOF == DOF_3) {
    // ----------------------------------------------------
    // 3-DOF MATH (Coxa, Femur, Tibia)
    // ----------------------------------------------------
    
    // 1. Coxa Angle
    float L_yz = sqrt(y * y + z * z);
    float L_p = sqrt(L_yz * L_yz - L_COXA * L_COXA);
    float alpha_coxa = atan2(y, -z);
    float beta_coxa = atan2(L_COXA, L_p);
    coxa_angle = alpha_coxa - beta_coxa;

    // 2. Tibia Angle
    float D_squared = x * x + L_p * L_p;
    float D = sqrt(D_squared);
    float cos_tibia = (D_squared - L_FEMUR * L_FEMUR - L_TIBIA * L_TIBIA) / (2.0 * L_FEMUR * L_TIBIA);
    cos_tibia = constrain(cos_tibia, -1.0, 1.0); 
    tibia_angle = -acos(cos_tibia); 

    // 3. Femur Angle
    float alpha_femur = atan2(x, L_p);
    float cos_femur = (L_FEMUR * L_FEMUR + D_squared - L_TIBIA * L_TIBIA) / (2.0 * L_FEMUR * D);
    cos_femur = constrain(cos_femur, -1.0, 1.0);
    float beta_femur = acos(cos_femur);
    femur_angle = alpha_femur + beta_femur; 

  } else if (currentDOF == DOF_2) {
    // ----------------------------------------------------
    // 2-DOF MATH (Femur, Tibia only - Y is ignored)
    // ----------------------------------------------------
    
    coxa_angle = 0.0; // No shoulder rotation

    // D is purely hypotenuse of X and Z.
    float D_squared = x * x + z * z;
    float D = sqrt(D_squared);

    // Tibia Angle
    float cos_tibia = (D_squared - L_FEMUR * L_FEMUR - L_TIBIA * L_TIBIA) / (2.0 * L_FEMUR * L_TIBIA);
    cos_tibia = constrain(cos_tibia, -1.0, 1.0); 
    tibia_angle = -acos(cos_tibia); 

    // Femur Angle
    float alpha_femur = atan2(x, -z);
    float cos_femur = (L_FEMUR * L_FEMUR + D_squared - L_TIBIA * L_TIBIA) / (2.0 * L_FEMUR * D);
    cos_femur = constrain(cos_femur, -1.0, 1.0);
    float beta_femur = acos(cos_femur);
    femur_angle = alpha_femur + beta_femur; 

  } else if (currentDOF == DOF_1) {
    // ----------------------------------------------------
    // 1-DOF MATH (Femur only - Just point at the target)
    // ----------------------------------------------------
    
    coxa_angle = 0.0;
    tibia_angle = 0.0;
    
    // Just point the femur at the X, Z coordinate
    femur_angle = atan2(x, -z);
  }

  // 4. Convert math results (Radians) to standard Motor angles (Degrees)
  coxa_angle = coxa_angle * 180.0 / PI;
  femur_angle = femur_angle * 180.0 / PI;
  tibia_angle = tibia_angle * 180.0 / PI;

  // 5. Apply Parallel Linkage adjustment if needed
  // In a parallel linkage, the Tibia motor is mounted to the body, 
  // so its angle must be offset by the Femur's tilt to keep the physical leg shape correct!
  if (currentLinkage == LINKAGE_PARALLEL) {
    tibia_angle = tibia_angle + femur_angle;
  }
}

// ==============================================================================
// 5. HARDWARE CONTROL (Talking to the physical motors)
// ==============================================================================

/*
 * setAngle takes a calculated math angle and safely sends it to the physical motor.
 */
void setAngle(ServoConfig servo, float angle) {
  // 1. Apply our manual tuning offset
  float desiredAngle = angle + servo.offset;
  
  // 2. Invert direction if this specific motor is mounted backwards
  if (servo.invert) {
    desiredAngle = 180.0f - desiredAngle;
  }

  // 3. Safety checks! Don't let negative numbers break the motor, 
  // and strictly limit it between 0 and 180 degrees.
  desiredAngle = abs(desiredAngle);
  desiredAngle = fmax(0.0f, fmin(180.0f, desiredAngle));

  // 4. Convert the safe 0-180 degree angle into electronic pulse timings (Microseconds)
  int pulseUs = (int)(desiredAngle * (servo.maxUs - servo.minUs) / 180.0f + servo.minUs);

  // 5. Send the pulse to the PCA9685 controller chip
  pwm.writeMicroseconds(servo.pin, pulseUs);
}

// ==============================================================================
// 6. PERIPHERALS (OLED Screen & Web Interface)
// ==============================================================================

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
  display.print(" | ");
  display.print((int)loopHz);
  display.print("Hz");
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

void handleConfig() {
  if (server.hasArg("dof")) {
    int d = server.arg("dof").toInt();
    if (d == 1) currentDOF = DOF_1;
    else if (d == 2) currentDOF = DOF_2;
    else currentDOF = DOF_3;
  }
  if (server.hasArg("linkage")) {
    int l = server.arg("linkage").toInt();
    if (l == 1) currentLinkage = LINKAGE_PARALLEL;
    else currentLinkage = LINKAGE_SERIAL;
  }
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

// ==============================================================================
// 7. BEHAVIOR LOGIC (Walking & Movement)
// ==============================================================================

/*
 * updateGaitTest cycles through 3 coordinates to make the leg "walk" in the air.
 * Now upgraded to be "smart" - it waits for the physical leg to arrive before stepping!
 */
void updateGaitTest() {
  if (isGaitTest) {
    // 1. Check how far the physical leg is from the current target
    float dx = targetX - currentX;
    float dy = targetY - currentY;
    float dz = targetZ - currentZ;
    float dist = sqrt(dx*dx + dy*dy + dz*dz);
    
    // 2. Only proceed to the next step if we have physically arrived! (within 1 millimeter)
    if (dist < 1.0) {
      // 3. Tiny pause at the end of each step (e.g., 50ms) before snapping to the next one
      if (millis() - lastGaitMs > 50) { 
        lastGaitMs = millis();
        
        // Pick the next position in our 3-step sequence
        if (gaitStep == 0) {
          targetX = 20; targetY = 20; targetZ = -90; // Step backward
          gaitStep = 1;
        } else if (gaitStep == 1) {
          targetX = -20; targetY = 20; targetZ = -90; // Step forward
          gaitStep = 2;
        } else {
          targetX = 0; targetY = 20; targetZ = -70; // Lift Leg
          gaitStep = 0;
        }
        targetUpdated = true;
      }
    } else {
      // If the leg is still moving, constantly reset the pause timer. 
      lastGaitMs = millis();
    }
  }
}

/*
 * updateInterpolation slowly moves 'currentX/Y/Z' closer to 'targetX/Y/Z'
 * every 20 milliseconds to prevent the servos from jerking violently.
 */
void updateInterpolation() {
  unsigned long currentMs = millis();
  
  if (currentMs - lastUpdateMs >= updateIntervalMs) {
    lastUpdateMs = currentMs;

    // Calculate distance to the target
    float dx = targetX - currentX;
    float dy = targetY - currentY;
    float dz = targetZ - currentZ;
    float dist = sqrt(dx*dx + dy*dy + dz*dz);
    
    // If we haven't reached the target yet...
    if (dist > 0.01) {
      // Step slightly closer to the target
      if (dist > moveSpeed) {
        currentX += (dx / dist) * moveSpeed;
        currentY += (dy / dist) * moveSpeed;
        currentZ += (dz / dist) * moveSpeed;
      } else {
        // We arrived! Snap to exact target.
        currentX = targetX;
        currentY = targetY;
        currentZ = targetZ;
      }

      // Convert our new intermediate position into motor angles
      float coxaAngle, femurAngle, tibiaAngle;
      calculateIK(currentX, currentY, currentZ, coxaAngle, femurAngle, tibiaAngle);

      // Write to the physical hardware
      setAngle(legs[FL_LEG].coxa, coxaAngle);
      setAngle(legs[FL_LEG].femur, femurAngle);
      setAngle(legs[FL_LEG].tibia, tibiaAngle);
    }
  }
}

// ==============================================================================
// 8. MAIN ARDUINO HOOKS (Setup & Loop)
// ==============================================================================

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // I2C for OLED screen

  // 1. Start OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connecting to WiFi...");
  display.display();

  // 2. Start WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP:");
  Serial.println(WiFi.localIP());

  // 3. Start Web Server
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/config", handleConfig);
  server.on("/gait", handleGait);
  server.begin();

  // 4. Start Motors
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
}

/*
 * The main loop runs constantly.
 * We've split the responsibilities into separate functions above so this stays clean!
 */
void loop() {
  loopCounter++;
  unsigned long currentMs = millis();
  if (currentMs - lastHzUpdateMs >= 1000) {
    loopHz = loopCounter / ((currentMs - lastHzUpdateMs) / 1000.0);
    loopCounter = 0;
    lastHzUpdateMs = currentMs;
    targetUpdated = true; // force redraw every second to show Hz
  }

  // Check for incoming web requests
  server.handleClient();

  // Run our walking logic
  updateGaitTest();

  // Update the OLED screen if the user gave us a new target
  if (targetUpdated) {
    float tCoxa, tFemur, tTibia;
    calculateIK(targetX, targetY, targetZ, tCoxa, tFemur, tTibia);
    updateScreen(tCoxa, tFemur, tTibia);
    targetUpdated = false;
  }

  // Slowly move the physical motors toward the target
  updateInterpolation();

  delay(10);
}