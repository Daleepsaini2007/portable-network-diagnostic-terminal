// Latency/loss statistics for a ping run. Pure C++.
#pragma once
#include <stdint.h>

namespace core {

class PingStats {
 public:
  void reset();
  void addReply(uint32_t rttMs);  // one echo reply received
  void addLoss();                 // one echo request that timed out

  uint32_t sent() const { return received_ + lost_; }  // resolved requests
  uint32_t received() const { return received_; }
  uint32_t lost() const { return lost_; }
  uint32_t minMs() const { return received_ ? minMs_ : 0; }
  uint32_t maxMs() const { return received_ ? maxMs_ : 0; }
  float avgMs() const;        // 0 if nothing received
  float lossPercent() const;  // 0..100; 0 if nothing sent

 private:
  uint32_t received_ = 0;
  uint32_t lost_ = 0;
  uint32_t minMs_ = 0;
  uint32_t maxMs_ = 0;
  uint64_t totalMs_ = 0;
};

enum class PingVerdict : uint8_t { NoData, Ok, PartialLoss, NoReply };
PingVerdict verdictFor(const PingStats& s);
const char* verdictLabel(PingVerdict v);

}  // namespace core
