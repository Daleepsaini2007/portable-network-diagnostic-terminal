#include "core/Connectivity.h"

namespace core {

ConnStatus evaluate(const ConnectivityLayers& l) {
  if (l.wifi == Layer::Fail) return ConnStatus::NoWifi;
  if (l.wifi == Layer::NotRun || l.gateway == Layer::NotRun ||
      l.dns == Layer::NotRun || l.internet == Layer::NotRun) {
    return ConnStatus::Incomplete;
  }
  if (l.gateway == Layer::Fail) {
    if (l.internet == Layer::Fail) return ConnStatus::GatewayUnreachable;
    // Traffic got through despite no ICMP answer from the gateway.
    return (l.dns == Layer::Fail) ? ConnStatus::DnsFailure : ConnStatus::Online;
  }
  if (l.dns == Layer::Fail) return ConnStatus::DnsFailure;
  if (l.internet == Layer::Fail) return ConnStatus::InternetUnreachable;
  return ConnStatus::Online;
}

bool gatewayIcmpSuspect(const ConnectivityLayers& l) {
  return l.gateway == Layer::Fail && l.dns == Layer::Pass && l.internet == Layer::Pass;
}

const char* statusLabel(ConnStatus s) {
  switch (s) {
    case ConnStatus::Online: return "ONLINE";
    case ConnStatus::NoWifi: return "NO WIFI";
    case ConnStatus::GatewayUnreachable: return "GATEWAY DOWN";
    case ConnStatus::DnsFailure: return "DNS FAILURE";
    case ConnStatus::InternetUnreachable: return "NO INTERNET";
    default: return "INCOMPLETE";
  }
}

const char* rssiLabel(int rssi) {
  if (rssi >= -55) return "STRONG";
  if (rssi >= -67) return "GOOD";
  if (rssi >= -75) return "FAIR";
  return "WEAK";
}

HealthResult computeHealth(const HealthInput& in) {
  if (in.status == ConnStatus::Incomplete) return {-1, HealthLabel::Unknown};
  if (in.status == ConnStatus::NoWifi || in.status == ConnStatus::GatewayUnreachable ||
      in.status == ConnStatus::InternetUnreachable) {
    return {0, HealthLabel::Offline};
  }
  int score = 100;
  if (in.status == ConnStatus::DnsFailure) score -= 50;
  if (in.rssiValid) {
    if (in.rssiDbm < -80) score -= 30;
    else if (in.rssiDbm < -70) score -= 15;
    else if (in.rssiDbm < -60) score -= 5;
  }
  if (in.latencyValid) {
    if (in.avgLatencyMs > 200) score -= 30;
    else if (in.avgLatencyMs > 100) score -= 15;
    else if (in.avgLatencyMs > 50) score -= 5;
  }
  if (in.lossValid) {
    int pen = static_cast<int>(in.lossPercent * 1.5f);
    score -= (pen > 40) ? 40 : pen;
  }
  if (score < 0) score = 0;
  HealthLabel label = score >= 80 ? HealthLabel::Healthy
                    : score >= 50 ? HealthLabel::Degraded
                                  : HealthLabel::Poor;
  return {score, label};
}

const char* healthLabelText(HealthLabel l) {
  switch (l) {
    case HealthLabel::Healthy: return "HEALTHY";
    case HealthLabel::Degraded: return "DEGRADED";
    case HealthLabel::Poor: return "POOR";
    case HealthLabel::Offline: return "OFFLINE";
    default: return "UNKNOWN";
  }
}

}  // namespace core
