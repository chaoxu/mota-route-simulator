#pragma once
#include "engine/bits.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace engine {
// Static empty corridors are contracted into weighted edges between cells
// that can interact. The path key retains exact FIFO discovery order.
struct CorridorGraph {
  struct Path {
    std::array<std::uint64_t, 6> words;
    auto operator<=>(const Path &) const = default;
    void append(int length, int direction) {
      words[length / 32] |= std::uint64_t(direction) << (62 - 2 * (length % 32));
    }
    Path followed_by(const Path &suffix, int length) const {
      auto out = *this;
      int whole = length / 32, shift = 2 * (length % 32);
      for (int i = whole; i < 6; ++i) {
        out.words[i] |= suffix.words[i - whole] >> shift;
        if (shift && i > whole)
          out.words[i] |= suffix.words[i - whole - 1] << (64 - shift);
      }
      return out;
    }
  };
  struct Hop {
    Path path;
    std::uint8_t target, distance;
  };
  struct Entry {
    CellMask reached, exits;
    std::array<std::uint8_t, 169> lookup{};
    std::vector<Hop> hops;
  };
  CellMask base;
  std::array<Entry, 169> entry;
  explicit CorridorGraph(CellMask);

  template <class Visit> CellMask traverse(int origin, CellMask considered, Visit visit) const {
    struct Key {
      Path path;
      int distance = 1000;
      bool operator<(const Key &b) const {
        return distance != b.distance ? distance < b.distance : path < b.path;
      }
    };
    std::array<Key, 169> best;
    std::array<std::uint8_t, 169> heap, slot;
    slot.fill(255);
    CellMask settled;
    settled.set(origin);
    CellMask reached = entry[origin].reached;
    reached.set(origin);
    int size = 0;
    auto up = [&](int at) {
      auto node = heap[at];
      while (at && best[node] < best[heap[(at - 1) / 2]]) {
        heap[at] = heap[(at - 1) / 2];
        slot[heap[at]] = std::uint8_t(at);
        at = (at - 1) / 2;
      }
      heap[at] = node;
      slot[node] = std::uint8_t(at);
    };
    auto expand = [&](int from, const Key &prefix) {
      for (auto remaining = entry[from].exits & considered & ~settled; remaining.any();) {
        auto cell = remaining.pop();
        const auto &hop = entry[from].hops[entry[from].lookup[cell]];
        int target = hop.target, distance = prefix.distance + hop.distance;
        if (distance > 168 || distance > best[target].distance)
          continue;
        Key next{prefix.path.followed_by(hop.path, prefix.distance), distance};
        if (!(next < best[target]))
          continue;
        best[target] = next;
        int at = slot[target];
        if (at == 255) {
          at = size++;
          heap[at] = std::uint8_t(target);
        }
        up(at);
      }
    };
    expand(origin, Key{{}, 0});
    while (size) {
      int node = heap[0];
      --size;
      if (size) {
        auto value = heap[size];
        int at = 0;
        while (at * 2 + 1 < size) {
          int child = at * 2 + 1;
          if (child + 1 < size && best[heap[child + 1]] < best[heap[child]])
            ++child;
          if (!(best[heap[child]] < best[value]))
            break;
          heap[at] = heap[child];
          slot[heap[at]] = std::uint8_t(at);
          at = child;
        }
        heap[at] = value;
        slot[value] = std::uint8_t(at);
      }
      slot[node] = 255;
      settled.set(node);
      if (visit(node)) {
        reached |= entry[node].reached;
        reached.set(node);
        expand(node, best[node]);
      }
    }
    return reached;
  }
};
} // namespace engine
