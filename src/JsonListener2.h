#ifndef _JSON_LISTENER_2_H_
#define _JSON_LISTENER_2_H_

#include <JsonListener.h>
#include <stack>

#define IS_ARRAY 0x80

#define MAP_KEY(x, y) if (strcmp(key, x) == 0) current = y;

template <typename T>
class JsonListener2 : public JsonListener {
  public:
  uint16_t index = 0;

  virtual void key(const char *key) override =0;
  inline void value(const char *value) override {
    handle_value(value);
    if (in_array())
      ++index;
  }

  virtual void handle_value(const char *value) =0;
  inline void endDocument() override {
    while (!stack.empty()) stack.pop();
  }
  inline void startDocument() override {
    stack.push(0);
  }
  inline void startArray() override {
    stack.push(current | IS_ARRAY);
  }
  inline void endArray() override {
    index = 0; // FIXME arrays of arrays are going to result in inconsistent index
    pop();
  }
  inline void startObject() override {
    stack.push(in_array() != 0 ? parent() : current);
    current = parent();
  }
  inline void endObject() override {
    pop();
    if (in_array())
      ++index;
  }
  inline void whitespace(char c) override {}

  inline bool in_array() {
    return (stack.top() & IS_ARRAY) != 0;
  }

  inline T parent() {
    return static_cast<T>(stack.top() & ~IS_ARRAY);
  }

  protected:
  T current = static_cast<T>(0);
  private:
  std::stack<uint8_t> stack;
  void pop() {
    stack.pop();
    current = parent();
  }
};

#endif