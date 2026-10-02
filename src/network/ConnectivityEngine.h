// Layered connectivity test: Wi-Fi -> gateway (ICMP) -> DNS -> internet
// (ICMP, then TCP/443 fallback). Runs in one worker task so the UI never blocks.
// This is a troubleshooting aid that points at the likely failing layer; it is
// not a validated measurement of network quality.
#pragma once
#include <stdint.h>
#include "core/Connectivity.h"
#include "core/PingStats.h"
#include "network/PingDiagnostics.h"
#include "network/Sync.h"

struct ConnSnapshot {
  bool running = false;
  bool everRun = false;
  int step = 0;  // 0 idle, 1 wifi, 2 gateway, 3 dns, 4 internet
  core::ConnectivityLayers layers;
  core::ConnStatus status = core::ConnStatus::Incomplete;
  bool gwIcmpSuspect = false;
  core::PingStats gwStats;
  core::PingStats inetStats;
  uint32_t dnsLatencyMs = 0;
  bool dnsLatencyValid = false;
  char dnsIp[16] = "N/A";
  char internetVia[8] = "-";  // "ICMP" | "TCP443"
  uint32_t tcpLatencyMs = 0;
  char gatewayIp[16] = "N/A";
  int rssi = 0;
  bool rssiValid = false;
  uint32_t finishedAtMs = 0;
  uint32_t finishedSeq = 0;  // increments each time a run completes
};

class ConnectivityEngine {
 public:
  bool start();   // false if already running
  void cancel();  // request early stop
  bool running() const;
  ConnSnapshot snapshot() const;

 private:
  static void taskEntry(void* arg);
  void run();
  bool runPing(const uint8_t ip[4], uint8_t count, uint32_t timeoutMs, core::PingStats& out);
  void publish(const ConnSnapshot& s);

  mutable SpinLock lock_;
  ConnSnapshot snap_;
  PingDiagnostics probe_;  // private instance: never contends with the Ping screen
  volatile bool cancel_ = false;
  volatile bool active_ = false;
  uint32_t finishedSeq_ = 0;
};
