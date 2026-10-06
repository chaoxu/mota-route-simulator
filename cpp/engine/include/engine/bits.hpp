#pragma once
#include <array>
#include <bit>
#include <bitset>
#include <cstdint>

namespace engine {
// A 13x13 floor occupies three words. libc++ implements general bitset shifts
// through bit iterators; explicit word shifts keep flood fills in registers.
struct CellMask {
  std::array<std::uint64_t, 3> words{};
  static constexpr std::uint64_t last = (std::uint64_t(1) << 41) - 1;
  constexpr CellMask() = default;
  CellMask(const std::bitset<169> &bits) {
    for (int i = 0; i < 169; ++i)
      if (bits[i])
        set(i);
  }
  constexpr bool operator[](int i) const { return (words[i / 64] >> (i % 64)) & 1; }
  constexpr CellMask &set(int i, bool value = true) {
    auto bit = std::uint64_t(1) << (i % 64);
    if (value)
      words[i / 64] |= bit;
    else
      words[i / 64] &= ~bit;
    return *this;
  }
  constexpr CellMask &reset(int i) { return set(i, false); }
  constexpr CellMask &reset() {
    words = {};
    return *this;
  }
  constexpr bool any() const { return (words[0] | words[1] | words[2]) != 0; }
  constexpr bool none() const { return !any(); }
  constexpr int count() const {
    return std::popcount(words[0]) + std::popcount(words[1]) + std::popcount(words[2]);
  }
  constexpr int first() const {
    for (int i = 0; i < 3; ++i)
      if (words[i])
        return i * 64 + std::countr_zero(words[i]);
    return 169;
  }
  constexpr int pop() {
    int i = first();
    if (i < 169)
      reset(i);
    return i;
  }
  constexpr bool operator==(const CellMask &) const = default;
  constexpr CellMask &operator|=(const CellMask &b) {
    for (int i = 0; i < 3; ++i)
      words[i] |= b.words[i];
    return *this;
  }
  constexpr CellMask &operator&=(const CellMask &b) {
    for (int i = 0; i < 3; ++i)
      words[i] &= b.words[i];
    return *this;
  }
  friend constexpr CellMask operator|(CellMask a, const CellMask &b) { return a |= b; }
  friend constexpr CellMask operator&(CellMask a, const CellMask &b) { return a &= b; }
  friend constexpr CellMask operator~(CellMask a) {
    for (auto &w : a.words)
      w = ~w;
    a.words[2] &= last;
    return a;
  }
  friend constexpr CellMask operator<<(const CellMask &a, unsigned n) {
    CellMask out;
    if (n >= 169)
      return out;
    if (!n)
      return a;
    unsigned whole = n / 64, part = n % 64;
    for (int i = 2; i >= int(whole); --i) {
      out.words[i] = a.words[i - whole] << part;
      if (part && i > int(whole))
        out.words[i] |= a.words[i - whole - 1] >> (64 - part);
    }
    out.words[2] &= last;
    return out;
  }
  friend constexpr CellMask operator>>(const CellMask &a, unsigned n) {
    CellMask out;
    if (n >= 169)
      return out;
    if (!n)
      return a;
    unsigned whole = n / 64, part = n % 64;
    for (unsigned i = 0; i + whole < 3; ++i) {
      out.words[i] = a.words[i + whole] >> part;
      if (part && i + whole + 1 < 3)
        out.words[i] |= a.words[i + whole + 1] << (64 - part);
    }
    return out;
  }
};
} // namespace engine
