// mcp_handler.h — интерфейс для работы с MCP23017 напрямую через I2C
#ifndef MCP_HANDLER_H
#define MCP_HANDLER_H

#include <stdint.h>
#include "config.h"

void setup_mcp_handler();                           // инициализация MCP (ядро 0)
void set_mcp_active_side(Side side);                // устанавливает активную сторону (ядро 0)
void read_mcp_buttons_tick();                       // обновляет кеш нажатий с MCP (ядро 0)
Side get_mcp_active_side();                         // возвращает активную сторону (ядро 1)
bool get_pin_state(uint8_t chipIndex, uint8_t pin); // возвращает состояние пина (ядро 1)
uint8_t get_mcp_count();                            // возвращает количество MCP
uint8_t get_mcp_addr(uint8_t chipIndex);           // возвращает адрес MCP по индексу
uint8_t get_mcp_initialized(uint8_t chipIndex);         // возвращает состояние активности MCP по индексу
#endif
