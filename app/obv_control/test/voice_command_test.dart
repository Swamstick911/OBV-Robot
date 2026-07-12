import 'package:flutter_test/flutter_test.dart';
import 'package:obv_control/services/voice_service.dart';

void main() {
  group('VoiceCommand.commandFor', () {
    test('maps directional phrases', () {
      expect(VoiceCommand.commandFor('go forward'), 'F');
      expect(VoiceCommand.commandFor('move ahead'), 'F');
      expect(VoiceCommand.commandFor('turn left'), 'L');
      expect(VoiceCommand.commandFor('turn right'), 'R');
      expect(VoiceCommand.commandFor('reverse'), 'B');
    });

    test('stop wins over other words', () {
      expect(VoiceCommand.commandFor('stop going forward'), 'S');
      expect(VoiceCommand.commandFor('halt'), 'S');
    });

    test('maps auto/avoid phrases to T', () {
      expect(VoiceCommand.commandFor('auto mode'), 'T');
      expect(VoiceCommand.commandFor('avoid obstacles'), 'T');
    });

    test('is case-insensitive', () {
      expect(VoiceCommand.commandFor('LEFT'), 'L');
    });

    test('returns null for unrecognised phrases', () {
      expect(VoiceCommand.commandFor('what time is it'), isNull);
    });
  });
}
