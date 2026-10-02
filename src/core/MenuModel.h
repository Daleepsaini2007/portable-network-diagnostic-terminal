// Selection + scroll-window model for a vertical menu. Pure C++.
#pragma once
#include <stddef.h>

namespace core {

class MenuModel {
 public:
  MenuModel(size_t count, size_t visibleRows) { configure(count, visibleRows); }
  void configure(size_t count, size_t visibleRows);
  void up();    // wraps to last
  void down();  // wraps to first
  void select(size_t idx);
  size_t selected() const { return sel_; }
  size_t first() const { return first_; }  // index of first visible row
  size_t count() const { return count_; }
  size_t visibleRows() const { return rows_; }

 private:
  void ensureVisible();
  size_t count_ = 0, rows_ = 1, sel_ = 0, first_ = 0;
};

}  // namespace core
