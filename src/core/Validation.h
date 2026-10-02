// Pure, hardware-independent input validation. No Arduino dependencies.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace core {

enum class TargetKind : uint8_t { Invalid, IPv4, Hostname };

// Strict dotted-quad parser: exactly 4 decimal octets 0..255, no leading
// zeros (ambiguous octal), no whitespace. `out` may be nullptr.
bool parseIPv4(const char* s, uint8_t out[4]);

// RFC 1123-style hostname: 1..253 chars, labels 1..63 chars of [A-Za-z0-9-],
// labels may not start/end with '-'. One trailing dot is tolerated.
// Strings consisting only of digits and dots are NOT hostnames.
bool isValidHostname(const char* s);

// Classifies user input as IPv4 literal, hostname, or invalid.
TargetKind classifyTarget(const char* s, uint8_t ip[4]);

}  // namespace core
