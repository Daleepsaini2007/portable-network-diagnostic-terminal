// Tiny critical-section helper for sharing small structs between the Arduino
// loop task and short-lived worker / lwIP ping tasks.
#pragma once
#include <Arduino.h>

class SpinLock {
 public:
  void lock() { portENTER_CRITICAL(&mux_); }
  void unlock() { portEXIT_CRITICAL(&mux_); }

 private:
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

class ScopedLock {
 public:
  explicit ScopedLock(SpinLock& l) : l_(l) { l_.lock(); }
  ~ScopedLock() { l_.unlock(); }

 private:
  SpinLock& l_;
};
