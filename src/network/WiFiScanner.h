// Passive-discovery style Wi-Fi scan (standard 802.11 beacon/probe scan only).
#pragma once
#include <stdint.h>

struct ScanEntry {
  char ssid[33];
  int8_t rssi;
  uint8_t channel;
  uint8_t auth;  // wifi_auth_mode_t
};

enum class ScanState : uint8_t { Idle, Scanning, Done, Failed };

class WiFiScanner {
 public:
  static constexpr int kMaxResults = 24;

  // Returns false if a scan is running or the cooldown has not elapsed.
  bool start();
  void update();
  ScanState state() const { return state_; }
  int count() const { return count_; }
  const ScanEntry& entry(int i) const { return results_[i]; }
  const char* error() const { return error_; }
  uint32_t ageMs() const;  // since last completed scan, UINT32_MAX if never
  bool cooldownActive() const;

 private:
  void finish(int n);
  ScanState state_ = ScanState::Idle;
  ScanEntry results_[kMaxResults];
  int count_ = 0;
  uint32_t startMs_ = 0;
  uint32_t doneMs_ = 0;
  bool everDone_ = false;
  char error_[32] = {0};
};
