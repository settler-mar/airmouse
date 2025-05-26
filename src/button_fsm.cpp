// button_fsm.cpp — FSM-контроллер кнопок
#include "button_fsm.h"
#include "config.h"
#include "config_storage.h"
#include "button_service.h"
#include "action_runner.h"
#include <GyverButton.h>

GButton gbtn[NUM_DEFAULT_KEYS];
bool onHold[NUM_DEFAULT_KEYS] = {false}; // массив для хранения состояния удержания кнопок

byte not_active_counter = 30; // количество тиков, после которых кнопки активируются
void setup_button_fsm()
{
  for (size_t i = 0; i < NUM_DEFAULT_KEYS; i++)
  {
    gbtn[i] = GButton();
    gbtn[i].setDebounce(50);
    gbtn[i].setTimeout(500);
    gbtn[i].setClickTimeout(400);
    gbtn[i].setStepTimeout(500);
    gbtn[i].setType(HIGH_PULL);
    gbtn[i].setDirection(NORM_OPEN);
    gbtn[i].setTickMode(false);
  }
#if DEBUG
  Serial.println("[FSM] FSM initialized for all buttons");
#endif
}

void update_button_fsm(const uint8_t *pressedCodes)
{
  // const auto &keys = hardwareKeys;
  // byte layer = get_active_layer();

  if (not_active_counter)
  {
    not_active_counter--;
#if DEBUG
    if (!not_active_counter)
    {
      Serial.println("[FSM] Starting button FSM");
    }
#endif
  }

  for (size_t i = 0; i < NUM_DEFAULT_KEYS; i++)
  {
    gbtn[i].tick(pressedCodes[i]);

    if (not_active_counter)
    {
      gbtn[i].resetStates();
      continue;
    }

    if (gbtn[i].isClick())
    {
      run_button_action(i, BUTTON_CLICK);
    }
    if (gbtn[i].isDouble())
    {
      run_button_action(i, BUTTON_DOUBLE);
    }
    if (pressedCodes[i])
    {
      if (gbtn[i].isHolded())
      {
        run_button_action(i, BUTTON_HOLD_START);
        onHold[i] = true;
      }
      if (gbtn[i].isStep())
      {
        run_button_action(i, BUTTON_HOLD_REPEAT);
      }
      if (gbtn[i].isStep(1))
      {
        run_button_action(i, BUTTON_ONE_CLICK_HOLD);
      }
    }
    if (gbtn[i].isRelease())
    {
      if (onHold[i])
      {
        onHold[i] = false;
        run_button_action(i, BUTTON_HOLD_RELEASE);
      }
      else
      {
        run_button_action(i, BUTTON_RELEASE);
      }
    }
  }
}
