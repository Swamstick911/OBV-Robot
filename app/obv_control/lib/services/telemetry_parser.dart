import '../models/telemetry.dart';

/// Pure parser for the robot's telemetry protocol.
///
/// A telemetry line looks like `A<angle>D<distance>`, e.g. `A90D42`.
/// Kept free of any I/O so it can be unit-tested in isolation.
class TelemetryParser {
  static final RegExp _pattern = RegExp(r'^A(\d{1,3})D(\d{1,4})$');

  /// Parses a single line into a [Telemetry], or returns `null` if the line
  /// is malformed or out of range.
  static Telemetry? parseLine(String line) {
    final match = _pattern.firstMatch(line.trim());
    if (match == null) return null;

    final angle = int.parse(match.group(1)!);
    final distance = int.parse(match.group(2)!);
    if (angle < 0 || angle > 180) return null;

    return Telemetry(angle: angle, distanceCm: distance);
  }
}
