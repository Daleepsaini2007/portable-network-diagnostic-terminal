#include "app/App.h"

namespace {
struct Item { const char* label; ScreenId id; };
const Item kItems[] = {
    {"Dashboard", ScreenId::Dashboard},     {"WiFi Scan", ScreenId::WifiScan},
    {"WiFi Connect", ScreenId::WifiConnect}, {"Network Info", ScreenId::NetInfo},
    {"Ping Test", ScreenId::Ping},          {"DNS Test", ScreenId::Dns},
    {"Connectivity", ScreenId::Connectivity}, {"Network Health", ScreenId::Health},
    {"Logs", ScreenId::Logs},               {"System", ScreenId::System},
};
}  // namespace

void MenuScreen::onEnter(App&) {}

void MenuScreen::onInput(App& app, const core::InputEvent& ev) {
  switch (ev.type) {
    case core::EvType::Up: menu_.up(); break;
    case core::EvType::Down: menu_.down(); break;
    case core::EvType::Select: app.go(kItems[menu_.selected()].id); break;
    default: break;
  }
}

void MenuScreen::render(App& app, DisplayManager& d) {
  d.title("NETWORK TERMINAL");
  for (size_t i = 0; i < menu_.visibleRows(); ++i) {
    size_t idx = menu_.first() + i;
    int r = DisplayManager::kFirstRow + (int)i;
    if (idx >= menu_.count()) { d.clearRow(r); continue; }
    char line[32];
    bool sel = idx == menu_.selected();
    snprintf(line, sizeof line, "%c %s", sel ? '>' : ' ', kItems[idx].label);
    d.row(r, line, sel ? DisplayManager::kAccent : DisplayManager::kFg);
  }
  char f[32];
  if (app.net.connected) snprintf(f, sizeof f, "WiFi: %.19s", app.net.ssid);
  else snprintf(f, sizeof f, "WiFi: not connected");
  d.footer(f);
}
