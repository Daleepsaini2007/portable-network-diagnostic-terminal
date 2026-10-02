#include "app/App.h"
#include "AppConfig.h"
#include "network/NetworkInfo.h"

void WifiScanScreen::onEnter(App& app) {
  shownCount_ = -1;
  if (app.wifi.hardwareOk() && app.scanner.state() != ScanState::Scanning) app.scanner.start();
}

void WifiScanScreen::onInput(App& app, const core::InputEvent& ev) {
  switch (ev.type) {
    case core::EvType::Up: list_.up(); break;
    case core::EvType::Down: list_.down(); break;
    case core::EvType::Refresh:
      if (app.wifi.hardwareOk()) app.scanner.start();  // ignored while scanning / cooling down
      break;
    case core::EvType::Select:
      if (app.scanner.state() == ScanState::Done && app.scanner.count() > 0) {
        const ScanEntry& e = app.scanner.entry((int)list_.selected());
        if (e.ssid[0] != '<') {  // skip "<hidden>"
          strlcpy(app.pendingSsid, e.ssid, sizeof app.pendingSsid);
          app.pendingOpen = (e.auth == 0);  // WIFI_AUTH_OPEN
          app.go(ScreenId::WifiConnect);
        }
      }
      break;
    case core::EvType::Back: app.back(); break;
    default: break;
  }
}

void WifiScanScreen::onTick(App& app) {
  ScanState s = app.scanner.state();
  if (s == ScanState::Scanning) shownCount_ = -1;
  else if (s == ScanState::Done && shownCount_ != app.scanner.count()) {
    shownCount_ = app.scanner.count();
    list_.configure((size_t)shownCount_, 7);
  }
}

void WifiScanScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  d.title("WIFI SCANNER");
  if (!app.wifi.hardwareOk()) {
    d.errorBox("WiFi hardware init failed", false);
    d.footer("");
    return;
  }
  ScanState s = app.scanner.state();
  if (s == ScanState::Scanning) {
    static const char* const dots[] = {"Scanning.", "Scanning..", "Scanning..."};
    d.clearRows(1, 9);
    d.row(4, dots[(millis() / 400) % 3], D::kWarn);
    d.footer("Esc:menu");
    return;
  }
  if (s == ScanState::Failed) {
    d.clearRows(1, 2);
    d.errorBox(app.scanner.error(), true);
    d.footer("");
    return;
  }
  if (s == ScanState::Idle) { d.clearRows(1, 9); d.row(4, "Press R to scan", D::kDim); d.footer("R:scan Esc:menu"); return; }

  int n = app.scanner.count();
  if (n == 0) {
    d.clearRows(1, 9);
    d.row(3, "No networks found", D::kWarn);
    d.row(5, "Press R to rescan", D::kDim);
    d.footer("R:rescan Esc:menu");
    return;
  }
  for (size_t i = 0; i < list_.visibleRows(); ++i) {
    size_t idx = list_.first() + i;
    int r = D::kFirstRow + (int)i;
    if (idx >= (size_t)n) { d.clearRow(r); continue; }
    const ScanEntry& e = app.scanner.entry((int)idx);
    char line[40];
    snprintf(line, sizeof line, "%c%-12.12s %4d %2u %-5.5s", idx == list_.selected() ? '>' : ' ',
             e.ssid, e.rssi, e.channel, wifiAuthName(e.auth));
    d.row(r, line, idx == list_.selected() ? D::kAccent : D::kFg);
  }
  const ScanEntry& sel = app.scanner.entry((int)list_.selected());
  char l8[40], l9[40];
  snprintf(l8, sizeof l8, "%.26s", sel.ssid);
  snprintf(l9, sizeof l9, "%d nets  %lus ago", n, (unsigned long)(app.scanner.ageMs() / 1000));
  d.row(8, l8, D::kDim);
  d.row(9, l9, D::kDim);
  d.footer("Enter:join R:rescan Esc");
}
