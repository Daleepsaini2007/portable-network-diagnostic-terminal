// Non-blocking Wi-Fi station management (connect / monitor / timeout).
#pragma once
#include <stdint.h>
#include <time.h>

enum class WifiState : uint8_t { Off, Idle, Connecting, Connected, Failed, Lost };

class WiFiManager {
 public:
  bool begin();                                    // false => Wi-Fi hardware/driver failure
  bool connect(const char* ssid, const char* pass);  // starts attempt, returns immediately
  bool retry();                                    // repeat last attempt
  void disconnect();                               // user-requested; wipes password from RAM
  void update();                                   // call every loop iteration

  WifiState state() const { return state_; }
  bool connected() const { return state_ == WifiState::Connected; }
  bool hardwareOk() const { return hwOk_; }
  const char* error() const { return error_; }
  const char* targetSsid() const { return ssid_; }
  uint32_t connectElapsedMs() const;
  bool hasCredentials() const { return ssid_[0] != '\0'; }
  // True once SNTP has set the system clock (needs internet; may never happen).
  bool timeSynced() const;
  int64_t utcEpoch() const;

 private:
  void fail(const char* why);
  WifiState state_ = WifiState::Off;
  bool hwOk_ = false;
  char ssid_[33] = {0};
  char pass_[65] = {0};
  char error_[40] = {0};
  uint32_t startMs_ = 0;
};
