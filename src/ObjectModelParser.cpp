#include <ObjectModelParser.h>
#include <Debug.h>
#include <Structure.h>

static JsonStreamingParser parser;
static ObjectModelListener listener;

#define MAP_KEY(x, y) if (strcmp(key, x) == 0) current = y;
#define MAP_VALUE(x, y, z) if (strcmp(value, x) == 0) z = y;
#define MAP_TEMP(x, y, z) if (current == x) y.x = strtof(z, NULL);

#undef DEBUG
#undef DEBUGF
#define DEBUG(x)
#define DEBUGF(arg...)

void ObjectModelListener::key(const char *key) {
       MAP_KEY("err",         err)
  else MAP_KEY("status",      status)
  else MAP_KEY("fans",        fans)
  else MAP_KEY("actualValue", actualValue)
  else MAP_KEY("heaters",     heaters)
  else MAP_KEY("active",      active_temp)
  else MAP_KEY("standby",     standby_temp)
  else MAP_KEY("current",     current_temp)
  else MAP_KEY("state",       state)
  else MAP_KEY("timesLeft",   timesLeft)
  else MAP_KEY("slicer",      slicer)
  else MAP_KEY("file",        file)
  else MAP_KEY("duration",    duration)
  else current = unknown;
}
void ObjectModelListener::handle_value(const char *value) {
  switch (current) {
    case err:
      DEBUGF("err: %s\n", value);
      break;
    case status:
      DEBUGF("status: %s\n", value);
           MAP_VALUE("idle",         idle,         model->state)
      else MAP_VALUE("starting",     starting,     model->state)
      else MAP_VALUE("updating",     updating,     model->state)
      else MAP_VALUE("halted",       halted,       model->state)
      else MAP_VALUE("off",          printer_off,  model->state)
      else MAP_VALUE("pausing",      pausing,      model->state)
      else MAP_VALUE("resuming",     resuming,     model->state)
      else MAP_VALUE("paused",       paused,       model->state)
      else MAP_VALUE("simulating",   simulating,   model->state)
      else MAP_VALUE("processing",   processing,   model->state)
      else MAP_VALUE("changingTool", changingTool, model->state)
      else MAP_VALUE("busy",         busy,         model->state)
      else MAP_VALUE("idle",         idle,         model->state)
      break;
      if (model->state == idle) {
        model->reset_print_progress();
      }
    case actualValue:
      model->fans[index] = strtof(value, NULL) * 100;
      DEBUGF("fan %d = %d%%\n", index, model->fans[index]);
      break;
    case active_temp:
    case current_temp:
    case standby_temp:
      if (parent() == heaters) {
             MAP_TEMP(active_temp,  model->heaters[index], value)
        else MAP_TEMP(standby_temp, model->heaters[index], value)
        else MAP_TEMP(current_temp, model->heaters[index], value)
      }
      break;
    case state:
      if (parent() == heaters) {
        DEBUGF("heater: %d = %s\n", index, value);
             MAP_VALUE("active",  active,     model->heaters[index].state)
        else MAP_VALUE("standby", standby,    model->heaters[index].state)
        else MAP_VALUE("fault",   fault,      model->heaters[index].state)
        else MAP_VALUE("tuning",  tuning,     model->heaters[index].state)
        else MAP_VALUE("off",     heater_off, model->heaters[index].state)
        DEBUGF("  current: %d = %.1f\n", index, model->heaters[index].current_temp);
        DEBUGF("  active:  %d = %.1f\n", index, model->heaters[index].active_temp);
        DEBUGF("  standby: %d = %.1f\n", index, model->heaters[index].standby_temp);
      }
      break;
    case duration:
      model->print_duration = strtol(value, NULL, 10);
      break;
    case slicer:
      if (parent() == timesLeft)
        model->print_remaining = strtol(value, NULL, 10);
      break;
    case file:
      if (parent() == timesLeft)
        model->print_remaining = strtol(value, NULL, 10);
      break;
    default:
      break;
  }
}

void parseObjectModel(char c, ObjectModel *model) {
  if (!listener.initialized) {
    listener.initialized = true;
    parser.setListener(&listener);
  }
  listener.model = model;

  parser.parse(c);

  if (listener.need_reset) {
    listener.need_reset = false;
    parser.reset();

    model->ready = true;
  }
}
