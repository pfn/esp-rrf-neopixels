#include <Adafruit_NeoPixel.h>

#include <Config.h>
#include <Structure.h>

#define COLD_TEMP 50
#define TEMP_HYSTERESIS 5
#define TRANSLATE(i, px) (config.leds[i].reverse \
  ? config.common_count - 1 - (px + config.leds[i].offset) % config.common_count \
  : (px + config.leds[i].offset) % config.common_count)
#define TRANSLATE_FILL(i, px, count) (max(0, (TRANSLATE(i, px) - count + 1) % config.common_count))

static Adafruit_NeoPixel *neopixels[MAX_LEDS] = {0};

uint8_t rescale_temp(uint8_t i, float temp, uint16_t setpoint) {
  // -2 to allow for overshoot and to animate to indicate activity
  if (temp < setpoint + TEMP_HYSTERESIS) {
    float x = max(0, (setpoint - config.leds[i].temp_base)) / (config.common_count - 2.0f);
    return round((temp - config.leds[i].temp_base) / x);
  } else {
    return round(config.common_count * max(setpoint, (uint16_t)COLD_TEMP) / temp);
  }
}

void render_connecting(bool in_config_portal) {
  static uint32_t last_tick = millis();
  static uint32_t color = 0;
  if (millis() - last_tick > 500) {
    last_tick = millis();
    if (in_config_portal) {
      color = color == 0 ? 0xff00ff : 0;
    } else {
      color = color == 0 ? 0xffff00 : 0;
    }
  }
  for (uint8_t i = 0; i < config.leds_count; ++i) {
    neopixels[i]->setPixelColor(0, color);
    neopixels[i]->show();
  }
}

void render_neopixels() {
  static uint8_t tick = 0;
  static uint8_t px[MAX_LEDS] = {1};

  // slow down animations so they aren't so anxiety inducing, but render often for responsiveness to change
  tick = (tick + 1) % 5;

  if (!object_model.ready || millis() - object_model.last_update > config.query_interval * 2) {
    for (uint8_t i = 0; i < config.leds_count; ++i) {
      neopixels[i]->fill(config.leds[i].startup_color);
      neopixels[i]->setPixelColor(TRANSLATE(i, px[i]) % config.common_count, 0);
      if (!tick)
        px[i] = (px[i] + 1) % config.common_count;
    }
  } else {
    uint8_t fill_count = -1;

    for (uint8_t i = 0; i < config.leds_count; ++i) {
      if (object_model.state == printer_off) {
        neopixels[i]->fill(0);
        continue;
      }

      uint8_t display_item = config.leds[i].display_item;

      if (display_item == 0) {
        // display priority:
        // show a heating heater
        // first faulted heater
        // printing, pause/resume, busy
        // first active heater
        // first standby heater
        // first off hot heater > 50C
        // first running fan
        // remaining printer status
        for (uint8_t t = 0; !display_item && t < MAX_TOOLS; ++t) {
          if (object_model.heaters[t].state == active &&
            object_model.heaters[t].current_temp < (object_model.heaters[t].active_temp - 2))
            display_item = HEATER_FLAG | t;
        }
        for (uint8_t t = 0; !display_item && t < MAX_TOOLS; ++t) {
          if (object_model.heaters[t].state == fault)
            display_item = HEATER_FLAG | t;
        }
        switch (object_model.state) {
          case processing:
          case simulating:
          case resuming:
          case pausing:
          case paused:
          case busy:
          case halted:
            display_item = DISPLAY_STATUS;
            break;
          default:
            break;
        }

        for (uint8_t t = 0; !display_item && t < MAX_TOOLS; ++t) {
          if (object_model.heaters[t].state == active)
            display_item = HEATER_FLAG | t;
        }

        for (uint8_t t = 0; !display_item && t < MAX_TOOLS; ++t) {
          if (object_model.heaters[t].state == standby)
            display_item = HEATER_FLAG | t;
        }

        for (uint8_t t = 0; !display_item && t < MAX_TOOLS; ++t) {
          if (object_model.heaters[t].state == heater_off
            && object_model.heaters[t].current_temp > COLD_TEMP)
            display_item = HEATER_FLAG | t;
        }

        for (uint8_t t = 0; !display_item && t < MAX_TOOLS; ++t) {
          if (object_model.fans[t] > 0)
            display_item = FAN_FLAG | t;
        }
      }

      if (display_item & HEATER_FLAG) {
        uint8_t heater = display_item & ITEM_MASK;
        if (heater == DISPLAY_ANY) {
          for (uint8_t h = 0; heater == DISPLAY_ANY && h < MAX_TOOLS; ++h) {
            if (object_model.heaters[h].state == fault)
              heater = h;
          }
          // first actively heating heater before any active
          for (uint8_t h = 0; heater == DISPLAY_ANY && h < MAX_TOOLS; ++h) {
            if ((object_model.heaters[h].state == active || object_model.heaters[h].state == tuning) &&
              object_model.heaters[h].current_temp < (object_model.heaters[h].active_temp - 2))
              heater = h;
          }
          for (uint8_t h = 0; heater == DISPLAY_ANY && h < MAX_TOOLS; ++h) {
            if (object_model.heaters[h].state == active || object_model.heaters[h].state == tuning)
              heater = h;
          }
          for (uint8_t h = 0; heater == DISPLAY_ANY && h < MAX_TOOLS; ++h) {
            if (object_model.heaters[h].state == standby)
              heater = h;
          }
          for (uint8_t h = 0; heater == DISPLAY_ANY && h < MAX_TOOLS; ++h) {
            if (object_model.heaters[h].current_temp > COLD_TEMP)
              heater = h;
          }

          if (heater == DISPLAY_ANY)
            heater = 0;
        }
        switch (object_model.heaters[heater].state) {
          case fault:
            neopixels[i]->fill(0);
            px[i] = (px[i] + 1) % 2;
            for (uint8_t j = 0; j < config.common_count; j += 2) {
              neopixels[i]->setPixelColor(j + px[i], config.heater.heating);
            }
            break;
          case active:
          case tuning:
          case standby:
            uint16_t setpoint;
            if (object_model.heaters[heater].state == active || object_model.heaters[heater].state == tuning) {
              setpoint = object_model.heaters[heater].active_temp;
            } else {
              setpoint = object_model.heaters[heater].standby_temp;
            }
            neopixels[i]->fill(config.heater.secondary);
            fill_count = max((uint8_t) 1, rescale_temp(i, object_model.heaters[heater].current_temp, setpoint));
            neopixels[i]->fill(object_model.heaters[heater].current_temp > setpoint + TEMP_HYSTERESIS
              ? config.heater.cooling : config.heater.heating, TRANSLATE_FILL(i, 0, fill_count), fill_count);
            break;
          case heater_off:
            fill_count = rescale_temp(i, object_model.heaters[heater].current_temp, 0);
            neopixels[i]->fill(config.state.idle);
            if (fill_count > 0) {
              neopixels[i]->fill(config.heater.cooling, TRANSLATE_FILL(i, 0, fill_count), fill_count);
            } else {
              neopixels[i]->setPixelColor(TRANSLATE(i, 0), config.heater.cooling);
            }
            break;
        }
      } else if (display_item & FAN_FLAG) {
        uint8_t fan = display_item & ITEM_MASK;
        if (fan == DISPLAY_ANY) {
          for (uint8_t f = 0; fan == DISPLAY_ANY && f < MAX_TOOLS; ++f) {
            if (object_model.fans[f] > 0)
              fan = f;
          }
          if (fan == DISPLAY_ANY)
            fan = 0;
        }

        // - 2 so that animation can occur to indicate activity
        fill_count = round(object_model.fans[fan]/100.0f * (config.common_count - 2));
        neopixels[i]->fill(config.fan.secondary);
        if (fill_count > 0)
          neopixels[i]->fill(config.fan.active, TRANSLATE_FILL(i, 0, fill_count), fill_count);
      } else {
        switch (object_model.state) {
          case starting:
            fill_count = 1;
            neopixels[i]->fill(config.state.starting);
            break;
          case updating:
            fill_count = 1;
            neopixels[i]->fill(config.state.updating);
          case halted:
            neopixels[i]->fill(0);
            px[i] = (px[i] + 1) % 2;
            for (uint8_t j = 0; j < config.common_count; j += 2) {
              neopixels[i]->setPixelColor(j + px[i], config.state.halted);
            }
            break;
          case printer_off: break; // no-op, handled at the top
          case pausing:
          case paused:
            fill_count = 1;
            neopixels[i]->fill(config.state.paused);
            break;
          case resuming:
          case simulating:
          case processing:
            fill_count = max(1.0f, round(object_model.print_progress() * (config.common_count - 2)));
            neopixels[i]->fill(config.printing.secondary);
            if (fill_count > 0)
              neopixels[i]->fill(config.printing.done, TRANSLATE_FILL(i, 0, fill_count), fill_count);
            break;
          case changingTool:
            fill_count = 1;
            neopixels[i]->fill(config.state.changingTool);
            break;
          case busy:
            fill_count = 1;
            neopixels[i]->fill(config.state.busy);
            break;
          case idle:
            neopixels[i]->fill(config.state.idle);
            break;
        }
      }

      if (fill_count > 0 && fill_count < config.common_count) {
        px[i] = max(fill_count, px[i]);
        neopixels[i]->setPixelColor(TRANSLATE(i, px[i]), 0);
        if (!tick)
          px[i] = (px[i] + 1) % config.common_count;
      }
    }
  }
  for (uint8_t i = 0; i < config.leds_count; ++i) {
    neopixels[i]->show();
  }
}

void init_neopixels() {
  config.leds_count = 0;
  config.common_count = 255;
  for (uint8_t i = 0; i < MAX_LEDS; ++i) {
    if (neopixels[i] != NULL) {
      delete neopixels[i];
      neopixels[i] = NULL;
    }
    if (config.leds[i].active) {
      pinMode(config.leds[i].pin, OUTPUT);
      neopixels[i] = new Adafruit_NeoPixel(config.leds[i].count, config.leds[i].pin, config.leds[i].type);
      neopixels[i]->begin();
      neopixels[i]->setBrightness(255);
      neopixels[i]->fill(config.state.starting);
      neopixels[i]->show();
      config.common_count = min(config.common_count, config.leds[i].count);
      ++config.leds_count;
    }
  }
  DEBUGF("Configured %d neopixels, %d LEDs each\n", config.leds_count, config.common_count);
  uint32_t animationColor[MAX_LEDS] = {0};

  for (uint8_t i = 0; i < config.leds_count; ++i) {
    animationColor[i] = config.leds[i].startup_color;
  }
  for (uint8_t i = 0; i < config.leds_count; ++i) { // once per ring
    for (uint8_t j = 0; j < config.common_count; ++j) {
      for (uint8_t p = 0; p < config.leds_count; ++p) { // spin LEDs around the ring
        neopixels[p]->fill(0);
        neopixels[p]->setPixelColor(TRANSLATE(p, j),
          animationColor[i + p % config.leds_count]); 
        neopixels[p]->show();
      }
      delay(30);
    }
  }
  for (uint8_t i = 0; i < config.leds_count; ++i) { // once per ring
    for(uint8_t j = 0; j < 255; ++j) { // fade in
      for (uint8_t p = 0; p < config.leds_count; ++p) {
        neopixels[p]->setBrightness(j); 
        neopixels[p]->fill(animationColor[i + p % config.leds_count]);
        neopixels[p]->show();
      }
      delay(2);
    }
    for (uint8_t j = 255; j > 0; --j) { // fade out
      for (uint8_t p = 0; p < config.leds_count; ++p) {
        neopixels[p]->setBrightness(j); 
        neopixels[p]->fill(animationColor[i + p % config.leds_count]);
        neopixels[p]->show();
      }
      delay(2);
    }
  }

  for (uint8_t p = 0; p < config.leds_count; ++p) {
    neopixels[p]->setBrightness(config.leds[p].brightness); 
  }
}