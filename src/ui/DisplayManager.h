// Row-based text UI for the Cardputer's 240x135 display.
// Every row keeps a cache of what is currently drawn, so render() can be called
// at any rate and only changed rows touch the panel (no flicker, no full redraws).
#pragma once
#include <stdint.h>

class DisplayManager {
 public:
  static constexpr int kCols = 26;       // characters per row (9 px/char)
  static constexpr int kRows = 11;       // row 0 = title, 1..9 = content, 10 = footer
  static constexpr int kFirstRow = 1;
  static constexpr int kLastRow = 9;
  static constexpr int kFooterRow = 10;

  // Palette (RGB565)
  static constexpr uint16_t kBg = 0x0000;
  static constexpr uint16_t kFg = 0xFFFF;
  static constexpr uint16_t kDim = 0x8410;
  static constexpr uint16_t kOk = 0x07E0;
  static constexpr uint16_t kBad = 0xF800;
  static constexpr uint16_t kWarn = 0xFFE0;
  static constexpr uint16_t kAccent = 0x07FF;
  static constexpr uint16_t kBar = 0x0010;  // dark blue title bar

  void begin();
  void invalidate();  // clear panel + forget all cached rows (use on screen change)
  void title(const char* text);
  void footer(const char* text);
  void row(int r, const char* text, uint16_t color = kFg);
  void rowKV(int r, const char* key, const char* value, uint16_t valueColor = kFg);
  void clearRow(int r) { row(r, "", kFg); }
  void clearRows(int from, int to);
  // "[ERROR] message" block with the standard footer hint.
  void errorBox(const char* msg, bool canRetry);
  void setBrightness(uint8_t b);

 private:
  void draw(int r, const char* text, uint16_t fg, uint16_t bg);
  char cache_[kRows][kCols + 1];
  uint16_t fgCache_[kRows];
  uint16_t bgCache_[kRows];
  bool valid_[kRows];
};
