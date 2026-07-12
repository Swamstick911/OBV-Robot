import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../state/robot_controller.dart';
import '../theme/app_theme.dart';

/// Animated segmented control for switching between the three modes.
class ModeSelector extends StatelessWidget {
  const ModeSelector({super.key});

  @override
  Widget build(BuildContext context) {
    final controller = context.watch<RobotController>();
    return Container(
      padding: const EdgeInsets.all(4),
      decoration: BoxDecoration(
        color: HudColors.panel,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: HudColors.panelBorder),
      ),
      child: Row(
        children: [
          _seg(controller, ControlMode.manual, Icons.gamepad, 'MANUAL'),
          _seg(controller, ControlMode.voice, Icons.mic, 'VOICE'),
          _seg(controller, ControlMode.auto, Icons.smart_toy, 'AUTO'),
        ],
      ),
    );
  }

  Widget _seg(RobotController ctrl, ControlMode m, IconData icon, String label) {
    final selected = ctrl.mode == m;
    return Expanded(
      child: GestureDetector(
        onTap: () => ctrl.setMode(m),
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 200),
          padding: const EdgeInsets.symmetric(vertical: 10),
          decoration: BoxDecoration(
            color: selected ? HudColors.cyan.withValues(alpha: 0.14) : Colors.transparent,
            borderRadius: BorderRadius.circular(9),
            border: Border.all(color: selected ? HudColors.cyan : Colors.transparent),
          ),
          child: Column(
            children: [
              Icon(icon, size: 20, color: selected ? HudColors.cyan : HudColors.textDim),
              const SizedBox(height: 3),
              Text(label, style: hudMono.copyWith(fontSize: 10, color: selected ? HudColors.cyan : HudColors.textDim)),
            ],
          ),
        ),
      ),
    );
  }
}
