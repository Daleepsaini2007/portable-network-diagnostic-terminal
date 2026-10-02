#include "app/App.h"
#include "AppConfig.h"
#include "core/PingStats.h"

void PingScreen::onEnter(App&) {
  if (!seeded_) { in_.set(PING_DEFAULT_TARGET); seeded_ = true; }
  mode_ = Mode::Input;
}

void PingScreen::onLeave(App& app) {
  app.ping.cancel();
  app.dns.cancel();
  mode_ = Mode::Input;
}

void PingScreen::fail(const char* msg) {
  strlcpy(err_, msg, sizeof err_);
  mode_ = Mode::Error;
}

void PingScreen::startPing(App& app) {
  if (!app.ping.start(args_.ip, args_.count, PING_TIMEOUT_MS, PING_INTERVAL_MS)) {
    fail("Could not start ping session");
    return;
  }
  stopped_ = false;
  mode_ = Mode::Running;
}

void PingScreen::startFromInput(App& app) {
  core::ParseStatus ps = core::parsePingArgs(in_.c_str(), args_);
  if (ps != core::ParseStatus::Ok) { fail(core::parseStatusText(ps)); return; }
  if (!app.wifi.connected()) { fail("WiFi disconnected. Connect first."); return; }
  if (args_.kind == core::TargetKind::Hostname) {
    if (!app.dns.start(args_.target)) { fail(dnsResultLabel(app.dns.snapshot().result)); return; }
    mode_ = Mode::Resolving;
  } else {
    startPing(app);
  }
}

void PingScreen::onInput(App& app, const core::InputEvent& ev) {
  using E = core::EvType;
  switch (mode_) {
    case Mode::Input:
      if (ev.type == E::Char) in_.insert(ev.ch);
      else if (ev.type == E::Backspace) in_.backspace();
      else if (ev.type == E::Clear) in_.clear();
      else if (ev.type == E::Select) startFromInput(app);
      else if (ev.type == E::Back) app.back();
      break;
    case Mode::Resolving:
      if (ev.type == E::Back) { app.dns.cancel(); mode_ = Mode::Input; }
      break;
    case Mode::Running:
      if (ev.type == E::Back) { app.ping.cancel(); stopped_ = true; }
      break;
    case Mode::Done:
      if (ev.type == E::Refresh) startFromInput(app);
      else if (ev.type == E::Select) mode_ = Mode::Input;
      else if (ev.type == E::Back) app.back();
      break;
    case Mode::Error:
      if (ev.type == E::Refresh) startFromInput(app);
      else if (ev.type == E::Back) mode_ = Mode::Input;
      break;
  }
}

void PingScreen::onTick(App& app) {
  if (mode_ == Mode::Resolving) {
    DnsSnapshot s = app.dns.snapshot();
    if (s.state == DnsState::Done) {
      if (s.result == DnsResult::Success && core::parseIPv4(s.ip, args_.ip)) startPing(app);
      else {
        app.logger.logError(app.sessionCtx(), "ping_dns", dnsResultLabel(s.result));
        fail(dnsResultLabel(s.result));
      }
    }
  } else if (mode_ == Mode::Running) {
    if (!app.wifi.connected()) {
      app.ping.cancel();
      app.logger.logError(app.sessionCtx(), "ping", "wifi disconnected during test");
      fail("WiFi disconnected during test");
      return;
    }
    PingSnapshot s = app.ping.snapshot();
    if (s.state == PingState::Done) {
      app.refreshNetInfo(true);
      const char* label = stopped_ ? "STOPPED" : core::verdictLabel(core::verdictFor(s.stats));
      app.logger.logPing(app.sessionCtx(), args_.target, s.ip, s.stats, label);
      mode_ = Mode::Done;
    }
  }
}

void PingScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  d.title("PING TEST");
  char b[40];
  if (mode_ == Mode::Input) {
    d.clearRows(1, 9);
    d.row(2, "Host or IPv4 [count]:", D::kDim);
    snprintf(b, sizeof b, "%.25s_", in_.length() > 25 ? in_.c_str() + in_.length() - 25 : in_.c_str());
    d.row(4, b, D::kAccent);
    d.row(6, "e.g. 8.8.8.8 4", D::kDim);
    d.row(7, "count 1-20, default 4", D::kDim);
    d.footer("Enter:start Esc:menu");
    return;
  }
  if (mode_ == Mode::Error) {
    d.clearRows(1, 2);
    d.errorBox(err_, true);
    d.footer("");
    return;
  }
  if (mode_ == Mode::Resolving) {
    d.clearRows(1, 9);
    d.row(2, "Resolving hostname...", D::kWarn);
    d.row(3, args_.target, D::kAccent);
    d.footer("Esc:cancel");
    return;
  }
  PingSnapshot s = app.ping.snapshot();
  snprintf(b, sizeof b, "Host: %.20s", args_.target);
  d.row(1, b);
  snprintf(b, sizeof b, "IP:   %s", s.ip);
  d.row(2, b);
  snprintf(b, sizeof b, "Sent: %lu/%u  Recv: %lu", (unsigned long)s.stats.sent(), s.requested,
           (unsigned long)s.stats.received());
  d.row(3, b);
  if (!s.haveLast) d.row(4, "Reply: waiting...", D::kDim);
  else if (s.lastWasTimeout) d.row(4, "Reply: request timed out", D::kBad);
  else {
    snprintf(b, sizeof b, "Reply: %lu bytes, %lu ms", (unsigned long)s.lastBytes, (unsigned long)s.lastRttMs);
    d.row(4, b, D::kOk);
  }
  d.clearRow(5);
  if (s.stats.received()) {
    snprintf(b, sizeof b, "Min %lu Avg %.1f Max %lu ms", (unsigned long)s.stats.minMs(), (double)s.stats.avgMs(),
             (unsigned long)s.stats.maxMs());
    d.row(6, b);
  } else d.row(6, "Min - Avg - Max -", D::kDim);
  snprintf(b, sizeof b, "Loss: %.0f%%", (double)s.stats.lossPercent());
  d.row(7, b, s.stats.lost() ? D::kWarn : D::kFg);
  d.clearRow(8);
  if (mode_ == Mode::Running) {
    d.rowKV(9, "Status: ", stopped_ ? "STOPPING..." : "RUNNING", D::kWarn);
    d.footer("Esc:stop");
  } else {
    core::PingVerdict v = core::verdictFor(s.stats);
    const char* t = stopped_ ? "STOPPED" : core::verdictLabel(v);
    d.rowKV(9, "Status: ", t, v == core::PingVerdict::Ok ? D::kOk : D::kBad);
    d.footer("R:again Enter:edit Esc");
  }
}
