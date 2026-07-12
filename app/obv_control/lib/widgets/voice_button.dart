import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:provider/provider.dart';

import '../services/voice_service.dart';
import '../state/robot_controller.dart';
import '../theme/app_theme.dart';

/// Voice-control widget: tap the mic, speak a command, watch it map to an action.
class VoiceButton extends StatefulWidget {
  const VoiceButton({super.key});

  @override
  State<VoiceButton> createState() => _VoiceButtonState();
}

class _VoiceButtonState extends State<VoiceButton> {
  final VoiceService _voice = VoiceService();
  bool _listening = false;
  String _words = '';
  String? _lastCmd;
  double _level = 0;

  Future<void> _toggle() async {
    final ctrl = context.read<RobotController>();
    if (_listening) {
      await _voice.stop();
      setState(() => _listening = false);
      return;
    }

    final mic = await Permission.microphone.request();
    if (!mic.isGranted) return;

    setState(() {
      _listening = true;
      _words = '';
      _lastCmd = null;
    });

    await _voice.listen(
      onWords: (w) => setState(() => _words = w),
      onLevel: (l) => setState(() => _level = l),
      onCommand: (cmd) {
        setState(() {
          _lastCmd = cmd;
          _listening = false;
        });
        if (cmd == null) return;
        if (cmd == 'T') {
          ctrl.engageAuto();
        } else if (cmd == 'S') {
          ctrl.stop();
        } else {
          ctrl.sendManual(cmd);
        }
      },
    );
  }

  @override
  void dispose() {
    _voice.stop();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final scale = 1.0 + (_level.clamp(0, 10) / 10) * 0.25;
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        GestureDetector(
          onTap: _toggle,
          child: AnimatedScale(
            scale: _listening ? scale : 1.0,
            duration: const Duration(milliseconds: 120),
            child: Container(
              width: 120,
              height: 120,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: _listening ? HudColors.cyan.withValues(alpha: 0.18) : HudColors.panel,
                border: Border.all(
                  color: _listening ? HudColors.cyan : HudColors.panelBorder,
                  width: 2,
                ),
                boxShadow: _listening
                    ? [const BoxShadow(color: HudColors.cyanGlow, blurRadius: 30, spreadRadius: 4)]
                    : null,
              ),
              child: Icon(
                _listening ? Icons.mic : Icons.mic_none,
                size: 52,
                color: _listening ? HudColors.cyan : HudColors.textDim,
              ),
            ),
          ),
        ),
        const SizedBox(height: 16),
        Text(
          _listening ? 'Listening…' : 'Tap to speak',
          style: hudMono.copyWith(color: HudColors.textDim, fontSize: 12),
        ),
        const SizedBox(height: 10),
        if (_words.isNotEmpty)
          Text(
            '"$_words"',
            textAlign: TextAlign.center,
            style: const TextStyle(color: HudColors.textPrimary, fontSize: 16),
          ),
        if (_lastCmd != null)
          Padding(
            padding: const EdgeInsets.only(top: 6),
            child: Text(
              '→ ${_cmdLabel(_lastCmd!)}',
              style: hudMono.copyWith(color: HudColors.cyan, fontSize: 13),
            ),
          ),
        const SizedBox(height: 14),
        Text(
          'Try: "forward" · "left" · "stop" · "auto"',
          textAlign: TextAlign.center,
          style: hudMono.copyWith(color: HudColors.textDim, fontSize: 10),
        ),
      ],
    );
  }

  String _cmdLabel(String c) => switch (c) {
        'F' => 'FORWARD',
        'B' => 'BACKWARD',
        'L' => 'LEFT',
        'R' => 'RIGHT',
        'S' => 'STOP',
        'T' => 'AUTO MODE',
        _ => c,
      };
}
