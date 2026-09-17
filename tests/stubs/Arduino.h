#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>

using std::isfinite;

#define OUTPUT 1
#define INPUT_PULLUP 2
#define SERIAL_8N1 0

inline unsigned long fakeTime = 0;
inline unsigned long millis() { return fakeTime; }
inline void delay(int) {}
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return 1; }
inline void ledcSetup(int, int, int) {}
inline void ledcAttachPin(int, int) {}
inline void ledcWriteTone(int, int) {}

template <class T>
T constrain(T value, T minimum, T maximum) {
  return std::max(minimum, std::min(value, maximum));
}

class String : public std::string {
 public:
  using std::string::operator=;
};

inline std::string serialOutput;

struct HardwareSerial {
  explicit HardwareSerial(int = 0) {}
  void begin(int, int = 0, int = 0, int = 0) {}
  int available() { return 0; }
  char read() { return 0; }
  void write(char) {}
  void println(const char* value) {
    serialOutput += value;
    serialOutput += '\n';
  }
  template <class... Args>
  void printf(const char* format, Args... args) {
    char buffer[1024];
    const int length = std::snprintf(buffer, sizeof(buffer), format, args...);
    if (length > 0) {
      serialOutput.append(buffer,
                          static_cast<size_t>(std::min<int>(length, sizeof(buffer) - 1)));
    }
  }
};

inline HardwareSerial Serial;
