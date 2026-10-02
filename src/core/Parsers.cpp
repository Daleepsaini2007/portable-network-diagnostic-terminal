#include "core/Parsers.h"
#include <string.h>

namespace core {

ParseStatus parsePingArgs(const char* input, PingArgs& out) {
  memset(&out, 0, sizeof out);
  out.count = kDefaultPingCount;
  if (!input) return ParseStatus::Empty;

  while (*input == ' ') ++input;
  if (!*input) return ParseStatus::Empty;

  size_t n = 0;
  while (input[n] && input[n] != ' ') ++n;
  if (n >= sizeof out.target) return ParseStatus::BadTarget;
  memcpy(out.target, input, n);
  out.target[n] = '\0';

  out.kind = classifyTarget(out.target, out.ip);
  if (out.kind == TargetKind::Invalid) return ParseStatus::BadTarget;

  const char* rest = input + n;
  while (*rest == ' ') ++rest;
  if (*rest) {
    unsigned v = 0;
    int digits = 0;
    const char* p = rest;
    while (*p >= '0' && *p <= '9') {
      v = v * 10 + static_cast<unsigned>(*p - '0');
      if (++digits > 3) return ParseStatus::BadCount;
      ++p;
    }
    while (*p == ' ') ++p;
    if (digits == 0 || *p != '\0') return ParseStatus::BadCount;
    if (v < 1 || v > kMaxPingCount) return ParseStatus::BadCount;
    out.count = static_cast<uint8_t>(v);
  }
  return ParseStatus::Ok;
}

const char* parseStatusText(ParseStatus s) {
  switch (s) {
    case ParseStatus::Ok: return "OK";
    case ParseStatus::Empty: return "Enter a host or IP";
    case ParseStatus::BadTarget: return "Invalid host / IP";
    case ParseStatus::BadCount: return "Count must be 1-20";
  }
  return "?";
}

}  // namespace core
