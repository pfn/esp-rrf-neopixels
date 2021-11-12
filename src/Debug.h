#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <Print.h>
#include <math.h>

#define DEBUGF(args...) do { debug_buffer.printf(args); if (!config.tx_passthru) Serial1.printf(args); } while (0)
#define DEBUGW(buf, size) do { debug_buffer.write(buf, size); if (!config.tx_passthru) Serial1.printf(args); } while (0)
#define DEBUG(x) do { debug_buffer.println(x); if (!config.tx_passthru) Serial1.println(x); } while (0)

class ring_buffer : public Print {
  public:
  ring_buffer(uint16_t size) : buffer_size(size) {
    buffer = (uint8_t *) malloc(size);
  }
  ~ring_buffer() {
    free(buffer);
  }

  inline uint8_t &operator[](size_t i) {
    return buffer[(read_pos + i) % content_size];
  }

  inline size_t write(uint8_t x) override {
    buffer[write_pos] = x;
    if (content_size < buffer_size) {
      content_size++;
    } else {
      read_pos = (read_pos + 1) % buffer_size;
    }
    write_pos = (write_pos + 1) % buffer_size;
    return 1;
  }

  inline int read() {
    if (content_size == 0) return -1;
    uint8_t r = buffer[read_pos];
    read_pos = (read_pos + 1) % buffer_size;
    content_size = std::max(content_size - 1, 0);
    return r;
  }

  inline size_t write(const uint8_t *b, size_t size) override {
    if (size >= buffer_size) {
      memcpy(buffer, b + buffer_size - size, buffer_size);
      read_pos = 0;
      write_pos = 0;
    } else if (write_pos + size > buffer_size) {
      uint16_t first = buffer_size - write_pos;
      uint16_t remaining = size - first;
      memcpy(buffer + write_pos, b, first);
      memcpy(buffer, b + first, remaining);
      write_pos = (write_pos + size) % buffer_size;
    } else {
      memcpy(buffer + write_pos, b, size);
      write_pos = (write_pos + size) % buffer_size;
    }
    if (size >= buffer_size) {
      content_size = buffer_size;
    } else if (content_size + size > buffer_size) {
      read_pos = (read_pos + size) % buffer_size;
      content_size = buffer_size;
    } else {
      content_size = content_size + size;
    }
    return size;
  }

  inline size_t size() {
    return content_size;
  }

  private:
  uint16_t content_size;
  uint16_t buffer_size;
  uint16_t read_pos;
  uint16_t write_pos;
  uint8_t *buffer;
};

extern ring_buffer debug_buffer;
#endif