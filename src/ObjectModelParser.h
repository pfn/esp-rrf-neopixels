#include <Arduino.h>
#include <JsonListener.h>
#include <JsonListener2.h>
#include <JsonStreamingParser.h>
#include <stack>

#include <Debug.h>
#include <Structure.h>

enum document_state {
  TOP, unknown,
  fans, actualValue,
  heaters, active_temp, standby_temp, current_temp, state,
  job, timesLeft, slicer, file, duration,
  status,
  err
};

#define IS_ARRAY 0x80

class ObjectModelListener : public JsonListener2<enum document_state> {
  public:
  ObjectModel* model;
  bool need_reset = false;
  bool initialized = false;

  virtual void key(const char *key) override;
  virtual void handle_value(const char *key) override;

  inline void endDocument() override {
     need_reset = true;
     JsonListener2::endDocument();
  }
};

void parseObjectModel(char c, ObjectModel *model);