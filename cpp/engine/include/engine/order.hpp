#pragma once
#include <array>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <type_traits>

namespace engine {
// Cell identities are unique and bounded. Preserve insertion order while
// removing a cell in constant time, without allocating or moving its peers.
template <class T> class CellOrder {
  static constexpr int end_index = 169;
  std::array<std::int16_t, 170> next_, previous_;
  int size_ = 0;
  static int index(T value) {
    if constexpr (std::is_same_v<T, int>)
      return value;
    else
      return value.index();
  }
  static T value(int i) {
    if constexpr (std::is_same_v<T, int>)
      return i;
    else
      return {i % 13, i / 13};
  }

public:
  CellOrder() {
    next_.fill(-1);
    previous_.fill(-1);
    next_[end_index] = previous_[end_index] = end_index;
  }
  template <bool Reverse> struct Iterator {
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using pointer = void;
    using reference = T;
    const CellOrder *order;
    int current;
    T operator*() const { return value(current); }
    Iterator &operator++() {
      current = (Reverse ? order->previous_ : order->next_)[current];
      return *this;
    }
    Iterator operator++(int) {
      auto old = *this;
      ++*this;
      return old;
    }
    bool operator==(const Iterator &) const = default;
  };
  auto begin() const { return Iterator<false>{this, next_[end_index]}; }
  auto end() const { return Iterator<false>{this, end_index}; }
  auto rbegin() const { return Iterator<true>{this, previous_[end_index]}; }
  auto rend() const { return Iterator<true>{this, end_index}; }
  int size() const { return size_; }
  bool empty() const { return size_ == 0; }
  bool contains(T v) const { return next_[index(v)] >= 0; }
  int next_index(T v) const { return next_[index(v)]; }
  void erase(T v) {
    const int i = index(v);
    if (next_[i] < 0)
      return;
    next_[previous_[i]] = next_[i];
    previous_[next_[i]] = previous_[i];
    next_[i] = previous_[i] = -1;
    --size_;
  }
  void insert_before_index(int before, T v) {
    const int i = index(v);
    if (i < 0 || i >= end_index || next_[i] >= 0)
      throw std::logic_error("Invalid cell order insertion");
    const int last = previous_[before];
    next_[last] = i;
    previous_[i] = last;
    next_[i] = before;
    previous_[before] = i;
    ++size_;
  }
  void push_back(T v) { insert_before_index(end_index, v); }
};
} // namespace engine
