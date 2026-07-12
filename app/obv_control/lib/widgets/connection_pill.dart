import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../services/bluetooth_service.dart';
import '../state/robot_controller.dart';
import '../theme/app_theme.dart';

/// Compact connection-status chip; tap to open the pairing screen.
class ConnectionPill extends StatelessWidget {
  final VoidCallback onTap;
  const ConnectionPill({super.key, required this.onTap});

  @override
  Widget build(BuildContext context) {
    final ctrl = context.watch<RobotController>();
    final (color, label, icon) = switch (ctrl.connection) {
      RobotConnectionState.connected => (HudColors.green, 'ONLINE', Icons.bluetooth_connected),
      RobotConnectionState.connecting => (HudColors.amber, 'LINKING', Icons.bluetooth_searching),
      RobotConnectionState.disconnected => (HudColors.red, 'OFFLINE', Icons.bluetooth_disabled),
    };

    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        decoration: BoxDecoration(
          color: HudColors.panel,
          borderRadius: BorderRadius.circular(20),
          border: Border.all(color: color.withValues(alpha: 0.6)),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(icon, size: 16, color: color),
            const SizedBox(width: 6),
            Text(label, style: hudMono.copyWith(color: color, fontSize: 11)),
          ],
        ),
      ),
    );
  }
}
