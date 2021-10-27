#ifndef _STRUCTURE_H_
#define _STRUCTURE_H_

#define MAX_TOOLS 6 // 1 | 2 | 4 = 7; minus 1, 7 means any/all

enum printer_state {
  starting,
  updating,
  halted,
  printer_off,
  pausing,
  resuming,
  paused,
  simulating,
  processing,
  changingTool,
  busy,
  idle
};

enum heater_state {
  active,
  standby,
  tuning,
  fault,
  heater_off
};

typedef struct {
  heater_state state = heater_off;
  float active_temp = 0;
  float standby_temp = 0;
  float current_temp = 0;
} heater_status;

class ObjectModel {
  public:
  printer_state state = printer_off;
  uint8_t fans[MAX_TOOLS] = {0};
  heater_status heaters[MAX_TOOLS];
  inline float print_progress() {
     return print_duration / (float) max(1u, print_duration + print_remaining);
  }
  bool ready = false;
  inline void reset_print_progress() {
    print_duration = 0;
    print_remaining = 0;
  }
  // private: TODO FIXME
  uint32_t print_duration;
  uint32_t print_remaining;
};

extern ObjectModel object_model;

#endif