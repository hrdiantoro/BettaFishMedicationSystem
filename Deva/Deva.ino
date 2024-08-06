#include "Header.h"

void setup() {
  usbSerial.begin(&Serial, 115200);

  menu.initialize();
  menu.setLen(16, 2);
  auto initMenu = menu.createMenu(2, "  System  ", "Initialize");
  menu.showMenu(initMenu, true);
  menu.freeMenu(initMenu);

  task.initialize(wifiConTask);

  initServo();

  phCalibration.setAnalogConfig(32, 3.3, 4095);
  wlCalibration.setAnalogConfig(34, 3.3, 4095);

  phCalibration.loadCalibration();
  wlCalibration.loadCalibration();

  sensor.addModule("temp", new DS18B20Sens(26));
  sensor.addModule("sonar", new UltrasonicSens(33, 25, 200));
  sensor.addModule("ph", []() -> BaseSens* {
    auto ph = new AnalogSens(32, 3.3, 4095.0, [](JsonVariant value, int analogValue, float voltage) {
      // value["val"] = phCalibration.voltageToValue(voltage, LINEAR_INTERPOLATION);
      phFilter.addMeasurement(voltage);
      float filteredValue = phFilter.getFilteredValue();
      value["avg"] = filteredValue;
      value["val"] = regressPh(filteredValue);
    });
    return ph;
  });
  sensor.addModule("turbid", []() -> BaseSens* {
    auto turbid = new AnalogSens(35, 3.3, 4095.0, [](JsonVariant value, int analogValue, float voltage) {
      value["val"] = voltage < 1.2 ? "Keruh" : "Jernih";  // 1.50, 0.81
    });
    return turbid;
  });
  sensor.addModule("wl1", new AnalogSens(34, 3.3, 4095.0));
  sensor.addModule("wl2", new AnalogSens(39, 3.3, 4095.0));
  sensor.addModule("wl3", new AnalogSens(36, 3.3, 4095.0));
  sensor.init();

  waterPumpTimer.setDuration(10000);
  waterPumpTimer.reset();
  waterPumpTimer.start();

  phUpTimer.setDuration(15000);
  phUpTimer.reset();
  phUpTimer.start();

  servoPhUpTimer.setDuration(800);
  servoPhUpTimer.reset();
  servoPhUpTimer.start();

  phDownTimer.setDuration(15000);
  phDownTimer.reset();
  phDownTimer.start();

  servoPhDownTimer.setDuration(800);
  servoPhDownTimer.reset();
  servoPhDownTimer.start();

  saltTimer.setDuration(15000);
  saltTimer.reset();
  saltTimer.start();

  servosaltTimer.setDuration(800);
  servosaltTimer.reset();
  servosaltTimer.start();

  buttonI2C.pinMode(P1, INPUT_PULLUP);
  buttonI2C.pinMode(P2, INPUT_PULLUP);
  buttonI2C.begin();
  buttonDown.init(&buttonI2C);
  buttonOk.init(&buttonI2C);
  buttonDown.setDebounceTime(75);
  buttonOk.setDebounceTime(75);

  buzzer.toggleInit(100, 5);
}

void loop() {
  sensor.update([]() {
    debug();
    static uint32_t heightAverageTimer;
    if (millis() - heightAverageTimer >= 100) {
      var.height = sensor["sonar"];
      var.height = 26 - var.height;
      heightFilter.addMeasurement(var.height);
      var.height = round(heightFilter.getFilteredValue());
      var.height = var.height < 0 ? 0 : var.height;
      heightAverageTimer = millis();
    }
    var.turbidity = sensor["turbid"]["volt"];  // Turbid on / off
    var.ph = sensor["ph"]["val"];
    var.temperature = sensor["temp"];
    var.waterLevel1 = sensor["wl1"]["volt"];
    var.waterLevel2 = sensor["wl2"]["volt"];
    var.waterLevel3 = sensor["wl3"]["volt"];

    // var.phFirebase = var.ph + (var.ph * 0.018 * ((float)random(-100, 100) / 100.0));
    // var.turbidityFirebase = var.turbidity + (var.turbidity * 0.018 * ((float)random(-100, 100) / 100.0));
    // var.waterLevel1Firebase = var.waterLevel1 + (var.waterLevel1 * 0.018 * ((float)random(-100, 100) / 100.0));
    // var.waterLevel2Firebase = var.waterLevel2 + (var.waterLevel2 * 0.018 * ((float)random(-100, 100) / 100.0));
    // var.waterLevel3Firebase = var.waterLevel3 + (var.waterLevel3 * 0.018 * ((float)random(-100, 100) / 100.0));

    var.statusTurbidity = var.turbidity <= var.turbidityThreshold ? "Keruh" : "Jernih";
    var.statusWaterLevel1 = var.waterLevel1 <= var.waterLevelThreshold ? "Habis" : "Penuh";
    var.statusWaterLevel2 = var.waterLevel2 <= var.waterLevelThreshold ? "Habis" : "Penuh";
    var.statusWaterLevel3 = var.waterLevel3 <= var.waterLevelThreshold ? "Habis" : "Penuh";

    // var.waterLevel1 = wlCalibration.voltageToValue(var.waterLevel1, LINEAR_INTERPOLATION);
    // var.waterLevel2 = wlCalibration.voltageToValue(var.waterLevel2, LINEAR_INTERPOLATION);
    // var.waterLevel3 = wlCalibration.voltageToValue(var.waterLevel3, LINEAR_INTERPOLATION);
  });

  usbSerial.receive(usbCommunicationTask);

  if (buttonDown.isPressed() || buttonOk.isPressed()) {
    buzzer.on();
    buzzer.offDelay(75);
  }

  MenuCursor cursor{
    .up = buttonOk.isPressed(),
    .down = buttonDown.isPressed(),
    .select = false,
    .back = false,
    .show = true
  };
  menu.onListen(&cursor, menuTaskCallback);

  DigitalOut::updateAll(&buzzer, &waterPump, &waterSolenoid, DigitalOut::stop());
  DigitalInI2C::updateAll(&buttonDown, &buttonOk, DigitalInI2C::stop());

  if (!var.systemEnable || !var.isWiFiConnect) {
    waterPump.off();
    waterSolenoid.off();
    buzzer.off();
    var.waterPumpState = 0;
    return;
  }

  var.waterPumpTrigger = var.turbidity <= var.turbidityThreshold;
  var.phUpTrigger = var.ph < var.minPh;
  var.phDownTrigger = var.ph > var.maxPh;

  if (var.waterPumpState == 0) {
    if (waterPumpTimer.isExpired()) {
      if (var.waterPumpTrigger) {
        var.waterPumpState = 1;
        buzzer.off();
        buzzer.toggleInit(100, 2);
        return;
      }
    }
    /////////////////// HEIGHT ///////////////////
    // if (var.height < 14) {
    //   if (waterPumpTimer.isExpired()) {
    //     waterPump.on();
    //     waterSolenoid.off();
    //   }
    // } else if (var.height > 15) {
    //   if (waterPumpTimer.isExpired()) {
    //     waterPump.off();
    //     waterSolenoid.on();
    //   }
    // } else {
    //   waterPump.off();
    //   waterSolenoid.off();
    // }
    /////////////////// WATER LEVEL ///////////////////
    if (var.waterLevel1 <= var.waterLevelThreshold || var.waterLevel1 <= var.waterLevelThreshold || var.waterLevel1 <= var.waterLevelThreshold) {
      static uint32_t timerBuzzer;
      if (millis() - timerBuzzer >= 4000) {
        buzzer.toggleInit(75, 2);
        timerBuzzer = millis();
      }
    } else {
      buzzer.off();
    }
    /////////////////// PH ///////////////////
    if (phUpTimer.isExpired()) {
      if (var.phUpTrigger && var.phUpServoDegree == var.phUpServoOff) {
        var.phUpServoDegree = var.phUpServoOn;
        var.phUpState = 1;
        phUpTimer.reset();
        servoPhUpTimer.reset();
      }
    }
    if (servoPhUpTimer.isExpired() && var.phUpServoDegree == var.phUpServoOn) {
      var.phUpServoDegree = var.phUpServoOff;
      servoPhUpTimer.reset();
      phUpTimer.reset();
    }
    servoPhUp.write(var.phUpServoDegree);

    if (phDownTimer.isExpired()) {
      if (var.phDownTrigger && var.phDownServoDegree == var.phDownServoOff) {
        var.phDownServoDegree = var.phDownServoOn;
        var.phDownState = 1;
        servoPhDownTimer.reset();
        phDownTimer.reset();
      }
    }
    if (servoPhDownTimer.isExpired() && var.phDownServoDegree == var.phDownServoOn) {
      var.phDownServoDegree = var.phDownServoOff;
      servoPhDownTimer.reset();
      phDownTimer.reset();
    }
    servoPhDown.write(var.phDownServoDegree);
  } else if (var.waterPumpState == 1) {
    if (var.height <= var.minWaterLevel) {  // 8
      var.waterPumpState = 2;
      waterPump.off();
      waterSolenoid.off();
      buzzer.off();
      buzzer.toggleInit(100, 2);
      delay(3000);
      return;
    }
    waterPump.on();
    waterSolenoid.off();
  } else if (var.waterPumpState == 2) {
    if (var.height >= var.maxWaterLevel) {  // 16
      var.waterPumpState = 0;
      waterPump.off();
      waterSolenoid.off();
      buzzer.off();
      buzzer.toggleInit(100, 2);
      servoSalt.write(var.saltServoOn);
      var.saltState = 1;
      delay(3000);
      servoSalt.write(var.saltServoOff);
      waterPumpTimer.setDuration(30000);
      waterPumpTimer.reset();
      waterPumpTimer.start();
      return;
    }
    waterPump.off();
    waterSolenoid.on();
  }
}

void debug() {
  if (!sensor.isReady()) return;
  if (var.sensorDebug == 1) sensor.debug(500, false);
  else if (var.sensorDebug == 2) sensor.debug("wl1");
  else if (var.sensorDebug == 3) {
    static uint32_t debugTimer;
    if (millis() - debugTimer >= 500) {
      Serial.print("| wl1: ");
      Serial.print(sensor["wl1"]["volt"].as<float>());
      Serial.print("| var.waterLevel1: ");
      Serial.print(var.waterLevel1);
      Serial.println();
      debugTimer = millis();
    }
  } else if (var.sensorDebug == 4) {
    Serial.print("| ena: ");
    Serial.print(var.systemEnable);
    Serial.print("| state: ");
    Serial.print(var.waterPumpState);
    Serial.print("| turb: ");
    Serial.print(var.turbidity);
    Serial.print("| ");
    Serial.print(var.statusTurbidity);
    Serial.print("| H: ");
    Serial.print(var.height);
    Serial.print("| ph: ");
    Serial.print(var.ph);
    Serial.print("| temp: ");
    Serial.print(var.temperature);
    Serial.print("| wl1: ");
    Serial.print(var.waterLevel1 <= var.waterLevelThreshold ? "Habis" : "Penuh");
    Serial.print("| wl2: ");
    Serial.print(var.waterLevel2 <= var.waterLevelThreshold ? "Habis" : "Penuh");
    Serial.print("| wl3: ");
    Serial.print(var.waterLevel3 <= var.waterLevelThreshold ? "Habis" : "Penuh");

    Serial.print("| wState: ");
    Serial.print(var.waterPumpState);
    Serial.print("| wTrigger: ");
    Serial.print(var.waterPumpTrigger);

    Serial.print("| T: ");
    Serial.print(waterPumpTimer.getSeconds());
    Serial.println();
  } else if (var.sensorDebug == 5) {
    Serial.print("| mLevel: ");
    Serial.print(var.maxWaterLevel);
    Serial.print("| nLevel: ");
    Serial.print(var.minWaterLevel);
    Serial.print("| mTurb: ");
    Serial.print(var.maxTurbidity);
    Serial.print("| nTurb: ");
    Serial.print(var.minTurbidity);
    Serial.print("| mPh: ");
    Serial.print(var.maxPh);
    Serial.print("| nPh: ");
    Serial.print(var.minPh);
    Serial.print("| mTemp: ");
    Serial.print(var.maxTemperature);
    Serial.print("| nTemp: ");
    Serial.print(var.minTemperature);
    Serial.println();
  }
}

void initServo() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servoPhUp.setPeriodHertz(50);
  servoPhUp.attach(12, 500, 2500);
  servoPhUp.write(var.phUpServoOff);
  servoPhDown.setPeriodHertz(50);
  servoPhDown.attach(14, 500, 2500);
  servoPhDown.write(var.phDownServoOff);
  servoSalt.setPeriodHertz(50);
  servoSalt.attach(27, 500, 2500);
  servoSalt.write(var.saltServoOff);
}