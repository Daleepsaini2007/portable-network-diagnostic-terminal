#include "app/App.h"
#include <M5Cardputer.h>
#include <string.h>
#include "AppConfig.h"

static int64_t epochNow();
static WiFiManager* gWifi = nullptr;
static int64_t epochNow() { return gWifi ? gWifi->utcEpoch() : -1; }

IScreen* App::screenFor(ScreenId id) {
  switch (id) {
    case ScreenId::Dashboard: return &dashboardScreen_;
    case ScreenId::WifiScan: return &scanScreen_;
    case ScreenId::WifiConnect: return &connectScreen_;
    case ScreenId::NetInfo: return &netInfoScreen_;
    case ScreenId::Ping: return &pingScreen_;
    case ScreenId::Dns: return &dnsScreen_;
    case ScreenId::Connectivity: return &connectivityScreen_;
    case ScreenId::Health: return &healthScreen_;
    case ScreenId::Logs: return &logsScreen_;
    case ScreenId::System: return &systemScreen_;
    default: return &menuScreen_;
  }
}

void App::bootScreen() {
  // Hardware self-test summary (also serves as the Phase-1 validation screen).
  display.invalidate();
  display.title("NETWORK TERMINAL");
  display.row(2, "Booting...", DisplayManager::kDim);

  gWifi = &wifi;
  bool wifiOk = wifi.begin();
  display.rowKV(3, "Display   ", "OK", DisplayManager::kOk);
  display.rowKV(4, "WiFi      ", wifiOk ? "OK" : "FAIL", wifiOk ? DisplayManager::kOk : DisplayManager::kBad);

  logger.setTimeSource(&epochNow);
  bool sdOk = logger.begin();
  display.rowKV(5, "microSD   ", sdOk ? "OK" : "NOT AVAILABLE", sdOk ? DisplayManager::kOk : DisplayManager::kWarn);

  SystemSnapshot s;
  sys.read(s);
  char v[24];
  if (s.batteryPercent >= 0) snprintf(v, sizeof v, "%d%%", s.batteryPercent);
  else snprintf(v, sizeof v, "N/A");
  display.rowKV(6, "Battery   ", v, DisplayManager::kFg);
  display.rowKV(7, "Device    ", s.device, DisplayManager::kFg);
  display.row(9, "Authorised use only", DisplayManager::kWarn);

  if (sdOk) {
    SessionContext ctx;
    logger.logSystem(ctx, "boot", "fw " FW_VERSION);
  }
  delay(1500);
}

void App::begin() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  display.begin();
  bootScreen();
  refreshNetInfo(true);

  // Auto-connect only to a network the user explicitly configured in config.h.
  if (WIFI_SSID[0] && strcmp(WIFI_SSID, CONFIG_PLACEHOLDER_SSID) != 0 && wifi.hardwareOk()) {
    wifi.connect(WIFI_SSID, WIFI_PASSWORD);
  }
  go(ScreenId::Menu);
}

void App::go(ScreenId id) {
  IScreen* old = screenFor(current_);
  if (old) old->onLeave(*this);
  current_ = id;
  display.invalidate();
  screenFor(id)->onEnter(*this);
  dirty_ = true;
}

void App::refreshNetInfo(bool force) {
  uint32_t now = millis();
  if (!force && now - lastNetRefresh_ < 1000) return;
  lastNetRefresh_ = now;
  readNetworkInfo(net);
}

SessionContext App::sessionCtx() {
  SessionContext c;
  c.ssid = net.connected ? net.ssid : "";
  c.ip = net.connected ? net.ip : "";
  c.gateway = net.connected ? net.gateway : "";
  c.dns = net.connected ? net.dns : "";
  c.rssi = net.rssi;
  c.rssiValid = net.rssiValid;
  return c;
}

void App::trackEvents() {
  // Wi-Fi state transitions -> system/error log (one record per transition).
  WifiState w = wifi.state();
  if (w != prevWifi_) {
    refreshNetInfo(true);
    SessionContext ctx = sessionCtx();
    if (w == WifiState::Connected) logger.logSystem(ctx, "wifi_connected", net.ssid);
    else if (w == WifiState::Lost && prevWifi_ == WifiState::Connected) logger.logSystem(ctx, "wifi_lost", "");
    else if (w == WifiState::Failed) logger.logError(ctx, "wifi", wifi.error());
    prevWifi_ = w;
    dirty_ = true;
  }

  // Scan finished -> one batched write.
  ScanState s = scanner.state();
  if (s != prevScan_) {
    if (prevScan_ == ScanState::Scanning) {
      SessionContext ctx = sessionCtx();
      if (s == ScanState::Done) logger.logScan(scanner, ctx);
      else if (s == ScanState::Failed) logger.logError(ctx, "scan", scanner.error());
    }
    prevScan_ = s;
    dirty_ = true;
  }

  // Connectivity run finished -> log once, whichever screen started it.
  lastConn = conn.snapshot();
  if (!lastConn.running && lastConn.finishedSeq != loggedConnSeq_) {
    loggedConnSeq_ = lastConn.finishedSeq;
    refreshNetInfo(true);
    logger.logConnectivity(sessionCtx(), lastConn);
    dirty_ = true;
  }
}

void App::tick() {
  M5Cardputer.update();
  wifi.update();
  scanner.update();
  ping.update();
  dns.update();
  trackEvents();
  refreshNetInfo();

  IScreen* scr = screenFor(current_);

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    auto st = M5Cardputer.Keyboard.keysState();
    core::KeyFlags k;
    k.up = st.up; k.down = st.down; k.left = st.left; k.right = st.right;
    k.enter = st.enter; k.backspace = st.backspace; k.del = st.del;
    k.esc = st.esc; k.tab = st.tab; k.fn = st.fn;
    char first = st.word.empty() ? 0 : st.word[0];
    k.word = &first;
    k.wordLen = st.word.empty() ? 0 : 1;
    core::InputEvent ev = core::translateKeys(k, scr->wantsText());
    if (ev.type != core::EvType::None) {
      scr->onInput(*this, ev);
      dirty_ = true;
    }
  }

  scr = screenFor(current_);  // onInput may have switched screens
  scr->onTick(*this);

  uint32_t now = millis();
  if (dirty_ || now - lastRender_ >= 150) {
    scr->render(*this, display);  // unchanged rows are skipped by DisplayManager
    lastRender_ = now;
    dirty_ = false;
  }
  delay(8);
}
