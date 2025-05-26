// button_fsm.h — интерфейс FSM-контроллера
#pragma once

#include <Arduino.h>

void setup_button_fsm();
void update_button_fsm(const uint8_t *pressedCodes);
