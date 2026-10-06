#include "engine/graph.hpp"
#include "engine/core.hpp"

namespace engine {
CorridorGraph::CorridorGraph(CellMask open) : base(open) {
  for (int origin = 0; origin < cells; ++origin) {
    auto &out = entry[origin];
    std::array<std::uint8_t, 256> seen{};
    std::array<std::uint8_t, cells> queue, depth{};
    std::array<Path, cells> path{};
    seen[255] = seen[origin] = 1;
    queue[0] = std::uint8_t(origin);
    int head = 0, tail = 1;
    while (head < tail) {
      auto current = queue[head++];
      for (int d = 0; d < 4; ++d) {
        auto next = auto_neighbors[current][d];
        if (seen[next])
          continue;
        seen[next] = 1;
        auto route = path[current];
        route.append(depth[current], d);
        auto length = std::uint8_t(depth[current] + 1);
        if (open[next]) {
          depth[next] = length;
          path[next] = route;
          queue[tail++] = next;
          out.reached.set(next);
        } else {
          out.exits.set(next);
          out.lookup[next] = std::uint8_t(out.hops.size());
          out.hops.push_back({route, next, length});
        }
      }
    }
  }
}
} // namespace engine
