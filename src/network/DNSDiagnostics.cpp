#include "network/DNSDiagnostics.h"
#include <Arduino.h>
#include <WiFi.h>
#include <stdio.h>
#include <string.h>
#include "AppConfig.h"
#include "core/Validation.h"

const char* dnsResultLabel(DnsResult r) {
  switch (r) {
    case DnsResult::Success: return "SUCCESS";
    case DnsResult::InvalidHost: return "INVALID HOSTNAME";
    case DnsResult::NoWifi: return "WIFI DISCONNECTED";
    case DnsResult::Timeout: return "TIMEOUT / NO SERVER";
    case DnsResult::NotFound: return "NOT FOUND / REFUSED";
    case DnsResult::Busy: return "RESOLVER BUSY";
    default: return "-";
  }
}

DnsResult DNSDiagnostics::resolveBlocking(const char* host, uint8_t ip[4], uint32_t& latencyMs) {
  IPAddress a;
  uint32_t t0 = millis();
  int ok = WiFi.hostByName(host, a);
  latencyMs = millis() - t0;
  if (ok && (uint32_t)a != 0) {
    for (int i = 0; i < 4; ++i) ip[i] = a[i];
    return DnsResult::Success;
  }
  return latencyMs >= 3000 ? DnsResult::Timeout : DnsResult::NotFound;
}

bool DNSDiagnostics::start(const char* host) {
  uint8_t tmp[4];
  DnsResult early = DnsResult::None;
  if (core::classifyTarget(host, tmp) == core::TargetKind::Invalid) early = DnsResult::InvalidHost;
  else if (WiFi.status() != WL_CONNECTED) early = DnsResult::NoWifi;
  else if (workerActive_) early = DnsResult::Busy;

  {
    ScopedLock g(lock_);
    if (early != DnsResult::None) {
      // keep the previous host text visible only for Busy; otherwise reset
      snap_.state = DnsState::Done;
      snap_.result = early;
      if (early != DnsResult::Busy) {
        snprintf(snap_.host, sizeof snap_.host, "%s", host ? host : "");
        snprintf(snap_.ip, sizeof snap_.ip, "N/A");
        snap_.latencyMs = 0;
      }
      return false;
    }
    ++seq_;
    snap_ = DnsSnapshot();
    snap_.state = DnsState::Running;
    snprintf(snap_.host, sizeof snap_.host, "%s", host);
    snprintf(reqHost_, sizeof reqHost_, "%s", host);
    startMs_ = millis();
  }
  workerActive_ = true;
  if (xTaskCreate(&DNSDiagnostics::taskEntry, "dns", 4096, this, 1, nullptr) != pdPASS) {
    workerActive_ = false;
    ScopedLock g(lock_);
    snap_.state = DnsState::Done;
    snap_.result = DnsResult::Busy;
    return false;
  }
  return true;
}

void DNSDiagnostics::taskEntry(void* arg) {
  auto* self = static_cast<DNSDiagnostics*>(arg);
  char host[64];
  uint32_t mySeq;
  {
    ScopedLock g(self->lock_);
    snprintf(host, sizeof host, "%s", self->reqHost_);
    mySeq = self->seq_;
  }
  uint8_t ip[4] = {0};
  uint32_t ms = 0;
  DnsResult r = resolveBlocking(host, ip, ms);
  {
    ScopedLock g(self->lock_);
    // Ignore the result if the UI already gave up or started a newer request.
    if (self->seq_ == mySeq && self->snap_.state == DnsState::Running) {
      self->snap_.result = r;
      self->snap_.latencyMs = ms;
      if (r == DnsResult::Success) {
        snprintf(self->snap_.ip, sizeof self->snap_.ip, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
      }
      self->snap_.state = DnsState::Done;
    }
  }
  self->workerActive_ = false;
  vTaskDelete(nullptr);
}

void DNSDiagnostics::update() {
  ScopedLock g(lock_);
  if (snap_.state == DnsState::Running && (millis() - startMs_) > DNS_TIMEOUT_MS) {
    ++seq_;  // invalidate the worker's eventual result
    snap_.state = DnsState::Done;
    snap_.result = DnsResult::Timeout;
    snap_.latencyMs = millis() - startMs_;
  }
}

void DNSDiagnostics::cancel() {
  ScopedLock g(lock_);
  if (snap_.state == DnsState::Running) {
    ++seq_;
    snap_.state = DnsState::Idle;
    snap_.result = DnsResult::None;
  }
}

bool DNSDiagnostics::running() const {
  ScopedLock g(lock_);
  return snap_.state == DnsState::Running;
}

DnsSnapshot DNSDiagnostics::snapshot() const {
  ScopedLock g(lock_);
  return snap_;
}
