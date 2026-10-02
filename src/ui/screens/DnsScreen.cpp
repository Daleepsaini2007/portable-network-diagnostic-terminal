#include "app/App.h"
#include "AppConfig.h"

void DnsScreen::onEnter(App&) {
  if (!seeded_) { in_.set(DNS_TEST_DEFAULT_HOST); seeded_ = true; }
  mode_ = Mode::Input;
}

void DnsScreen::onLeave(App& app) {
  app.dns.cancel();
  mode_ = Mode::Input;
}

void DnsScreen::start(App& app) {
  bool started = app.dns.start(in_.c_str());
  if (started) { mode_ = Mode::Running; return; }
  DnsSnapshot s = app.dns.snapshot();  // early failure: invalid host / wifi down / busy
  if (s.result != DnsResult::Busy) {
    app.refreshNetInfo(true);
    app.logger.logDns(app.sessionCtx(), s.host, s.ip, 0, dnsResultLabel(s.result));
  }
  mode_ = Mode::Done;
}

void DnsScreen::onInput(App& app, const core::InputEvent& ev) {
  using E = core::EvType;
  switch (mode_) {
    case Mode::Input:
      if (ev.type == E::Char) in_.insert(ev.ch);
      else if (ev.type == E::Backspace) in_.backspace();
      else if (ev.type == E::Clear) in_.clear();
      else if (ev.type == E::Select) start(app);
      else if (ev.type == E::Back) app.back();
      break;
    case Mode::Running:
      if (ev.type == E::Back) { app.dns.cancel(); mode_ = Mode::Input; }
      break;
    case Mode::Done:
      if (ev.type == E::Refresh) start(app);
      else if (ev.type == E::Select) mode_ = Mode::Input;
      else if (ev.type == E::Back) app.back();
      break;
  }
}

void DnsScreen::onTick(App& app) {
  if (mode_ != Mode::Running) return;
  DnsSnapshot s = app.dns.snapshot();
  if (s.state == DnsState::Done) {
    app.refreshNetInfo(true);
    app.logger.logDns(app.sessionCtx(), s.host, s.ip, s.latencyMs, dnsResultLabel(s.result));
    mode_ = Mode::Done;
  }
}

void DnsScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  d.title("DNS TEST");
  char b[40];
  if (mode_ == Mode::Input) {
    d.clearRows(1, 9);
    d.row(2, "Hostname to resolve:", D::kDim);
    snprintf(b, sizeof b, "%.25s_", in_.length() > 25 ? in_.c_str() + in_.length() - 25 : in_.c_str());
    d.row(4, b, D::kAccent);
    d.row(6, "Uses the DHCP DNS server", D::kDim);
    d.row(7, "Cached answers can be", D::kDim);
    d.row(8, "very fast (~0 ms).", D::kDim);
    d.footer("Enter:start Esc:menu");
    return;
  }
  DnsSnapshot s = app.dns.snapshot();
  d.row(1, "Host:", D::kDim);
  snprintf(b, sizeof b, "%.26s", s.host);
  d.row(2, b, D::kAccent);
  d.row(3, "Result:", D::kDim);
  d.row(4, s.ip);
  d.row(5, "Response:", D::kDim);
  if (s.result == DnsResult::Success || (s.state == DnsState::Done && s.latencyMs)) {
    snprintf(b, sizeof b, "%lu ms", (unsigned long)s.latencyMs);
    d.row(6, b);
  } else d.row(6, "N/A");
  d.row(7, "Status:", D::kDim);
  if (mode_ == Mode::Running) {
    d.row(8, "RESOLVING...", D::kWarn);
    d.footer("Esc:cancel");
  } else {
    d.row(8, dnsResultLabel(s.result), s.result == DnsResult::Success ? D::kOk : D::kBad);
    d.footer("R:again Enter:edit Esc");
  }
  d.clearRow(9);
}
