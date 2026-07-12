import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';

import 'screens/control_screen.dart';
import 'services/bluetooth_service.dart';
import 'state/robot_controller.dart';
import 'theme/app_theme.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setPreferredOrientations([DeviceOrientation.portraitUp]);
  runApp(OBVApp(bt: RobotBluetoothService()));
}

class OBVApp extends StatefulWidget {
  final RobotBluetoothService bt;
  const OBVApp({super.key, required this.bt});

  @override
  State<OBVApp> createState() => _OBVAppState();
}

class _OBVAppState extends State<OBVApp> with WidgetsBindingObserver {
  late final RobotController _controller;

  @override
  void initState() {
    super.initState();
    _controller = RobotController(widget.bt);
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    // Safety: never leave the robot driving when the app leaves the foreground.
    if (state == AppLifecycleState.paused || state == AppLifecycleState.inactive) {
      _controller.stop();
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _controller.dispose();
    widget.bt.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider.value(
      value: _controller,
      child: MaterialApp(
        title: 'OBV Robot',
        debugShowCheckedModeBanner: false,
        theme: buildHudTheme(),
        home: const ControlScreen(),
      ),
    );
  }
}
