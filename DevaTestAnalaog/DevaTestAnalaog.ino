const int numSensor = 5;
String name[numSensor] = { "ph    ", "turbid", "wl1   ", "wl2   ", "wl3   " };
int pinAnalog[numSensor] = { 32, 35, 34, 39, 36 };
int sensorIndex = 0;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < numSensor; i++) {
    pinMode(pinAnalog[i], INPUT);
  }
}

void loop() {
  static uint32_t sensorTimer;
  if (millis() - sensorTimer >= 1000) {
    sensorIndex++;
    if (sensorIndex >= 5) sensorIndex = 0;

    int adcRaw = analogRead(pinAnalog[sensorIndex]);
    String sensorName = name[sensorIndex];

    Serial.print("| i: ");
    Serial.print(sensorIndex);
    Serial.print("| sensorName: ");
    Serial.print(sensorName);
    Serial.print("| adcRaw: ");
    Serial.print(adcRaw);
    Serial.println();

    sensorTimer = millis();
  }
}
