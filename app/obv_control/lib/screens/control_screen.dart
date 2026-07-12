import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../state/robot_controller.dart';
import '../theme/app_theme.dart';
import '../widgets/connection_pill.dart';
import '../widgets/dpad.dart';
import '../widgets/mode_selector.dart';
import '../widgets/radar_view.dart';
import '../widgets/speed_slider.dart';
import '../widgets/voice_button.dart';
import 'connection_screen.dart';

/// The main cockpit: header + radar + mode selector + the active mode's controls.
class ControlScreen extends StatelessWidget {
  const ControlScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final ctrl = context.watch<RobotController>();
    return Scaffold(
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(14),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Row(
                children: [
                  Text('OBV',
                      style: hudMono.copyWith(
                          fontSize: 20, color: HudColors.cyan, fontWeight: FontWeight.bold)),
                  Text(' ROBOT', style: hudMono.copyWith(fontSize: 20, color: HudColors.textPrimary)),
                  const Spacer(),
                  ConnectionPill(onTap: () => _openConnection(context)),
                ],
              ),
              const SizedBox(height: 12),
              const RadarView(),
              const SizedBox(height: 14),
              const ModeSelector(),
              const SizedBox(height: 14),
              Expanded(child: _modeBody(ctrl)),
              if (!ctrl.isConnected)
                Padding(
                  padding: const EdgeInsets.only(top: 8),
                  child: _banner(context),
                ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _modeBody(RobotController ctrl) {
    switch (ctrl.mode) {
      case ControlMode.manual:
        return const _ManualBody();
      case ControlMode.voice:
        return const Center(child: VoiceButton());
      case ControlMode.auto:
        return const _AutoBody();
    }
  }

  Widget _banner(BuildContext context) {
    return GestureDetector(
      onTap: () => _openConnection(context),
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 10),
        decoration: BoxDecoration(
          color: HudColors.red.withValues(alpha: 0.12),
          borderRadius: BorderRadius.circular(10),
          border: Border.all(color: HudColors.red.withValues(alpha: 0.5)),
        ),
        child: Text(
          'Not connected — tap to pair with the HC-05',
          textAlign: TextAlign.center,
          style: hudMono.copyWith(color: HudColors.red, fontSize: 12),
        ),
      ),
    );
  }

  void _openConnection(BuildContext context) {
    Navigator.of(context).push(MaterialPageRoute(builder: (_) => const ConnectionScreen()));
  }
}

class _ManualBody extends StatelessWidget {
  const _ManualBody();

  @override
  Widget build(BuildContext context) {
    return const Column(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Expanded(child: Center(child: DPad())),
        Padding(padding: EdgeInsets.only(bottom: 4), child: SpeedSlider()),
      ],
    );
  }
}

class _AutoBody extends StatelessWidget {
  const _AutoBody();

  @override
  Widget build(BuildContext context) {
    final ctrl = context.watch<RobotController>();
    final active = ctrl.autoEngaged;
    final color = active ? HudColors.red : HudColors.cyan;
    return Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(
            active ? 'AUTONOMOUS' : 'STANDBY',
            style: hudMono.copyWith(
                color: active ? HudColors.green : HudColors.textDim, fontSize: 14, letterSpacing: 2),
          ),
          const SizedBox(height: 20),
          GestureDetector(
            onTap: () => active ? ctrl.stop() : ctrl.engageAuto(),
            child: Container(
              width: 160,
              height: 160,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: color.withValues(alpha: 0.14),
                border: Border.all(color: color, width: 2),
                boxShadow: [BoxShadow(color: color.withValues(alpha: 0.35), blurRadius: 24, spreadRadius: 2)],
              ),
              child: Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(active ? Icons.stop : Icons.play_arrow, size: 52, color: color),
                    Text(active ? 'STOP' : 'ENGAGE', style: hudMono.copyWith(color: color, fontSize: 14)),
                  ],
                ),
              ),
            ),
          ),
          const SizedBox(height: 20),
          Text(
            'Robot drives itself and avoids obstacles',
            textAlign: TextAlign.center,
            style: hudMono.copyWith(color: HudColors.textDim, fontSize: 11),
          ),
        ],
      ),
    );
  }
}
