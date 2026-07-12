/// A single ultrasonic reading streamed from the robot.
///
/// The firmware sends lines like `A90D42` — servo at 90°, obstacle 42 cm away.
class Telemetry {
  /// Servo angle in degrees, 0..180 (90 = straight ahead).
  final int angle;

  /// Measured distance to the nearest obstacle, in centimetres.
  final int distanceCm;

  const Telemetry({required this.angle, required this.distanceCm});

  @override
  String toString() => 'Telemetry(angle: $angle°, distance: ${distanceCm}cm)';
}
