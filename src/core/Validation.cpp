#include "core/Validation.h"

namespace core {

static bool isDigit(char c) { return c >= '0' && c <= '9'; }
static bool isAlnum(char c) {
  return isDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool parseIPv4(const char* s, uint8_t out[4]) {
  if (!s) return false;
  uint8_t tmp[4];
  int octet = 0;
  const char* p = s;
  while (octet < 4) {
    if (!isDigit(*p)) return false;
    int digits = 0;
    int value = 0;
    const char* start = p;
    while (isDigit(*p)) {
      value = value * 10 + (*p - '0');
      if (++digits > 3 || value > 255) return false;
      ++p;
    }
    if (digits > 1 && *start == '0') return false;  // leading zero
    tmp[octet++] = static_cast<uint8_t>(value);
    if (octet < 4) {
      if (*p != '.') return false;
      ++p;
    }
  }
  if (*p != '\0') return false;
  if (out) {
    for (int i = 0; i < 4; ++i) out[i] = tmp[i];
  }
  return true;
}

bool isValidHostname(const char* s) {
  if (!s || !*s) return false;
  size_t len = 0;
  while (s[len]) ++len;
  if (s[len - 1] == '.') --len;  // tolerate one trailing dot
  if (len == 0 || len > 253) return false;

  bool allNumericOrDots = true;
  size_t labelLen = 0;
  for (size_t i = 0; i < len; ++i) {
    char c = s[i];
    if (c == '.') {
      if (labelLen == 0) return false;    // empty label
      if (s[i - 1] == '-') return false;  // label ends with '-'
      labelLen = 0;
      continue;
    }
    if (!(isAlnum(c) || c == '-')) return false;
    if (c == '-' && labelLen == 0) return false;  // label starts with '-'
    if (!isDigit(c)) allNumericOrDots = false;
    if (++labelLen > 63) return false;
  }
  if (labelLen == 0) return false;     // e.g. "a.com.." leaves an empty last label
  if (s[len - 1] == '-') return false;
  if (allNumericOrDots) return false;  // e.g. "999.1.1.1" is not a hostname
  return true;
}

TargetKind classifyTarget(const char* s, uint8_t ip[4]) {
  if (!s || !*s) return TargetKind::Invalid;
  if (parseIPv4(s, ip)) return TargetKind::IPv4;
  if (isValidHostname(s)) return TargetKind::Hostname;
  return TargetKind::Invalid;
}

}  // namespace core
