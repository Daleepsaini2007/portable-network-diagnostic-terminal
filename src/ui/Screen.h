// Interface implemented by every full-screen view.
#pragma once
#include "core/InputMap.h"

class App;
class DisplayManager;

class IScreen {
 public:
  virtual ~IScreen() {}
  virtual void onEnter(App&) {}
  virtual void onLeave(App&) {}
  // true while a text field is being edited (printable keys become characters)
  virtual bool wantsText() const { return false; }
  virtual void onInput(App& app, const core::InputEvent& ev) = 0;
  virtual void onTick(App&) {}
  // Must be idempotent: DisplayManager only redraws rows whose content changed.
  virtual void render(App& app, DisplayManager& d) = 0;
};
