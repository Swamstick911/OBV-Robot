/*
 * OBV Robot — WiFi-control firmware (LITE)
 * ----------------------------------------
 * Minimal build for the ESP32 WiFi bridge, one-way link:
 *   ESP32 GPIO17 (TX) ----> Arduino D10  (SoftwareSerial RX)
 *
 * Receives single-character drive commands and runs the 4 motors. No sonar,
 * servo, telemetry, or auto mode.
 *
 * IMPORTANT: the current motion command is RE-ISSUED every 100ms. The L293D
 * shield latches motor direction in a 74HC595 shift register that electrical
 * noise from the motors can corrupt, which makes the motors cut out. Refreshing
 * the command keeps them running steadily (same trick as a heartbeat).
 *
 * Commands:  F/U forward   B/D backward   L left   R right   S stop   0-9 speed
 */

#include <AFMotor.h>
#include <SoftwareSerial.h>

AF_DCMotor motor1(1);   // Right Front
AF_DCMotor motor2(2);   // Left Front
AF_DCMotor motor3(3);   // Left Rear
AF_DCMotor motor4(4);   // Right Rear

// Commands from the ESP32 arrive on D10. Pin 2 is a harmless unused dummy TX.
SoftwareSerial espSerial(10, 2);

int currentSpeed = 200;
char currentMotion = 'S';
unsigned long lastRefresh = 0;
const unsigned long REFRESH_MS = 100;

void setup() {
  espSerial.begin(9600);
  setAllSpeed(currentSpeed);
  stopAll();
}

void loop() {
  while (espSerial.available()) {
    handleCommand((char)espSerial.read());
  }

  // Re-issue the current motion so a shift-register glitch can't stop us.
  if (millis() - lastRefresh >= REFRESH_MS) {
    lastRefresh = millis();
    applyMotion(currentMotion);
  }
}

void handleCommand(char c) {
  if (c >= '0' && c <= '9') {                 // speed
    currentSpeed = map(c - '0', 0, 9, 0, 255);
    setAllSpeed(currentSpeed);
    return;
  }
  switch (c) {
    case 'F': case 'U': currentMotion = 'F'; break;
    case 'B': case 'D': currentMotion = 'B'; break;
    case 'L':           currentMotion = 'L'; break;
    case 'R':           currentMotion = 'R'; break;
    case 'S': case 'T': currentMotion = 'S'; break;
    default: return;                          // ignore unknown
  }
  applyMotion(currentMotion);                 // act immediately, too
}

void applyMotion(char m) {
  switch (m) {
    case 'F': moveForward();    break;
    case 'B': moveBackward();   break;
    case 'L': pivotTurnLeft();  break;
    case 'R': pivotTurnRight(); break;
    default:  stopAll();        break;        // 'S'
  }
}

// ===============================
// MOTOR HELPERS
// ===============================
void moveForward() {
  setAllSpeed(currentSpeed);
  motor1.run(FORWARD); motor2.run(FORWARD); motor3.run(FORWARD); motor4.run(FORWARD);
}

void moveBackward() {
  setAllSpeed(currentSpeed);
  motor1.run(BACKWARD); motor2.run(BACKWARD); motor3.run(BACKWARD); motor4.run(BACKWARD);
}

void pivotTurnLeft() {
  setAllSpeed(currentSpeed);
  motor1.run(FORWARD);  motor4.run(FORWARD);   // right side forward
  motor2.run(BACKWARD); motor3.run(BACKWARD);  // left side backward
}

void pivotTurnRight() {
  setAllSpeed(currentSpeed);
  motor1.run(BACKWARD); motor4.run(BACKWARD);  // right side backward
  motor2.run(FORWARD);  motor3.run(FORWARD);   // left side forward
}

void stopAll() {
  motor1.run(RELEASE); motor2.run(RELEASE); motor3.run(RELEASE); motor4.run(RELEASE);
}

void setAllSpeed(int s) {
  motor1.setSpeed(s); motor2.setSpeed(s); motor3.setSpeed(s); motor4.setSpeed(s);
}
