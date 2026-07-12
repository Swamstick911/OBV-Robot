import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';

import '../state/robot_controller.dart';
import '../theme/app_theme.dart';

/// Directional pad for manual driving. Holding a button drives; releasing stops.
class DPad extends StatelessWidget {
  const DPad({super.key});

  @override
  Widget build(BuildContext context) {
    final ctrl = context.read<RobotController>();
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        _dir(ctrl, 'F', Icons.keyboard_arrow_up),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            _dir(ctrl, 'L', Icons.keyboard_arrow_left),
            _stop(ctrl),
            _dir(ctrl, 'R', Icons.keyboard_arrow_right),
          ],
        ),
        _dir(ctrl, 'B', Icons.keyboard_arrow_down),
      ],
    );
  }

  Widget _dir(RobotController ctrl, String cmd, IconData icon) {
    return Padding(
      padding: const EdgeInsets.all(6),
      child: _HoldButton(
        onDown: () {
          HapticFeedback.selectionClick();
          ctrl.sendManual(cmd);
        },
        onUp: ctrl.stop,
        child: Icon(icon, size: 34, color: HudColors.cyan),
      ),
    );
  }

  Widget _stop(RobotController ctrl) {
    return Padding(
      padding: const EdgeInsets.all(6),
      child: _HoldButton(
        color: HudColors.red,
        onDown: () {
          HapticFeedback.mediumImpact();
          ctrl.stop();
        },
        onUp: () {},
        child: Text('STOP',
            style: hudMono.copyWith(color: HudColors.red, fontSize: 12, fontWeight: FontWeight.bold)),
      ),
    );
  }
}

class _HoldButton extends StatefulWidget {
  final VoidCallback onDown;
  final VoidCallback onUp;
  final Widget child;
  final Color color;

  const _HoldButton({
    required this.onDown,
    required this.onUp,
    required this.child,
    this.color = HudColors.cyan,
  });

  @override
  State<_HoldButton> createState() => _HoldButtonState();
}

class _HoldButtonState extends State<_HoldButton> {
  bool _pressed = false;

  void _set(bool v) => setState(() => _pressed = v);

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: (_) {
        _set(true);
        widget.onDown();
      },
      onTapUp: (_) {
        _set(false);
        widget.onUp();
      },
      onTapCancel: () {
        _set(false);
        widget.onUp();
      },
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 90),
        width: 74,
        height: 74,
        decoration: BoxDecoration(
          color: _pressed ? widget.color.withValues(alpha: 0.18) : HudColors.panel,
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: _pressed ? widget.color : HudColors.panelBorder, width: 1.5),
          boxShadow: _pressed
              ? [BoxShadow(color: widget.color.withValues(alpha: 0.4), blurRadius: 14)]
              : null,
        ),
        child: Center(child: widget.child),
      ),
    );
  }
}
