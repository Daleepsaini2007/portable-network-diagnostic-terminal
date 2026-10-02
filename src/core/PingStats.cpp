#include "core/PingStats.h"

namespace core {

void PingStats::reset() { *this = PingStats(); }

void PingStats::addReply(uint32_t rttMs) {
  if (received_ == 0) {
    minMs_ = maxMs_ = rttMs;
  } else {
    if (rttMs < minMs_) minMs_ = rttMs;
    if (rttMs > maxMs_) maxMs_ = rttMs;
  }
  totalMs_ += rttMs;
  ++received_;
}

void PingStats::addLoss() { ++lost_; }

float PingStats::avgMs() const {
  return received_ ? static_cast<float>(totalMs_) / static_cast<float>(received_) : 0.0f;
}

float PingStats::lossPercent() const {
  uint32_t s = sent();
  return s ? (100.0f * static_cast<float>(lost_)) / static_cast<float>(s) : 0.0f;
}

PingVerdict verdictFor(const PingStats& s) {
  if (s.sent() == 0) return PingVerdict::NoData;
  if (s.received() == 0) return PingVerdict::NoReply;
  if (s.lost() > 0) return PingVerdict::PartialLoss;
  return PingVerdict::Ok;
}

const char* verdictLabel(PingVerdict v) {
  switch (v) {
    case PingVerdict::Ok: return "SUCCESS";
    case PingVerdict::PartialLoss: return "PARTIAL LOSS";
    case PingVerdict::NoReply: return "NO REPLY";
    default: return "NO DATA";
  }
}

}  // namespace core
