// Host tests: validation, ping statistics, layered connectivity, health heuristic.
#include <unity.h>
#include "core/Validation.h"
#include "core/PingStats.h"
#include "core/Connectivity.h"

using namespace core;

void setUp() {}
void tearDown() {}

// ---------------- IPv4 / hostname validation ----------------
void test_ipv4_valid() {
  uint8_t ip[4];
  TEST_ASSERT_TRUE(parseIPv4("8.8.8.8", ip));
  TEST_ASSERT_EQUAL_UINT8(8, ip[0]);
  TEST_ASSERT_TRUE(parseIPv4("192.168.1.1", ip));
  TEST_ASSERT_EQUAL_UINT8(1, ip[3]);
  TEST_ASSERT_TRUE(parseIPv4("0.0.0.0", ip));
  TEST_ASSERT_TRUE(parseIPv4("255.255.255.255", ip));
  TEST_ASSERT_TRUE(parseIPv4("1.2.3.4", nullptr));
}

void test_ipv4_invalid() {
  const char* bad[] = {"", "1.2.3", "1.2.3.4.5", "256.1.1.1", "1.2.3.256", "a.b.c.d",
                       "1..2.3", "1.2.3.", ".1.2.3", "01.2.3.4", "1.2.3.04",
                       " 1.2.3.4", "1.2.3.4 ", "1.2.3.4x", "1234.1.1.1", "-1.2.3.4"};
  for (const char* s : bad) TEST_ASSERT_FALSE_MESSAGE(parseIPv4(s, nullptr), s);
  TEST_ASSERT_FALSE(parseIPv4(nullptr, nullptr));
}

void test_hostname_valid() {
  TEST_ASSERT_TRUE(isValidHostname("example.com"));
  TEST_ASSERT_TRUE(isValidHostname("a-b.example.co.uk"));
  TEST_ASSERT_TRUE(isValidHostname("router"));
  TEST_ASSERT_TRUE(isValidHostname("example.com."));
  TEST_ASSERT_TRUE(isValidHostname("host1.local"));
  TEST_ASSERT_TRUE(isValidHostname("1host.example"));
}

void test_hostname_invalid() {
  const char* bad[] = {"", ".", "-a.com", "a-.com", "a..com", ".a.com", "a b.com",
                       "a_b.com", "exa$mple.com", "999.1.1.1", "1.2.3", "a.com.."};
  for (const char* s : bad) TEST_ASSERT_FALSE_MESSAGE(isValidHostname(s), s);
  TEST_ASSERT_FALSE(isValidHostname(nullptr));
}

void test_hostname_length_limits() {
  char label63[64];
  for (int i = 0; i < 63; ++i) label63[i] = 'a';
  label63[63] = '\0';
  TEST_ASSERT_TRUE(isValidHostname(label63));
  char label64[65];
  for (int i = 0; i < 64; ++i) label64[i] = 'a';
  label64[64] = '\0';
  TEST_ASSERT_FALSE(isValidHostname(label64));
  // 254-char name (too long)
  char big[300];
  int n = 0;
  for (int l = 0; l < 51; ++l) { for (int i = 0; i < 4; ++i) big[n++] = 'a'; big[n++] = '.'; }
  big[n++] = 'a';
  big[n] = '\0';
  TEST_ASSERT_TRUE(n > 253);
  TEST_ASSERT_FALSE(isValidHostname(big));
}

void test_classify_target() {
  uint8_t ip[4] = {0};
  TEST_ASSERT_EQUAL((int)TargetKind::IPv4, (int)classifyTarget("1.1.1.1", ip));
  TEST_ASSERT_EQUAL_UINT8(1, ip[0]);
  TEST_ASSERT_EQUAL((int)TargetKind::Hostname, (int)classifyTarget("example.com", ip));
  TEST_ASSERT_EQUAL((int)TargetKind::Invalid, (int)classifyTarget("999.1.1.1", ip));
  TEST_ASSERT_EQUAL((int)TargetKind::Invalid, (int)classifyTarget("", ip));
  TEST_ASSERT_EQUAL((int)TargetKind::Invalid, (int)classifyTarget(nullptr, ip));
}

// ---------------- Ping statistics ----------------
void test_stats_empty() {
  PingStats s;
  TEST_ASSERT_EQUAL_UINT32(0, s.sent());
  TEST_ASSERT_EQUAL_UINT32(0, s.minMs());
  TEST_ASSERT_EQUAL_UINT32(0, s.maxMs());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.avgMs());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.lossPercent());  // no divide-by-zero
  TEST_ASSERT_EQUAL((int)PingVerdict::NoData, (int)verdictFor(s));
}

void test_stats_min_avg_max() {
  PingStats s;
  s.addReply(21); s.addReply(24); s.addReply(29); s.addReply(22);
  TEST_ASSERT_EQUAL_UINT32(4, s.sent());
  TEST_ASSERT_EQUAL_UINT32(4, s.received());
  TEST_ASSERT_EQUAL_UINT32(21, s.minMs());
  TEST_ASSERT_EQUAL_UINT32(29, s.maxMs());
  TEST_ASSERT_EQUAL_FLOAT(24.0f, s.avgMs());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.lossPercent());
  TEST_ASSERT_EQUAL((int)PingVerdict::Ok, (int)verdictFor(s));
}

void test_stats_single_reply_min_equals_max() {
  PingStats s;
  s.addReply(7);
  TEST_ASSERT_EQUAL_UINT32(7, s.minMs());
  TEST_ASSERT_EQUAL_UINT32(7, s.maxMs());
}

void test_stats_zero_ms_reply_counts_for_min() {
  PingStats s;
  s.addReply(0);  // sub-millisecond LAN reply truncates to 0
  s.addReply(3);
  TEST_ASSERT_EQUAL_UINT32(0, s.minMs());
  TEST_ASSERT_EQUAL_FLOAT(1.5f, s.avgMs());
}

void test_stats_loss() {
  PingStats s;
  s.addReply(10); s.addLoss(); s.addReply(30); s.addLoss();
  TEST_ASSERT_EQUAL_UINT32(4, s.sent());
  TEST_ASSERT_EQUAL_FLOAT(50.0f, s.lossPercent());
  TEST_ASSERT_EQUAL_FLOAT(20.0f, s.avgMs());  // losses excluded from avg
  TEST_ASSERT_EQUAL((int)PingVerdict::PartialLoss, (int)verdictFor(s));
}

void test_stats_total_loss() {
  PingStats s;
  for (int i = 0; i < 4; ++i) s.addLoss();
  TEST_ASSERT_EQUAL_FLOAT(100.0f, s.lossPercent());
  TEST_ASSERT_EQUAL_UINT32(0, s.minMs());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.avgMs());
  TEST_ASSERT_EQUAL((int)PingVerdict::NoReply, (int)verdictFor(s));
}

void test_stats_reset() {
  PingStats s;
  s.addReply(5); s.addLoss();
  s.reset();
  TEST_ASSERT_EQUAL_UINT32(0, s.sent());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, s.avgMs());
}

void test_stats_no_overflow_on_large_totals() {
  PingStats s;
  for (int i = 0; i < 100000; ++i) s.addReply(4000000u);  // 4e11 ms total > 2^32
  TEST_ASSERT_EQUAL_FLOAT(4000000.0f, s.avgMs());
}

// ---------------- Layered connectivity ----------------
static ConnectivityLayers L(Layer w, Layer g, Layer d, Layer i) {
  ConnectivityLayers l; l.wifi = w; l.gateway = g; l.dns = d; l.internet = i; return l;
}
const Layer P = Layer::Pass, F = Layer::Fail, N = Layer::NotRun;

void test_conn_all_ok() {
  TEST_ASSERT_EQUAL((int)ConnStatus::Online, (int)evaluate(L(P, P, P, P)));
}
void test_conn_no_wifi() {
  TEST_ASSERT_EQUAL((int)ConnStatus::NoWifi, (int)evaluate(L(F, N, N, N)));
}
void test_conn_gateway_down() {
  TEST_ASSERT_EQUAL((int)ConnStatus::GatewayUnreachable, (int)evaluate(L(P, F, F, F)));
}
void test_conn_dns_failure_spec_example() {
  // Spec example: WiFi ok, Gateway ok, DNS fail, Internet fail -> DNS FAILURE.
  // Our engine tests internet by IP, but even when it also fails the first
  // failing layer below the gateway is DNS.
  TEST_ASSERT_EQUAL((int)ConnStatus::DnsFailure, (int)evaluate(L(P, P, F, F)));
  TEST_ASSERT_EQUAL((int)ConnStatus::DnsFailure, (int)evaluate(L(P, P, F, P)));
}
void test_conn_internet_down() {
  TEST_ASSERT_EQUAL((int)ConnStatus::InternetUnreachable, (int)evaluate(L(P, P, P, F)));
}
void test_conn_gateway_ignores_icmp_but_traffic_flows() {
  ConnectivityLayers l = L(P, F, P, P);
  TEST_ASSERT_EQUAL((int)ConnStatus::Online, (int)evaluate(l));
  TEST_ASSERT_TRUE(gatewayIcmpSuspect(l));
  TEST_ASSERT_FALSE(gatewayIcmpSuspect(L(P, P, P, P)));
}
void test_conn_incomplete() {
  TEST_ASSERT_EQUAL((int)ConnStatus::Incomplete, (int)evaluate(L(P, N, N, N)));
  TEST_ASSERT_EQUAL((int)ConnStatus::Incomplete, (int)evaluate(L(N, N, N, N)));
  TEST_ASSERT_EQUAL((int)ConnStatus::Incomplete, (int)evaluate(L(P, P, P, N)));
}
void test_status_labels() {
  TEST_ASSERT_EQUAL_STRING("ONLINE", statusLabel(ConnStatus::Online));
  TEST_ASSERT_EQUAL_STRING("DNS FAILURE", statusLabel(ConnStatus::DnsFailure));
  TEST_ASSERT_EQUAL_STRING("NO WIFI", statusLabel(ConnStatus::NoWifi));
}

// ---------------- Health heuristic ----------------
void test_health_perfect() {
  HealthInput in; in.status = ConnStatus::Online;
  in.rssiValid = true; in.rssiDbm = -52;
  in.latencyValid = true; in.avgLatencyMs = 24;
  in.lossValid = true; in.lossPercent = 0;
  HealthResult r = computeHealth(in);
  TEST_ASSERT_EQUAL(100, r.score);
  TEST_ASSERT_EQUAL((int)HealthLabel::Healthy, (int)r.label);
}
void test_health_degraded_by_signal_latency_loss() {
  HealthInput in; in.status = ConnStatus::Online;
  in.rssiValid = true; in.rssiDbm = -78;        // -15
  in.latencyValid = true; in.avgLatencyMs = 120; // -15
  in.lossValid = true; in.lossPercent = 10;      // -15
  HealthResult r = computeHealth(in);
  TEST_ASSERT_EQUAL(55, r.score);
  TEST_ASSERT_EQUAL((int)HealthLabel::Degraded, (int)r.label);
}
void test_health_loss_penalty_capped() {
  HealthInput in; in.status = ConnStatus::Online;
  in.lossValid = true; in.lossPercent = 100;
  TEST_ASSERT_EQUAL(60, computeHealth(in).score);  // capped at -40
}
void test_health_offline_and_unknown() {
  HealthInput in; in.status = ConnStatus::NoWifi;
  TEST_ASSERT_EQUAL((int)HealthLabel::Offline, (int)computeHealth(in).label);
  in.status = ConnStatus::Incomplete;
  HealthResult r = computeHealth(in);
  TEST_ASSERT_EQUAL(-1, r.score);
  TEST_ASSERT_EQUAL((int)HealthLabel::Unknown, (int)r.label);
}
void test_health_missing_metrics_ignored() {
  HealthInput in; in.status = ConnStatus::Online;
  TEST_ASSERT_EQUAL(100, computeHealth(in).score);
}
void test_health_dns_failure_penalty() {
  HealthInput in; in.status = ConnStatus::DnsFailure;
  HealthResult r = computeHealth(in);
  TEST_ASSERT_EQUAL(50, r.score);
  TEST_ASSERT_EQUAL((int)HealthLabel::Degraded, (int)r.label);
}
void test_rssi_labels() {
  TEST_ASSERT_EQUAL_STRING("STRONG", rssiLabel(-50));
  TEST_ASSERT_EQUAL_STRING("GOOD", rssiLabel(-60));
  TEST_ASSERT_EQUAL_STRING("FAIR", rssiLabel(-70));
  TEST_ASSERT_EQUAL_STRING("WEAK", rssiLabel(-90));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_ipv4_valid); RUN_TEST(test_ipv4_invalid);
  RUN_TEST(test_hostname_valid); RUN_TEST(test_hostname_invalid);
  RUN_TEST(test_hostname_length_limits); RUN_TEST(test_classify_target);
  RUN_TEST(test_stats_empty); RUN_TEST(test_stats_min_avg_max);
  RUN_TEST(test_stats_single_reply_min_equals_max);
  RUN_TEST(test_stats_zero_ms_reply_counts_for_min);
  RUN_TEST(test_stats_loss); RUN_TEST(test_stats_total_loss);
  RUN_TEST(test_stats_reset); RUN_TEST(test_stats_no_overflow_on_large_totals);
  RUN_TEST(test_conn_all_ok); RUN_TEST(test_conn_no_wifi);
  RUN_TEST(test_conn_gateway_down); RUN_TEST(test_conn_dns_failure_spec_example);
  RUN_TEST(test_conn_internet_down);
  RUN_TEST(test_conn_gateway_ignores_icmp_but_traffic_flows);
  RUN_TEST(test_conn_incomplete); RUN_TEST(test_status_labels);
  RUN_TEST(test_health_perfect); RUN_TEST(test_health_degraded_by_signal_latency_loss);
  RUN_TEST(test_health_loss_penalty_capped); RUN_TEST(test_health_offline_and_unknown);
  RUN_TEST(test_health_missing_metrics_ignored); RUN_TEST(test_health_dns_failure_penalty);
  RUN_TEST(test_rssi_labels);
  return UNITY_END();
}
