// All screen classes. Implementations live in ui/screens/*.cpp.
#pragma once
#include "app/AppState.h"
#include "core/MenuModel.h"
#include "core/Parsers.h"
#include "core/TextInput.h"
#include "storage/Logger.h"
#include "ui/Screen.h"

class MenuScreen : public IScreen {
 public:
  void onEnter(App&) override;
  void onInput(App&, const core::InputEvent&) override;
  void render(App&, DisplayManager&) override;

 private:
  core::MenuModel menu_{10, 9};
};

class DashboardScreen : public IScreen {
 public:
  void onInput(App&, const core::InputEvent&) override;
  void render(App&, DisplayManager&) override;
};

class WifiScanScreen : public IScreen {
 public:
  void onEnter(App&) override;
  void onInput(App&, const core::InputEvent&) override;
  void onTick(App&) override;
  void render(App&, DisplayManager&) override;

 private:
  core::MenuModel list_{0, 7};
  int shownCount_ = -1;
};

class WifiConnectScreen : public IScreen {
 public:
  void onEnter(App&) override;
  bool wantsText() const override { return mode_ == Mode::InSsid || mode_ == Mode::InPass; }
  void onInput(App&, const core::InputEvent&) override;
  void onTick(App&) override;
  void render(App&, DisplayManager&) override;

 private:
  enum class Mode : uint8_t { Choose, InSsid, InPass, Connecting, Done, Error };
  enum class Act : uint8_t { Saved, Manual, Disconnect };
  void rebuild(App&);
  void beginConnect(App&, const char* ssid, const char* pass);
  Mode mode_ = Mode::Choose;
  core::MenuModel menu_{0, 5};
  Act acts_[3];
  const char* labels_[3];
  int nActs_ = 0;
  core::TextInput ssid_{32, core::CharSet::Printable};
  core::TextInput pass_{64, core::CharSet::Printable};
  bool reveal_ = false;
  bool ssidFromScan_ = false;
  char target_[33] = "";
};

class NetInfoScreen : public IScreen {
 public:
  void onInput(App&, const core::InputEvent&) override;
  void render(App&, DisplayManager&) override;
};

class PingScreen : public IScreen {
 public:
  void onEnter(App&) override;
  void onLeave(App&) override;
  bool wantsText() const override { return mode_ == Mode::Input; }
  void onInput(App&, const core::InputEvent&) override;
  void onTick(App&) override;
  void render(App&, DisplayManager&) override;

 private:
  enum class Mode : uint8_t { Input, Resolving, Running, Done, Error };
  void startFromInput(App&);
  void startPing(App&);
  void fail(const char* msg);
  Mode mode_ = Mode::Input;
  core::TextInput in_{48, core::CharSet::Target};
  core::PingArgs args_;
  char err_[48] = "";
  bool stopped_ = false;
  bool seeded_ = false;
};

class DnsScreen : public IScreen {
 public:
  void onEnter(App&) override;
  void onLeave(App&) override;
  bool wantsText() const override { return mode_ == Mode::Input; }
  void onInput(App&, const core::InputEvent&) override;
  void onTick(App&) override;
  void render(App&, DisplayManager&) override;

 private:
  enum class Mode : uint8_t { Input, Running, Done };
  void start(App&);
  Mode mode_ = Mode::Input;
  core::TextInput in_{62, core::CharSet::Target};
  bool seeded_ = false;
};

class ConnectivityScreen : public IScreen {
 public:
  void onEnter(App&) override;
  void onLeave(App&) override;
  void onInput(App&, const core::InputEvent&) override;
  void render(App&, DisplayManager&) override;
};

class HealthScreen : public IScreen {
 public:
  void onInput(App&, const core::InputEvent&) override;
  void render(App&, DisplayManager&) override;
};

class LogsScreen : public IScreen {
 public:
  void onEnter(App&) override;
  void onInput(App&, const core::InputEvent&) override;
  void render(App&, DisplayManager&) override;

 private:
  enum class Mode : uint8_t { Overview, Viewer };
  Mode mode_ = Mode::Overview;
  core::MenuModel menu_{static_cast<size_t>(LogCat::kCount), 5};
  LogCat viewCat_ = LogCat::Scans;
  char tail_[768] = "";
  int tailLines_ = 0;
  int hscroll_ = 0;
  bool usageKnown_ = false;
  uint64_t used_ = 0, total_ = 0;
};

class SystemScreen : public IScreen {
 public:
  void onInput(App&, const core::InputEvent&) override;
  void render(App&, DisplayManager&) override;
};
