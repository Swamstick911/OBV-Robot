#include <AFMotor.h>
#include <Servo.h>

// ===============================
// MOTOR CONNECTIONS
// M1 = Right Front
// M2 = Left Front
// M3 = Left Rear
// M4 = Right Rear
// ===============================

AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);
AF_DCMotor motor4(4);

// Ultrasonic Sensor
#define trigPin A1
#define echoPin A2

// Servo
Servo scanServo;

// Speed
const int moveSpeed = 200;
const int turnSpeed = 200;
const int reverseSpeed = 200;

char btCommand = 'S';

long duration;
int distance;

// ===============================
// SETUP
// ===============================
void setup() {

  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  scanServo.attach(9);
  scanServo.write(90);

  setAllSpeed(moveSpeed);
}

// ===============================
// LOOP
// ===============================
void loop() {

  if (Serial.available()) {
    btCommand = Serial.read();
  }

   T = Obstacle Avoidance Mode
  if (btCommand == 'T') {
    autoMode();
  }
  else {
    manualControl(btCommand);
  }
}

// ===============================
// MANUAL BLUETOOTH CONTROL
// ===============================
void manualControl(char cmd) {

  switch (cmd) {

    case 'F'
    case 'U'
      moveForward();
      break;

    case 'B'
    case 'D'
      moveBackward();
      break;

    case 'L'
      pivotTurnLeft();
      break;

    case 'R'
      pivotTurnRight();
      break;

    case 'S'
      stopAll();
      break;
  }
}

// ===============================
// OBSTACLE AVOIDANCE
// ===============================
void autoMode() {

  int front = readDistance(90);

  if (front  25) {

    moveForward();

  }
  else {

    stopAll();
    delay(200);

    int left = readDistance(150);
    delay(200);

    int right = readDistance(30);
    delay(200);

    scanServo.write(90);

    if (left  right && left  25) {

      pivotTurnLeft();
      delay(600);

    }
    else if (right  left && right  25) {

      pivotTurnRight();
      delay(600);

    }
    else {

      moveBackward();
      delay(500);

      pivotTurnRight();
      delay(700);

    }

    stopAll();
  }
}

// ===============================
// DISTANCE FUNCTION
// ===============================
int readDistance(int angle) {

  scanServo.write(angle);
  delay(350);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 20000);

  if (duration == 0)
    return 300;

  distance = duration  0.034  2;

  Serial.print(Angle );
  Serial.print(angle);
  Serial.print( Distance );
  Serial.println(distance);

  return distance;
}

// ===============================
// MOTOR FUNCTIONS
// ===============================
void moveForward() {

  setAllSpeed(moveSpeed);

  motor1.run(FORWARD);
  motor2.run(FORWARD);
  motor3.run(FORWARD);
  motor4.run(FORWARD);

}

void moveBackward() {

  setAllSpeed(reverseSpeed);

  motor1.run(BACKWARD);
  motor2.run(BACKWARD);
  motor3.run(BACKWARD);
  motor4.run(BACKWARD);

}

void pivotTurnLeft() {

  setAllSpeed(turnSpeed);

   Right Side Forward
  motor1.run(FORWARD);
  motor4.run(FORWARD);

   Left Side Backward
  motor2.run(BACKWARD);
  motor3.run(BACKWARD);

}

void pivotTurnRight() {

  setAllSpeed(turnSpeed);

   Right Side Backward
  motor1.run(BACKWARD);
  motor4.run(BACKWARD);

   Left Side Forward
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