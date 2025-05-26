#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include <stdbool.h>

uint32_t get_uint32_from_char(char **str_ptr, char delim);
int16_t get_int_from_char(char **str_ptr, char delim);
uint32_t parse_hex_color(const char *hex);
#endif // HELPERS_H