import 'dart:async';

import 'package:flutter/foundation.dart';

import '../models/telemetry.dart';
import '../services/bluetooth_service.dart';

enum ControlMode { manual, voice, auto }

/// The app's central state, exposed to the UI via Provider.
///
/// Holds the current mode, speed, connection state and the latest telemetry,
/// and forwards user intent to the [RobotBluetoothService].
class RobotController extends ChangeNotifier {
  final RobotBluetoothService bt;

  late final StreamSubscription _telemetrySub;
  late final StreamSubscription _stateSub;

  RobotConnectionState connection = RobotConnectionState.disconnected;
  ControlMode mode = ControlMode.manual;

  /// Speed level 0..9 (mapped to 0..255 on the firmware).
  int speed = 6;

  /// True while the robot is in autonomous obstacle-avoidance mode.
  bool autoEngaged = false;

  /// Last manual command sent (for button highlight).
  String? lastCommand;

  // Radar data: latest reading per angle, plus the most recent overall.
  final Map<int, Telemetry> _byAngle = {};
  Telemetry? latest;

  RobotController(this.bt) {
    _telemetrySub = bt.telemetry.listen(_onTelemetry);
    _stateSub = bt.connectionState.listen(_onState);
  }

  bool get isConnected => connection == RobotConnectionState.connected;

  List<Telemetry> get radarPoints => _byAngle.values.toList(growable: false);

  void _onState(RobotConnectionState s) {
    connection = s;
    if (s != RobotConnectionState.connected) {
      _byAngle.clear();
      latest = null;
      autoEngaged = false;
      lastCommand = null;
    }
    notifyListeners();
  }

  void _onTelemetry(Telemetry t) {
    _byAngle[t.angle] = t;
    latest = t;
    notifyListeners();
  }

  void setMode(ControlMode m) {
    if (mode == m) return;
    stop(); // safety: never leave the robot driving when switching modes
    mode = m;
    notifyListeners();
  }

  void setSpeed(int level) {
    speed = level.clamp(0, 9);
    bt.sendCommand(speed.toString());
    notifyListeners();
  }

  /// Sends a manual drive command (`F`/`B`/`L`/`R`).
  void sendManual(String cmd) {
    lastCommand = cmd;
    autoEngaged = false;
    bt.sendCommand(cmd);
    notifyListeners();
  }

  void engageAuto() {
    autoEngaged = true;
    bt.sendCommand('T');
    notifyListeners();
  }

  void stop() {
    lastCommand = 'S';
    autoEngaged = false;
    bt.sendCommand('S');
    notifyListeners();
  }

  @override
  void dispose() {
    _telemetrySub.cancel();
    _stateSub.cancel();
    super.dispose();
  }
}
