#include "mouse_control.h"
#include "config.h"
#include "button_service.h"
#include <MPU6050_light.h>
#include <BleMouse.h>
#include "mcp_handler.h"

MPU6050 mpu(Wire);

static float lastAngleX = 0;
static float lastAngleY = 0;
static float lastAngleZ = 0;
static bool initialized = false;
bool enabled = false;
byte currentSide = 255;

// === Порог сработок для фильтрации переключения стороны ===
#define SIDE_SWITCH_THRESHOLD 5
static uint8_t mouseSwitchCounter = 0;
static uint8_t keyboardSwitchCounter = 0;

// === Настройка гироскопа ===
void setup_mouse_control()
{
#if DEBUG
  Serial.println("[MOUSE] Initializing MPU6050...");
#endif
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  byte status = mpu.begin();
  if (status != 0)
  {
#if DEBUG
    Serial.println("[MOUSE] Error MPU6050, код: " + String(status));
#endif
    return;
  }
  delay(500);
  mpu.calcGyroOffsets();
  initialized = true;
  enabled = true;
#if DEBUG
  Serial.println("[MOUSE] MPU6050 initialized");
#endif
}

// === Обновление положения курсора (вызов из loop/core) ===
void update_mouse_control()
{
  if (!initialized)
    return;
  mpu.update();
  processMouseMove();
}

// === Обработка движения мыши на основе углов ===
void processMouseMove()
{
  if (!initialized)
    return;

  float accZ = mpu.getAccZ();
#ifdef MOUSE_INVERT_Z
  accZ = -accZ;
#elif defined(MOUSE_SIDE_INVERT)
  // Для обратной совместимости
  accZ = -accZ;
#endif

  // === Фильтр шумов при переключении стороны ===
  if (accZ > MOUSE_SIDE_ZONE)
  {
    mouseSwitchCounter++;
    keyboardSwitchCounter = 0;
    if (mouseSwitchCounter >= SIDE_SWITCH_THRESHOLD && currentSide != SIDE_MOUSE)
    {
#if DEBUG && DEBUG_MOUSE_STATE
      Serial.println("SWITCH TO MOUSE");
#endif
      currentSide = SIDE_MOUSE;
      mouse_control_enable();
      set_mcp_active_side(SIDE_MOUSE);
    }
  }
  else if (accZ < -MOUSE_SIDE_ZONE)
  {
    keyboardSwitchCounter++;
    mouseSwitchCounter = 0;
    if (keyboardSwitchCounter >= SIDE_SWITCH_THRESHOLD && currentSide != SIDE_KEYBOARD)
    {
#if DEBUG && DEBUG_MOUSE_STATE
      Serial.println("SWITCH TO KEYBOARD");
#endif
      currentSide = SIDE_KEYBOARD;
      mouse_control_disable();
      set_mcp_active_side(SIDE_KEYBOARD);
    }
  }
  else
  {
    // Если в зоне стабильности — сбрасываем счётчики
    mouseSwitchCounter = 0;
    keyboardSwitchCounter = 0;
#if DEBUG && DEBUG_MOUSE_STATE
    Serial.println("STABLE");
#endif
  }

  // === Получаем углы наклона по осям X и Y ===
  float angleX = mpu.getAngleX();
  float angleY = mpu.getAngleY();

  // Поворот датчика относительно оси устройства
#if MOUSE_SENSOR_ROTATION == 90
  {
    float t = angleX;
    angleX = -angleY;
    angleY = t;
  }
#elif MOUSE_SENSOR_ROTATION == 180
  angleX = -angleX;
  angleY = -angleY;
#elif MOUSE_SENSOR_ROTATION == 270
  {
    float t = angleX;
    angleX = angleY;
    angleY = -t;
  }
#endif

  // Инверсия осей
#ifdef MOUSE_INVERT_X
  angleX = -angleX;
#endif
#ifdef MOUSE_INVERT_Y
  angleY = -angleY;
#endif

  float dx = angleX - lastAngleX;
  float dy = angleY - lastAngleY;

#if DEBUG && DEBUG_MOUSE_STATE
  Serial.print("[MOUSE] ");
  Serial.print("X: ");
  Serial.print(angleX);
  Serial.print(" Y: ");
  Serial.print(angleY);
  Serial.print(" dX: ");
  Serial.print(dx);
  Serial.print(" dY: ");
  Serial.println(dy);
#endif

  lastAngleX = angleX;
  lastAngleY = angleY;

  if (!is_hid_connected() && !enabled && currentSide == SIDE_MOUSE)
    return;

  if (fabs(dx) < MOUSE_DEADZONE && fabs(dy) < MOUSE_DEADZONE)
    return;

  float velocity = sqrt(dx * dx + dy * dy);
  float accFactor = velocity > MOUSE_ACCEL_THRESHOLD ? MOUSE_ACCEL_MULTIPLIER : 1.0;
  float precision = (velocity < MOUSE_PRECISION_THRESHOLD) ? MOUSE_PRECISION_SCALE : 1.0;

  int moveX = dx * MOUSE_SENSITIVITY * accFactor * precision;
  int moveY = dy * MOUSE_SENSITIVITY * accFactor * precision;

  if (moveX != 0 || moveY != 0)
  {
    if (enabled)
    {
      moveX = constrain(moveX, -127, 127);
      moveY = constrain(moveY, -127, 127);
      Mouse.move(moveX, moveY);
    }
  }
}

void mouse_control_enable()
{
  if (initialized && !enabled)
  {
    enabled = true;
#if DEBUG
    Serial.println("[MOUSE] MPU6050 enabled");
#endif
  }
}
void mouse_control_disable()
{
  if (!enabled)
    return;
  enabled = false;
#if DEBUG
  Serial.println("[MOUSE] MPU6050 disabled");
#endif
}
void mouse_control_toggle()
{
  if (!initialized)
    return;
  enabled = !enabled;
}
