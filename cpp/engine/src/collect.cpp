#include "engine/core.hpp"
#include <algorithm>
namespace engine {
namespace {
constexpr auto horizontal = [] {
  std::array<CellMask, 2> masks;
  for (int i = 0; i < cells; ++i) {
    if (i % side)
      masks[0].set(i);
    if (i % side + 1 < side)
      masks[1].set(i);
  }
  return masks;
}();
CellMask adjacent(CellMask mask) {
  return (mask >> side) | (mask << side) | ((mask & horizontal[0]) >> 1) | ((mask & horizontal[1]) << 1);
}
} // namespace
void Game::remove_batch(int f, CellMask removed, CellMask passable, int origin) {
  auto &fs = edit_floor(f);
  fs.observations.batch(passable, removed, origin);
  fs.disabled_record = true;
  for (auto todo = removed; todo.any();) {
    int i = todo.pop();
    auto p = cell_positions[i];
    auto &c = fs.cell[i];
    auto_dirty |= world->cell_dependents[f][i];
    c.retained = {};
    c.present = false;
    c.disabled = false;
    c.payload = 0;
    c.changed = 0;
    c.touch_precompiled = false;
    c.event_entry = false;
    fs.order.erase(p);
    fs.hazard_order.erase(p);
    fs.hazard_cells.reset(i);
    fs.sweep_rule[i] = SweepRule::empty;
    fs.enter_rule[i] = EnterRule::empty;
    fs.empty_regions.update(i, true);
    fs.walk_regions.update(i, true);
  }
  fs.revision += removed.count();
  fs.transit |= removed;
  fs.terminal |= removed;
  fs.empty_cells |= removed;
  fs.sweep_candidates |= removed;
  fs.sweep_empty |= removed;
  auto keep = ~removed;
  fs.item_cells &= keep;
  fs.potion_cells &= keep;
  fs.enemy_cells &= keep;
  fs.fake_wall_cells &= keep;
  fs.lava_cells &= keep;
  fs.stairs &= keep;
  fs.change_cells &= keep;
  fs.batch_regular &= keep;
  fs.batch_conditional &= keep;
}
bool Game::collect_batch(int f, CellMask &reached) {
  if (!use_batches || !hazard(f).plain || world->floors[f].ratio < 0)
    return false;
  if (!flag_truth(world->ids.no_auto_check))
    for (int index : world->floors[f].automatic)
      if (world->automatic[index].multiple ||
          std::find(auto_executed.begin(), auto_executed.end(), index) == auto_executed.end())
        return false;
  const auto &fs = *floors[f];
  if (fs.empty_regions.dirty)
    return false;
  ++batch_probes;
  CellMask items, walls, enemies;
  if (automatic.item)
    items = (automatic.item == 2 ? fs.item_cells & ~fs.potion_cells : fs.item_cells) & ~blocked_items[f];
  if (automatic.wall) {
    walls = fs.fake_wall_cells;
    if (hero[snow])
      walls |= fs.lava_cells;
  }
  if (automatic.battle)
    enemies = fs.enemy_cells & ~world->floors[f].auto_battle_blocked;
  auto interact = fs.sweep_candidates & ~fs.change_cells;
  items &= interact;
  walls &= interact;
  enemies &= interact;
  auto unsafe = ((items | walls) & ~fs.batch_regular) | fs.batch_conditional;
  auto always = (items | walls) & fs.batch_regular;
  constexpr std::uint64_t excluded = (1ULL << 12) | (1ULL << 13) | (1ULL << 14) | (1ULL << 19) |
                                     (1ULL << 21) | (1ULL << 26) | (1ULL << 27) | (1ULL << 28) | (1ULL << 29);
  int origin = hero.pos.index();
  reached = fs.empty_regions.region(origin);
  reached.set(origin);
  if (!fs.empty_regions.active[origin])
    for (auto n : auto_neighbors[origin])
      if (n != 255)
        reached |= fs.empty_regions.region(n);
  auto other = reached;
  other.reset(origin);
  if ((other & ~fs.sweep_empty).any())
    return false;
  CellMask removed, examined, blocked_enemies, encountered;
  std::array<Number, stat_count> delta{};
  for (;;) {
    auto boundary = adjacent(reached) & ~reached;
    if ((boundary & unsafe).any())
      return false;
    auto fresh = boundary & always;
    for (auto todo = boundary & enemies & ~examined; todo.any();) {
      int i = todo.pop();
      const auto &actor = enemy(fs.cell[i].retained);
      if (actor.special & excluded)
        continue;
      encountered.set(i);
      if (damage(fs.cell[i].retained) != 0) {
        blocked_enemies.set(i);
        continue;
      }
      if (!fs.batch_regular[i] || (actor.exp_present && !exp_null))
        return false;
      fresh.set(i);
    }
    examined |= boundary;
    if (fresh.none())
      break;
    removed |= fresh;
    reached |= fresh;
    for (auto todo = fresh; todo.any();) {
      int i = todo.pop();
      const auto &entity = world->entities[fs.cell[i].retained.entity];
      if (entity.kind == Kind::item && entity.item_class == 0) {
        const auto &effect = entity.effect;
        Number ratio = effect.ratio ? world->floors[f].ratio : 1;
        for (int n = 0; n < effect.count; ++n)
          delta[effect.stat[n]] += effect.amount[n] * ratio;
      }
      for (auto neighbor : auto_neighbors[i])
        if (neighbor != 255) {
          auto room = fs.empty_regions.region(neighbor);
          if ((room & ~fs.sweep_empty).any())
            return false;
          reached |= room;
        }
    }
  }
  // With nonnegative combat terms and no reflection/defense-scaled damage,
  // raising attack or defense cannot turn a zero-damage fight into damage.
  // Every blocked boundary enemy must remain blocked after all collected
  // upgrades. This proves the same eligibility at every possible FIFO point.
  if (delta[atk] || delta[defense]) {
    if (hero[atk] < 0 || hero[defense] < 0 || hero[mdef] < 0)
      return false;
    for (auto todo = encountered; todo.any();) {
      const auto &actor = enemy(fs.cell[todo.pop()].retained);
      if (actor.hp <= 0 || actor.atk < 0 || actor.def < 0 || actor.n < 0 ||
          ((actor.special & (1ULL << 7)) && actor.def_value != 0) ||
          ((actor.special & (1ULL << 8)) && actor.atk_value != 0))
        return false;
    }
    auto prospective = hero;
    prospective[atk] += delta[atk];
    prospective[defense] += delta[defense];
    for (auto todo = blocked_enemies; todo.any();)
      if (damage_for(fs.cell[todo.pop()].retained, prospective) == 0)
        return false;
  }
  batch_exhausted = true;
  if (removed.none())
    return true;
  auto passable = fs.sweep_empty | removed;
  Number reward = 0, hatred_count = 0;
  bool got_item = false;
  for (auto todo = removed; todo.any();) {
    int i = todo.pop();
    auto b = fs.cell[i].retained;
    const auto &entity = world->entities[b.entity];
    if (entity.kind == Kind::item) {
      got_item = true;
      if (entity.item_class != 0) {
        set_item(entity.item_index, item_count(entity.item_index) + 1, true);
      }
    } else if (entity.kind == Kind::enemy) {
      const auto &actor = enemy(b);
      reward += actor.money;
      ++hatred_count;
    }
  }
  for (int s = 0; s < stat_count; ++s)
    hero.values[s] += delta[s];
  if (got_item && !flag_value(world->ids.item_hint))
    set_flag(world->ids.item_hint, Opaque{-1, false});
  if (hatred_count) {
    if (!flag_truth(world->ids.curse))
      hero[gold] += reward * (hero[coin] ? 2 : 1);
    set_number_flag(world->ids.energy, Number(0));
    set_number_flag(world->ids.hatred, flag_number(world->ids.hatred) + hatred_count * world->hatred);
    has_exp = true;
    exp_null = true;
  }
  for(auto todo=removed;todo.any();) {
    const auto i=todo.pop();const auto entity=fs.cell[i].retained.entity;
    if(world->entities[entity].kind==Kind::enemy) record_native_battle(f,cell_positions[i],entity);
  }
  remove_batch(f, removed, passable, origin);
  ++batch_count;
  batch_cells += removed.count();
  return true;
}
} // namespace engine
