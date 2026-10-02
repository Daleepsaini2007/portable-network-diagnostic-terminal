#include "app/App.h"
#include "AppConfig.h"

static char gSavedLabel[40];

static bool savedConfigured() {
  return WIFI_SSID[0] && strcmp(WIFI_SSID, CONFIG_PLACEHOLDER_SSID) != 0;
}

void WifiConnectScreen::rebuild(App& app) {
  nActs_ = 0;
  if (savedConfigured()) {
    snprintf(gSavedLabel, sizeof gSavedLabel, "Saved: %.18s", WIFI_SSID);
    acts_[nActs_] = Act::Saved; labels_[nActs_++] = gSavedLabel;
  }
  acts_[nActs_] = Act::Manual; labels_[nActs_++] = "Enter SSID manually";
  if (app.wifi.connected() || app.wifi.hasCredentials()) {
    acts_[nActs_] = Act::Disconnect; labels_[nActs_++] = "Disconnect";
  }
  menu_.configure((size_t)nActs_, 5);
}

void WifiConnectScreen::beginConnect(App& app, const char* ssid, const char* pass) {
  strlcpy(target_, ssid, sizeof target_);
  mode_ = app.wifi.connect(ssid, pass) ? Mode::Connecting : Mode::Error;
}

void WifiConnectScreen::onEnter(App& app) {
  reveal_ = false;
  pass_.clear();
  if (app.pendingSsid[0]) {
    strlcpy(target_, app.pendingSsid, sizeof target_);
    bool open = app.pendingOpen;
    app.pendingSsid[0] = '\0';
    app.pendingOpen = false;
    if (open) beginConnect(app, target_, "");
    else mode_ = Mode::InPass;
    return;
  }
  mode_ = (app.wifi.state() == WifiState::Connecting) ? Mode::Connecting : Mode::Choose;
  rebuild(app);
}

void WifiConnectScreen::onInput(App& app, const core::InputEvent& ev) {
  using E = core::EvType;
  switch (mode_) {
    case Mode::Choose:
      if (ev.type == E::Up) menu_.up();
      else if (ev.type == E::Down) menu_.down();
      else if (ev.type == E::Back) app.back();
      else if (ev.type == E::Select && nActs_ > 0) {
        switch (acts_[menu_.selected()]) {
          case Act::Saved: beginConnect(app, WIFI_SSID, WIFI_PASSWORD); break;
          case Act::Manual: ssid_.clear(); mode_ = Mode::InSsid; break;
          case Act::Disconnect: app.wifi.disconnect(); rebuild(app); break;
        }
      }
      break;
    case Mode::InSsid:
      if (ev.type == E::Char) ssid_.insert(ev.ch);
      else if (ev.type == E::Backspace) ssid_.backspace();
      else if (ev.type == E::Clear) ssid_.clear();
      else if (ev.type == E::Back) { mode_ = Mode::Choose; rebuild(app); }
      else if (ev.type == E::Select && !ssid_.empty()) {
        strlcpy(target_, ssid_.c_str(), sizeof target_);
        pass_.clear();
        mode_ = Mode::InPass;
      }
      break;
    case Mode::InPass:
      if (ev.type == E::Char) pass_.insert(ev.ch);
      else if (ev.type == E::Backspace) pass_.backspace();
      else if (ev.type == E::Clear) pass_.clear();
      else if (ev.type == E::Toggle) reveal_ = !reveal_;
      else if (ev.type == E::Back) { pass_.clear(); mode_ = Mode::Choose; rebuild(app); }
      else if (ev.type == E::Select) { beginConnect(app, target_, pass_.c_str()); pass_.clear(); }
      break;
    case Mode::Connecting:
      if (ev.type == E::Back) { app.wifi.disconnect(); mode_ = Mode::Choose; rebuild(app); }
      break;
    case Mode::Done:
      if (ev.type == E::Select) app.go(ScreenId::Dashboard);
      else if (ev.type == E::Back) app.back();
      break;
    case Mode::Error:
      if (ev.type == E::Refresh) mode_ = app.wifi.retry() ? Mode::Connecting : Mode::Error;
      else if (ev.type == E::Back) { mode_ = Mode::Choose; rebuild(app); }
      break;
  }
}

void WifiConnectScreen::onTick(App& app) {
  if (mode_ == Mode::Connecting) {
    WifiState s = app.wifi.state();
    if (s == WifiState::Connected) mode_ = Mode::Done;
    else if (s == WifiState::Failed || s == WifiState::Off) mode_ = Mode::Error;
  }
}

void WifiConnectScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  d.title("WIFI CONNECT");
  char b[40];
  switch (mode_) {
    case Mode::Choose:
      if (!app.wifi.hardwareOk()) { d.errorBox("WiFi hardware init failed", false); d.footer(""); return; }
      d.clearRows(1, 2);
      for (size_t i = 0; i < 5; ++i) {
        int r = 3 + (int)i;
        if ((int)i >= nActs_) { d.clearRow(r); continue; }
        snprintf(b, sizeof b, "%c %s", i == menu_.selected() ? '>' : ' ', labels_[i]);
        d.row(r, b, i == menu_.selected() ? D::kAccent : D::kFg);
      }
      d.footer("Enter:select Esc:menu");
      break;
    case Mode::InSsid:
      d.clearRows(1, 9);
      d.row(2, "Network name (SSID):", D::kDim);
      snprintf(b, sizeof b, "%.25s_", ssid_.length() > 25 ? ssid_.c_str() + ssid_.length() - 25 : ssid_.c_str());
      d.row(4, b, D::kAccent);
      d.footer("Enter:ok Esc:back");
      break;
    case Mode::InPass: {
      d.clearRows(1, 9);
      snprintf(b, sizeof b, "SSID: %.20s", target_);
      d.row(2, b, D::kDim);
      d.row(3, "Password:", D::kDim);
      size_t n = pass_.length();
      char shown[28];
      size_t off = n > 25 ? n - 25 : 0;
      size_t k = 0;
      for (size_t i = off; i < n && k < 25; ++i) shown[k++] = reveal_ ? pass_.c_str()[i] : '*';
      shown[k++] = '_';
      shown[k] = '\0';
      d.row(4, shown, D::kAccent);
      d.row(6, "Only join networks you own", D::kWarn);
      d.row(7, "or may test.", D::kWarn);
      d.footer("Enter:go Tab:show Esc");
      break;
    }
    case Mode::Connecting:
      d.clearRows(1, 9);
      d.row(2, "Connecting to:", D::kDim);
      d.row(3, target_, D::kAccent);
      snprintf(b, sizeof b, "%lu / %d s", (unsigned long)(app.wifi.connectElapsedMs() / 1000),
               WIFI_CONNECT_TIMEOUT_MS / 1000);
      d.row(5, b, D::kWarn);
      d.footer("Esc:cancel");
      break;
    case Mode::Done:
      d.clearRows(1, 9);
      d.row(2, "CONNECTED", D::kOk);
      snprintf(b, sizeof b, "SSID: %.19s", app.net.ssid);
      d.row(4, b);
      snprintf(b, sizeof b, "IP:   %s", app.net.ip);
      d.row(5, b);
      d.footer("Enter:dashboard Esc");
      break;
    case Mode::Error:
      d.clearRows(1, 2);
      snprintf(b, sizeof b, "WiFi: %s", app.wifi.error()[0] ? app.wifi.error() : "failed");
      d.errorBox(b, true);
      d.footer("");
      break;
  }
}
