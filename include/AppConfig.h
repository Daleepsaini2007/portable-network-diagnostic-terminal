// Central compile-time configuration. User overrides go in include/config.h.
#pragma once

#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"  // placeholders only: device will prompt for credentials
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#define CONFIG_PLACEHOLDER_SSID "YOUR_WIFI_NAME"

#ifndef INTERNET_PROBE_IP
#define INTERNET_PROBE_IP "1.1.1.1"      // ICMP echo + TCP/443 reachability probe
#endif
#ifndef DNS_TEST_DEFAULT_HOST
#define DNS_TEST_DEFAULT_HOST "example.com"
#endif
#ifndef PING_DEFAULT_TARGET
#define PING_DEFAULT_TARGET "8.8.8.8"
#endif

#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 15000
#endif
#ifndef WIFI_SCAN_TIMEOUT_MS
#define WIFI_SCAN_TIMEOUT_MS 12000
#endif
#ifndef WIFI_SCAN_COOLDOWN_MS
#define WIFI_SCAN_COOLDOWN_MS 3000       // avoid back-to-back scans
#endif
#ifndef PING_TIMEOUT_MS
#define PING_TIMEOUT_MS 1000
#endif
#ifndef PING_INTERVAL_MS
#define PING_INTERVAL_MS 500
#endif
#ifndef DNS_TIMEOUT_MS
#define DNS_TIMEOUT_MS 6000              // UI gives up waiting after this
#endif
#ifndef TCP_PROBE_TIMEOUT_MS
#define TCP_PROBE_TIMEOUT_MS 2500
#endif
#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

// SD card SPI pins used by M5Stack's official Cardputer SD example.
#define SD_PIN_SCK 40
#define SD_PIN_MISO 39
#define SD_PIN_MOSI 14
#define SD_PIN_CS 12
#define SD_SPI_HZ 25000000

#define LOG_ROOT "/network_logs"

#define FW_VERSION "0.1.0"
