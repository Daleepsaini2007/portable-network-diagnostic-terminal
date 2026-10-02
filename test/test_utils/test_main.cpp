// Host tests: CSV log formatting, timestamps, text input, menu model.
#include <string.h>
#include <unity.h>
#include "core/LogFormat.h"
#include "core/TextInput.h"
#include "core/MenuModel.h"

using namespace core;

void setUp() {}
void tearDown() {}

// ---------------- CsvBuilder ----------------
void test_csv_basic_row() {
  char b[128];
  CsvBuilder c(b, sizeof b);
  c.u32(1234).str("HomeWiFi").i32(-52).f1(24.0f).finishLine();
  TEST_ASSERT_TRUE(c.ok());
  TEST_ASSERT_EQUAL_STRING("1234,HomeWiFi,-52,24.0\n", b);
}
void test_csv_empty_field_and_null() {
  char b[64];
  CsvBuilder c(b, sizeof b);
  c.str("a").empty().str(nullptr).str("z");
  TEST_ASSERT_EQUAL_STRING("a,,,z", b);
}
void test_csv_quotes_commas_and_quotes() {
  char b[64];
  CsvBuilder c(b, sizeof b);
  c.str("Cafe, \"Free\" WiFi");
  TEST_ASSERT_EQUAL_STRING("\"Cafe, \"\"Free\"\" WiFi\"", b);
}
void test_csv_newline_in_field_is_quoted() {
  char b[64];
  CsvBuilder c(b, sizeof b);
  c.str("a\nb");
  TEST_ASSERT_EQUAL_STRING("\"a\nb\"", b);
}
void test_csv_formula_injection_neutralised() {
  char b[64];
  CsvBuilder c(b, sizeof b);
  c.str("=HYPERLINK(\"x\")").str("@cmd").str("+1").str("-1");
  TEST_ASSERT_EQUAL_STRING("\"'=HYPERLINK(\"\"x\"\")\",\"'@cmd\",\"'+1\",\"'-1\"", b);
  // numeric helpers must NOT be altered (negative RSSI stays numeric)
  char b2[32];
  CsvBuilder d(b2, sizeof b2);
  d.i32(-70);
  TEST_ASSERT_EQUAL_STRING("-70", b2);
}
void test_csv_overflow_is_safe() {
  char b[8];
  CsvBuilder c(b, sizeof b);
  c.str("0123456789ABCDEF");
  TEST_ASSERT_FALSE(c.ok());
  TEST_ASSERT_TRUE(strlen(b) < sizeof b);  // always NUL-terminated, no overrun
}
void test_csv_zero_capacity() {
  char b[1];
  CsvBuilder c(b, 0);
  c.str("x");
  TEST_ASSERT_FALSE(c.ok());
}

// ---------------- Timestamps ----------------
void test_utc_known_values() {
  char b[32];
  TEST_ASSERT_TRUE(formatUtc(b, sizeof b, 0));
  TEST_ASSERT_EQUAL_STRING("1970-01-01T00:00:00Z", b);
  formatUtc(b, sizeof b, 1735689600);  // verified with Python datetime
  TEST_ASSERT_EQUAL_STRING("2025-01-01T00:00:00Z", b);
  formatUtc(b, sizeof b, 1709164800);  // leap day
  TEST_ASSERT_EQUAL_STRING("2024-02-29T00:00:00Z", b);
  formatUtc(b, sizeof b, 1790933285);
  TEST_ASSERT_EQUAL_STRING("2026-10-02T09:28:05Z", b);
  formatUtc(b, sizeof b, 4107542400LL);  // 2100 is not a leap year
  TEST_ASSERT_EQUAL_STRING("2100-03-01T00:00:00Z", b);
}
void test_utc_rejects_bad_input() {
  char b[32];
  TEST_ASSERT_FALSE(formatUtc(b, sizeof b, -1));
  TEST_ASSERT_FALSE(formatUtc(b, 10, 0));
  TEST_ASSERT_FALSE(formatUtc(nullptr, 32, 0));
}
void test_uptime_format() {
  char b[32];
  formatUptime(b, sizeof b, 0);
  TEST_ASSERT_EQUAL_STRING("00:00:00", b);
  formatUptime(b, sizeof b, 3723000ULL);
  TEST_ASSERT_EQUAL_STRING("01:02:03", b);
  formatUptime(b, sizeof b, 90061000ULL);
  TEST_ASSERT_EQUAL_STRING("1d 01:01:01", b);
}

// ---------------- TextInput ----------------
void test_input_insert_backspace_clear() {
  TextInput t(8);
  TEST_ASSERT_TRUE(t.insert('a')); TEST_ASSERT_TRUE(t.insert('b'));
  TEST_ASSERT_EQUAL_STRING("ab", t.c_str());
  TEST_ASSERT_TRUE(t.backspace());
  TEST_ASSERT_EQUAL_STRING("a", t.c_str());
  t.clear();
  TEST_ASSERT_TRUE(t.empty());
  TEST_ASSERT_FALSE(t.backspace());
}
void test_input_capacity_limit() {
  TextInput t(3);
  t.insert('1'); t.insert('2'); t.insert('3');
  TEST_ASSERT_FALSE(t.insert('4'));
  TEST_ASSERT_EQUAL_STRING("123", t.c_str());
  TextInput big(1000);  // clamped to kMaxCap
  TEST_ASSERT_EQUAL(TextInput::kMaxCap, big.capacity());
}
void test_input_charset_filter() {
  TextInput t(16, CharSet::Target);
  TEST_ASSERT_TRUE(t.insert('a')); TEST_ASSERT_TRUE(t.insert('.'));
  TEST_ASSERT_TRUE(t.insert('-')); TEST_ASSERT_TRUE(t.insert(' '));
  TEST_ASSERT_FALSE(t.insert('$')); TEST_ASSERT_FALSE(t.insert('/'));
  TEST_ASSERT_FALSE(t.insert('\n'));
  TEST_ASSERT_EQUAL_STRING("a.- ", t.c_str());
}
void test_input_printable_rejects_control() {
  TextInput t(8);
  TEST_ASSERT_FALSE(t.insert('\x01'));
  TEST_ASSERT_FALSE(t.insert('\x7f'));
  TEST_ASSERT_TRUE(t.insert('~'));
}
void test_input_set_truncates() {
  TextInput t(4);
  t.set("abcdefgh");
  TEST_ASSERT_EQUAL_STRING("abcd", t.c_str());
  t.set(nullptr);
  TEST_ASSERT_TRUE(t.empty());
}

// ---------------- MenuModel ----------------
void test_menu_down_up_wrap() {
  MenuModel m(3, 3);
  TEST_ASSERT_EQUAL(0, m.selected());
  m.up();
  TEST_ASSERT_EQUAL(2, m.selected());  // wraps to last
  m.down();
  TEST_ASSERT_EQUAL(0, m.selected());  // wraps to first
}
void test_menu_scroll_window() {
  MenuModel m(10, 4);
  for (int i = 0; i < 4; ++i) m.down();
  TEST_ASSERT_EQUAL(4, m.selected());
  TEST_ASSERT_EQUAL(1, m.first());  // window follows selection
  m.select(9);
  TEST_ASSERT_EQUAL(6, m.first());
  m.down();  // wrap to 0 -> window returns to top
  TEST_ASSERT_EQUAL(0, m.first());
  m.up();    // wrap to last
  TEST_ASSERT_EQUAL(9, m.selected());
  TEST_ASSERT_EQUAL(6, m.first());
}
void test_menu_empty_and_clamp() {
  MenuModel m(0, 4);
  m.up(); m.down(); m.select(5);
  TEST_ASSERT_EQUAL(0, m.selected());
  MenuModel n(3, 4);
  n.select(99);
  TEST_ASSERT_EQUAL(2, n.selected());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_csv_basic_row); RUN_TEST(test_csv_empty_field_and_null);
  RUN_TEST(test_csv_quotes_commas_and_quotes); RUN_TEST(test_csv_newline_in_field_is_quoted);
  RUN_TEST(test_csv_formula_injection_neutralised); RUN_TEST(test_csv_overflow_is_safe);
  RUN_TEST(test_csv_zero_capacity);
  RUN_TEST(test_utc_known_values); RUN_TEST(test_utc_rejects_bad_input);
  RUN_TEST(test_uptime_format);
  RUN_TEST(test_input_insert_backspace_clear); RUN_TEST(test_input_capacity_limit);
  RUN_TEST(test_input_charset_filter); RUN_TEST(test_input_printable_rejects_control);
  RUN_TEST(test_input_set_truncates);
  RUN_TEST(test_menu_down_up_wrap); RUN_TEST(test_menu_scroll_window);
  RUN_TEST(test_menu_empty_and_clamp);
  return UNITY_END();
}
