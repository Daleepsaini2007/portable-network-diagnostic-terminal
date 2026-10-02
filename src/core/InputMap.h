// Translates raw keyboard flags into logical input events. Pure C++.
// The Cardputer has no dedicated arrow keys: arrows are Fn+; , . /
// (reported by M5Cardputer as up/left/down/right flags) and the keycaps
// ; , . / carry arrow glyphs, so in navigation mode we accept them plain too.
#pragma once
#include <stddef.h>

namespace core {

struct KeyFlags {
  bool up = false, down = false, left = false, right = false;
  bool enter = false, backspace = false, del = false, esc = false, tab = false;
  bool fn = false;
  const char* word = nullptr;  // printable chars pressed (not NUL-terminated)
  size_t wordLen = 0;
};

enum class EvType : unsigned char {
  None, Up, Down, Left, Right, Select, Back, Refresh, Clear, Toggle, Backspace, Char
};

struct InputEvent {
  EvType type = EvType::None;
  char ch = 0;
};

// textMode=true: printable keys become Char events; Back only from Esc;
// Backspace edits; Fn+Backspace (del) clears the field.
// textMode=false: navigation shortcuts apply (; up, . down, r refresh, c clear).
InputEvent translateKeys(const KeyFlags& k, bool textMode);

}  // namespace core
