// Application orchestrator: owns every service, routes input, drives screens
// and decides when events are logged.
#pragma once
#include "app/AppState.h"
#include "network/ConnectivityEngine.h"
#include "network/DNSDiagnostics.h"
#include "network/NetworkInfo.h"
#include "network/PingDiagnostics.h"
#include "network/WiFiManager.h"
#include "network/WiFiScanner.h"
#include "storage/Logger.h"
#include "system/SystemInfo.h"
#include "ui/DisplayManager.h"
#include "ui/Screens.h"

class App {
 public:
  void begin();
  void tick();  // call from loop()

  void go(ScreenId id);
  void back() { go(ScreenId::Menu); }
  ScreenId current() const { return current_; }

  // ---- services (public: screens are thin views over them) ----
  DisplayManager display;
  WiFiManager wifi;
  WiFiScanner scanner;
  PingDiagnostics ping;
  DNSDiagnostics dns;
  ConnectivityEngine conn;
  Logger logger;
  SystemInfo sys;

  // ---- shared state ----
  NetworkInfo net;          // refreshed about once per second
  ConnSnapshot lastConn;    // refreshed every tick
  char pendingSsid[33] = "";  // set by the scan screen to pre-fill the connect screen
  bool pendingOpen = false;

  void refreshNetInfo(bool force = false);
  SessionContext sessionCtx();  // pointers valid until the next refreshNetInfo()
  void markDirty() { dirty_ = true; }

 private:
  IScreen* screenFor(ScreenId id);
  void bootScreen();
  void trackEvents();

  ScreenId current_ = ScreenId::Menu;
  bool dirty_ = true;
  uint32_t lastRender_ = 0;
  uint32_t lastNetRefresh_ = 0;
  WifiState prevWifi_ = WifiState::Off;
  ScanState prevScan_ = ScanState::Idle;
  uint32_t loggedConnSeq_ = 0;

  MenuScreen menuScreen_;
  DashboardScreen dashboardScreen_;
  WifiScanScreen scanScreen_;
  WifiConnectScreen connectScreen_;
  NetInfoScreen netInfoScreen_;
  PingScreen pingScreen_;
  DnsScreen dnsScreen_;
  ConnectivityScreen connectivityScreen_;
  HealthScreen healthScreen_;
  LogsScreen logsScreen_;
  SystemScreen systemScreen_;
};
