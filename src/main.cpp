// main.cpp — основной файл прошивки (разделение по ядрам с учетом I2C)
#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "mcp_handler.h"
#include "button_service.h"
#include "mouse_control.h"
#include "ir_service.h"
#include "config_storage.h"
#include "led_service.h"
#include "button_fsm.h"
#include "web_interface.h"
#include "ble.h"
#include "action_runner.h"
#include "mode_manager.h"
#include "sleep_manager.h"
#include "ip5306.h"

TaskHandle_t TaskIOHandle;
TaskHandle_t TaskAppHandle;

#if DEBUG
// === Отладка ===

void scan_i2c()
{
  byte error, address;
  int nDevices;
  nDevices = 0;

  for (address = 1; address < 127; address++)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      if (nDevices > 0)
      {
        Serial.print(",");
      }
      Serial.print("I2C-address: 0x");
      Serial.print(address, HEX);
      Serial.print(" (");
      Serial.print(address);
      Serial.print(") - OK\n");
      // Serial.print("I2C-устройство: ");
      // Serial.print(I2CDeviceName(address));
      // Serial.print("\n");
      nDevices++;
    }
    else if (error == 4)
    {
      if (nDevices > 0)
      {
        Serial.print(",");
      }
      Serial.print("I2C-address: 0x");
      Serial.print(address, HEX);
      Serial.print(" (");
      Serial.print(address);
      Serial.print(") - ");
      // Serial.print("I2C-устройство: ");
      // Serial.print(I2CDeviceName(address));
      Serial.print(" (error)\n");
      nDevices++;
    }
  }
  if (nDevices == 0)
  {
    Serial.println("I2C-no devices found\n");
  }
  else
  {
    Serial.print("I2C-Devices found: ");
    Serial.println(nDevices);
  }
}

#endif

// Core 0: I2C задачи — гироскоп + MCP23017 + питание
void task_io(void *param)
{
#if DEBUG
  Serial.print("[MAIN] Starting IO task on core ");
  Serial.println(xPortGetCoreID());
#endif
  for (;;)
  {
    update_mouse_control();  // гироскоп
    read_mcp_buttons_tick(); // кешируем нажатия MCP
    ip5306_poll();           // опрос IP5306
    delay(10);
  }
}

// Core 1: логика, GPIO, BLE HID
void task_app(void *param)
{
#if DEBUG
  Serial.print("[MAIN] Starting App task on core ");
  Serial.println(xPortGetCoreID());
#endif
  for (;;)
  {
    led_service_loop(); // обработка LED
    sleepManagerLoop(); // обработка сна
    ble_loop();
    update_buttons();       // использует кеш MCP + GPIO
    ip5306_update_status(); // обновление статуса питания
    delay(10);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("[MAIN] Starting setup...");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  delay(20);
#if DEBUG
  Serial.println("[MAIN] I2C initialized");
  Serial.println("[MAIN] I2C SDA: " + String(I2C_SDA_PIN));
  Serial.println("[MAIN] I2C SCL: " + String(I2C_SCL_PIN));
  scan_i2c();
  Serial.println("[MAIN] I2C scan done");
#endif

  // === Конфигурация ===
  load_config();
  // print_config();

  uint8_t mode = ModeManager::getMode();
  pinMode(WIFI_MODE_RUN_PIN, INPUT);
  while (digitalRead(WIFI_MODE_RUN_PIN) == LOW)
  {
    delay(100);
    if (digitalRead(WIFI_MODE_RUN_PIN) == HIGH)
    {
      mode = 2;
      break;
    }
  }
#if DEBUG
  Serial.printf("[MAIN] Boot mode: %d\n", mode);
#endif

  // === Модули ===
  if (mode == 1)
  {
    ble_start();
    sleepManagerBegin();
  }
  init_action_runner();   // инициализация Action Runner
  setup_led_service();    // инициализация LED
  setup_mcp_handler();    // инициализация MCP23017
  setup_mouse_control();  // инициализация гироскопа для мыши
  ir_setup();             // инициализация IR
  setup_button_service(); // инициализация сервиса кнопок
  setup_button_fsm();     // инициализация FSM-контроллера кнопок
  ip5306_init();          // инициализация питания

  if (mode == 2)
  {
    ModeManager::resetToDefault();
    wifi_start();
  }

  delay(100);

#if DEBUG
  Serial.println("[MAIN] Modules initialized");
#endif

  // === Задачи по ядрам ===
  xTaskCreatePinnedToCore(task_io, "IO_Task", 4096, NULL, 1, &TaskIOHandle, 0); // ядро 0
  delay(100);
  xTaskCreatePinnedToCore(task_app, "App_Task", 4096, NULL, 1, &TaskAppHandle, 1); // ядро 1
}

void loop()
{
  web_loop(); // обработка веб-интерфейса
  ir_loop();  // обработка IR. Обучение, сохранение
  delay(10);
}
