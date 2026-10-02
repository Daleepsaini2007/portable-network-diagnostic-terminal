// Layered connectivity evaluation and a heuristic health score. Pure C++.
#pragma once
#include <stdint.h>

namespace core {

enum class Layer : uint8_t { NotRun, Pass, Fail };

struct ConnectivityLayers {
  Layer wifi = Layer::NotRun;
  Layer gateway = Layer::NotRun;
  Layer dns = Layer::NotRun;
  Layer internet = Layer::NotRun;
};

enum class ConnStatus : uint8_t {
  Incomplete,
  Online,
  NoWifi,
  GatewayUnreachable,
  DnsFailure,
  InternetUnreachable
};

// Decides the most likely failing layer. A gateway that ignores ICMP but still
// forwards traffic is reported ONLINE (see gatewayIcmpSuspect()).
ConnStatus evaluate(const ConnectivityLayers& l);
const char* statusLabel(ConnStatus s);
// True when gateway failed but DNS+internet passed (router likely drops ICMP).
bool gatewayIcmpSuspect(const ConnectivityLayers& l);

// ---- Heuristic only: NOT an industry-standard score. ----
struct HealthInput {
  ConnStatus status = ConnStatus::Incomplete;
  bool rssiValid = false;
  int rssiDbm = 0;
  bool latencyValid = false;
  float avgLatencyMs = 0;
  bool lossValid = false;
  float lossPercent = 0;
};

enum class HealthLabel : uint8_t { Unknown, Healthy, Degraded, Poor, Offline };

struct HealthResult {
  int score;  // 0..100 heuristic, -1 when not computable
  HealthLabel label;
};

HealthResult computeHealth(const HealthInput& in);
const char* healthLabelText(HealthLabel l);
const char* rssiLabel(int rssiDbm);  // STRONG / GOOD / FAIR / WEAK (heuristic)

}  // namespace core
