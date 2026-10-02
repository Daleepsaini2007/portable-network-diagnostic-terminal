#include "app/App.h"
#include "core/Connectivity.h"
#include "core/LogFormat.h"

void DashboardScreen::onInput(App& app, const core::InputEvent& ev) {
  if (ev.type == core::EvType::Back) app.back();
  else if (ev.type == core::EvType::Refresh) app.conn.start();
}

void DashboardScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  d.title("DASHBOARD");
  const NetworkInfo& n = app.net;
  WifiState ws = app.wifi.state();
  const char* st = "Disconnected";
  uint16_t sc = D::kBad;
  switch (ws) {
    case WifiState::Connected: st = "Connected"; sc = D::kOk; break;
    case WifiState::Connecting: st = "Connecting..."; sc = D::kWarn; break;
    case WifiState::Lost: st = "Connection lost"; sc = D::kBad; break;
    case WifiState::Failed: st = "Failed"; sc = D::kBad; break;
    case WifiState::Off: st = "Hardware off"; sc = D::kBad; break;
    default: break;
  }
  char b[32];
  d.rowKV(1, "WiFi: ", st, sc);
  d.rowKV(2, "SSID: ", n.ssid);
  d.rowKV(3, "IP:   ", n.ip);
  if (n.rssiValid) {
    snprintf(b, sizeof b, "%d dBm %s", n.rssi, core::rssiLabel(n.rssi));
    d.rowKV(4, "RSSI: ", b);
  } else d.rowKV(4, "RSSI: ", "N/A");
  d.rowKV(5, "GW:   ", n.gateway);
  d.rowKV(6, "DNS:  ", n.dns);

  const ConnSnapshot& c = app.lastConn;
  if (c.running) d.rowKV(7, "Net:  ", "CHECKING...", D::kWarn);
  else if (c.everRun) {
    snprintf(b, sizeof b, "%s (%lus ago)", core::statusLabel(c.status),
             (unsigned long)((millis() - c.finishedAtMs) / 1000));
    d.rowKV(7, "Net:  ", b, c.status == core::ConnStatus::Online ? D::kOk : D::kBad);
  } else d.rowKV(7, "Net:  ", "NOT CHECKED", D::kDim);

  SystemSnapshot s;
  app.sys.read(s);
  if (s.batteryPercent >= 0) snprintf(b, sizeof b, "%d%%", s.batteryPercent);
  else snprintf(b, sizeof b, "N/A");
  char up[20];
  core::formatUptime(up, sizeof up, s.uptimeMs);
  char line[32];
  snprintf(line, sizeof line, "%s  Up %s", b, up);
  d.rowKV(8, "Bat:  ", line);
  snprintf(line, sizeof line, "%s  Log:%s", app.logger.ready() ? "Ready" : "N/A",
           app.logger.enabled() ? "ON" : "OFF");
  d.rowKV(9, "SD:   ", line, app.logger.ready() ? D::kFg : D::kWarn);
  d.footer("R:check internet Esc:menu");
}
