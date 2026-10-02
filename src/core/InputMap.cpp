#include "core/InputMap.h"

namespace core {

InputEvent translateKeys(const KeyFlags& k, bool textMode) {
  InputEvent e;
  if (k.up)    { e.type = EvType::Up;    return e; }
  if (k.down)  { e.type = EvType::Down;  return e; }
  if (k.left)  { e.type = EvType::Left;  return e; }
  if (k.right) { e.type = EvType::Right; return e; }
  if (k.esc)   { e.type = EvType::Back;  return e; }
  if (k.del)   { e.type = EvType::Clear; return e; }
  if (k.enter) { e.type = EvType::Select; return e; }
  if (k.tab)   { e.type = EvType::Toggle; return e; }
  if (k.backspace) {
    e.type = textMode ? EvType::Backspace : EvType::Back;
    return e;
  }
  if (k.word && k.wordLen > 0) {
    char c = k.word[0];
    if (textMode) { e.type = EvType::Char; e.ch = c; return e; }
    switch (c) {
      case ';': e.type = EvType::Up; return e;
      case '.': e.type = EvType::Down; return e;
      case ',': e.type = EvType::Left; return e;
      case '/': e.type = EvType::Right; return e;
      case 'r': case 'R': e.type = EvType::Refresh; return e;
      case 'c': case 'C': e.type = EvType::Clear; return e;
      default: e.type = EvType::Char; e.ch = c; return e;
    }
  }
  return e;
}

}  // namespace core
