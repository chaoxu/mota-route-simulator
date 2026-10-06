#include "engine/core.hpp"
#include <algorithm>
#include <queue>
#include <tuple>
namespace engine {
// Preserve the reference planner's costs, discovery order and target-cell
// exception. Each returned step still executes through the simulator.
std::vector<Pos> Game::path_to(int f, Pos target, bool click) const {
  if (f != hero.floor)
    throw Error("Path requested on another floor");
  if (!target.valid())
    return {};
  if (target == hero.pos)
    return {hero.pos};
  auto allowed = [&](Pos p) {
    if (!p.valid() || p.x < 1 || p.y < 1 || p.x > 11 || p.y > 11)
      return false;
    if (p == target)
      return true;
    auto b = block(f, p);
    const auto &e = world->entities[b.entity];
    const auto &v = props(f, p);
    if (floors[f]->foreground[p.index()] == 103)
      return false;
    if ((b.entity || event_enabled(f, p)) && (v.specified & 1) && v.no_pass && e.kind != Kind::enemy &&
        e.kind != Kind::door)
      return false;
    if (!b.entity)
      return true;
    if (e.kind == Kind::enemy)
      return !click && damage(b) < hero[hp];
    if (e.kind == Kind::door)
      return !click && ((e.id == "yellowDoor" && hero[yk]) || (e.id == "blueDoor" && hero[bk]) ||
                        (e.id == "redDoor" && hero[rk]));
    return !v.no_pass && v.trigger != Trigger::change_floor;
  };
  auto cost = [&](Pos p) {
    auto b = block(f, p);
    const auto &e = world->entities[b.entity];
    Number base = 1;
    if (e.kind == Kind::door)
      base = e.id == "yellowDoor" ? 90 : e.id == "blueDoor" ? 140 : e.id == "redDoor" ? 180 : 9999;
    else if (e.kind == Kind::enemy) {
      auto d = damage(b);
      base = d == 0 ? 3 : 1000 + (d == infinite_damage ? 99999 : d);
    }
    return base + map_damage(f, p) * 100;
  };
  std::array<Number, cells> best;
  best.fill(infinite_damage);
  std::array<int, cells> parent;
  parent.fill(-1);
  using Entry = std::tuple<Number, int, int>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
  int start = hero.pos.index(), serial = 0;
  best[start] = 0;
  queue.emplace(0, 0, start);
  constexpr Pos route_dirs[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  while (!queue.empty()) {
    auto [weight, order, i] = queue.top();
    queue.pop();
    if (weight != best[i])
      continue;
    auto p = cell_positions[i];
    if (p == target)
      break;
    for (auto delta : route_dirs) {
      auto next = add(p, delta);
      if (!can_move(f, p, next) || !allowed(next))
        continue;
      auto next_cost = weight + cost(next);
      int j = next.index();
      if (next_cost >= best[j])
        continue;
      best[j] = next_cost;
      parent[j] = i;
      queue.emplace(next_cost, ++serial, j);
    }
  }
  if (parent[target.index()] < 0)
    return {};
  std::vector<Pos> result;
  for (int i = target.index(); i >= 0; i = parent[i])
    result.push_back(cell_positions[i]);
  std::reverse(result.begin(), result.end());
  return result;
}
} // namespace engine
