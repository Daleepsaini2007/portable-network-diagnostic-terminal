#include "core/LogFormat.h"
#include <stdio.h>
#include <string.h>

namespace core {

CsvBuilder::CsvBuilder(char* buf, size_t cap) : buf_(buf), cap_(cap) {
  if (cap_ > 0) buf_[0] = '\0';
  else ok_ = false;
}

void CsvBuilder::put(char c) {
  if (!ok_) return;
  if (len_ + 1 >= cap_) { ok_ = false; return; }  // keep room for NUL
  buf_[len_++] = c;
  buf_[len_] = '\0';
}

void CsvBuilder::sep() {
  if (!first_) put(',');
  first_ = false;
}

CsvBuilder& CsvBuilder::str(const char* s) {
  sep();
  if (!s) s = "";
  bool quote = false;
  for (const char* p = s; *p; ++p) {
    if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') { quote = true; break; }
  }
  bool formula = (s[0] == '=' || s[0] == '+' || s[0] == '-' || s[0] == '@' ||
                  s[0] == '\t' || s[0] == '\r');
  if (formula) quote = true;
  if (quote) put('"');
  if (formula) put('\'');
  for (const char* p = s; *p; ++p) {
    if (*p == '"') put('"');
    put(*p);
  }
  if (quote) put('"');
  return *this;
}

CsvBuilder& CsvBuilder::i32(int32_t v) {
  char t[16];
  snprintf(t, sizeof t, "%ld", static_cast<long>(v));
  sep();
  for (const char* p = t; *p; ++p) put(*p);
  return *this;
}

CsvBuilder& CsvBuilder::u32(uint32_t v) {
  char t[16];
  snprintf(t, sizeof t, "%lu", static_cast<unsigned long>(v));
  sep();
  for (const char* p = t; *p; ++p) put(*p);
  return *this;
}

CsvBuilder& CsvBuilder::f1(float v) {
  char t[24];
  snprintf(t, sizeof t, "%.1f", static_cast<double>(v));
  sep();
  for (const char* p = t; *p; ++p) put(*p);
  return *this;
}

CsvBuilder& CsvBuilder::empty() {
  sep();
  return *this;
}

CsvBuilder& CsvBuilder::finishLine() {
  put('\n');
  return *this;
}

// Howard Hinnant's civil-from-days algorithm.
bool formatUtc(char* out, size_t cap, int64_t epoch) {
  if (!out || cap < 21 || epoch < 0) return false;
  int64_t days = epoch / 86400;
  int64_t rem = epoch % 86400;
  int hh = static_cast<int>(rem / 3600);
  int mm = static_cast<int>((rem % 3600) / 60);
  int ss = static_cast<int>(rem % 60);
  days += 719468;
  int64_t era = days / 146097;
  int64_t doe = days - era * 146097;
  int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int64_t y = yoe + era * 400;
  int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  int64_t mp = (5 * doy + 2) / 153;
  int d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
  int m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
  if (m <= 2) ++y;
  snprintf(out, cap, "%04d-%02d-%02dT%02d:%02d:%02dZ", static_cast<int>(y), m, d, hh, mm, ss);
  return true;
}

void formatUptime(char* out, size_t cap, uint64_t ms) {
  if (!out || cap == 0) return;
  uint64_t s = ms / 1000;
  unsigned d = static_cast<unsigned>(s / 86400);
  unsigned h = static_cast<unsigned>((s % 86400) / 3600);
  unsigned m = static_cast<unsigned>((s % 3600) / 60);
  unsigned sec = static_cast<unsigned>(s % 60);
  if (d) snprintf(out, cap, "%ud %02u:%02u:%02u", d, h, m, sec);
  else snprintf(out, cap, "%02u:%02u:%02u", h, m, sec);
}

}  // namespace core
