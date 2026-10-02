#include "network/WiFiScanner.h"
#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include "AppConfig.h"

bool WiFiScanner::cooldownActive() const {
  return everDone_ && (millis() - doneMs_) < WIFI_SCAN_COOLDOWN_MS;
}

uint32_t WiFiScanner::ageMs() const { return everDone_ ? (millis() - doneMs_) : UINT32_MAX; }

bool WiFiScanner::start() {
  if (state_ == ScanState::Scanning || cooldownActive()) return false;
  error_[0] = '\0';
  WiFi.scanDelete();
  int16_t r = WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/true);
  if (r == WIFI_SCAN_FAILED) {
    strlcpy(error_, "Scan failed to start", sizeof error_);
    state_ = ScanState::Failed;
    return true;
  }
  startMs_ = millis();
  state_ = ScanState::Scanning;
  return true;
}

void WiFiScanner::finish(int n) {
  count_ = 0;
  for (int i = 0; i < n && count_ < kMaxResults; ++i) {
    ScanEntry& e = results_[count_++];
    String s = WiFi.SSID(i);  // one short-lived String per entry; scan is user-triggered
    strlcpy(e.ssid, s.length() ? s.c_str() : "<hidden>", sizeof e.ssid);
    int32_t rssi = WiFi.RSSI(i);
    e.rssi = (int8_t)constrain(rssi, -127, 0);
    e.channel = (uint8_t)WiFi.channel(i);
    e.auth = (uint8_t)WiFi.encryptionType(i);
  }
  // insertion sort by RSSI (strongest first); n <= 24
  for (int i = 1; i < count_; ++i) {
    ScanEntry key = results_[i];
    int j = i - 1;
    while (j >= 0 && results_[j].rssi < key.rssi) { results_[j + 1] = results_[j]; --j; }
    results_[j + 1] = key;
  }
  WiFi.scanDelete();  // free the driver's copy
  doneMs_ = millis();
  everDone_ = true;
  state_ = ScanState::Done;
}

void WiFiScanner::update() {
  if (state_ != ScanState::Scanning) return;
  int16_t r = WiFi.scanComplete();
  if (r >= 0) {
    finish(r);  // r == 0 is a valid "no networks found" result
  } else if (r == WIFI_SCAN_FAILED) {
    strlcpy(error_, "Scan failed", sizeof error_);
    state_ = ScanState::Failed;
  } else if (millis() - startMs_ > WIFI_SCAN_TIMEOUT_MS) {
    WiFi.scanDelete();
    strlcpy(error_, "Scan timeout", sizeof error_);
    state_ = ScanState::Failed;
  }
}
