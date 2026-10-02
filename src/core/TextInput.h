// Fixed-capacity single-line text editor. Pure C++, no heap.
#pragma once
#include <stddef.h>

namespace core {

enum class CharSet { Printable, Target };  // Target: [A-Za-z0-9.-_: ]

class TextInput {
 public:
  static constexpr size_t kMaxCap = 64;
  explicit TextInput(size_t capacity = 32, CharSet set = CharSet::Printable);
  void configure(size_t capacity, CharSet set);
  bool insert(char c);   // false if rejected (full or disallowed)
  bool backspace();      // false if already empty
  void clear();
  void set(const char* s);  // truncates to capacity, drops disallowed chars
  const char* c_str() const { return buf_; }
  size_t length() const { return len_; }
  bool empty() const { return len_ == 0; }
  size_t capacity() const { return cap_; }

 private:
  bool allowed(char c) const;
  char buf_[kMaxCap + 1];
  size_t len_ = 0;
  size_t cap_;
  CharSet set_;
};

}  // namespace core
