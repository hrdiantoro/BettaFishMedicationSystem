CalibrationData getCalibrationValueFromUser(int i) {
  Serial.println();
  CalibrationData cal;
  while (true) {
    float voltage = analogRead(32) * (3.3 / 4095.0);
    static uint32_t voltageTimer;
    if (millis() - voltageTimer >= 1000) {
      Serial.print("| voltage: ");
      Serial.print(voltage);
      Serial.print("| Masukan Data ke " + String(i + 1) + ": ");
      Serial.println();
      voltageTimer = millis();
    }
    if (Serial.available()) {
      float calibrationValueUser = Serial.readStringUntil('\n').toFloat();
      cal.voltage = voltage;
      cal.calibrationValue = calibrationValueUser;
      return cal;
    }
  }
  return cal;
}

void calibratePhMeter() {
  Serial.println("| Kalibrasi pH");
  Serial.print("| Masukan Angka Kalibrasi: ");
  int numPoints = 0;
  while (numPoints == 0) {
    if (Serial.available()) {
      numPoints = Serial.readStringUntil('\n').toInt();
    }
  }

  Serial.println(numPoints);
  calibrationDataArray = new CalibrationData[numPoints];
  calibrationDataCount = numPoints;

  for (int i = 0; i < numPoints; i++) {
    Serial.print("| Masukan Data ke " + String(i + 1) + ": ");
    CalibrationData cal = getCalibrationValueFromUser(i);
    calibrationDataArray[i].calibrationValue = cal.calibrationValue;
    calibrationDataArray[i].voltage = cal.voltage;
    Serial.print("| pH: ");
    Serial.print(calibrationDataArray[i].calibrationValue);
    Serial.print("| volt: ");
    Serial.print(calibrationDataArray[i].voltage);
    Serial.println();
  }

  preferences.begin("phCalibration", false);
  preferences.putUInt("numPoints", numPoints);
  for (int i = 0; i < numPoints; i++) {
    String phKey = "ph" + String(i);
    String voltKey = "volt" + String(i);
    preferences.putFloat(phKey.c_str(), calibrationDataArray[i].calibrationValue);
    preferences.putFloat(voltKey.c_str(), calibrationDataArray[i].voltage);
  }
  preferences.end();
  Serial.println("| Kalibrasi Selesai");

  delete[] calibrationDataArray;
  calibrationDataArray = nullptr;
}

void loadCalibratePhMeter() {
  preferences.begin("phCalibration", true);
  int numPoints = preferences.getUInt("numPoints", 0);
  calibrationDataCount = numPoints;
  if (numPoints > 0) {
    if (calibrationDataArray != nullptr) delete[] calibrationDataArray;
    calibrationDataArray = new CalibrationData[numPoints];
    Serial.print("| ph Load: ");
    Serial.println(calibrationDataCount);
    for (int i = 0; i < numPoints; i++) {
      String phKey = "ph" + String(i);
      String voltKey = "volt" + String(i);
      calibrationDataArray[i].calibrationValue = preferences.getFloat(phKey.c_str(), 0.0);
      calibrationDataArray[i].voltage = preferences.getFloat(voltKey.c_str(), 0.0);

      Serial.print("| index: ");
      Serial.print(i + 1);
      Serial.print("| calibrationValue: ");
      Serial.print(calibrationDataArray[i].calibrationValue);
      Serial.print("| voltage: ");
      Serial.print(calibrationDataArray[i].voltage);
      Serial.println();
    }
    Serial.print("| ESP.getFreeHeap(): ");
    Serial.print(ESP.getFreeHeap());
    Serial.println();
  }
  preferences.end();
}

float lagrangeInterpolation(float voltage) {
  float result = 0.0;
  for (int i = 0; i < calibrationDataCount; i++) {
    float term = calibrationDataArray[i].calibrationValue;
    for (int j = 0; j < calibrationDataCount; j++) {
      if (i != j) {
        term *= (voltage - calibrationDataArray[j].voltage) / (calibrationDataArray[i].voltage - calibrationDataArray[j].voltage);
      }
    }
    result += term;
  }
  return result;
}

float voltageToPH(float voltage) {
  if (calibrationDataCount < 2) return 0.0;
  if (voltage < calibrationDataArray[0].voltage) {
    return calibrationDataArray[0].calibrationValue;
  } else if (voltage > calibrationDataArray[calibrationDataCount - 1].voltage) {
    return calibrationDataArray[calibrationDataCount - 1].calibrationValue;
  }
  return lagrangeInterpolation(voltage);
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return (0);
  }
  time(&now);
  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  // Serial.print("| timeStr: ");
  // Serial.print(timeStr);
  // Serial.println();
  return now;
}