#pragma once
#include "engine/bits.hpp"
#include "engine/order-log.hpp"
#include <boost/container/small_vector.hpp>
#include <memory>

namespace engine {
// Disabled/enabled insertion order is an observation, not gameplay state.
// A bulk collection records its traversal rule, and export reconstructs its
// exact FIFO removal order. Every gameplay effect has already been applied.
struct ObservationLog {
  struct Orders {
    CellLog<int> disabled, enabled;
  };
  struct Batch {
    CellMask passable, removed;
    int origin;
  };
  std::shared_ptr<const Orders> seed;
  boost::container::small_vector<std::uint32_t, 16> changes;
  boost::container::small_vector<Batch, 1> batches;
  void point(int i, bool enabled) { changes.push_back(std::uint32_t(i) | (enabled ? 256 : 0)); }
  void batch(CellMask passable, CellMask removed, int origin) {
    if (batches.size() >= 0x80000000ULL)
      throw std::length_error("Observation log exhausted");
    changes.push_back(0x80000000U | std::uint32_t(batches.size()));
    batches.push_back({passable, removed, origin});
  }
  Orders materialize() const {
    Orders out = seed ? *seed : Orders{};
    auto point = [&](int i, bool enabled) {
      out.disabled.erase(i);
      out.enabled.erase(i);
      (enabled ? out.enabled : out.disabled).push_back(i);
    };
    for (auto change : changes) {
      if (!(change & 0x80000000U)) {
        point(change & 255, change & 256);
        continue;
      }
      const auto &batch = batches.at(change & 0x7fffffffU);
      std::array<std::uint8_t, 169> queue, seen{};
      queue[0] = std::uint8_t(batch.origin);
      seen[batch.origin] = 1;
      int head = 0, tail = 1;
      while (head < tail) {
        int current = queue[head++];
        for (int next : {current - 13, current % 13 ? current - 1 : -1, current + 13,
                         current % 13 < 12 ? current + 1 : -1}) {
          if (next < 0 || next >= 169 || seen[next] || !batch.passable[next])
            continue;
          seen[next] = 1;
          if (batch.removed[next])
            point(next, false);
          queue[tail++] = std::uint8_t(next);
        }
      }
    }
    return out;
  }
};
} // namespace engine
