#include <Adafruit_NeoPixel.h>
#include <JsonListener2.h>
#include <JsonStreamingParser.h>
#include <FS.h>

#define MAX_LEDS 6

#define DISPLAY_STATUS 0x20
#define HEATER_FLAG 0x8
#define FAN_FLAG 0x10
#define ITEM_MASK 0x7
#define DISPLAY_ANY ITEM_MASK

typedef struct {
    uint32_t starting;
    uint32_t updating;
    uint32_t paused;
    uint32_t halted;
    uint32_t changingTool;
    uint32_t busy;
    uint32_t idle;
} state_colors;

typedef struct {
    uint32_t heating;
    uint32_t cooling;
    uint32_t secondary;
} heater_colors;

typedef struct {
    uint32_t active;
    uint32_t secondary;
} fan_colors;

typedef struct {
    uint32_t done;
    uint32_t secondary;
} printing_colors;

typedef struct {
  uint8_t pin;
  uint32_t startup_color;
  uint8_t display_item;
  bool active = false;
  bool reverse = false;
  neoPixelType type = NEO_RGB;
  uint8_t count = 16;
  uint8_t offset = 0; // animation pixel offset
  uint8_t brightness = 8;
  uint8_t temp_base = 20; // scale is temp_base to set point plus divided by (count - 2)
} led_config;

typedef struct {
  bool loaded = false;
  uint8_t leds_count = 0;
  uint8_t common_count = 255;
  led_config leds[MAX_LEDS];
  state_colors state;
  heater_colors heater;
  fan_colors fan;
  printing_colors printing;
} Config;

extern Config config;

enum cfg_state { cfg_TOP, cfg_unk,
 cfg_pin, cfg_start_color, cfg_display_item,
 cfg_offset, cfg_reverse, cfg_count,
 cfg_brightness, cfg_temp_base,
 cfg_starting, cfg_updating, cfg_paused, cfg_changingTool, cfg_busy, cfg_idle, cfg_halted,
 cfg_heating, cfg_cooling, cfg_secondary, cfg_active, cfg_done,
 cfg_heater, cfg_fan, cfg_printing,
 cfg_type
};

class ConfigListener : public JsonListener2<enum cfg_state> {
  public:
  Config config;
  virtual void key(const char *key) override;
  virtual void handle_value(const char *key) override;
  inline void endDocument() {
    JsonListener2::endDocument();
  }
};

Config parseConfig(File f);