import 'package:speech_to_text/speech_to_text.dart';
import 'package:speech_to_text/speech_recognition_result.dart';

/// Maps recognised speech to robot command characters.
///
/// Pure logic, so the phrase table is unit-testable without a microphone.
class VoiceCommand {
  /// Returns the command char for a phrase, or `null` if nothing matches.
  static String? commandFor(String phrase) {
    final p = phrase.toLowerCase();
    // Order matters: check "stop" first so "stop going forward" stops.
    if (_has(p, ['stop', 'halt', 'brake', 'freeze'])) return 'S';
    if (_has(p, ['auto', 'avoid', 'obstacle', 'self drive', 'explore'])) return 'T';
    if (_has(p, ['forward', 'ahead', 'go', 'front'])) return 'F';
    if (_has(p, ['back', 'reverse', 'behind'])) return 'B';
    if (_has(p, ['left'])) return 'L';
    if (_has(p, ['right'])) return 'R';
    return null;
  }

  static bool _has(String s, List<String> keys) => keys.any(s.contains);
}

/// Thin wrapper around [SpeechToText] for the voice-control mode.
class VoiceService {
  final SpeechToText _speech = SpeechToText();
  bool _available = false;

  bool get isListening => _speech.isListening;

  Future<bool> init() async {
    _available = await _speech.initialize(
      onError: (_) {},
      onStatus: (_) {},
    );
    return _available;
  }

  /// Starts a listening session.
  ///
  /// [onWords] fires with partial + final transcripts, [onLevel] with the mic
  /// sound level (for the waveform), and [onCommand] once with the mapped
  /// command char when a final result arrives.
  Future<void> listen({
    required void Function(String words) onWords,
    required void Function(double level) onLevel,
    required void Function(String? command) onCommand,
  }) async {
    if (!_available) {
      _available = await init();
      if (!_available) return;
    }
    await _speech.listen(
      onResult: (SpeechRecognitionResult r) {
        onWords(r.recognizedWords);
        if (r.finalResult) {
          onCommand(VoiceCommand.commandFor(r.recognizedWords));
        }
      },
      onSoundLevelChange: onLevel,
      listenOptions: SpeechListenOptions(
        partialResults: true,
        listenFor: const Duration(seconds: 6),
        pauseFor: const Duration(seconds: 3),
      ),
    );
  }

  Future<void> stop() => _speech.stop();
}
