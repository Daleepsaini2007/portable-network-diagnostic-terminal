#include "app/App.h"
#include "core/Connectivity.h"

void ConnectivityScreen::onEnter(App& app) { app.conn.start(); }
void ConnectivityScreen::onLeave(App& app) { if (app.conn.running()) app.conn.cancel(); }

void ConnectivityScreen::onInput(App& app, const core::InputEvent& ev) {
  if (ev.type == core::EvType::Refresh) app.conn.start();
  else if (ev.type == core::EvType::Back) app.back();
}

static const char* layerStr(core::Layer l, bool running, int stepIdx, int curStep) {
  if (l == core::Layer::Pass) return "OK";
  if (l == core::Layer::Fail) return "FAIL";
  return (running && curStep == stepIdx) ? "..." : "--";
}
static uint16_t layerCol(core::Layer l) {
  return l == core::Layer::Pass ? DisplayManager::kOk : l == core::Layer::Fail ? DisplayManager::kBad
                                                                              : DisplayManager::kDim;
}

void ConnectivityScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  d.title("CONNECTIVITY");
  const ConnSnapshot& c = app.lastConn;
  char b[32];

  snprintf(b, sizeof b, "%s", layerStr(c.layers.wifi, c.running, 1, c.step));
  d.rowKV(1, "WiFi      ", b, layerCol(c.layers.wifi));

  if (c.layers.gateway == core::Layer::Pass && c.gwStats.received())
    snprintf(b, sizeof b, "OK  %.0f ms", (double)c.gwStats.avgMs());
  else snprintf(b, sizeof b, "%s", layerStr(c.layers.gateway, c.running, 2, c.step));
  d.rowKV(2, "Gateway   ", b, layerCol(c.layers.gateway));

  if (c.layers.dns == core::Layer::Pass) snprintf(b, sizeof b, "OK  %lu ms", (unsigned long)c.dnsLatencyMs);
  else snprintf(b, sizeof b, "%s", layerStr(c.layers.dns, c.running, 3, c.step));
  d.rowKV(3, "DNS       ", b, layerCol(c.layers.dns));

  if (c.layers.internet == core::Layer::Pass) {
    if (c.internetVia[0] == 'I') snprintf(b, sizeof b, "OK  ICMP %.0f ms", (double)c.inetStats.avgMs());
    else snprintf(b, sizeof b, "OK  TCP %lu ms", (unsigned long)c.tcpLatencyMs);
  } else snprintf(b, sizeof b, "%s", layerStr(c.layers.internet, c.running, 4, c.step));
  d.rowKV(4, "Internet  ", b, layerCol(c.layers.internet));

  d.clearRow(5);
  snprintf(b, sizeof b, "GW %s", c.gatewayIp);
  d.row(6, b, D::kDim);
  if (c.running) {
    d.rowKV(7, "STATUS: ", "TESTING...", D::kWarn);
    d.clearRow(8);
  } else if (c.everRun) {
    d.rowKV(7, "STATUS: ", core::statusLabel(c.status),
            c.status == core::ConnStatus::Online ? D::kOk : D::kBad);
    d.row(8, c.gwIcmpSuspect ? "GW ignores ICMP (traffic ok)" : "", D::kWarn);
  } else {
    d.rowKV(7, "STATUS: ", "NOT RUN", D::kDim);
    d.clearRow(8);
  }
  d.row(9, "Troubleshooting aid only", D::kDim);
  d.footer(c.running ? "Esc:menu" : "R:rerun Esc:menu");
}
