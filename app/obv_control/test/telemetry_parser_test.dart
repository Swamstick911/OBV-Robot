import 'package:flutter_test/flutter_test.dart';
import 'package:obv_control/services/telemetry_parser.dart';

void main() {
  group('TelemetryParser', () {
    test('parses a well-formed line', () {
      final t = TelemetryParser.parseLine('A90D42');
      expect(t, isNotNull);
      expect(t!.angle, 90);
      expect(t.distanceCm, 42);
    });

    test('trims surrounding whitespace and carriage returns', () {
      final t = TelemetryParser.parseLine('  A30D5\r');
      expect(t, isNotNull);
      expect(t!.angle, 30);
      expect(t.distanceCm, 5);
    });

    test('rejects malformed lines', () {
      expect(TelemetryParser.parseLine('garbage'), isNull);
      expect(TelemetryParser.parseLine('A90'), isNull);
      expect(TelemetryParser.parseLine('D42'), isNull);
      expect(TelemetryParser.parseLine('AxxDyy'), isNull);
      expect(TelemetryParser.parseLine(''), isNull);
    });

    test('rejects out-of-range angles', () {
      expect(TelemetryParser.parseLine('A200D10'), isNull);
    });
  });
}
