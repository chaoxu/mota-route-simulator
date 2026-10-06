#pragma once
#include <array>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <type_traits>

namespace engine {
// Insertion order for unique cells, with one store on erase and append-only
// insertion. Stale entries are filtered by their current position. Compact
// at twice the cell count, bounding memory and preserving exact live order.
template <class T> class CellLog {
  std::array<std::int16_t, 169> last_;
  std::array<std::uint8_t, 338> log_{};
  int end_ = 0, size_ = 0;
  static int index(T v) {
    if constexpr (std::is_same_v<T, int>)
      return v;
    else
      return v.index();
  }
  static T value(int i) {
    if constexpr (std::is_same_v<T, int>)
      return i;
    else
      return {i % 13, i / 13};
  }
  void compact() {
    int end = 0;
    for (int i = 0; i < end_; ++i) {
      int cell = log_[i];
      if (last_[cell] == i) {
        last_[cell] = std::int16_t(end);
        log_[end++] = std::uint8_t(cell);
      }
    }
    end_ = end;
  }

public:
  CellLog() { last_.fill(-1); }
  template <bool Reverse> struct Iterator {
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = void;
    using reference = T;
    const CellLog *order;
    int cursor;
    void settle() {
      while (cursor >= 0 && cursor < order->end_ && order->last_[order->log_[cursor]] != cursor)
        cursor += Reverse ? -1 : 1;
    }
    T operator*() const { return value(order->log_[cursor]); }
    Iterator &operator++() {
      cursor += Reverse ? -1 : 1;
      settle();
      return *this;
    }
    Iterator operator++(int) {
      auto old = *this;
      ++*this;
      return old;
    }
    bool operator==(const Iterator &) const = default;
  };
  auto begin() const {
    Iterator<false> it{this, 0};
    it.settle();
    return it;
  }
  auto end() const { return Iterator<false>{this, end_}; }
  auto rbegin() const {
    Iterator<true> it{this, end_ - 1};
    it.settle();
    return it;
  }
  auto rend() const { return Iterator<true>{this, -1}; }
  int size() const { return size_; }
  bool empty() const { return size_ == 0; }
  bool contains(T v) const { return last_[index(v)] >= 0; }
  int next_index(T v) const {
    int i = last_[index(v)];
    if (i < 0)
      return 169;
    Iterator<false> it{this, i};
    ++it;
    return it == end() ? 169 : index(*it);
  }
  void erase(T v) {
    auto &last = last_[index(v)];
    if (last >= 0) {
      last = -1;
      --size_;
    }
  }
  void push_back(T v) {
    int i = index(v);
    if (i < 0 || i >= 169 || last_[i] >= 0)
      throw std::logic_error("Invalid cell log insertion");
    if (end_ == 338)
      compact();
    last_[i] = std::int16_t(end_);
    log_[end_++] = std::uint8_t(i);
    ++size_;
  }
};
} // namespace engine
