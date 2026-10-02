// Parsers for user-entered strings. Pure C++.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "core/Validation.h"

namespace core {

enum class ParseStatus : uint8_t { Ok, Empty, BadTarget, BadCount };

struct PingArgs {
  char target[64];
  TargetKind kind;
  uint8_t ip[4];
  uint8_t count;
};

constexpr uint8_t kDefaultPingCount = 4;
constexpr uint8_t kMaxPingCount = 20;

// Parses "<host-or-ip> [count]" (count 1..20, default 4).
ParseStatus parsePingArgs(const char* input, PingArgs& out);
const char* parseStatusText(ParseStatus s);

}  // namespace core
