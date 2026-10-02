// Asynchronous ICMP echo using ESP-IDF's ping_sock (esp_ping_*) API.
// Threading: start()/update()/cancel() belong to ONE owner task; callbacks run
// in the IDF ping task; snapshot() is safe from any task.
#pragma once
#include <stdint.h>
#include "core/PingStats.h"
#include "network/Sync.h"

extern "C" {
#include "ping/ping_sock.h"
}

enum class PingState : uint8_t { Idle, Running, Done };

struct PingSnapshot {
  PingState state = PingState::Idle;
  char ip[16] = "";
  uint8_t requested = 0;
  core::PingStats stats;
  uint32_t lastRttMs = 0;
  uint32_t lastBytes = 0;
  bool haveLast = false;
  bool lastWasTimeout = false;
};

class PingDiagnostics {
 public:
  // false => could not start (already running, or ESP-IDF refused the session).
  bool start(const uint8_t ip[4], uint8_t count, uint32_t timeoutMs, uint32_t intervalMs);
  void cancel();
  void update();  // finalises a finished session (frees the IDF session)
  bool running() const;
  PingSnapshot snapshot() const;

 private:
  static void onSuccess(esp_ping_handle_t h, void* arg);
  static void onTimeout(esp_ping_handle_t h, void* arg);
  static void onEnd(esp_ping_handle_t h, void* arg);

  esp_ping_handle_t handle_ = nullptr;
  volatile bool ended_ = false;
  mutable SpinLock lock_;
  PingSnapshot snap_;
};
