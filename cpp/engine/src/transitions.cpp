#include "engine/core.hpp"
#include <algorithm>
namespace engine {
namespace {
void require(bool value, const char *message) {
  if (!value)
    throw Error(message);
}
} // namespace
bool Game::step_transaction(Direction d, bool strict) {
  if (unsigned(d) >= 4)
    return false;
  int f = hero.floor;
  Pos from = hero.pos, to = add(from, delta(d));
  if (!to.valid() || props(f, from).out_event)
    return false;
  ensure_geometry(f);
  const auto &fs = *floors[f];
  auto rule = fs.enter_rule[to.index()];
  if (rule == EnterRule::generic)
    return false;
  facing = d;
  require(hero[hp] > 0, "Cannot move at zero HP");
  require(to.x >= 1 && to.y >= 1 && to.x <= 11 && to.y <= 11 && can_move(f, from, to), "Movement blocked");
  auto b = block(f, to);
  auto injury = map_damage(f, to);
  Number cost = rule == EnterRule::enemy ? damage(b) : 0;
  bool blocked = rule == EnterRule::door ||
                 (rule == EnterRule::enemy && (cost >= infinite_damage || cost + injury >= hero[hp]));
  require(injury < std::max(Number(1), hero[hp]), "Lethal hazard");
  bool delete_block = false;
  bool enough = true;
  if (rule == EnterRule::door)
    for (const auto &key : props(f, to).keys)
      if (item_count(key.item) < key.count)
        enough = false;
  if (blocked && flag_truth(world->ids.feature) && flag_number(world->ids.energy) > 0)
    delete_block = rule == EnterRule::enemy || !enough;
  if ((!blocked || delete_block) && hazard(f).reactions[to.index()])
    throw Unsupported("Directional repulse/ambush");
  // A negative map field can make the entry predicate differ from the later
  // battle-death check. Keep the original failure path, including its writes.
  if (!blocked && rule == EnterRule::enemy && cost >= hero[hp] - std::max(Number(0), injury))
    return false;
  auto serial = event_serial;
  auto move = [&](bool update_energy) {
    hero.pos = to;
    if (update_energy)
      set_number_flag(world->ids.energy, flag_number(world->ids.energy) - 1);
    if (injury > 0) {
      hero[hp] -= injury;
      hero[dmg] += injury;
      require(hero[hp] > 0, "Map damage death");
    }
  };
  if (delete_block) {
    set_number_flag(world->ids.energy, Number(0));
    remove(f, to);
    erase_event(f, to);
    move(true);
    tile_event(f, to);
    auto_sweep(f);
    return true;
  }
  if (blocked) {
    if (rule == EnterRule::enemy)
      throw Error("Cannot win battle");
    if (!enough) {
      if (strict)
        throw Error("Missing door key");
      return true;
    }
    for (const auto &key : props(f, to).keys)
      if (!key.preserve)
        set_item(key.item, item_count(key.item) - key.count);
    const auto &id = world->entities[b.entity].id;
    if (id == "yellowDoor")
      ++hero[yd];
    if (id == "blueDoor")
      ++hero[bd];
    if (id == "redDoor")
      ++hero[rd];
    remove(f, to);
    after(2, f, to);
    return true;
  }
  // A normal fight overwrites energy before any event can read it. Resolve
  // combat once, then commit the move and fight as one semantic transition.
  move(rule != EnterRule::enemy);
  if (rule == EnterRule::enemy)
    fight(f, to, b, cost);
  else if (rule == EnterRule::item) {
    apply_item(f, b.entity);
    remove(f, to);
    after(1, f, to);
  }
  if (hero.floor == f && hero.pos == to)
    tile_event(f, to);
  if (serial == event_serial)
    auto_sweep(hero.floor);
  return true;
}
} // namespace engine
