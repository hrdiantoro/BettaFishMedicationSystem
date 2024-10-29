void wifiConTask() {
  task.setInitCoreID(1);
  disableCore1WDT();
  disableCore0WDT();
  task.createTask(10000, [](void* pvParameter) {
    auth.apiKey = "AIzaSyCC3q3_ENzR33ajT-zVhPEaUuSjbxmeDBc";
    auth.databaseURL = "https://devaapps-d5f1a-default-rtdb.firebaseio.com/";
    auth.projectID = "devaapps-d5f1a";

    auth.user.email = "admin@gmail.com";
    auth.user.password = "admin123";

    // if (!wm.autoConnect("DevaAP", ""))  // 192.168.4.1
    //   Serial.println("| Failed to connect");

    // firebase.connectToWiFi("TIMEOSPACE", "1234Saja");
    firebase.connectToWiFi("TIMEOSPACE", "1234Saja");
    // firebase.connectToWiFi("Devaskripsi", "devapratama");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    firebase.waitConnection(3000);
    firebase.init(&auth);

    var.isWiFiConnect = 1;

    buzzer.toggleInit(100, 2);
    buzzer.onToOffDelay(450);
    buzzer.toggleInit(100, 2);

    for (;;) {
      if (firebase.isConnect()) {
        static uint32_t dataSendTimer;
        if (millis() - dataSendTimer >= 1000) {
          JsonDocument setJson;
          // setJson["kekeruhan"] = var.statusTurbidity;
          setJson["kekeruhan"] = String((int)var.turbidity);
          setJson["ketinggian"] = String(var.height, 2);
          setJson["ph"] = String(var.ph, 2);
          setJson["suhu"] = String(var.temperature, 2);
          setJson["waterlevel1"] = var.statusWaterLevel1;
          setJson["waterlevel2"] = var.statusWaterLevel2;
          setJson["waterlevel3"] = var.statusWaterLevel3;
          // setJson["waterlevel1"] = String(var.waterLevel1, 2);
          // setJson["waterlevel2"] = String(var.waterLevel2, 2);
          // setJson["waterlevel3"] = String(var.waterLevel3, 2);
          firebase.setJson("/data", setJson, FirebaseModule::resultStatusCallback);

          firebase.getJson(
            "/limit", [](JsonVariant getJson) {
              var.maxWaterLevel = getJson["maxLevel"].as<String>().toFloat();
              var.maxTurbidity = getJson["maxNTU"].as<String>().toFloat();
              var.maxPh = getJson["maxPh"].as<String>().toFloat();
              var.maxTemperature = getJson["maxLevel"].as<String>().toFloat();

              var.minWaterLevel = getJson["minLevel"].as<String>().toFloat();
              var.minTurbidity = getJson["minNTU"].as<String>().toFloat();
              var.minPh = getJson["minPh"].as<String>().toFloat();
              var.minTemperature = getJson["minLevel"].as<String>().toFloat();
            },
            FirebaseModule::resultStatusCallback);

          uint32_t epoch = getTime();
          epoch += gmtOffset_sec;
          DateTimeNTP currentTime(epoch);
          static String timestampPrint;
          timestampPrint = currentTime.timestamp();
          timestampPrint.replace("T", " ");

          if (var.phUpState) {
            firebase.pushJson(
              "/riwayat_obat", [](JsonVariant pushJson) -> JsonVariant {
                pushJson["jenis_obat"] = "Ph Up";
                pushJson["ml"] = "3";
                pushJson["timestamp"] = timestampPrint;
                return pushJson;
              },
              FirebaseModule::resultStatusCallback);
            var.phUpState = 0;
          }
          if (var.phDownState) {
            firebase.pushJson(
              "/riwayat_obat", [](JsonVariant pushJson) -> JsonVariant {
                pushJson["jenis_obat"] = "Ph Down";
                pushJson["ml"] = "3";
                pushJson["timestamp"] = timestampPrint;
                return pushJson;
              },
              FirebaseModule::resultStatusCallback);
            var.phDownState = 0;
          }
          if (var.saltState) {
            firebase.pushJson(
              "/data_riwayat", [](JsonVariant pushJson) -> JsonVariant {
                // pushJson["kekeruhan"] = var.statusTurbidity;
                pushJson["kekeruhan"] = String((int)var.turbidity);
                pushJson["ketinggian"] = String(var.height, 2);
                pushJson["ph"] = String(var.ph);
                pushJson["suhu"] = String(var.temperature);
                pushJson["tanggal"] = timestampPrint;
                return pushJson;
              },
              FirebaseModule::resultStatusCallback);
            var.saltState = 0;
          }

          dataSendTimer = millis();
        }
      }
      task.delay(20);
    }
  });

  task.createTask(6000, [](void* pvParameter) {
    disableCore0WDT();
    for (;;) {
      if (!var.isWiFiConnect) {
        static uint32_t wifiConnectTimer;
        if (millis() - wifiConnectTimer >= 2500) {
          buzzer.on();
          buzzer.offDelay(50);
          wifiConnectTimer = millis();
        }
      }
      task.delay(20);
    }
  });
}