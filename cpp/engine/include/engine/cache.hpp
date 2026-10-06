#pragma once
#include <array>
#include <memory>
#include <utility>

namespace engine {
// Copies share a table and its immutable-to-read entries. A write detaches
// only the pointer table and the affected entry. An unshared worker reuses
// both allocations. Cache revisions are consequently local to each branch.
template <class T, std::size_t N> class CacheTable {
  using Table = std::array<std::shared_ptr<T>, N>;
  std::shared_ptr<Table> table_;
  std::shared_ptr<T> &slot(std::size_t i) {
    if (!table_)
      table_ = std::make_shared<Table>();
    else if (table_.use_count() != 1)
      table_ = std::make_shared<Table>(*table_);
    return (*table_)[i];
  }

public:
  const T *operator[](std::size_t i) const { return table_ ? (*table_)[i].get() : nullptr; }
  T &reset(std::size_t i) {
    auto &entry = slot(i);
    if (!entry || entry.use_count() != 1)
      entry = std::make_shared<T>();
    else
      *entry = T{};
    return *entry;
  }
  void set(std::size_t i, T value) {
    auto &entry = slot(i);
    if (!entry || entry.use_count() != 1)
      entry = std::make_shared<T>(std::move(value));
    else
      *entry = std::move(value);
  }
};
} // namespace engine
