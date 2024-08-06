#define ENABLE_MODULE_FIREBASE_HANDLER
#define ENABLE_MODULE_TASK_HANDLER
#define ENABLE_MODULE_TIMER_DURATION
#define ENABLE_MODULE_TIMER_TASK
#define ENABLE_MODULE_SERIAL_HARD
#define ENABLE_MODULE_SERIAL_SWAP
#define ENABLE_MODULE_DIGITAL_INPUT
#define ENABLE_MODULE_DIGITAL_OUTPUT
#define ENABLE_MODULE_LCD_MENU
#define ENABLE_MODULE_DATETIME_NTP

#define ENABLE_SENSOR_MODULE
#define ENABLE_SENSOR_MODULE_UTILITY
#define ENABLE_SENSOR_DS18B20
#define ENABLE_SENSOR_ULTRASONIC
#define ENABLE_SENSOR_ANALOG

#include "Kinematrix.h"
#include "Preferences.h"
// #include "WiFiManager.h"
#include "ESP32Servo.h"
#include "PCF8574.h"

////////// Utility //////////
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;  // Offset for WIB (UTC+7)
const int daylightOffset_sec = 0;

// WiFiManager wm;
TaskHandle task;
FirebaseModule firebase;
FirebaseAuthentication auth;
TimerTask firebaseTimer(20000);
Preferences preferences;

TimerDuration waterPumpTimer;
TimerDuration phUpTimer;
TimerDuration servoPhUpTimer;
TimerDuration phDownTimer;
TimerDuration servoPhDownTimer;
TimerDuration saltTimer;
TimerDuration servosaltTimer;

////////// Sensor //////////
SensorModule sensor;
AnalogCalibration phCalibration("phCalibration", &preferences);
AnalogCalibration wlCalibration("wlCalibration", &preferences);
MovingAverageFilter phFilter(100);
MovingAverageFilter heightFilter(30);

////////// Communication //////////
HardSerial usbSerial;

////////// Input Module //////////
PCF8574 buttonI2C(0x20);
DigitalInI2C buttonDown(P1);
DigitalInI2C buttonOk(P2);

////////// Output Module //////////
LcdMenu menu(0x27, 16, 2);
DigitalOut buzzer(4);
DigitalOut waterPump(18, true);
DigitalOut waterSolenoid(19, true);

Servo servoPhUp;
Servo servoPhDown;
Servo servoSalt;

////////// Global Variable //////////
struct CalibrationData {
  float voltage;
  float calibrationValue;
};

CalibrationData *calibrationDataArray = nullptr;
int calibrationDataCount = 0;

struct SystemData {
  String statusTurbidity;
  String statusWaterLevel1;
  String statusWaterLevel2;
  String statusWaterLevel3;

  const float turbidityThreshold = 0.78;
  const float waterLevelThreshold = 0.5;

  float turbidity = 0.5;
  float height = 0.0;
  float ph = 0.0;
  float temperature = 0.0;
  float waterLevel1 = 0.5;
  float waterLevel2 = 0.5;
  float waterLevel3 = 0.5;

  float turbidityFirebase;
  float heightFirebase;
  float phFirebase;
  float temperatureFirebase;
  float waterLevel1Firebase;
  float waterLevel2Firebase;
  float waterLevel3Firebase;

  int sensorDebug;
  String buttonStatus = "";

  float maxWaterLevel;
  float maxTurbidity;
  float maxPh;
  float maxTemperature;

  float minWaterLevel;
  float minTurbidity;
  float minPh;
  float minTemperature;

  int systemEnable = 1;
  int isWiFiConnect = 0;

  int waterPumpState = 0;
  int waterPumpTrigger = 0;

  int phUpState = 0;
  int phUpTrigger = 0;
  int phUpServoOn = 60;
  int phUpServoOff = 140;
  int phUpServoDegree = 140;

  int phDownState = 0;
  int phDownTrigger = 0;
  int phDownServoOn = 45;
  int phDownServoOff = 130;
  int phDownServoDegree = 130;

  int saltState = 0;
  int saltTrigger = 0;
  int saltServoOn = 0;
  int saltServoOff = 90;
  int saltServoDegree = 90;
};

SystemData var;