#include "ui/DisplayManager.h"
#include <M5Cardputer.h>
#include <stdio.h>
#include <string.h>

static constexpr int kRowH = 12;

void DisplayManager::begin() {
  auto& d = M5Cardputer.Display;
  d.setRotation(1);  // landscape, keyboard below the panel
  d.setBrightness(96);
  d.setFont(&fonts::Font0);
  d.setTextSize(1.5f);
  d.setTextWrap(false);
  invalidate();
}

void DisplayManager::setBrightness(uint8_t b) { M5Cardputer.Display.setBrightness(b); }

void DisplayManager::invalidate() {
  M5Cardputer.Display.fillScreen(kBg);
  for (int i = 0; i < kRows; ++i) {
    cache_[i][0] = '\0';
    valid_[i] = false;
  }
}

void DisplayManager::draw(int r, const char* text, uint16_t fg, uint16_t bg) {
  if (r < 0 || r >= kRows) return;
  char line[kCols + 1];
  size_t n = strlen(text);
  if (n > (size_t)kCols) n = kCols;
  memcpy(line, text, n);
  memset(line + n, ' ', kCols - n);  // pad: overwrites the whole row, no fillRect needed
  line[kCols] = '\0';

  if (valid_[r] && fgCache_[r] == fg && bgCache_[r] == bg && memcmp(cache_[r], line, kCols) == 0) return;

  auto& d = M5Cardputer.Display;
  d.setTextColor(fg, bg);
  d.drawString(line, 0, r * kRowH + 1);
  // the last 6 px of width beyond 26*9 = 234 px
  d.fillRect(kCols * 9, r * kRowH, 240 - kCols * 9, kRowH, bg);
  memcpy(cache_[r], line, kCols + 1);
  fgCache_[r] = fg;
  bgCache_[r] = bg;
  valid_[r] = true;
}

void DisplayManager::title(const char* text) { draw(0, text, kFg, kBar); }
void DisplayManager::footer(const char* text) { draw(kFooterRow, text, kDim, kBg); }
void DisplayManager::row(int r, const char* text, uint16_t color) { draw(r, text, color, kBg); }

void DisplayManager::rowKV(int r, const char* key, const char* value, uint16_t valueColor) {
  // The key part is dim, the value part coloured: draw as one row with two colours
  // by drawing the whole row in the value colour only when the key is empty.
  char line[kCols + 1];
  snprintf(line, sizeof line, "%s%s", key, value);
  size_t kl = strlen(key);
  if (kl > (size_t)kCols) kl = kCols;
  // cache key must include colour of value; simple approach: draw whole row, then
  // recolour the key prefix. Use cache of the combined string + valueColor.
  if (r < 0 || r >= kRows) return;
  char padded[kCols + 1];
  size_t n = strlen(line);
  if (n > (size_t)kCols) n = kCols;
  memcpy(padded, line, n);
  memset(padded + n, ' ', kCols - n);
  padded[kCols] = '\0';
  if (valid_[r] && fgCache_[r] == valueColor && bgCache_[r] == kBg && memcmp(cache_[r], padded, kCols) == 0) return;
  auto& d = M5Cardputer.Display;
  d.setTextColor(kDim, kBg);
  char kpart[kCols + 1];
  memcpy(kpart, padded, kl);
  kpart[kl] = '\0';
  d.drawString(kpart, 0, r * kRowH + 1);
  d.setTextColor(valueColor, kBg);
  d.drawString(padded + kl, (int)(kl * 9), r * kRowH + 1);
  d.fillRect(kCols * 9, r * kRowH, 240 - kCols * 9, kRowH, kBg);
  memcpy(cache_[r], padded, kCols + 1);
  fgCache_[r] = valueColor;
  bgCache_[r] = kBg;
  valid_[r] = true;
}

void DisplayManager::clearRows(int from, int to) {
  for (int r = from; r <= to; ++r) row(r, "");
}

void DisplayManager::errorBox(const char* msg, bool canRetry) {
  row(3, "[ERROR]", kBad);
  // wrap message over rows 4-6
  char buf[kCols * 3 + 1];
  strlcpy(buf, msg, sizeof buf);
  size_t len = strlen(buf);
  for (int i = 0; i < 3; ++i) {
    char part[kCols + 1] = {0};
    size_t off = (size_t)i * kCols;
    if (off < len) strlcpy(part, buf + off, sizeof part);
    row(4 + i, part, kFg);
  }
  row(8, canRetry ? "[R] Retry   [Esc] Back" : "[Esc] Back", kWarn);
}
