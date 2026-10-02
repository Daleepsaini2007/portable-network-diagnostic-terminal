#include "core/MenuModel.h"

namespace core {

void MenuModel::configure(size_t count, size_t visibleRows) {
  count_ = count;
  rows_ = visibleRows ? visibleRows : 1;
  sel_ = 0;
  first_ = 0;
}

void MenuModel::ensureVisible() {
  if (sel_ < first_) first_ = sel_;
  if (sel_ >= first_ + rows_) first_ = sel_ - rows_ + 1;
}

void MenuModel::up() {
  if (count_ == 0) return;
  sel_ = (sel_ == 0) ? count_ - 1 : sel_ - 1;
  ensureVisible();
}

void MenuModel::down() {
  if (count_ == 0) return;
  sel_ = (sel_ + 1 >= count_) ? 0 : sel_ + 1;
  ensureVisible();
}

void MenuModel::select(size_t idx) {
  if (count_ == 0) return;
  sel_ = idx >= count_ ? count_ - 1 : idx;
  ensureVisible();
}

}  // namespace core
