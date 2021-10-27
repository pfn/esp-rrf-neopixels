#include <Config.h>
#include <Debug.h>

#undef DEBUG
#undef DEBUGF
#define DEBUG(x)
#define DEBUGF(arg...)

Config config;
void ConfigListener::key(const char *key) {
  DEBUGF("key: %s", key);
       MAP_KEY("pin",           cfg_pin)
  else MAP_KEY("startup_color", cfg_start_color)
  else MAP_KEY("display_item",  cfg_display_item)
  else MAP_KEY("type",          cfg_type)
  else MAP_KEY("offset",        cfg_offset)
  else MAP_KEY("count",         cfg_count)
  else MAP_KEY("reverse",       cfg_reverse)
  else MAP_KEY("brightness",    cfg_brightness)
  else MAP_KEY("temp_base",     cfg_temp_base)
  else MAP_KEY("starting",      cfg_starting)
  else MAP_KEY("updating",      cfg_updating)
  else MAP_KEY("paused",        cfg_paused)
  else MAP_KEY("changingTool",  cfg_changingTool)
  else MAP_KEY("busy",          cfg_busy)
  else MAP_KEY("idle",          cfg_idle)
  else MAP_KEY("heating",       cfg_heating)
  else MAP_KEY("cooling",       cfg_cooling)
  else MAP_KEY("secondary",     cfg_secondary)
  else MAP_KEY("active",        cfg_active)
  else MAP_KEY("done",          cfg_done)
  else MAP_KEY("heater",        cfg_heater)
  else MAP_KEY("fan",           cfg_fan)
  else MAP_KEY("printing",      cfg_printing)
  else MAP_KEY("halted",        cfg_halted)
  else current = cfg_unk;
}

#define CASE_COLOR(cat, key) case cfg_##key: config.cat.key = strtol(value + 1, NULL, 16); break;

void ConfigListener::handle_value(const char *value) {
  DEBUGF("  = %s\n", value);
  switch (current) {
    case cfg_pin:
      config.leds[index].active = true;
      config.leds[index].pin = strtol(value, NULL, 10);
      break;
    case cfg_start_color:
      config.leds[index].startup_color = strtol(value + 1, NULL, 16);
      break;
    case cfg_type:
      config.leds[index].type = strtol(value, NULL, 10);
      break;
    case cfg_offset:
      config.leds[index].offset = strtol(value, NULL, 10);
      break;
    case cfg_count:
      config.leds[index].count = strtol(value, NULL, 10);
      break;
    case cfg_display_item:
      config.leds[index].display_item = strtol(value, NULL, 10);
      break;
    case cfg_brightness:
      config.leds[index].brightness = strtol(value, NULL, 10);
      break;
    case cfg_reverse:
      config.leds[index].reverse = value[0] == 't';
      break;
    case cfg_temp_base:
      config.leds[index].temp_base = strtol(value, NULL, 10);
      break;
    CASE_COLOR(state,    starting)
    CASE_COLOR(state,    updating)
    CASE_COLOR(state,    paused)
    CASE_COLOR(state,    changingTool)
    CASE_COLOR(state,    busy)
    CASE_COLOR(state,    idle)
    CASE_COLOR(heater,   heating)
    CASE_COLOR(heater,   cooling)
    CASE_COLOR(fan,      active)
    CASE_COLOR(printing, done)
    CASE_COLOR(state,    halted)
    case cfg_secondary:
      switch (parent()) {
        case cfg_heater:
          config.heater.secondary = strtol(value + 1, NULL, 16);
          break;
        case cfg_fan:
          config.fan.secondary = strtol(value + 1, NULL, 16);
          break;
        case cfg_printing:
          config.printing.secondary = strtol(value + 1, NULL, 16);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

Config parseConfig(File f) {
  char buf[384];
  JsonStreamingParser parser;
  ConfigListener listener;
  parser.setListener(&listener);
  while (f.available() > 0) {
    size_t read = f.readBytes(buf, 384);
    for (size_t i = 0; i < read; ++i)
      parser.parse(buf[i]);
  }
  listener.config.loaded = true;
  return listener.config;
}