import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_blue_classic/flutter_blue_classic.dart';

import '../models/telemetry.dart';
import 'telemetry_parser.dart';

enum RobotConnectionState { disconnected, connecting, connected }

/// Owns the Bluetooth Classic (SPP) link to the HC-05.
///
/// Nothing else in the app talks to Bluetooth directly. It exposes:
///  - [sendCommand] to write a single command character to the robot,
///  - [telemetry] a stream of parsed [Telemetry] readings,
///  - [connectionState] a stream of link state changes.
class RobotBluetoothService {
  final FlutterBlueClassic _fbc = FlutterBlueClassic();

  BluetoothConnection? _connection;
  StreamSubscription<Uint8List>? _inputSub;
  String _buffer = '';

  final _telemetryController = StreamController<Telemetry>.broadcast();
  final _stateController = StreamController<RobotConnectionState>.broadcast();

  Stream<Telemetry> get telemetry => _telemetryController.stream;
  Stream<RobotConnectionState> get connectionState => _stateController.stream;

  bool get isConnected => _connection?.isConnected ?? false;

  /// Paired ("bonded") devices — the HC-05 shows up here once paired in
  /// Android's Bluetooth settings.
  Future<List<BluetoothDevice>> pairedDevices() async {
    final devices = await _fbc.bondedDevices;
    return devices ?? const [];
  }

  Future<BluetoothAdapterState> adapterState() => _fbc.adapterStateNow;

  /// Asks the OS to turn Bluetooth on if it's off.
  Future<bool> enableBluetooth() => _fbc.turnOn();

  /// Connects to a device by MAC address and starts listening for telemetry.
  Future<bool> connect(String address) async {
    _stateController.add(RobotConnectionState.connecting);
    try {
      final conn = await _fbc.connect(address);
      if (conn == null || !conn.isConnected) {
        _stateController.add(RobotConnectionState.disconnected);
        return false;
      }
      _connection = conn;
      _buffer = '';
      _inputSub = conn.input?.listen(
        _onData,
        onDone: _handleDisconnect,
        onError: (_) => _handleDisconnect(),
        cancelOnError: true,
      );
      _stateController.add(RobotConnectionState.connected);
      return true;
    } catch (_) {
      _stateController.add(RobotConnectionState.disconnected);
      return false;
    }
  }

  /// Buffers incoming bytes and emits one [Telemetry] per complete `\n` line.
  void _onData(Uint8List data) {
    _buffer += utf8.decode(data, allowMalformed: true);

    int newline;
    while ((newline = _buffer.indexOf('\n')) != -1) {
      final line = _buffer.substring(0, newline);
      _buffer = _buffer.substring(newline + 1);
      final reading = TelemetryParser.parseLine(line);
      if (reading != null) _telemetryController.add(reading);
    }

    // Guard against unbounded growth if we only ever receive garbage.
    if (_buffer.length > 256) _buffer = '';
  }

  /// Sends a single command character (e.g. `F`, `S`, `T`, `5`) to the robot.
  void sendCommand(String command) {
    final conn = _connection;
    if (conn != null && conn.isConnected) {
      conn.writeString(command);
    }
  }

  void _handleDisconnect() {
    _inputSub?.cancel();
    _inputSub = null;
    _connection?.dispose();
    _connection = null;
    if (!_stateController.isClosed) {
      _stateController.add(RobotConnectionState.disconnected);
    }
  }

  Future<void> disconnect() async {
    _inputSub?.cancel();
    _inputSub = null;
    try {
      await _connection?.finish();
    } catch (_) {}
    _connection?.dispose();
    _connection = null;
    if (!_stateController.isClosed) {
      _stateController.add(RobotConnectionState.disconnected);
    }
  }

  void dispose() {
    _inputSub?.cancel();
    _connection?.dispose();
    _telemetryController.close();
    _stateController.close();
  }
}
