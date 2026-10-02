#include "core/TextInput.h"

namespace core {

TextInput::TextInput(size_t capacity, CharSet set) : cap_(capacity), set_(set) {
  configure(capacity, set);
}

void TextInput::configure(size_t capacity, CharSet set) {
  cap_ = capacity > kMaxCap ? kMaxCap : capacity;
  set_ = set;
  clear();
}

bool TextInput::allowed(char c) const {
  if (set_ == CharSet::Printable) return c >= 0x20 && c <= 0x7E;
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         c == '.' || c == '-' || c == '_' || c == ':' || c == ' ';
}

bool TextInput::insert(char c) {
  if (len_ >= cap_ || !allowed(c)) return false;
  buf_[len_++] = c;
  buf_[len_] = '\0';
  return true;
}

bool TextInput::backspace() {
  if (len_ == 0) return false;
  buf_[--len_] = '\0';
  return true;
}

void TextInput::clear() {
  len_ = 0;
  buf_[0] = '\0';
}

void TextInput::set(const char* s) {
  clear();
  if (!s) return;
  for (; *s; ++s) insert(*s);
}

}  // namespace core
