import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../models/telemetry.dart';
import '../theme/app_theme.dart';

/// Paints a semicircular sonar display fed by real ultrasonic telemetry.
///
/// Angle mapping: 90° = straight up (front), 0° = right, 180° = left.
class RadarPainter extends CustomPainter {
  final List<Telemetry> points;
  final int? sweepAngle;
  final double maxRangeCm;
  final int thresholdCm;

  RadarPainter({
    required this.points,
    required this.sweepAngle,
    this.maxRangeCm = 120,
    this.thresholdCm = 30,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final cx = size.width / 2;
    final cy = size.height;
    final radius = math.min(size.width / 2, size.height) * 0.98;
    final center = Offset(cx, cy);

    Offset project(double angleDeg, double distanceCm) {
      final r = (distanceCm.clamp(0, maxRangeCm) / maxRangeCm) * radius;
      final rad = angleDeg * math.pi / 180.0;
      return Offset(cx + r * math.cos(rad), cy - r * math.sin(rad));
    }

    // Background wedge.
    final wedge = Path()
      ..moveTo(cx, cy)
      ..arcTo(Rect.fromCircle(center: center, radius: radius), math.pi, math.pi, false)
      ..close();
    canvas.drawPath(wedge, Paint()..color = HudColors.bgElevated);

    // Range rings + cm labels.
    final ringPaint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1
      ..color = HudColors.cyanDim.withValues(alpha: 0.5);
    final tp = TextPainter(textDirection: TextDirection.ltr);
    for (int i = 1; i <= 4; i++) {
      final rr = radius * i / 4;
      canvas.drawArc(Rect.fromCircle(center: center, radius: rr), math.pi, math.pi, false, ringPaint);
      final cm = (maxRangeCm * i / 4).round();
      tp.text = TextSpan(text: '$cm', style: hudMono.copyWith(color: HudColors.textDim, fontSize: 9));
      tp.layout();
      tp.paint(canvas, Offset(cx + 3, cy - rr - 12));
    }

    // Angle spokes.
    final spokePaint = Paint()
      ..color = HudColors.cyanDim.withValues(alpha: 0.35)
      ..strokeWidth = 1;
    for (final a in [0, 45, 90, 135, 180]) {
      canvas.drawLine(center, project(a.toDouble(), maxRangeCm), spokePaint);
    }

    // Sweep line following the live servo angle.
    if (sweepAngle != null) {
      final end = project(sweepAngle!.toDouble(), maxRangeCm);
      final sweepPaint = Paint()
        ..shader = LinearGradient(
          colors: [HudColors.cyan.withValues(alpha: 0.0), HudColors.cyan],
        ).createShader(Rect.fromPoints(center, end))
        ..strokeWidth = 2;
      canvas.drawLine(center, end, sweepPaint);
    }

    // Blips — heat up from cyan → amber → red as obstacles get closer.
    for (final t in points) {
      if (t.distanceCm <= 0 || t.distanceCm > maxRangeCm) continue;
      final p = project(t.angle.toDouble(), t.distanceCm.toDouble());
      final color = t.distanceCm < thresholdCm
          ? HudColors.red
          : (t.distanceCm < thresholdCm * 2 ? HudColors.amber : HudColors.cyan);
      canvas.drawCircle(
        p,
        7,
        Paint()
          ..color = color.withValues(alpha: 0.25)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
      );
      canvas.drawCircle(p, 3.2, Paint()..color = color);
    }

    canvas.drawCircle(center, 4, Paint()..color = HudColors.cyan);
  }

  @override
  bool shouldRepaint(covariant RadarPainter old) => true;
}
