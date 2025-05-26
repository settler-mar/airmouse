// https://www.lcsc.com/datasheet/lcsc_datasheet_2409280001_INJOINIC-IP5306-I2C_C488349.pdf
#include "ip5306.h"
#include "config.h"
#include <Arduino.h>
#include "led_service.h"
#include <Wire.h>
#include "ble.h"

// ================== Регистры IP5306 ==================
#define IP5306_SYS_CTL0 0x00
#define IP5306_SYS_CTL1 0x01
#define IP5306_CHG_CTL0 0x20
#define IP5306_CHG_CTL1 0x21
#define IP5306_CHG_CTL3 0x23
#define IP5306_CHG_CTL4 0x24
#define IP5306_READ_BATTERY_LEVEL 0x78

// ================== Локальные переменные ==================
static bool ip5306_available = false;
static uint8_t battery_level = 100;
static unsigned long last_poll_time = 0;
static bool has_charging_data = false; // Флаг для наличия свежих данных

static bool was_charging = false;
static bool needs_charge_called = false;

// ================== I2C ==================
static void i2c_write_byte(uint8_t reg, uint8_t data)
{
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

static bool i2c_read_byte(uint8_t reg, uint8_t *data)
{
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0)
    return false;

  if (Wire.requestFrom(IP5306_ADDR, 1) != 1)
    return false;
  *data = Wire.read();
  return true;
}

// ================== Инициализация ==================
void ip5306_init()
{
  uint8_t dummy;
  ip5306_available = i2c_read_byte(IP5306_SYS_CTL0, &dummy);
  if (!ip5306_available)
  {
    Serial.println("[IP5306] IP5306 not found");
    return;
  }

  // Настройка напряжения
  uint8_t chg_ctl4;
  if (i2c_read_byte(IP5306_CHG_CTL4, &chg_ctl4))
  {
    chg_ctl4 &= ~(0x07);
    chg_ctl4 |= (IP5306_CHARGE_VOLTAGE_SEL & 0x07);
    i2c_write_byte(IP5306_CHG_CTL4, chg_ctl4);
  }

  // Настройка тока зарядки
  uint16_t current = IP5306_CHARGE_CURRENT_MA;
  if (current > 2100)
    current = 2100;
  current /= 100;
  current *= 100;

  float targetA = current / 1000.0f;
  float remaining = targetA - 0.05f;
  if (remaining < 0)
    remaining = 0;

  uint8_t bits = 0;
  if (remaining >= 1.6f)
  {
    bits |= (1 << 4);
    remaining -= 1.6f;
  }
  if (remaining >= 0.8f)
  {
    bits |= (1 << 3);
    remaining -= 0.8f;
  }
  if (remaining >= 0.4f)
  {
    bits |= (1 << 2);
    remaining -= 0.4f;
  }
  if (remaining >= 0.2f)
  {
    bits |= (1 << 1);
    remaining -= 0.2f;
  }
  if (remaining >= 0.1f)
  {
    bits |= (1 << 0);
  }

  uint8_t chg_ctl3;
  if (i2c_read_byte(IP5306_CHG_CTL3, &chg_ctl3))
  {
    chg_ctl3 &= 0xE0;
    chg_ctl3 |= bits;
    i2c_write_byte(IP5306_CHG_CTL3, chg_ctl3);
  }

  // Auto Power On
  uint8_t sys_ctl0;
  if (i2c_read_byte(IP5306_SYS_CTL0, &sys_ctl0))
  {
    if (IP5306_AUTO_POWER_ON)
      sys_ctl0 |= (1 << 4);
    else
      sys_ctl0 &= ~(1 << 4);
    i2c_write_byte(IP5306_SYS_CTL0, sys_ctl0);
  }

  // Поведение кнопки Power Key
  uint8_t sys_ctl1;
  if (i2c_read_byte(IP5306_SYS_CTL1, &sys_ctl1))
  {
    sys_ctl1 |= (1 << 1);
    i2c_write_byte(IP5306_SYS_CTL1, sys_ctl1);
  }
#if DEBUG
  Serial.println("[IP5306] Initialized successfully");
#endif
}

// ================== Опрос чипа (I2C-only) ==================
void ip5306_poll()
{
  if (!ip5306_available)
    return;

  unsigned long now = millis();
  if (now - last_poll_time < IP5306_POLL_INTERVAL)
    return;
  last_poll_time = now;

  // Чтение уровня заряда
  uint8_t level_raw;
  if (!i2c_read_byte(IP5306_READ_BATTERY_LEVEL, &level_raw))
    return;
  battery_level = (level_raw & 0x0F) * 25;

  // Управление зарядкой
  uint8_t chg_ctl1;
  if (i2c_read_byte(IP5306_CHG_CTL1, &chg_ctl1))
  {
#if IP5306_AUTO_CHARGE_CONTROL
    if (battery_level >= 100)
    {
      chg_ctl1 &= ~(1 << 4);
    }
    else if (battery_level <= 98)
    {
      chg_ctl1 |= (1 << 4);
    }
#else
    chg_ctl1 |= (1 << 4);
#endif
    i2c_write_byte(IP5306_CHG_CTL1, chg_ctl1);
  }

  // Проверяем зарядку
  uint8_t chg_ctl0;
  was_charging = false;
  if (i2c_read_byte(IP5306_CHG_CTL0, &chg_ctl0) && (chg_ctl0 & (1 << 4)))
  {
    was_charging = true;
  }

  has_charging_data = true; // Отмечаем, что данные опроса обновлены
}

// ================== Обновление статуса LED (НЕ I2C) ==================
void ip5306_update_status()
{
  if (!has_charging_data)
    return;
  has_charging_data = false; // Сброс флага

  if (battery_level <= BATTERY_LEVEL_LOW)
  {
    LED_STATUS_BATTERY_LOW;
    return;
  }

  if (battery_level < BATTERY_LEVEL_NEEDS_CHARGE && !needs_charge_called)
  {
    LED_STATUS_BATTERY_NEADS_CHARGE;
    needs_charge_called = true;
    return;
  }
  if (battery_level >= BATTERY_LEVEL_NEEDS_CHARGE)
  {
    needs_charge_called = false;
  }

  if (was_charging)
  {
    LED_STATUS_BATTERY_CHARGING;
  }
  else if (battery_level == 100)
  {
    LED_STATUS_BATTERY_FULL;
  }

  set_battery_level(battery_level);
}

// ================== Основной loop ==================
void ip5306_loop()
{
  ip5306_poll();
  ip5306_update_status();
}

// ================== Прочие функции ==================
void power_off()
{
  if (ip5306_available)
  {
    i2c_write_byte(IP5306_SYS_CTL0, 0x00);
  }
  else
  {
    esp_deep_sleep_start();
  }
}

/*void setSleepEnabled(bool enabled)
{
  if (!ip5306_available)
    return;

  uint8_t sys_ctl0;
  if (i2c_read_byte(IP5306_SYS_CTL0, &sys_ctl0))
  {
    if (enabled)
      sys_ctl0 &= ~(1 << 3);
    else
      sys_ctl0 |= (1 << 3);
    i2c_write_byte(IP5306_SYS_CTL0, sys_ctl0);
  }
}*/

void handle_power_status_api(AsyncWebServer &server)
{
  server.on("/led_status", HTTP_GET, [](AsyncWebServerRequest *request)
            {String json = "{";
  json += "\"battery_level\":" + String(battery_level) + ",";
  json += "\"was_charging\":" + String(was_charging ? "true" : "false") + ",";
  json += "\"needs_charge\":" + String(needs_charge_called ? "true" : "false") + ",";
  json += "\"ip5306_available\":" + String(ip5306_available ? "true" : "false") + ",";
  json += "\"charging_data_available\":" + String(has_charging_data ? "true" : "false");
  json += "}";
            request->send(200, "application/json", json); });
}
