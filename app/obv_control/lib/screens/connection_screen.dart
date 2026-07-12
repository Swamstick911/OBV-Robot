import 'package:flutter/material.dart';
import 'package:flutter_blue_classic/flutter_blue_classic.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:provider/provider.dart';

import '../state/robot_controller.dart';
import '../theme/app_theme.dart';

/// Lists paired Bluetooth devices and connects to the chosen one (the HC-05).
class ConnectionScreen extends StatefulWidget {
  const ConnectionScreen({super.key});

  @override
  State<ConnectionScreen> createState() => _ConnectionScreenState();
}

class _ConnectionScreenState extends State<ConnectionScreen> {
  List<BluetoothDevice> _devices = [];
  bool _loading = false;
  String? _connectingAddress;

  @override
  void initState() {
    super.initState();
    _refresh();
  }

  Future<void> _refresh() async {
    setState(() => _loading = true);
    final ctrl = context.read<RobotController>();
    await [
      Permission.bluetoothConnect,
      Permission.bluetoothScan,
      Permission.locationWhenInUse,
    ].request();
    try {
      final devices = await ctrl.bt.pairedDevices();
      if (mounted) setState(() => _devices = devices);
    } catch (_) {}
    if (mounted) setState(() => _loading = false);
  }

  Future<void> _connect(BluetoothDevice d) async {
    final ctrl = context.read<RobotController>();
    final messenger = ScaffoldMessenger.of(context);
    final navigator = Navigator.of(context);
    setState(() => _connectingAddress = d.address);
    final ok = await ctrl.bt.connect(d.address);
    if (!mounted) return;
    setState(() => _connectingAddress = null);
    if (ok) {
      navigator.pop();
    } else {
      messenger.showSnackBar(
        const SnackBar(content: Text('Connection failed. Make sure the robot is powered on.')),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: HudColors.bg,
        title: Text('PAIR ROBOT', style: hudMono.copyWith(fontSize: 16, color: HudColors.cyan)),
        actions: [
          IconButton(
            onPressed: _loading ? null : _refresh,
            icon: const Icon(Icons.refresh, color: HudColors.cyan),
          ),
        ],
      ),
      body: SafeArea(
        child: _loading
            ? const Center(child: CircularProgressIndicator(color: HudColors.cyan))
            : _devices.isEmpty
                ? _empty()
                : ListView.builder(
                    padding: const EdgeInsets.all(12),
                    itemCount: _devices.length,
                    itemBuilder: (_, i) => _tile(_devices[i]),
                  ),
      ),
    );
  }

  Widget _empty() => Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Icon(Icons.bluetooth_disabled, size: 48, color: HudColors.textDim),
              const SizedBox(height: 12),
              Text(
                'No paired devices.\n\nPair the HC-05 in Android\nBluetooth settings first\n(PIN 1234 or 0000), then refresh.',
                textAlign: TextAlign.center,
                style: hudMono.copyWith(color: HudColors.textDim, fontSize: 12, height: 1.5),
              ),
            ],
          ),
        ),
      );

  Widget _tile(BluetoothDevice d) {
    final connecting = _connectingAddress == d.address;
    return Container(
      margin: const EdgeInsets.only(bottom: 10),
      decoration: BoxDecoration(
        color: HudColors.panel,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: HudColors.panelBorder),
      ),
      child: ListTile(
        leading: const Icon(Icons.bluetooth, color: HudColors.cyan),
        title: Text(d.name ?? 'Unknown device', style: const TextStyle(color: HudColors.textPrimary)),
        subtitle: Text(d.address, style: hudMono.copyWith(color: HudColors.textDim, fontSize: 11)),
        trailing: connecting
            ? const SizedBox(
                width: 20,
                height: 20,
                child: CircularProgressIndicator(strokeWidth: 2, color: HudColors.cyan),
              )
            : const Icon(Icons.chevron_right, color: HudColors.textDim),
        onTap: connecting ? null : () => _connect(d),
      ),
    );
  }
}
