// Host tests: ping-argument parser and keyboard-to-event translation.
#include <string.h>
#include <unity.h>
#include "core/Parsers.h"
#include "core/InputMap.h"

using namespace core;

void setUp() {}
void tearDown() {}

void test_parse_ip_default_count() {
  PingArgs a;
  TEST_ASSERT_EQUAL((int)ParseStatus::Ok, (int)parsePingArgs("8.8.8.8", a));
  TEST_ASSERT_EQUAL_STRING("8.8.8.8", a.target);
  TEST_ASSERT_EQUAL((int)TargetKind::IPv4, (int)a.kind);
  TEST_ASSERT_EQUAL_UINT8(4, a.count);
  TEST_ASSERT_EQUAL_UINT8(8, a.ip[0]);
}
void test_parse_host_with_count() {
  PingArgs a;
  TEST_ASSERT_EQUAL((int)ParseStatus::Ok, (int)parsePingArgs("example.com 10", a));
  TEST_ASSERT_EQUAL((int)TargetKind::Hostname, (int)a.kind);
  TEST_ASSERT_EQUAL_UINT8(10, a.count);
}
void test_parse_whitespace_tolerant() {
  PingArgs a;
  TEST_ASSERT_EQUAL((int)ParseStatus::Ok, (int)parsePingArgs("  1.1.1.1   3  ", a));
  TEST_ASSERT_EQUAL_UINT8(3, a.count);
  TEST_ASSERT_EQUAL_STRING("1.1.1.1", a.target);
}
void test_parse_empty() {
  PingArgs a;
  TEST_ASSERT_EQUAL((int)ParseStatus::Empty, (int)parsePingArgs("", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::Empty, (int)parsePingArgs("   ", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::Empty, (int)parsePingArgs(nullptr, a));
}
void test_parse_bad_target() {
  PingArgs a;
  TEST_ASSERT_EQUAL((int)ParseStatus::BadTarget, (int)parsePingArgs("999.1.1.1", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::BadTarget, (int)parsePingArgs("bad_host!", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::BadTarget, (int)parsePingArgs("-x.com 4", a));
  char longname[80];
  memset(longname, 'a', sizeof longname - 1);
  longname[sizeof longname - 1] = '\0';
  TEST_ASSERT_EQUAL((int)ParseStatus::BadTarget, (int)parsePingArgs(longname, a));
}
void test_parse_bad_count() {
  PingArgs a;
  TEST_ASSERT_EQUAL((int)ParseStatus::BadCount, (int)parsePingArgs("8.8.8.8 0", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::BadCount, (int)parsePingArgs("8.8.8.8 21", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::BadCount, (int)parsePingArgs("8.8.8.8 abc", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::BadCount, (int)parsePingArgs("8.8.8.8 4 5", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::BadCount, (int)parsePingArgs("8.8.8.8 99999", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::Ok, (int)parsePingArgs("8.8.8.8 20", a));
  TEST_ASSERT_EQUAL((int)ParseStatus::Ok, (int)parsePingArgs("8.8.8.8 1", a));
}
void test_parse_status_text_non_null() {
  for (int i = 0; i <= 3; ++i) TEST_ASSERT_NOT_NULL(parseStatusText((ParseStatus)i));
}

// ---------------- Key translation ----------------
static KeyFlags word(const char* w) {
  KeyFlags k; k.word = w; k.wordLen = strlen(w); return k;
}
void test_keys_arrows_via_fn_flags() {
  KeyFlags k; k.fn = true; k.up = true;
  TEST_ASSERT_EQUAL((int)EvType::Up, (int)translateKeys(k, false).type);
  k = KeyFlags(); k.down = true;
  TEST_ASSERT_EQUAL((int)EvType::Down, (int)translateKeys(k, true).type);  // also in text mode
}
void test_keys_nav_mode_glyph_keys() {
  TEST_ASSERT_EQUAL((int)EvType::Up, (int)translateKeys(word(";"), false).type);
  TEST_ASSERT_EQUAL((int)EvType::Down, (int)translateKeys(word("."), false).type);
  TEST_ASSERT_EQUAL((int)EvType::Left, (int)translateKeys(word(","), false).type);
  TEST_ASSERT_EQUAL((int)EvType::Right, (int)translateKeys(word("/"), false).type);
  TEST_ASSERT_EQUAL((int)EvType::Refresh, (int)translateKeys(word("r"), false).type);
  TEST_ASSERT_EQUAL((int)EvType::Clear, (int)translateKeys(word("c"), false).type);
}
void test_keys_text_mode_passes_characters_literally() {
  InputEvent e = translateKeys(word("."), true);
  TEST_ASSERT_EQUAL((int)EvType::Char, (int)e.type);
  TEST_ASSERT_EQUAL_CHAR('.', e.ch);
  e = translateKeys(word("r"), true);
  TEST_ASSERT_EQUAL((int)EvType::Char, (int)e.type);
  TEST_ASSERT_EQUAL_CHAR('r', e.ch);
}
void test_keys_nav_other_char() {
  InputEvent e = translateKeys(word("l"), false);
  TEST_ASSERT_EQUAL((int)EvType::Char, (int)e.type);
  TEST_ASSERT_EQUAL_CHAR('l', e.ch);
}
void test_keys_enter_esc_backspace() {
  KeyFlags k; k.enter = true;
  TEST_ASSERT_EQUAL((int)EvType::Select, (int)translateKeys(k, false).type);
  k = KeyFlags(); k.esc = true;
  TEST_ASSERT_EQUAL((int)EvType::Back, (int)translateKeys(k, true).type);
  k = KeyFlags(); k.backspace = true;
  TEST_ASSERT_EQUAL((int)EvType::Back, (int)translateKeys(k, false).type);
  TEST_ASSERT_EQUAL((int)EvType::Backspace, (int)translateKeys(k, true).type);
  k = KeyFlags(); k.del = true;
  TEST_ASSERT_EQUAL((int)EvType::Clear, (int)translateKeys(k, true).type);
  k = KeyFlags(); k.tab = true;
  TEST_ASSERT_EQUAL((int)EvType::Toggle, (int)translateKeys(k, true).type);
}
void test_keys_nothing_pressed() {
  KeyFlags k;
  TEST_ASSERT_EQUAL((int)EvType::None, (int)translateKeys(k, false).type);
  KeyFlags w; w.word = ""; w.wordLen = 0;
  TEST_ASSERT_EQUAL((int)EvType::None, (int)translateKeys(w, true).type);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_ip_default_count); RUN_TEST(test_parse_host_with_count);
  RUN_TEST(test_parse_whitespace_tolerant); RUN_TEST(test_parse_empty);
  RUN_TEST(test_parse_bad_target); RUN_TEST(test_parse_bad_count);
  RUN_TEST(test_parse_status_text_non_null);
  RUN_TEST(test_keys_arrows_via_fn_flags); RUN_TEST(test_keys_nav_mode_glyph_keys);
  RUN_TEST(test_keys_text_mode_passes_characters_literally);
  RUN_TEST(test_keys_nav_other_char); RUN_TEST(test_keys_enter_esc_backspace);
  RUN_TEST(test_keys_nothing_pressed);
  return UNITY_END();
}
