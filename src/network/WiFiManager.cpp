#include "network/WiFiManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include "AppConfig.h"

bool WiFiManager::begin() {
  WiFi.persistent(false);  // never write credentials to flash (NVS)
  hwOk_ = WiFi.mode(WIFI_STA);
  if (hwOk_) {
    WiFi.setAutoReconnect(false);
    // Modem sleep adds up to ~100 ms to ICMP latency; disable for accurate diagnostics.
    WiFi.setSleep(false);
    state_ = WifiState::Idle;
  } else {
    state_ = WifiState::Off;
    strlcpy(error_, "WiFi init failed", sizeof error_);
  }
  return hwOk_;
}

bool WiFiManager::connect(const char* ssid, const char* pass) {
  if (!hwOk_ || !ssid || !*ssid) return false;
  strlcpy(ssid_, ssid, sizeof ssid_);
  strlcpy(pass_, pass ? pass : "", sizeof pass_);
  return retry();
}

bool WiFiManager::retry() {
  if (!hwOk_ || !hasCredentials()) return false;
  error_[0] = '\0';
  WiFi.disconnect(false, false);
  WiFi.setAutoReconnect(false);
  WiFi.begin(ssid_, pass_[0] ? pass_ : nullptr);
  startMs_ = millis();
  state_ = WifiState::Connecting;
  return true;
}

void WiFiManager::disconnect() {
  if (hwOk_) {
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false, false);
  }
  memset(pass_, 0, sizeof pass_);
  ssid_[0] = '\0';
  error_[0] = '\0';
  state_ = hwOk_ ? WifiState::Idle : WifiState::Off;
}

uint32_t WiFiManager::connectElapsedMs() const {
  return state_ == WifiState::Connecting ? (millis() - startMs_) : 0;
}

void WiFiManager::fail(const char* why) {
  strlcpy(error_, why, sizeof error_);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  state_ = WifiState::Failed;
}

void WiFiManager::update() {
  if (!hwOk_) return;
  wl_status_t st = WiFi.status();
  switch (state_) {
    case WifiState::Connecting: {
      uint32_t el = millis() - startMs_;
      if (st == WL_CONNECTED) {
        state_ = WifiState::Connected;
        WiFi.setAutoReconnect(true);  // recover automatically if the AP drops us later
        configTime(0, 0, NTP_SERVER);  // async SNTP; only used for log timestamps
      } else if (st == WL_NO_SSID_AVAIL && el > 2000) {
        fail("SSID not found");
      } else if (st == WL_CONNECT_FAILED) {
        fail("Connect/auth failed");
      } else if (el > WIFI_CONNECT_TIMEOUT_MS) {
        fail("Timeout (check password)");
      }
      break;
    }
    case WifiState::Connected:
      if (st != WL_CONNECTED) state_ = WifiState::Lost;
      break;
    case WifiState::Lost:
      if (st == WL_CONNECTED) state_ = WifiState::Connected;
      break;
    default:
      break;
  }
}

bool WiFiManager::timeSynced() const { return time(nullptr) > 1735689600; }  // after 2025-01-01
int64_t WiFiManager::utcEpoch() const { return timeSynced() ? (int64_t)time(nullptr) : -1; }
