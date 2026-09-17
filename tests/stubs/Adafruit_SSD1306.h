#pragma once

#define SSD1306_WHITE 1
#define SSD1306_SWITCHCAPVCC 1

struct Adafruit_SSD1306 {
  Adafruit_SSD1306(int, int, void*, int) {}
  bool begin(int, int) { return true; }
  void clearDisplay() {}
  void setTextColor(int) {}
  void setTextSize(int) {}
  void setCursor(int, int) {}
  void print(const char*) {}
  void drawLine(int, int, int, int, int) {}
  template <class... Args>
  void printf(const char*, Args...) {}
  void display() {}
};
