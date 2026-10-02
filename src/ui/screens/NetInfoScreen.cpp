#include "app/App.h"

void NetInfoScreen::onInput(App& app, const core::InputEvent& ev) {
  if (ev.type == core::EvType::Back) app.back();
  else if (ev.type == core::EvType::Refresh) app.refreshNetInfo(true);
}

void NetInfoScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  d.title("NETWORK INFO");
  const NetworkInfo& n = app.net;
  char b[32];
  d.rowKV(1, "State: ", n.connected ? "Connected" : "Disconnected", n.connected ? D::kOk : D::kBad);
  d.rowKV(2, "SSID:  ", n.ssid);
  d.rowKV(3, "IP:    ", n.ip);
  d.rowKV(4, "Mask:  ", n.mask);
  d.rowKV(5, "GW:    ", n.gateway);
  d.rowKV(6, "DNS:   ", n.dns);
  if (n.rssiValid) snprintf(b, sizeof b, "%d dBm  CH %d", n.rssi, n.channel);
  else snprintf(b, sizeof b, "N/A");
  d.rowKV(7, "RSSI:  ", b);
  d.rowKV(8, "Auth:  ", n.auth);
  d.rowKV(9, "MAC:   ", n.mac);
  d.footer("R:refresh  Esc:menu");
}
