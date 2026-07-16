/*
 * OBV Robot — firmware (v2)
 * -------------------------
 * Object-aVoidance robot with three control modes:
 *   - MANUAL : single-character drive commands over Bluetooth (HC-05)
 *   - VOICE  : same commands, sent by the phone's speech recognition
 *   - AUTO   : autonomous obstacle avoidance
 *
 * Telemetry is streamed so the app can draw a live radar.
 *
 * v2 change — reliable AUTO mode:
 *   In MANUAL/VOICE the servo sweeps continuously (nice radar). But a
 *   constantly-sweeping sensor is aimed forward only briefly, so it misses
 *   obstacles dead ahead. So in AUTO the servo now HOLDS FORWARD (90 deg) and
 *   watches continuously; it only glances left/right to pick a turn direction
 *   once an obstacle is found. Distances are median-filtered to reject noise.
 *
 * Command protocol (app -> Arduino), one ASCII char each:
 *   F / U : forward        B / D : backward
 *   L     : pivot left      R    : pivot right
 *   S     : stop (also exits AUTO)
 *   T     : enter AUTO (obstacle-avoidance) mode
 *   0..9  : set speed (mapped to 0..255)
 *
 * Telemetry (Arduino -> app), one line per reading:
 *   A<angle>D<distance>\n     e.g.  A90D42
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
int servoPos = 90; // last commanded servo angle

// ===============================
// STATE
// ===============================
enum Mode { MANUAL, AUTO };
Mode mode = MANUAL;
int currentSpeed = 200; // 0..255, set via '0'..'9'

// Continuous sweep — MANUAL/VOICE only (for the radar)
const int SWEEP_MIN = 30;
const int SWEEP_MAX = 150;
const int SWEEP_STEP = 6;
const unsigned long SWEEP_INTERVAL = 60;
int sweepAngle = 90;
int sweepDir = 1;
unsigned long lastSweepTime = 0;

// AUTO obstacle avoidance (non-blocking state machine)
const int OBSTACLE_CM = 30;          // stop when something is this close ahead
const unsigned long TURN_MS = 550;   // how long to pivot when avoiding
const unsigned long SETTLE_MS = 220; // let the servo settle before a side reading

// While driving, AUTO gently scans a forward arc (not a single point), so the
// robot senses distance across the near-front and the radar shows a real arc.
const int FSCAN_MIN = 60;
const int FSCAN_MAX = 120;
const int FSCAN_STEP = 15;
const unsigned long FSCAN_INTERVAL = 70; // pace: lets the servo settle between reads
int fscanAngle = 90;
int fscanDir = 1;

enum AutoState { A_DRIVE, A_BRAKE, A_LOOK_L, A_LOOK_R, A_TURN };
AutoState aState = A_DRIVE;
unsigned long aTimer = 0;
unsigned long lastAutoRead = 0;
int aLeft = 300, aRight = 300;

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  scanServo.attach(9);
  pointServo(90);
  setAllSpeed(currentSpeed);
  stopAll();
}

// ===============================
// LOOP
// ===============================
void loop() {
  handleSerial();               // always respond to commands first
  if (mode == AUTO) {
    autoStep();                 // AUTO drives the servo + reads itself
  } else {
    updateSweep();              // MANUAL/VOICE: continuous radar sweep
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
      aState = A_DRIVE;
      fscanAngle = 90;
      lastAutoRead = 0;
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
    default: /* ignore */                 break;
  }
}

// ===============================
// MANUAL/VOICE: continuous radar sweep
// ===============================
void updateSweep() {
  unsigned long now = millis();
  if (now - lastSweepTime < SWEEP_INTERVAL) return;
  lastSweepTime = now;

  int d = pingCm();             // single fast ping is fine for the radar
  streamTelemetry(sweepAngle, d);

  sweepAngle += sweepDir * SWEEP_STEP;
  if (sweepAngle >= SWEEP_MAX) { sweepAngle = SWEEP_MAX; sweepDir = -1; }
  if (sweepAngle <= SWEEP_MIN) { sweepAngle = SWEEP_MIN; sweepDir =  1; }
  pointServo(sweepAngle);
}

// ===============================
// AUTO: forward-arc scanning avoidance
// ===============================
void autoStep() {
  unsigned long now = millis();

  switch (aState) {
    case A_DRIVE:
      if (now - lastAutoRead >= FSCAN_INTERVAL) {
        lastAutoRead = now;
        int d = readDistanceMedian();              // clean reading at current angle
        streamTelemetry(fscanAngle, d);            // radar shows the forward arc
        if (d <= OBSTACLE_CM) {                    // obstacle anywhere in the arc
          stopAll();
          aTimer = now + 150;
          aState = A_BRAKE;
          return;
        }
        // step the sensor across the forward arc for the next read
        fscanAngle += fscanDir * FSCAN_STEP;
        if (fscanAngle >= FSCAN_MAX) { fscanAngle = FSCAN_MAX; fscanDir = -1; }
        if (fscanAngle <= FSCAN_MIN) { fscanAngle = FSCAN_MIN; fscanDir =  1; }
        pointServo(fscanAngle);
      }
      moveForward();
      break;

    case A_BRAKE:                                  // pause, then look left
      if (now >= aTimer) {
        pointServo(150);
        aTimer = now + SETTLE_MS;
        aState = A_LOOK_L;
      }
      break;

    case A_LOOK_L:
      if (now >= aTimer) {
        aLeft = readDistanceMedian();
        streamTelemetry(150, aLeft);
        pointServo(30);                            // now look right
        aTimer = now + SETTLE_MS;
        aState = A_LOOK_R;
      }
      break;

    case A_LOOK_R:
      if (now >= aTimer) {
        aRight = readDistanceMedian();
        streamTelemetry(30, aRight);
        pointServo(90);                            // face forward for the turn
        if (aLeft >= aRight) pivotTurnLeft();      // turn toward the open side
        else                 pivotTurnRight();
        aTimer = now + TURN_MS;
        aState = A_TURN;
      }
      break;

    case A_TURN:
      if (now >= aTimer) {
        stopAll();
        fscanAngle = 90;              // re-center the forward scan
        fscanDir = 1;
        lastAutoRead = 0;            // read again immediately
        aState = A_DRIVE;
      }
      break;
  }
}

// ===============================
// DISTANCE
// ===============================
// Median of 3 pings — rejects the occasional noisy/inflated reading.
int readDistanceMedian() {
  int a = pingCm(); delay(8);
  int b = pingCm(); delay(8);
  int c = pingCm();
  if (a > b) { int t = a; a = b; b = t; }
  if (b > c) { int t = b; b = c; c = t; }
  if (a > b) { int t = a; a = b; b = t; }
  return b;
}

int pingCm() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH, 12000UL); // ~2 m timeout
  if (duration == 0) return 300;                             // no echo -> far
  return (int)(duration * 0.0343 / 2.0);
}

void streamTelemetry(int angle, int distance) {
  Serial.print('A');
  Serial.print(angle);
  Serial.print('D');
  Serial.println(distance);
}

void pointServo(int a) {
  if (a != servoPos) {
    scanServo.write(a);
    servoPos = a;
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
  motor1.run(FORWARD);
  motor4.run(FORWARD);
  motor2.run(BACKWARD);
  motor3.run(BACKWARD);
}

void pivotTurnRight() {
  setAllSpeed(currentSpeed);
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
