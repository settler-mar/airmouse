#include "helpers.h"
#include "config.h"

#ifndef HEPLPERS_CPP
#define HEPLPERS_CPP

uint32_t get_uint32_from_char(char **str_ptr, char delim)
{
  uint32_t value = 0;
  char *str = *str_ptr;

  while (*str)
  {
    if (*str >= '0' && *str <= '9')
    {
      value = value * 10 + (*str - '0');
    }
    else if (*str == delim)
    {
      str++; // move past the delimiter
      break;
    }
    else
    {
      break;
    }
    str++;
  }
  *str_ptr = str;
  return value;
}
// str as linked list
int16_t get_int_from_char(char **str_ptr, char delim)
{
  int16_t value = 0;
  char *str = *str_ptr;
  bool negative = false;
  while (*str)
  {
    if (*str >= '0' && *str <= '9')
    {
      value = value * 10 + (*str - '0');
    }
    else if (*str == '-')
    {
      negative = true;
    }
    else if (*str == '+')
    {
      negative = false;
    }
    else if (*str == delim)
    {
      str++; // move past the delimiter
      break;
    }
    else
    {
      break;
    }
    str++;
  }
  *str_ptr = str;
  return negative ? -value : value;
}

uint32_t parse_hex_color(const char *hex) {
  if (!hex || hex[0] != '#') return 0;
  uint32_t r = 0, g = 0, b = 0;
  sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b);
  return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}
#endif // HELPERS_H