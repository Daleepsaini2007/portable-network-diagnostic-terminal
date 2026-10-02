// DNS resolution test. WiFi.hostByName() can block for ~15 s, so the UI path
// runs it in a short-lived worker task and the UI just polls.
#pragma once
#include <stdint.h>
#include "network/Sync.h"

enum class DnsState : uint8_t { Idle, Running, Done };
enum class DnsResult : uint8_t { None, Success, InvalidHost, NoWifi, Timeout, NotFound, Busy };

struct DnsSnapshot {
  DnsState state = DnsState::Idle;
  DnsResult result = DnsResult::None;
  char host[64] = "";
  char ip[16] = "N/A";
  uint32_t latencyMs = 0;
};

const char* dnsResultLabel(DnsResult r);

class DNSDiagnostics {
 public:
  bool start(const char* host);  // check snapshot().result if it returns false
  void update();                 // UI-side watchdog
  void cancel();                 // abandon waiting (worker finishes in background)
  bool running() const;
  DnsSnapshot snapshot() const;

  // Blocking; for use from a worker task only. A negative answer that arrives
  // quickly is NotFound; a slow failure is reported as Timeout. The Arduino
  // API gives no finer error code, so this split is a heuristic.
  static DnsResult resolveBlocking(const char* host, uint8_t ip[4], uint32_t& latencyMs);

 private:
  static void taskEntry(void* arg);
  mutable SpinLock lock_;
  DnsSnapshot snap_;
  uint32_t seq_ = 0;
  uint32_t startMs_ = 0;
  char reqHost_[64] = "";
  volatile bool workerActive_ = false;
};
