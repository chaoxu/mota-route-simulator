#include "engine/core.hpp"
#include "engine/graph.hpp"
namespace engine {
// The floor compiler assigns a small transition opcode to every active cell.
// This work queue preserves up/left/down/right discovery and defers scripted
// continuations exactly as the event scheduler requires.
bool Game::sweep_interact(int f, int next, CellMask danger) {
  auto rule = floors[f]->sweep_rule[next];
  constexpr std::uint64_t excluded = (1ULL << 12) | (1ULL << 13) | (1ULL << 14) | (1ULL << 19) |
                                     (1ULL << 21) | (1ULL << 26) | (1ULL << 27) | (1ULL << 28) | (1ULL << 29);
  auto p = cell_positions[next];
  auto b = floors[f]->cell[next].retained;
  if (rule == SweepRule::conditional) {
    const auto &change = world->instructions[world->floors[f].change[next]];
    bool ignore = change.ignore_present ? change.keep : flag_truth(world->ids.ignore_floor);
    if (!(ignore && b.tile == 0) || event_enabled(f, p))
      return false;
    const auto &e = world->entities[b.entity];
    if (!b.entity)
      return !danger[next];
    if (e.features & EntityFeature::fake_wall)
      rule = SweepRule::wall;
    else if (e.features & EntityFeature::lava)
      rule = SweepRule::lava;
    else if (e.kind == Kind::item)
      rule = e.features & EntityFeature::potion ? SweepRule::potion : SweepRule::item;
    else if (e.kind == Kind::enemy && !world->floors[f].auto_battle_blocked[next])
      rule = SweepRule::enemy;
    else
      return false;
  }
  switch (rule) {
  case SweepRule::wall:
  case SweepRule::lava:
    if (!automatic.wall || (rule == SweepRule::lava && !hero[snow]))
      return false;
    remove(f, p);
    break;
  case SweepRule::item:
  case SweepRule::potion:
    if (!automatic.item || (rule == SweepRule::potion && automatic.item == 2) || blocked_items[f][next] ||
        danger[next])
      return false;
    apply_item(f, b.entity);
    remove(f, p);
    after(1, f, p);
    break;
  case SweepRule::enemy:
    if (!automatic.battle || (enemy(b).special & excluded) || damage(b) != 0)
      return false;
    apply_block(f, p, b);
    break;
  default:
    throw Error("Invalid sweep rule");
  }

  return true;
}
CellMask Game::sweep_ordered(int f) {
  CellMask closure;
  if (collect_batch(f, closure))
    return closure;
  const auto &graph = world->corridors[f];
  const auto &floor = *floors[f];
  if (use_corridors && graph && hazard(f).plain &&
      (graph->base & ~(floor.empty_cells & floor.transit & ~floor.fire & ~floor.change_cells)).none()) {
    ++graph_sweeps;
    return graph->traverse(hero.pos.index(), floor.sweep_candidates, [&](int i) {
      ++graph_visits;
      return floors[f]->sweep_rule[i] == SweepRule::empty || sweep_interact(f, i, {});
    });
  }
  std::array<std::uint8_t, 256> seen{};
  seen[255] = 1;
  seen[hero.pos.index()] = 1;
  std::array<std::uint8_t, cells> queue;
  queue[0] = std::uint8_t(hero.pos.index());
  int head = 0, tail = 1;
  const auto *active = floors[f].get();
  CellMask danger;
  const Hazard *hazards = nullptr;
  Number health = 0;
  auto refresh = [&] {
    hazards = &hazard(f);
    health = hero[hp];
    danger = hazards->reactions | hazards->positive;
    for (auto mask = hazards->pincer; mask.any();) {
      auto i = mask.pop();
      danger.set(i, hazards->reactions[i] || map_damage(f, cell_positions[i]) > 0);
    }
  };
  refresh();
  while (head < tail) {
    auto current = queue[head++];
    for (auto next : auto_neighbors[current]) {
      if (seen[next])
        continue;
      seen[next] = 1;
      auto rule = active->sweep_rule[next];
      if (rule == SweepRule::empty) {
        if (!danger[next])
          queue[tail++] = next;
        continue;
      }
      if (rule == SweepRule::blocked)
        continue;
      if (!sweep_interact(f, next, danger))
        continue;
      active = floors[f].get();
      if (hazards->revision != active->hazard_revision || hazards->context != hazard_context ||
          (hazards->pincer.any() && health != hero[hp]))
        refresh();
      if (!danger[next])
        queue[tail++] = next;
    }
  }
  CellMask reached;
  for (int i = 0; i < tail; ++i)
    reached.set(queue[i]);
  return reached;
}
} // namespace engine
