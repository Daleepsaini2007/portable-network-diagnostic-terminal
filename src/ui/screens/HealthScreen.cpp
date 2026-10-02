#include "app/App.h"
#include "core/Connectivity.h"

void HealthScreen::onInput(App& app, const core::InputEvent& ev) {
  if (ev.type == core::EvType::Refresh) app.conn.start();
  else if (ev.type == core::EvType::Back) app.back();
}

static const char* okStr(core::Layer l) { return l == core::Layer::Pass ? "OK" : l == core::Layer::Fail ? "FAIL" : "--"; }
static uint16_t okCol(core::Layer l) {
  return l == core::Layer::Pass ? DisplayManager::kOk : l == core::Layer::Fail ? DisplayManager::kBad
                                                                              : DisplayManager::kDim;
}

void HealthScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  d.title("NETWORK HEALTH");
  const ConnSnapshot& c = app.lastConn;
  if (!c.everRun && !c.running) {
    d.clearRows(1, 9);
    d.row(3, "No test data yet.", D::kWarn);
    d.row(5, "Press R to run the", D::kDim);
    d.row(6, "connectivity test.", D::kDim);
    d.footer("R:run Esc:menu");
    return;
  }
  d.rowKV(1, "WiFi:      ", okStr(c.layers.wifi), okCol(c.layers.wifi));
  d.rowKV(2, "Gateway:   ", okStr(c.layers.gateway), okCol(c.layers.gateway));
  d.rowKV(3, "DNS:       ", okStr(c.layers.dns), okCol(c.layers.dns));
  d.rowKV(4, "Internet:  ", okStr(c.layers.internet), okCol(c.layers.internet));

  core::HealthInput in;
  in.status = c.status;
  char b[32];
  if (app.net.rssiValid) {
    in.rssiValid = true; in.rssiDbm = app.net.rssi;
    snprintf(b, sizeof b, "%d dBm", app.net.rssi);
    d.rowKV(5, "RSSI:      ", b);
  } else d.rowKV(5, "RSSI:      ", "N/A");
  if (c.inetStats.received()) {
    in.latencyValid = true; in.avgLatencyMs = c.inetStats.avgMs();
    snprintf(b, sizeof b, "%.0f ms", (double)in.avgLatencyMs);
    d.rowKV(6, "Latency:   ", b);
  } else d.rowKV(6, "Latency:   ", "N/A");
  if (c.inetStats.sent()) {
    in.lossValid = true; in.lossPercent = c.inetStats.lossPercent();
    snprintf(b, sizeof b, "%.0f%%", (double)in.lossPercent);
    d.rowKV(7, "Loss:      ", b);
  } else d.rowKV(7, "Loss:      ", "N/A");

  core::HealthResult h = core::computeHealth(in);
  uint16_t col = h.label == core::HealthLabel::Healthy ? D::kOk
               : h.label == core::HealthLabel::Degraded ? D::kWarn
               : h.label == core::HealthLabel::Unknown ? D::kDim : D::kBad;
  if (c.running) d.rowKV(8, "STATUS: ", "TESTING...", D::kWarn);
  else d.rowKV(8, "STATUS: ", core::healthLabelText(h.label), col);
  if (h.score >= 0 && !c.running) snprintf(b, sizeof b, "Score %d/100 (heuristic)", h.score);
  else b[0] = '\0';
  d.row(9, b, D::kDim);
  d.footer("R:rerun Esc:menu");
}
