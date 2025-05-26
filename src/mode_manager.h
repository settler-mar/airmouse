#pragma once
#include <Preferences.h>
#define MODE_BLE 1
#define MODE_WIFI 2

namespace ModeManager
{

  inline uint8_t DEFAULT_MODE = MODE_BLE;
  inline const char *NAMESPACE = "boot_mode";
  inline const char *KEY = "mode";
  inline byte current_mode = 0;
  inline bool is_load_mode = false;

  inline void setMode(uint8_t mode)
  {
    Preferences prefs;
    prefs.begin(NAMESPACE, false);
    prefs.putUChar(KEY, mode);
    prefs.end();
  }

  inline uint8_t getMode()
  {
    if (is_load_mode)
      return current_mode;

    Preferences prefs;
    prefs.begin(NAMESPACE, true);
    current_mode = prefs.getUChar(KEY, DEFAULT_MODE);
    prefs.end();
    return current_mode;
  }

  inline void resetToDefault()
  {
    setMode(DEFAULT_MODE);
  }

  inline void toggleMode(uint8_t alt_mode = MODE_WIFI)
  {
    if (current_mode == DEFAULT_MODE)
    {
      setMode(alt_mode);
    }
    else
    {
      setMode(DEFAULT_MODE);
    }
  }

} // namespace ModeManager
