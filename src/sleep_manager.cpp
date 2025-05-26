#include "sleep_manager.h"
#include "esp_sleep.h"
#include "config.h"
#include "led_service.h"

static unsigned long lastActivityTime = 0;
static bool backlightOffDone = false;
static bool statusLedOffDone = false;
static bool sleepEntered = false;
static bool sleepEnabled = true; // если false — отключаем sleep (например, при питании от сети)

static byte wakePinState = 0;

void sleepManagerBegin()
{
  lastActivityTime = millis();

  pinMode(PIN_TILT_WAKE, INPUT);

  // Сброс состояний
  backlightOffDone = false;
  statusLedOffDone = false;
  sleepEntered = false;
}

void resetSleepTimer()
{
  lastActivityTime = millis();
  // wakePinActivity = false;

  if (backlightOffDone || statusLedOffDone)
  {
#if DEBUG
    Serial.println("[SLEEP] Resetting LED states...");
#endif
    set_led_sleep(false); // Включаем полную подсветку
  }

  backlightOffDone = false;
  statusLedOffDone = false;
  sleepEntered = false;
}

void setSleepEnabled(bool enabled)
{
  sleepEnabled = enabled;
  if (!enabled)
  {
    resetSleepTimer();
  }
}

void sleepManagerLoop()
{
  if (!sleepEnabled)
  {
    resetSleepTimer();
    return;
  }

  unsigned long currentMillis = millis();

  // Опрос wakePinActivity (сработал WAKE_PIN)
  // if (wakePinActivity)
  byte wakePinStateNew = digitalRead(PIN_TILT_WAKE);
  if (wakePinState != wakePinStateNew)
  {
    wakePinState = wakePinStateNew;
#if DEBUG && DEBUG_WAKE_PIN
    Serial.println("[SLEEP] Activity on WAKE_PIN detected! Resetting sleep timer.");
#endif
    resetSleepTimer();
  }

  // Первый этап: выключить только подсветку (оставить статусный LED)
  if (!backlightOffDone && currentMillis - lastActivityTime > SLEEP_BACKLIGHT_TIMEOUT)
  {
#if DEBUG
    Serial.println("[SLEEP] Turning off backlight...");
#endif
    set_led_backlight_sleep(true); // Выключаем только подсветку
    backlightOffDone = true;
  }

  // Второй этап: выключить статусный светодиод
  if (!statusLedOffDone && currentMillis - lastActivityTime > SLEEP_STATUS_LED_TIMEOUT)
  {
#if DEBUG
    Serial.println("[SLEEP] Turning off status LED...");
#endif
    set_led_sleep(true); // Выключаем всё, включая статусный LED
    statusLedOffDone = true;
  }

  // Третий этап: вход в light sleep
  if (!sleepEntered && currentMillis - lastActivityTime > SLEEP_TIMEOUT)
  {
    sleepEntered = true;

#if DEBUG
    Serial.println("[SLEEP] Entering light sleep...");
    delay(100);
#endif

    // Настроим пробуждение по таймеру (POWER_OFF_TIMEOUT_US)
    esp_sleep_enable_timer_wakeup(SLEEP_POWER_OFF_TIMEOUT * 1000ULL);

    // Настроим пробуждение по пину (EXT0 – фронт HIGH)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_TILT_WAKE, !wakePinStateNew);

    // Входим в light sleep
    esp_light_sleep_start();

    // После пробуждения
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER && !sleepEntered)
    {
#if DEBUG
      Serial.println("[SLEEP] Power off timer expired. Entering deep sleep.");
      delay(100);
#endif
      esp_deep_sleep_start(); // Перезагрузка после пробуждения
    }
    else if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
#if DEBUG
      Serial.println("[SLEEP] Woken up by tilt sensor (EXT0). Resetting sleep timer.");
#endif
      resetSleepTimer();
    }
    else
    {
#if DEBUG
      Serial.printf("[SLEEP] Woken up by unknown reason: %d\n", wakeup_reason);
#endif
      resetSleepTimer();
    }
  }
}
