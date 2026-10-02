#include "network/PingDiagnostics.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

bool PingDiagnostics::start(const uint8_t ip[4], uint8_t count, uint32_t timeoutMs, uint32_t intervalMs) {
  update();
  if (handle_ != nullptr || count == 0) return false;

  esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
  cfg.count = count;
  cfg.timeout_ms = timeoutMs;
  cfg.interval_ms = intervalMs;
  cfg.data_size = 32;  // classic 32-byte payload
  cfg.task_stack_size = 4096;
  IP_ADDR4(&cfg.target_addr, ip[0], ip[1], ip[2], ip[3]);

  esp_ping_callbacks_t cbs = {};
  cbs.cb_args = this;
  cbs.on_ping_success = &PingDiagnostics::onSuccess;
  cbs.on_ping_timeout = &PingDiagnostics::onTimeout;
  cbs.on_ping_end = &PingDiagnostics::onEnd;

  {
    ScopedLock g(lock_);
    snap_ = PingSnapshot();
    snap_.requested = count;
    snprintf(snap_.ip, sizeof snap_.ip, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    snap_.state = PingState::Running;
    ended_ = false;
  }

  esp_ping_handle_t h = nullptr;
  if (esp_ping_new_session(&cfg, &cbs, &h) != ESP_OK || h == nullptr) {
    ScopedLock g(lock_);
    snap_.state = PingState::Idle;
    return false;
  }
  handle_ = h;
  if (esp_ping_start(h) != ESP_OK) {
    esp_ping_delete_session(h);
    handle_ = nullptr;
    ScopedLock g(lock_);
    snap_.state = PingState::Idle;
    return false;
  }
  return true;
}

void PingDiagnostics::cancel() {
  if (handle_) esp_ping_stop(handle_);  // IDF then fires on_ping_end
}

void PingDiagnostics::update() {
  if (handle_ && ended_) {
    // delete_session only clears a flag; the IDF task frees itself afterwards.
    esp_ping_delete_session(handle_);
    handle_ = nullptr;
    ScopedLock g(lock_);
    snap_.state = PingState::Done;
  }
}

bool PingDiagnostics::running() const {
  ScopedLock g(lock_);
  return snap_.state == PingState::Running;
}

PingSnapshot PingDiagnostics::snapshot() const {
  ScopedLock g(lock_);
  return snap_;
}

void PingDiagnostics::onSuccess(esp_ping_handle_t h, void* arg) {
  auto* self = static_cast<PingDiagnostics*>(arg);
  uint32_t rtt = 0, bytes = 0;
  esp_ping_get_profile(h, ESP_PING_PROF_TIMEGAP, &rtt, sizeof rtt);
  esp_ping_get_profile(h, ESP_PING_PROF_SIZE, &bytes, sizeof bytes);
  ScopedLock g(self->lock_);
  self->snap_.stats.addReply(rtt);
  self->snap_.lastRttMs = rtt;
  self->snap_.lastBytes = bytes;
  self->snap_.haveLast = true;
  self->snap_.lastWasTimeout = false;
}

void PingDiagnostics::onTimeout(esp_ping_handle_t h, void* arg) {
  auto* self = static_cast<PingDiagnostics*>(arg);
  ScopedLock g(self->lock_);
  self->snap_.stats.addLoss();
  self->snap_.haveLast = true;
  self->snap_.lastWasTimeout = true;
}

void PingDiagnostics::onEnd(esp_ping_handle_t h, void* arg) {
  static_cast<PingDiagnostics*>(arg)->ended_ = true;  // finalised from update()
}
