// CSV record building + timestamp/uptime formatting. Pure C++ (no heap).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace core {

// Appends comma-separated fields into a caller-provided buffer.
//  - str() quotes fields containing , " CR LF and doubles embedded quotes.
//  - str() neutralises spreadsheet formula injection: a field starting with
//    = + - @ TAB or CR gets a leading apostrophe (SSIDs are attacker-controlled).
//  - Overflow never writes past the buffer; ok() turns false.
class CsvBuilder {
 public:
  CsvBuilder(char* buf, size_t cap);
  CsvBuilder& str(const char* s);
  CsvBuilder& i32(int32_t v);
  CsvBuilder& u32(uint32_t v);
  CsvBuilder& f1(float v);  // one decimal place
  CsvBuilder& empty();
  CsvBuilder& finishLine();  // appends '\n'
  bool ok() const { return ok_; }
  size_t size() const { return len_; }
  const char* c_str() const { return buf_; }

 private:
  void sep();
  void put(char c);
  char* buf_;
  size_t cap_;
  size_t len_ = 0;
  bool ok_ = true;
  bool first_ = true;
};

// "YYYY-MM-DDTHH:MM:SSZ" from a Unix epoch (UTC). Returns false if cap < 21
// or epoch < 0.
bool formatUtc(char* out, size_t cap, int64_t epochSeconds);
// "HH:MM:SS" or "Nd HH:MM:SS".
void formatUptime(char* out, size_t cap, uint64_t uptimeMs);

}  // namespace core
