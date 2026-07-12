import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../state/robot_controller.dart';
import '../theme/app_theme.dart';

/// Speed control (0–9), sent to the firmware as a digit command.
class SpeedSlider extends StatelessWidget {
  const SpeedSlider({super.key});

  @override
  Widget build(BuildContext context) {
    final ctrl = context.watch<RobotController>();
    return Row(
      children: [
        const Icon(Icons.speed, size: 18, color: HudColors.textDim),
        const SizedBox(width: 8),
        Text('SPEED', style: hudMono.copyWith(fontSize: 11, color: HudColors.textDim)),
        Expanded(
          child: Slider(
            value: ctrl.speed.toDouble(),
            min: 0,
            max: 9,
            divisions: 9,
            label: '${ctrl.speed}',
            onChanged: (v) => ctrl.setSpeed(v.round()),
          ),
        ),
        SizedBox(
          width: 26,
          child: Text(
            '${ctrl.speed}',
            textAlign: TextAlign.right,
            style: hudMono.copyWith(color: HudColors.cyan, fontSize: 15),
          ),
        ),
      ],
    );
  }
}
