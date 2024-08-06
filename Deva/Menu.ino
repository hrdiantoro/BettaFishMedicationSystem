void menuTaskCallback() {
  static auto mainMenu = menu.createMenu(menu.begin(7), "Air   :", "Tinggi: ", "pH    : ", "Suhu  : ", "pHUp  : ", "pHDown: ", "Garam : ");

  static uint32_t displayTimer;
  if (millis() - displayTimer >= 500) {
    menu.formatMenu(mainMenu, menu.get(0), "Air   :%6s", var.statusTurbidity.c_str());
    menu.formatMenu(mainMenu, menu.get(1), "Tinggi: %5.2f", var.height);
    menu.formatMenu(mainMenu, menu.get(2), "pH    : %5.2f", var.ph);
    menu.formatMenu(mainMenu, menu.get(3), "Suhu  : %5.2f", var.temperature);
    menu.formatMenu(mainMenu, menu.get(4), "pHUp  : %5s", var.statusWaterLevel1.c_str());
    menu.formatMenu(mainMenu, menu.get(5), "pHDown: %5s", var.statusWaterLevel2.c_str());
    menu.formatMenu(mainMenu, menu.get(6), "Garam : %5s", var.statusWaterLevel3.c_str());
    displayTimer = millis();
  }

  menu.showMenu(mainMenu);
}