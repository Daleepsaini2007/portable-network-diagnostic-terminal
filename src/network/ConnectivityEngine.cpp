#include "network/ConnectivityEngine.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <string.h>
#include <stdio.h>
#include "AppConfig.h"
#include "core/Validation.h"
#include "network/DNSDiagnostics.h"

using core::Layer;

bool ConnectivityEngine::start() {
  if (active_) return false;
  {
    ScopedLock g(lock_);
    uint32_t keepSeq = snap_.finishedSeq;
    snap_ = ConnSnapshot();
    snap_.finishedSeq = keepSeq;
    snap_.running = true;
    snap_.step = 1;
  }
  cancel_ = false;
  active_ = true;
  if (xTaskCreate(&ConnectivityEngine::taskEntry, "conn", 6144, this, 1, nullptr) != pdPASS) {
    active_ = false;
    ScopedLock g(lock_);
    snap_.running = false;
    return false;
  }
  return true;
}

void ConnectivityEngine::cancel() {
  cancel_ = true;
  probe_.cancel();
}

bool ConnectivityEngine::running() const {
  ScopedLock g(lock_);
  return snap_.running;
}

ConnSnapshot ConnectivityEngine::snapshot() const {
  ScopedLock g(lock_);
  return snap_;
}

void ConnectivityEngine::publish(const ConnSnapshot& s) {
  ScopedLock g(lock_);
  uint32_t seq = snap_.finishedSeq;
  snap_ = s;
  snap_.finishedSeq = seq;
}

void ConnectivityEngine::taskEntry(void* arg) {
  auto* self = static_cast<ConnectivityEngine*>(arg);
  self->run();
  vTaskDelete(nullptr);
}

// Blocking ping used only from the worker task.
bool ConnectivityEngine::runPing(const uint8_t ip[4], uint8_t count, uint32_t timeoutMs, core::PingStats& out) {
  if (!probe_.start(ip, count, timeoutMs, 200)) return false;
  while (probe_.running()) {
    if (cancel_) probe_.cancel();
    vTaskDelay(pdMS_TO_TICKS(25));
    probe_.update();
  }
  probe_.update();
  out = probe_.snapshot().stats;
  return true;
}

void ConnectivityEngine::run() {
  ConnSnapshot s = snapshot();  // starts from the reset snapshot made in start()

  // ---- 1. Wi-Fi association ----
  s.step = 1;
  bool wifiUp = (WiFi.status() == WL_CONNECTED);
  s.layers.wifi = wifiUp ? Layer::Pass : Layer::Fail;
  if (wifiUp) {
    s.rssi = WiFi.RSSI();
    s.rssiValid = true;
  }
  publish(s);

  if (wifiUp && !cancel_) {
    // ---- 2. Gateway reachability (ICMP echo) ----
    s.step = 2;
    publish(s);
    IPAddress gw = WiFi.gatewayIP();
    if ((uint32_t)gw == 0) {
      s.layers.gateway = Layer::Fail;
    } else {
      uint8_t gip[4] = {gw[0], gw[1], gw[2], gw[3]};
      snprintf(s.gatewayIp, sizeof s.gatewayIp, "%u.%u.%u.%u", gip[0], gip[1], gip[2], gip[3]);
      bool started = runPing(gip, 2, 800, s.gwStats);
      s.layers.gateway = (started && s.gwStats.received() > 0) ? Layer::Pass : Layer::Fail;
    }
    publish(s);
  }

  if (wifiUp && !cancel_) {
    // ---- 3. DNS resolution ----
    s.step = 3;
    publish(s);
    uint8_t ip[4] = {0};
    uint32_t ms = 0;
    DnsResult r = DNSDiagnostics::resolveBlocking(DNS_TEST_DEFAULT_HOST, ip, ms);
    s.layers.dns = (r == DnsResult::Success) ? Layer::Pass : Layer::Fail;
    s.dnsLatencyMs = ms;
    s.dnsLatencyValid = (r == DnsResult::Success);
    if (r == DnsResult::Success) snprintf(s.dnsIp, sizeof s.dnsIp, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    publish(s);
  }

  if (wifiUp && !cancel_) {
    // ---- 4. Internet endpoint (by IP, so it is independent of DNS) ----
    s.step = 4;
    publish(s);
    uint8_t pip[4];
    if (core::parseIPv4(INTERNET_PROBE_IP, pip)) {
      bool started = runPing(pip, 3, 1000, s.inetStats);
      if (started && s.inetStats.received() > 0) {
        s.layers.internet = Layer::Pass;
        strlcpy(s.internetVia, "ICMP", sizeof s.internetVia);
      } else if (!cancel_) {
        // Many networks drop ICMP; fall back to a TCP/443 handshake.
        WiFiClient c;
        uint32_t t0 = millis();
        bool ok = c.connect(IPAddress(pip[0], pip[1], pip[2], pip[3]), 443, TCP_PROBE_TIMEOUT_MS);
        s.tcpLatencyMs = millis() - t0;
        c.stop();
        s.layers.internet = ok ? Layer::Pass : Layer::Fail;
        if (ok) strlcpy(s.internetVia, "TCP443", sizeof s.internetVia);
      }
    } else {
      s.layers.internet = Layer::Fail;  // misconfigured INTERNET_PROBE_IP
    }
  }

  s.status = core::evaluate(s.layers);
  s.gwIcmpSuspect = core::gatewayIcmpSuspect(s.layers);
  s.step = 0;
  s.running = false;
  s.everRun = !cancel_;  // a cancelled run is not reported as a result
  s.finishedAtMs = millis();
  {
    ScopedLock g(lock_);
    snap_ = s;
    if (!cancel_) snap_.finishedSeq = ++finishedSeq_;
    else snap_.finishedSeq = finishedSeq_;
  }
  active_ = false;
}
