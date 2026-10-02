#include "app/App.h"

static void loadTail(App& app, LogCat cat, char* buf, size_t cap, int& lines) {
  lines = app.logger.tail(cat, buf, cap, 9);
}

void LogsScreen::onEnter(App&) { mode_ = Mode::Overview; hscroll_ = 0; }

void LogsScreen::onInput(App& app, const core::InputEvent& ev) {
  using E = core::EvType;
  if (mode_ == Mode::Overview) {
    switch (ev.type) {
      case E::Up: menu_.up(); break;
      case E::Down: menu_.down(); break;
      case E::Select:
        viewCat_ = static_cast<LogCat>(menu_.selected());
        hscroll_ = 0;
        loadTail(app, viewCat_, tail_, sizeof tail_, tailLines_);
        mode_ = Mode::Viewer;
        break;
      case E::Back: app.back(); break;
      case E::Char:
        if (ev.ch == 'l' || ev.ch == 'L') app.logger.setEnabled(!app.logger.enabled());
        else if (ev.ch == 'm' || ev.ch == 'M') { app.logger.remount(); usageKnown_ = false; }
        else if (ev.ch == 'i' || ev.ch == 'I') usageKnown_ = app.logger.usage(used_, total_);
        break;
      case E::Refresh: usageKnown_ = app.logger.usage(used_, total_); break;
      default: break;
    }
  } else {
    if (ev.type == E::Left && hscroll_ >= 13) hscroll_ -= 13;
    else if (ev.type == E::Right) hscroll_ += 13;
    else if (ev.type == E::Refresh) loadTail(app, viewCat_, tail_, sizeof tail_, tailLines_);
    else if (ev.type == E::Back) mode_ = Mode::Overview;
  }
}

void LogsScreen::render(App& app, DisplayManager& d) {
  using D = DisplayManager;
  char b[40];
  if (mode_ == Mode::Viewer) {
    snprintf(b, sizeof b, "LOG: %s", logCatName(viewCat_));
    d.title(b);
    if (tailLines_ == 0) {
      d.clearRows(1, 9);
      d.row(3, app.logger.ready() ? "No records yet" : "SD unavailable", D::kWarn);
    } else {
      const char* p = tail_;
      for (int r = 1; r <= 9; ++r) {
        if (!*p || r > tailLines_) { d.clearRow(r); continue; }
        const char* nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        char seg[D::kCols + 1] = {0};
        if ((size_t)hscroll_ < len) {
          size_t n = len - hscroll_;
          if (n > (size_t)D::kCols) n = D::kCols;
          memcpy(seg, p + hscroll_, n);
        }
        d.row(r, seg);
        p = nl ? nl + 1 : p + len;
      }
    }
    d.footer("<,>:scroll R:reload Esc");
    return;
  }

  d.title("LOGS");
  SdState st = app.logger.state();
  bool ok = st == SdState::Ready;
  d.rowKV(1, "SD: ", sdStateText(st), ok ? D::kOk : D::kBad);
  if (usageKnown_) snprintf(b, sizeof b, "%lu/%lu MB used", (unsigned long)(used_ >> 20), (unsigned long)(total_ >> 20));
  else if (app.logger.cardSizeBytes()) snprintf(b, sizeof b, "%lu MB (I:usage)", (unsigned long)(app.logger.cardSizeBytes() >> 20));
  else snprintf(b, sizeof b, "N/A");
  d.rowKV(2, "Card: ", b);
  snprintf(b, sizeof b, "#%lu  rec %lu  fail %lu", (unsigned long)app.logger.sessionId(),
           (unsigned long)app.logger.recordsWritten(), (unsigned long)app.logger.writeFailures());
  d.rowKV(3, "Sess: ", b);
  d.rowKV(4, "Log:  ", app.logger.enabled() ? "ON  (L toggles)" : "OFF (L toggles)",
          app.logger.enabled() ? D::kOk : D::kWarn);
  for (size_t i = 0; i < menu_.visibleRows(); ++i) {
    size_t idx = menu_.first() + i;
    int r = 5 + (int)i;
    if (idx >= menu_.count()) { d.clearRow(r); continue; }
    snprintf(b, sizeof b, "%c %s", idx == menu_.selected() ? '>' : ' ', logCatName(static_cast<LogCat>(idx)));
    d.row(r, b, idx == menu_.selected() ? D::kAccent : D::kFg);
  }
  d.footer(ok ? "Enter:view M:mount Esc" : "[M] Mount SD  [Esc] Back");
}
