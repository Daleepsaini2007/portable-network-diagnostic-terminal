// CSV diagnostic logging on the microSD card. Records are appended only when a
// test completes (never continuously) and every failure path is reported.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "core/Connectivity.h"
#include "core/LogFormat.h"
#include "core/PingStats.h"
#include "network/ConnectivityEngine.h"
#include "network/WiFiScanner.h"

enum class LogCat : uint8_t { Scans, Ping, Dns, Connectivity, System, Errors, kCount };

enum class SdState : uint8_t {
  NotInit,      // begin() not called yet
  Ready,
  NoCard,       // mount failed / no card detected
  InitFailed,   // card mounted but the log directory tree could not be created
  WriteFailed,  // last write failed (card removed or error)
  Full          // write failed and free space is (nearly) zero
};

const char* logCatName(LogCat c);
const char* sdStateText(SdState s);

struct SessionContext {  // what each record is tagged with
  const char* ssid = "";
  const char* ip = "";
  const char* gateway = "";
  const char* dns = "";
  int rssi = 0;
  bool rssiValid = false;
};

class Logger {
 public:
  using EpochFn = int64_t (*)();  // returns UTC epoch or -1 when unknown
  void setTimeSource(EpochFn fn) { epochFn_ = fn; }

  bool begin();  // mount SD, create directories, allocate session id
  bool remount() { return begin(); }
  bool ready() const { return state_ == SdState::Ready; }
  SdState state() const { return state_; }
  void setEnabled(bool on) { enabled_ = on; }
  bool enabled() const { return enabled_; }

  uint32_t sessionId() const { return session_; }
  uint32_t recordsWritten() const { return records_; }
  uint32_t writeFailures() const { return failures_; }

  // Card information. usage() walks the FAT and can take seconds: call on demand only.
  uint64_t cardSizeBytes() const { return cardBytes_; }
  bool usage(uint64_t& usedBytes, uint64_t& totalBytes) const;

  // Record helpers (each returns true if the record reached the card).
  bool logScan(const WiFiScanner& sc, const SessionContext& ctx);
  bool logPing(const SessionContext& ctx, const char* host, const char* ip,
               const core::PingStats& st, const char* result);
  bool logDns(const SessionContext& ctx, const char* host, const char* ip,
              uint32_t latencyMs, const char* result);
  bool logConnectivity(const SessionContext& ctx, const ConnSnapshot& c);
  bool logSystem(const SessionContext& ctx, const char* event, const char* detail);
  bool logError(const SessionContext& ctx, const char* component, const char* message);

  // Copies the last `maxLines` lines of a category file into out (NUL-terminated,
  // '\n' separated). Returns the number of lines copied (0 if none/unavailable).
  int tail(LogCat c, char* out, size_t cap, int maxLines);

 private:
  bool appendRaw(LogCat c, const char* data, size_t len);
  bool ensureHeader(LogCat c);
  void prefix(core::CsvBuilder& b);
  void markWriteFailure();
  void path(LogCat c, char* out, size_t cap) const;

  EpochFn epochFn_ = nullptr;
  SdState state_ = SdState::NotInit;
  bool enabled_ = true;
  uint32_t session_ = 0;
  uint32_t records_ = 0;
  uint32_t failures_ = 0;
  uint64_t cardBytes_ = 0;
  bool headerDone_[static_cast<int>(LogCat::kCount)] = {false};
};
