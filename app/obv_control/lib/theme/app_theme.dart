import 'package:flutter/material.dart';

/// "Sonar HUD" palette — a tactical, dark command-console aesthetic.
class HudColors {
  static const bg = Color(0xFF080B11);
  static const bgElevated = Color(0xFF0E141D);
  static const panel = Color(0xFF121A24);
  static const panelBorder = Color(0xFF1E2C3A);

  static const cyan = Color(0xFF22E5C7);
  static const cyanGlow = Color(0x5522E5C7);
  static const cyanDim = Color(0xFF0F5A52);

  static const amber = Color(0xFFFFB020);
  static const red = Color(0xFFFF4D5E);
  static const green = Color(0xFF3DDC84);

  static const textPrimary = Color(0xFFE6F1F5);
  static const textDim = Color(0xFF6B8299);
}

/// Monospace style for instrument-panel readouts (distances, angles).
const TextStyle hudMono = TextStyle(
  fontFamily: 'monospace',
  fontFeatures: [FontFeature.tabularFigures()],
  letterSpacing: 1.0,
);

ThemeData buildHudTheme() {
  final base = ThemeData.dark(useMaterial3: true);
  return base.copyWith(
    scaffoldBackgroundColor: HudColors.bg,
    colorScheme: base.colorScheme.copyWith(
      primary: HudColors.cyan,
      secondary: HudColors.amber,
      surface: HudColors.panel,
      error: HudColors.red,
      onPrimary: HudColors.bg,
      onSurface: HudColors.textPrimary,
    ),
    textTheme: base.textTheme.apply(
      bodyColor: HudColors.textPrimary,
      displayColor: HudColors.textPrimary,
    ),
    sliderTheme: base.sliderTheme.copyWith(
      activeTrackColor: HudColors.cyan,
      inactiveTrackColor: HudColors.panelBorder,
      thumbColor: HudColors.cyan,
      overlayColor: HudColors.cyanGlow,
    ),
  );
}
