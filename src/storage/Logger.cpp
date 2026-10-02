#include "storage/Logger.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <stdio.h>
#include <string.h>
#include "AppConfig.h"
#include "core/LogFormat.h"

using core::CsvBuilder;

static const char* const kDirs[] = {"sessions", "scans", "ping", "dns", "connectivity", "system", "errors"};

static const char* const kHeaders[] = {
    "session,uptime_ms,utc,ssid,rssi_dbm,channel,auth\n",
    "session,uptime_ms,utc,ssid,host,ip,sent,received,loss_pct,min_ms,avg_ms,max_ms,result\n",
    "session,uptime_ms,utc,ssid,host,resolved_ip,latency_ms,result\n",
    "session,uptime_ms,utc,ssid,ip,rssi_dbm,gateway,dns_server,wifi,gateway_ok,dns_ok,internet_ok,internet_via,probe_avg_ms,probe_loss_pct,status\n",
    "session,uptime_ms,utc,event,heap_free,battery_pct,detail\n",
    "session,uptime_ms,utc,component,message\n",
};

const char* logCatName(LogCat c) {
  static const char* const n[] = {"scans", "ping", "dns", "connectivity", "system", "errors"};
  return n[static_cast<int>(c)];
}

const char* sdStateText(SdState s) {
  switch (s) {
    case SdState::Ready: return "Ready";
    case SdState::NoCard: return "No card / mount failed";
    case SdState::InitFailed: return "Init failed (dirs)";
    case SdState::WriteFailed: return "Write failed";
    case SdState::Full: return "Card full";
    default: return "Not initialised";
  }
}

void Logger::path(LogCat c, char* out, size_t cap) const {
  snprintf(out, cap, LOG_ROOT "/%s/%s.csv", logCatName(c), logCatName(c));
}

bool Logger::begin() {
  state_ = SdState::NotInit;
  for (bool& h : headerDone_) h = false;
  SD.end();  // safe if not mounted; allows retry after card re-insert
  SPI.begin(SD_PIN_SCK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS);
  if (!SD.begin(SD_PIN_CS, SPI, SD_SPI_HZ) || SD.cardType() == CARD_NONE) {
    state_ = SdState::NoCard;
    return false;
  }
  cardBytes_ = SD.cardSize();

  char p[64];
  if (!SD.exists(LOG_ROOT) && !SD.mkdir(LOG_ROOT)) { state_ = SdState::InitFailed; return false; }
  for (const char* d : kDirs) {
    snprintf(p, sizeof p, LOG_ROOT "/%s", d);
    if (!SD.exists(p) && !SD.mkdir(p)) { state_ = SdState::InitFailed; return false; }
  }

  // Session id: persistent counter in sessions/next_id.txt
  uint32_t id = 1;
  const char* idPath = LOG_ROOT "/sessions/next_id.txt";
  if (SD.exists(idPath)) {
    File f = SD.open(idPath, FILE_READ);
    if (f) {
      char buf[12] = {0};
      f.readBytes(buf, sizeof buf - 1);
      f.close();
      uint32_t v = (uint32_t)strtoul(buf, nullptr, 10);
      if (v > 0) id = v;
    }
  }
  File w = SD.open(idPath, FILE_WRITE);  // truncates
  if (!w) { state_ = SdState::InitFailed; return false; }
  char nb[16];
  int nl = snprintf(nb, sizeof nb, "%lu", (unsigned long)(id + 1));
  size_t wrote = w.write(reinterpret_cast<const uint8_t*>(nb), nl);
  w.close();
  if (wrote != (size_t)nl) { state_ = SdState::InitFailed; return false; }
  session_ = id;
  state_ = SdState::Ready;
  return true;
}

bool Logger::usage(uint64_t& used, uint64_t& total) const {
  if (state_ == SdState::NotInit || state_ == SdState::NoCard) return false;
  total = SD.totalBytes();
  used = SD.usedBytes();
  return total > 0;
}

void Logger::markWriteFailure() {
  ++failures_;
  state_ = SdState::WriteFailed;
  // Distinguish "card full" from "card gone": only evaluated on failure (usedBytes is slow).
  uint64_t total = SD.totalBytes();
  if (total > 0 && (total - SD.usedBytes()) < 4096) state_ = SdState::Full;
}

bool Logger::ensureHeader(LogCat c) {
  int i = static_cast<int>(c);
  if (headerDone_[i]) return true;
  char p[64];
  path(c, p, sizeof p);
  if (!SD.exists(p)) {
    File f = SD.open(p, FILE_WRITE);
    if (!f) return false;
    size_t n = strlen(kHeaders[i]);
    size_t w = f.write(reinterpret_cast<const uint8_t*>(kHeaders[i]), n);
    f.close();
    if (w != n) return false;
  }
  headerDone_[i] = true;
  return true;
}

bool Logger::appendRaw(LogCat c, const char* data, size_t len) {
  if (!enabled_ || state_ == SdState::NotInit || state_ == SdState::NoCard ||
      state_ == SdState::InitFailed) {
    return false;
  }
  if (!ensureHeader(c)) { markWriteFailure(); return false; }
  char p[64];
  path(c, p, sizeof p);
  File f = SD.open(p, FILE_APPEND);
  if (!f) { markWriteFailure(); return false; }
  size_t w = f.write(reinterpret_cast<const uint8_t*>(data), len);
  f.close();
  if (w != len) { markWriteFailure(); return false; }
  if (state_ != SdState::Ready) state_ = SdState::Ready;  // recovered
  ++records_;
  return true;
}

void Logger::prefix(CsvBuilder& b) {
  b.u32(session_).u32(millis());
  char ts[24];
  int64_t e = epochFn_ ? epochFn_() : -1;
  if (e >= 0 && core::formatUtc(ts, sizeof ts, e)) b.str(ts);
  else b.empty();
}

bool Logger::logScan(const WiFiScanner& sc, const SessionContext& ctx) {
  if (sc.count() == 0) return false;
  char batch[1024];
  size_t used = 0;
  bool allOk = true;
  for (int i = 0; i < sc.count(); ++i) {
    char row[160];
    CsvBuilder b(row, sizeof row);
    prefix(b);
    const ScanEntry& e = sc.entry(i);
    b.str(e.ssid).i32(e.rssi).u32(e.channel).u32(e.auth).finishLine();
    if (!b.ok()) continue;
    if (used + b.size() > sizeof batch) {
      allOk &= appendRaw(LogCat::Scans, batch, used);
      used = 0;
    }
    memcpy(batch + used, row, b.size());
    used += b.size();
  }
  if (used) allOk &= appendRaw(LogCat::Scans, batch, used);
  return allOk;
}

bool Logger::logPing(const SessionContext& ctx, const char* host, const char* ip,
                     const core::PingStats& st, const char* result) {
  char row[256];
  CsvBuilder b(row, sizeof row);
  prefix(b);
  b.str(ctx.ssid).str(host).str(ip).u32(st.sent()).u32(st.received()).f1(st.lossPercent());
  if (st.received()) b.u32(st.minMs()).f1(st.avgMs()).u32(st.maxMs());
  else b.empty().empty().empty();
  b.str(result).finishLine();
  return b.ok() && appendRaw(LogCat::Ping, row, b.size());
}

bool Logger::logDns(const SessionContext& ctx, const char* host, const char* ip,
                    uint32_t latencyMs, const char* result) {
  char row[256];
  CsvBuilder b(row, sizeof row);
  prefix(b);
  b.str(ctx.ssid).str(host).str(ip).u32(latencyMs).str(result).finishLine();
  return b.ok() && appendRaw(LogCat::Dns, row, b.size());
}

static const char* layerText(core::Layer l) {
  return l == core::Layer::Pass ? "pass" : l == core::Layer::Fail ? "fail" : "not_run";
}

bool Logger::logConnectivity(const SessionContext& ctx, const ConnSnapshot& c) {
  char row[320];
  CsvBuilder b(row, sizeof row);
  prefix(b);
  b.str(ctx.ssid).str(ctx.ip);
  if (ctx.rssiValid) b.i32(ctx.rssi); else b.empty();
  b.str(ctx.gateway).str(ctx.dns)
   .str(layerText(c.layers.wifi)).str(layerText(c.layers.gateway))
   .str(layerText(c.layers.dns)).str(layerText(c.layers.internet))
   .str(c.internetVia);
  if (c.inetStats.received()) b.f1(c.inetStats.avgMs()); else b.empty();
  if (c.inetStats.sent()) b.f1(c.inetStats.lossPercent()); else b.empty();
  b.str(core::statusLabel(c.status)).finishLine();
  return b.ok() && appendRaw(LogCat::Connectivity, row, b.size());
}

bool Logger::logSystem(const SessionContext& ctx, const char* event, const char* detail) {
  char row[256];
  CsvBuilder b(row, sizeof row);
  prefix(b);
  b.str(event).u32(ESP.getFreeHeap()).empty().str(detail).finishLine();
  return b.ok() && appendRaw(LogCat::System, row, b.size());
}

bool Logger::logError(const SessionContext& ctx, const char* component, const char* message) {
  char row[256];
  CsvBuilder b(row, sizeof row);
  prefix(b);
  b.str(component).str(message).finishLine();
  return b.ok() && appendRaw(LogCat::Errors, row, b.size());
}

int Logger::tail(LogCat c, char* out, size_t cap, int maxLines) {
  if (!out || cap < 2) return 0;
  out[0] = '\0';
  if (state_ == SdState::NotInit || state_ == SdState::NoCard || state_ == SdState::InitFailed) return 0;
  char p[64];
  path(c, p, sizeof p);
  File f = SD.open(p, FILE_READ);
  if (!f) return 0;
  size_t sz = f.size();
  size_t want = cap - 1;
  size_t start = sz > want ? sz - want : 0;
  f.seek(start);
  size_t n = f.readBytes(out, want);
  f.close();
  out[n] = '\0';
  // Drop a leading partial line if we started mid-file.
  char* begin = out;
  if (start > 0) {
    char* nl = strchr(out, '\n');
    begin = nl ? nl + 1 : out + n;
  }
  // Skip the header row if it is the first line of the file.
  if (start == 0 && strncmp(begin, "session,", 8) == 0) {
    char* nl = strchr(begin, '\n');
    begin = nl ? nl + 1 : begin + strlen(begin);
  }
  // Keep only the last maxLines lines.
  int lines = 0;
  for (char* q = begin; *q; ++q) if (*q == '\n') ++lines;
  while (lines > maxLines && *begin) {
    char* nl = strchr(begin, '\n');
    if (!nl) break;
    begin = nl + 1;
    --lines;
  }
  if (begin != out) memmove(out, begin, strlen(begin) + 1);
  return lines;
}
