void usbCommunicationTask(const String& dataRecv) {
  String data = dataRecv;
  String dataHeader = usbSerial.getStrData(data, 0, "#");
  String dataValue = usbSerial.getStrData(data, 1, "#");
  if (isDigit(data[0]) || isDigit(data[1])) {
    // nums
  } else {
    dataHeader.toUpperCase();
    if (dataHeader == "R") ESP.restart();
    if (dataHeader == "BTN") var.buttonStatus = dataValue;  // BTN#W

    if (dataHeader == "D") var.sensorDebug = constrain(dataValue.toInt(), 0, 5);  // E#0
    if (dataHeader == "TESTTEMP") var.temperature = dataValue.toFloat();          // TESTTEMP#28.5
    if (dataHeader == "TESTHEIGHT") var.height = dataValue.toFloat();             // TESTHEIGHT#30
    if (dataHeader == "TESTTURBID") var.turbidity = dataValue.toFloat();          // TESTTURBID#0.5
    if (dataHeader == "TESTPH") var.ph = dataValue.toFloat();                     // TESTPH#7.5

    if (dataHeader == "SYSENA") {  // SYSENA
      var.systemEnable = !var.systemEnable;
      Serial.print("| var.systemEnable: ");
      Serial.print(var.systemEnable);
      Serial.println();
    }
    // D#4
    // if (dataHeader == "TURB") var.turbidity = (var.turbidity == 0.5) ? 1.5 : 0.5;      // no value
    if (dataHeader == "TURB") var.turbidity = dataValue.toInt();                       // no value
    if (dataHeader == "HEIG") var.height = dataValue.toFloat();                        // sensor
    if (dataHeader == "PEHA") var.ph = dataValue.toFloat();                            // 3.74, 6.89, 9.31
    if (dataHeader == "TEMP") var.temperature = dataValue.toFloat();                   // sensor
    if (dataHeader == "WAT1") var.waterLevel1 = (var.waterLevel1 == 0.5) ? 1.5 : 0.5;  // no value / ph up
    if (dataHeader == "WAT2") var.waterLevel2 = (var.waterLevel2 == 0.5) ? 1.5 : 0.5;  // no value / ph down
    if (dataHeader == "WAT3") var.waterLevel3 = (var.waterLevel3 == 0.5) ? 1.5 : 0.5;  // no value / garam

    if (dataHeader == "CALPH") calibratePhMeter();
    if (dataHeader == "LOADPH") loadCalibratePhMeter();
    if (dataHeader == "SETPH") var.ph = dataValue.toFloat();
    if (dataHeader == "CALWL1") wlCalibration.calibrateSensor();
    if (dataHeader == "LOADWL1") wlCalibration.loadCalibration();

    if (dataHeader == "TESTPUMP") waterPump.getState() ? waterPump.off() : waterPump.on();
    if (dataHeader == "TESTSOLENOID") waterSolenoid.getState() ? waterSolenoid.off() : waterSolenoid.on();
    if (dataHeader == "SERVOPHUP") servoPhUp.write(constrain(dataValue.toInt(), 0, 180));      // SERVOPHUP#140 | SERVOPHUP#60
    if (dataHeader == "SERVOPHDOWN") servoPhDown.write(constrain(dataValue.toInt(), 0, 180));  // SERVOPHDOWN#130 | SERVOSALT#45
    if (dataHeader == "SERVOSALT") servoSalt.write(constrain(dataValue.toInt(), 0, 180));      // SERVOSALT#90 | SERVOSALT#0
    if (dataHeader == "SERVOALL") {                                                            // SERVOALL#90
      servoPhUp.write(constrain(dataValue.toInt(), 0, 180));
      servoPhDown.write(constrain(dataValue.toInt(), 0, 180));
      servoSalt.write(constrain(dataValue.toInt(), 0, 180));
    }

    if (dataHeader == "TESTPHSERVO") var.ph = dataValue.toFloat();  // TESTPHSERVO#5.21 5.21, 7.43, 9.18
    if (dataHeader == "SERVOPHUPON") servoPhUp.write(60);
    if (dataHeader == "SERVOPHUPOFF") servoPhUp.write(140);
    if (dataHeader == "SERVOPHDOWNON") servoPhDown.write(45);
    if (dataHeader == "SERVOPHDOWNOFF") servoPhDown.write(130);
    if (dataHeader == "SERVOSALTON") servoSalt.write(0);
    if (dataHeader == "SERVOSALTOFF") servoSalt.write(90);

    if (dataHeader == "SYSWPUMPSTATE") {  // SYSWPUMPSTATE#1
      var.waterPumpState = dataValue.toInt();
      Serial.print("| var.waterPumpState: ");
      Serial.print(var.waterPumpState);
      Serial.println();
    }

    if (dataHeader == "SYSWPUMPTRIGGER") {  // SYSWPUMPTRIGGER
      var.waterPumpTrigger = !var.waterPumpTrigger;
      Serial.print("| var.waterPumpTrigger: ");
      Serial.print(var.waterPumpTrigger);
      Serial.println();
    }
  }
}