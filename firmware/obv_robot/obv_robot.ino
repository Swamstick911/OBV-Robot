/*
 * OBV Robot — firmware
 * ---------------------
 * Object-aVoidance robot with three control modes:
 *   - MANUAL : driven by single-character commands over Bluetooth (HC-05)
 *   - AUTO   : autonomous obstacle avoidance
 *   - (voice mode lives in the app; it just sends the same MANUAL commands)
 *
 * The servo sweeps the ultrasonic sensor continuously in ALL modes and
 * streams telemetry so the app can draw a live radar.
 *
 * Command protocol (app -> Arduino), one ASCII char each:
 *   F / U : forward        B / D : backward
 *   L     : pivot left      R    : pivot right
 *   S     : stop (also exits AUTO)
 *   T     : enter AUTO (obstacle-avoidance) mode
 *   0..9  : set speed (mapped to 0..255)
 *
 * Telemetry (Arduino -> app), one line per reading, newline-terminated:
 *   A<angle>D<distance>\n     e.g.  A90D42
 *
 * Design note: the loop is NON-BLOCKING (no long delay() chains). Incoming
 * commands are checked on every pass, so mode-switch / stop respond instantly.
 */

#include <AFMotor.h>
#include <Servo.h>

// ===============================
// MOTORS  (M1=RF, M2=LF, M3=LR, M4=RR)
// ===============================
AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);
AF_DCMotor motor4(4);

// ===============================
// SENSOR / SERVO
// ===============================
#define trigPin A1
#define echoPin A2
Servo scanServo;

// ===============================
// STATE
// ===============================
enum Mode { MANUAL, AUTO };
Mode mode = MANUAL;

int currentSpeed = 200;              // 0..255, set via '0'..'9'

// Servo sweep (non-blocking)
const int SWEEP_MIN  = 30;
const int SWEEP_MAX  = 150;
const int SWEEP_STEP = 6;
const unsigned long SWEEP_INTERVAL = 50;   // ms between steps
int sweepAngle = 90;
int sweepDir   = 1;                          // +1 / -1
unsigned long lastSweepTime = 0;

// Latest distance per sector (cm), updated as the sweep passes through
int frontDistance = 300;
int leftDistance  = 300;
int rightDistance = 300;

// Auto-mode obstacle avoidance
const int OBSTACLE_CM = 30;
enum AutoState { DRIVING, TURNING };
AutoState autoState = DRIVING;
unsigned long autoTurnUntil = 0;
const unsigned long TURN_MS = 600;

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  scanServo.attach(9);
  scanServo.write(sweepAngle);

  setAllSpeed(currentSpeed);
  stopAll();
}

// ===============================
// LOOP  (non-blocking)
// ===============================
void loop() {
  handleSerial();     // 1. always respond to commands first
  updateSweep();      // 2. sweep servo + stream telemetry on a timer
  if (mode == AUTO) { // 3. run the active mode
    autoStep();
  }
}

// ===============================
// SERIAL / COMMANDS
// ===============================
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == 'T') {                       // enter auto mode
      mode = AUTO;
      autoState = DRIVING;
    } else if (c >= '0' && c <= '9') {    // set speed (any mode)
      currentSpeed = map(c - '0', 0, 9, 0, 255);
      setAllSpeed(currentSpeed);
    } else {                              // manual command -> exit auto
      mode = MANUAL;
      applyManual(c);
    }
  }
}

void applyManual(char c) {
  switch (c) {
    case 'F': case 'U': moveForward();  break;
    case 'B': case 'D': moveBackward(); break;
    case 'L':           pivotTurnLeft();  break;
    case 'R':           pivotTurnRight(); break;
    case 'S':           stopAll();        break;
    default: /* ignore unknown */         break;
  }
}

// ===============================
// SWEEP + TELEMETRY
// ===============================
void updateSweep() {
  unsigned long now = millis();
  if (now - lastSweepTime < SWEEP_INTERVAL) return;
  lastSweepTime = now;

  // The servo has been at sweepAngle since last tick, so it has settled.
  int d = readDistance();
  streamTelemetry(sweepAngle, d);

  // Store into the matching sector for the auto-mode logic.
  if (sweepAngle < 75)       rightDistance = d;
  else if (sweepAngle > 105) leftDistance  = d;
  else                       frontDistance = d;

  // Advance the servo for the next tick.
  sweepAngle += sweepDir * SWEEP_STEP;
  if (sweepAngle >= SWEEP_MAX) { sweepAngle = SWEEP_MAX; sweepDir = -1; }
  if (sweepAngle <= SWEEP_MIN) { sweepAngle = SWEEP_MIN; sweepDir =  1; }
  scanServo.write(sweepAngle);
}

int readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 20000);   // ~3.4 m timeout
  if (duration == 0) return 300;                    // no echo -> treat as far
  return (int)(duration * 0.034 / 2);
}

void streamTelemetry(int angle, int distance) {
  Serial.print('A');
  Serial.print(angle);
  Serial.print('D');
  Serial.println(distance);
}

// ===============================
// AUTO MODE (non-blocking state machine)
// ===============================
void autoStep() {
  unsigned long now = millis();

  switch (autoState) {
    case DRIVING:
      if (frontDistance <= OBSTACLE_CM) {
        stopAll();
        // Turn toward whichever side is more open.
        if (leftDistance >= rightDistance) pivotTurnLeft();
        else                               pivotTurnRight();
        autoTurnUntil = now + TURN_MS;
        autoState = TURNING;
      } else {
        moveForward();
      }
      break;

    case TURNING:
      if (now >= autoTurnUntil) {
        stopAll();
        autoState = DRIVING;
      }
      break;
  }
}

// ===============================
// MOTOR HELPERS
// ===============================
void moveForward() {
  setAllSpeed(currentSpeed);
  motor1.run(FORWARD);
  motor2.run(FORWARD);
  motor3.run(FORWARD);
  motor4.run(FORWARD);
}

void moveBackward() {
  setAllSpeed(currentSpeed);
  motor1.run(BACKWARD);
  motor2.run(BACKWARD);
  motor3.run(BACKWARD);
  motor4.run(BACKWARD);
}

void pivotTurnLeft() {
  setAllSpeed(currentSpeed);
  // Right side forward, left side backward -> rotate left in place.
  motor1.run(FORWARD);
  motor4.run(FORWARD);
  motor2.run(BACKWARD);
  motor3.run(BACKWARD);
}

void pivotTurnRight() {
  setAllSpeed(currentSpeed);
  // Right side backward, left side forward -> rotate right in place.
  motor1.run(BACKWARD);
  motor4.run(BACKWARD);
  motor2.run(FORWARD);
  motor3.run(FORWARD);
}

void stopAll() {
  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);
  motor4.run(RELEASE);
}

void setAllSpeed(int speed) {
  motor1.setSpeed(speed);
  motor2.setSpeed(speed);
  motor3.setSpeed(speed);
  motor4.setSpeed(speed);
}
