#pragma once
#include "engine/bits.hpp"
#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace engine {
// Union by size changes only component roots when cells open. Closed cells
// invalidate the graph, and scripted batches share a lazy rebuild. Queries
// never compress paths or mutate a floor shared by independent Game clones.
template <bool KeepMembers> struct Regions {
  CellMask active, used;
  bool dirty = false;
  std::array<std::uint8_t, 169> label{}, size{};
  std::conditional_t<KeepMembers, std::array<CellMask, 169>, std::array<CellMask, 0>> members{};
  Regions() { label.fill(255); }
  int root(int i) const {
    while (label[i] != i && label[i] != 255)
      i = label[i];
    return label[i];
  }
  template <class F> void neighbors(int i, F visit) const {
    if (i >= 13 && active[i - 13])
      visit(i - 13);
    if (i + 13 < 169 && active[i + 13])
      visit(i + 13);
    if (i % 13 && active[i - 1])
      visit(i - 1);
    if (i % 13 < 12 && active[i + 1])
      visit(i + 1);
  }
  void add(int i) {
    label[i] = std::uint8_t(i);
    size[i] = 1;
    if constexpr (KeepMembers) {
      members[i].reset();
      members[i].set(i);
    }
    int winner = i;
    neighbors(i, [&](int n) {
      int other = root(n);
      if (other == winner)
        return;
      if (size[other] > size[winner])
        std::swap(other, winner);
      label[other] = std::uint8_t(winner);
      size[winner] += size[other];
      if constexpr (KeepMembers)
        members[winner] |= members[other];
    });
  }
  void update(int i, bool enabled) {
    if (active[i] == enabled)
      return;
    active.set(i, enabled);
    if (dirty)
      return;
    if (enabled)
      add(i);
    else
      dirty = true;
  }
  void rebuild() {
    if (!dirty)
      return;
    label.fill(255);
    size.fill(0);
    used.reset();
    static constexpr auto horizontal = [] {
      std::array<CellMask, 2> mask;
      for (int i = 0; i < 169; ++i) {
        if (i % 13)
          mask[0].set(i);
        if (i % 13 < 12)
          mask[1].set(i);
      }
      return mask;
    }();
    for (auto todo = active; todo.any();) {
      int r = todo.first();
      CellMask region, frontier;
      region.set(r);
      frontier = region;
      while (frontier.any()) {
        auto next = (frontier >> 13) | (frontier << 13) | ((frontier & horizontal[0]) >> 1) |
                    ((frontier & horizontal[1]) << 1);
        frontier = next & todo & ~region;
        region |= frontier;
      }
      used.set(r);
      size[r] = std::uint8_t(region.count());
      if constexpr (KeepMembers)
        members[r] = region;
      for (auto remaining = region; remaining.any();)
        label[remaining.pop()] = std::uint8_t(r);
      todo &= ~region;
    }
    dirty = false;
  }
  CellMask region(int i) const
    requires(KeepMembers)
  {
    return active[i] ? members[root(i)] : CellMask{};
  }
};
} // namespace engine
