import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../state/robot_controller.dart';
import '../theme/app_theme.dart';
import 'radar_painter.dart';

/// The sonar panel: header readout + the live radar sweep.
class RadarView extends StatelessWidget {
  const RadarView({super.key});

  @override
  Widget build(BuildContext context) {
    final controller = context.watch<RobotController>();
    final latest = controller.latest;
    final near = latest != null && latest.distanceCm < 30;

    return Container(
      decoration: BoxDecoration(
        color: HudColors.panel,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: HudColors.panelBorder),
      ),
      padding: const EdgeInsets.fromLTRB(12, 12, 12, 10),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Row(
            children: [
              const Icon(Icons.radar, size: 16, color: HudColors.cyan),
              const SizedBox(width: 6),
              Text('SONAR', style: hudMono.copyWith(color: HudColors.textDim, fontSize: 12)),
              const Spacer(),
              Text(
                latest == null ? '--- cm' : '${latest.distanceCm.toString().padLeft(3)} cm',
                style: hudMono.copyWith(color: near ? HudColors.red : HudColors.cyan, fontSize: 14),
              ),
            ],
          ),
          const SizedBox(height: 8),
          AspectRatio(
            aspectRatio: 2,
            child: CustomPaint(
              painter: RadarPainter(
                points: controller.radarPoints,
                sweepAngle: latest?.angle,
              ),
              size: Size.infinite,
            ),
          ),
        ],
      ),
    );
  }
}
