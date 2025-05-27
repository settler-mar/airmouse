// mcp_handler.cpp — обработка MCP23017 напрямую через I2C (универсальный)
#include "mcp_handler.h"
#include "config.h"
#include "config_storage.h"
#include <Wire.h>

#define MCP_IODIRA 0x00
#define MCP_IODIRB 0x01
#define MCP_GPPUA 0x0C
#define MCP_GPPUB 0x0D
#define MCP_GPIOA 0x12
#define MCP_GPIOB 0x13

static Side activeSide = SIDE_KEYBOARD;
static const uint8_t MCP[] = MCP_ADDR;
static const size_t MCP_COUNT = sizeof(MCP) / sizeof(MCP[0]);

static bool *mcp_active;
static bool *mcp_initialized;
static uint8_t *mcp_gpioa;
static uint8_t *mcp_gpiob;

// static uint8_t cachedStates[32];
static size_t cachedCount = 0;

uint8_t get_mcp_count()
{
  return MCP_COUNT;
}

uint8_t get_mcp_addr(uint8_t chipIndex)
{
  if (chipIndex < MCP_COUNT)
    return MCP[chipIndex];
  return 0xFFFF; // неверный индекс
}
uint8_t get_mcp_initialized(uint8_t chipIndex)
{
  if (chipIndex < MCP_COUNT)
    return mcp_initialized[chipIndex] ? 1 : 0;
  return 0; // неверный индекс
}

void mcp_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

uint8_t mcp_read_reg(uint8_t addr, uint8_t reg)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(addr, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0xFF;
}

void setup_mcp_handler()
{
#if DEBUG
  Serial.println("[MCP] MCP23017 setup");
#endif
  mcp_active = new bool[MCP_COUNT]();
  mcp_initialized = new bool[MCP_COUNT]();
  mcp_gpioa = new uint8_t[MCP_COUNT];
  mcp_gpiob = new uint8_t[MCP_COUNT];

  for (size_t i = 0; i < MCP_COUNT; i++)
  {
    Wire.beginTransmission(MCP[i]);
    if (Wire.endTransmission() == 0)
    {
      mcp_write_reg(MCP[i], MCP_IODIRA, 0xFF);
      mcp_write_reg(MCP[i], MCP_IODIRB, 0xFF);
      mcp_write_reg(MCP[i], MCP_GPPUA, 0xFF);
      mcp_write_reg(MCP[i], MCP_GPPUB, 0xFF);
      mcp_write_reg(MCP[i], MCP_GPIOA, 0xFF);
      mcp_write_reg(MCP[i], MCP_GPIOB, 0xFF);
      mcp_initialized[i] = true;
#if DEBUG
      Serial.printf("[MCP] MCP23017 addr=0x%02X index=%d initialized\n", MCP[i], i);
#endif
    }
    else
    {
#if DEBUG
      Serial.printf("[MCP] MCP23017 addr=0x%02X index=%d not found\n", MCP[i], i);
#endif
    }
  }
}

void set_mcp_active_side(Side side)
{
  activeSide = side;
  for (size_t i = 0; i < MCP_COUNT; i++)
    mcp_active[i] = false;

  const auto *hw = get_keys_config();
  for (size_t i = 0; i < NUM_DEFAULT_KEYS; i++)
  {
    if (hw[i].source != HARDWARE_KEY_SOURCE_MCP)
      continue;
    if (hw[i].side != side and hw[i].side != SIDE_BOTH)
      continue;
    if (hw[i].sourceIndex < MCP_COUNT)
      mcp_active[hw[i].sourceIndex] = true;
  }
#if DEBUG
  Serial.printf("[MCP] Active side: %s\n", side == SIDE_MOUSE ? "MOUSE" : "KEYBOARD");
  for (size_t i = 0; i < MCP_COUNT; i++)
    Serial.printf("[MCP] MCP[%d] active: %d\n", i, mcp_active[i]);
#endif
}

void read_mcp_buttons_tick()
{
  cachedCount = 0;
  for (size_t i = 0; i < MCP_COUNT; i++)
  {
    if (mcp_active[i] && mcp_initialized[i])
    {
      mcp_gpioa[i] = mcp_read_reg(MCP[i], MCP_GPIOA);
      mcp_gpiob[i] = mcp_read_reg(MCP[i], MCP_GPIOB);
    }
    else
    {
      mcp_gpioa[i] = 0xFF;
      mcp_gpiob[i] = 0xFF;
    }
  }
#if DEBUG && DEBUG_MCP_STATE
  Serial.print("[MCP] State: ");
  for (size_t i = 0; i < MCP_COUNT; i++)
  {
    Serial.printf("%02X ", mcp_gpioa[i]);
    Serial.printf("%02X ", mcp_gpiob[i]);
  }
  Serial.println();
#endif
}

bool get_pin_state(uint8_t chipIndex, uint8_t pin)
{
  if (chipIndex >= MCP_COUNT || !mcp_initialized[chipIndex])
    return false;

  if (pin < 8)
    return !(mcp_gpioa[chipIndex] & (1 << pin));
  else if (pin < 16)
    return !(mcp_gpiob[chipIndex] & (1 << (pin - 8)));
  return false;
}

Side get_mcp_active_side()
{
  return activeSide;
}
