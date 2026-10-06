#include "engine/core.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace engine {
namespace {
thread_local const std::function<void(int,Pos,int)> *battle_observer=nullptr;
}
ScopedBattleObserver::ScopedBattleObserver(std::function<void(int,Pos,int)> callback)
  :callback_(std::move(callback)),previous_(battle_observer) {battle_observer=&callback_;}
ScopedBattleObserver::~ScopedBattleObserver() {battle_observer=previous_;}
void record_native_battle(int floor,Pos pos,int entity) {
  if(battle_observer) (*battle_observer)(floor,pos,entity);
}
static void require(bool ok, const char *message) {
  if (!ok)
    throw Error(message);
}
void Game::detach() {
  for (int f = world->first_floor; f < floors_count; ++f)
    if (floors[f])
      edit_floor(f);
  if (flags.use_count() != 1)
    flags = std::make_shared<Flags>(*flags);
  flags->scalar.detach();
  if (resolved_enemies && resolved_enemies.use_count() != 1) {
    resolved_enemies = std::make_shared<std::vector<Actor>>(*resolved_enemies);
    actor_data = resolved_enemies->data();
  }
  if (overrides.use_count() != 1)
    overrides = std::make_shared<OverrideTable>(*overrides);
}
Direction direction(std::string_view s) {
  for (int i = 0; i < 10; ++i)
    if (s == direction_name(Direction(i)))
      return Direction(i);
  throw Error("Invalid direction");
}
std::string_view direction_name(Direction d) {
  constexpr std::string_view names[] = {"up",       "down",    "left",      "right",   "leftup",
                                        "leftdown", "rightup", "rightdown", "forward", "backward"};
  return names[int(d)];
}
Pos delta(Direction d) {
  if (int(d) < 4)
    return directions[int(d)];
  switch (d) {
  case Direction::leftup:
    return {-1, -1};
  case Direction::leftdown:
    return {-1, 1};
  case Direction::rightup:
    return {1, -1};
  case Direction::rightdown:
    return {1, 1};
  default:
    throw Unsupported("Relative direction needs facing");
  }
}
int floor_number(std::string_view s) {
  require(s.size() >= 3 && s.size() <= 4 && s.starts_with("MT"), "Invalid floor name");
  int f = 0;
  for (char c : s.substr(2)) {
    require(c >= '0' && c <= '9', "Invalid floor name");
    f = f * 10 + c - '0';
  }
  require(f <= 50, "Invalid floor");
  return f;
}
std::string floor_name(int f) {
  return "MT" + std::to_string(f);
}
std::optional<Value> Game::flag_value(int id) const {
  if (unsigned(id) < FlagIds::count) {
    if (builtin_types[id] == 2)
      return Value(builtin_numbers[id]);
    if (builtin_types[id] == 1)
      return Value(bool(builtin_numbers[id]));
    return builtin_flags[id];
  }
  static const std::optional<Value> missing;
  if (id < 0 || std::size_t(id) >= flags->scalar.size())
    return missing;
  return flags->scalar[id];
}
Value Game::flag(int id, Value fallback) const {
  const auto &v = flag_value(id);
  return v ? *v : fallback;
}
void Game::store_flag(int id, std::optional<Value> v) {
  if (std::size_t(id) < world->flag_dependents.size())
    auto_dirty |= world->flag_dependents[id];
  if (unsigned(id) < FlagIds::count) {
    builtin_numbers[id] = v ? integer(*v) : 0;
    const auto bit = std::uint32_t(1) << id;
    if (v && rule_truth(*v))
      builtin_truth |= bit;
    else
      builtin_truth &= ~bit;
    builtin_types[id] = v ? std::uint8_t(v->index()) : 255;
    if (builtin_types[id] == 1 || builtin_types[id] == 2)
      builtin_flags[id].reset();
    else
      builtin_flags[id] = std::move(v);
    return;
  }
  if (flags.use_count() != 1)
    flags = std::make_shared<Flags>(*flags);
  flags->identity.reset();
  flags->scalar.set(id, std::move(v));
}
void Game::set_flag(int id, Value v) {
  if (unsigned(id) < FlagIds::count)
    if (auto n = std::get_if<Number>(&v)) {
      set_number_flag(id, *n);
      return;
    }
  require(id >= 0 && std::size_t(id) < flags->scalar.size(), "Flag outside compiled world");
  const auto &old = flag_value(id);
  if (v.index() == 0 && !old)
    return;
  if (v.index() != 0 && old && *old == v)
    return;
  if (v.index() != 0 && world->is_temporary[id])
    temporaries_possible = true;
  if (v.index() == 0)
    store_flag(id, std::nullopt);
  else
    store_flag(id, std::move(v));
  const auto &i = world->ids;
  if (id == i.magic_immune || id == i.no_zone || id == i.no_repulse || id == i.no_laser ||
      id == i.no_between || id == i.no_ambush)
    ++hazard_context;
}
void Game::clear_temporaries() {
  if (!temporaries_possible)
    return;
  for (int id : world->temporary_flags)
    if (flag_value(id))
      set_flag(id, {});
  temporaries_possible = false;
}
void Game::assign(Target t, Value v, Assign op, int f, Pos p) {
  if (op != Assign::set) {
    Number old = integer(read(t, f, p)), n = integer(v);
    switch (op) {
    case Assign::add:
      v = old + n;
      break;
    case Assign::sub:
      v = old - n;
      break;
    case Assign::mul:
      v = old * n;
      break;
    case Assign::div:
      require(n != 0, "Division zero");
      v = old / n;
      break;
    case Assign::mod:
      require(n != 0, "Modulo zero");
      v = old % n;
      break;
    default:
      break;
    }
  }
  switch (t.kind) {
  case Ref::flag:
    set_flag(t.index, v);
    if (t.index == world->ids.times)
      hero[times1] = integer(v);
    if (t.index == world->ids.magic_immune)
      hero[magicImmune] = truth(v);
    return;
  case Ref::switch_flag:
    set_flag(world->floors[f].switch_slot[p.index()], v);
    return;
  case Ref::stat:
    hero.values.at(t.index) = integer(v);
    return;
  case Ref::item: {
    auto n = integer(v);
    set_item(t.index, n, n > item_count(t.index));
    return;
  }
  case Ref::status_x:
    hero.pos.x = int(integer(v));
    return;
  case Ref::status_y:
    hero.pos.y = int(integer(v));
    return;
  case Ref::status_direction:
    facing = direction(text(v));
    return;
  case Ref::extra:
    if (flags.use_count() != 1)
      flags = std::make_shared<Flags>(*flags);
    flags->identity.reset();
    flags->extra.at(t.index) = v;
    return;
  default:
    throw Unsupported("Read-only assignment target");
  }
}
Number Game::item_count(int id) const {
  if (id < 0)
    return 0;
  require(id >= 0 && id < int(world->item_names.size()), "Invalid item ID");
  int s = world->item_stat[id];
  return s >= 0 ? hero.values[s] : flags->inventory[id];
}
void Game::set_item(int id, Number n, bool increase) {
  if (world->item_class[id] == 0)
    return;
  n = std::max(Number(0), n);
  if (increase && world->item_class[id] == 1)
    n = std::min(Number(1), n);
  int s = world->item_stat[id];
  if (s >= 0)
    hero.values[s] = n;
  else {
    if (flags.use_count() != 1)
      flags = std::make_shared<Flags>(*flags);
    flags->identity.reset();
    flags->inventory[id] = n;
  }
}
const Cell &Game::cell(int f, Pos p) const {
  static const Cell empty;
  return f >= world->first_floor && f < floors_count && p.valid() ? floors[f]->cell[p.index()] : empty;
}
bool Game::present(int f, Pos p, bool disabled) const {
  const auto &c = cell(f, p);
  return c.present && (disabled || !c.disabled);
}
Block Game::block(int f, Pos p) const {
  const auto &c = cell(f, p);
  return c.present && !c.disabled ? c.retained : Block{};
}
const Props &Game::retained_props(const Cell &c) const {
  if (c.changed)
    return world->composed[c.changed - 1];
  if (c.payload)
    return world->payloads[c.payload];
  return world->entities[c.retained.entity].props;
}
const Props &Game::props(int f, Pos p) const {
  static const Props empty;
  const auto &c = cell(f, p);
  return c.present && !c.disabled ? retained_props(c) : empty;
}
bool Game::event_enabled(int f, Pos p) const {
  const auto &c = cell(f, p);
  return c.event_entry && c.present && !c.disabled;
}
FloorState &Game::edit_floor(int f) {
  require(f >= world->first_floor && f < floors_count, "Invalid floor mutation");
  record_floor(f);
  if (floors[f].use_count() != 1)
    floors[f] = std::make_shared<FloorState>(*floors[f]);
  floors[f]->identity.reset();
  return *floors[f];
}
void Game::sync_cell(int f, Pos p) {
  auto &fs = edit_floor(f);
  auto &c = fs.cell[p.index()];
  c.event_entry = c.present && !c.disabled && retained_props(c).has_data &&
                  retained_props(c).trigger != Trigger::change_floor;
  ++fs.revision;
  mark_geometry(f, p);
}
void Game::disabled_flag(int f, Pos p, bool disabled) {
  auto &fs = edit_floor(f);
  fs.disabled_record = true;
  fs.observations.point(p.index(), !disabled);
}
void Game::remove(int f, Pos p, bool destruct) {
  if (!present(f, p, true))
    return;
  auto_dirty |= world->cell_dependents[f][p.index()];
  auto old = cell(f, p).retained, active = block(f, p);
  invalidate_hazard(f, active, {});
  if (!destruct && (old.tile == 121 || old.tile == 122))
    set_number_flag(world->ids.energy, Number(2));
  auto &fs = edit_floor(f);
  int i = p.index();
  auto &c = fs.cell[i];
  if (destruct && world->floors[f].source_event[i])
    c.source_destroyed = true;
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
  fs.disabled_record = true;
  fs.observations.point(i, false);
  ++fs.revision;
  fs.enemy_cells.reset(i);
  // An ordinary removal has a known empty-cell result. Preserve pending
  // scripted changes, but do not route this common transition through a
  // complete cell-rule compilation or connectivity rebuild.
  const auto &prior = world->entities[active.entity].props;
  if (fs.dirty_cells[i] || prior.in || prior.out) {
    mark_geometry(f, p);
    return;
  }
  fs.sweep_rule[i] = fs.fire[i] ? SweepRule::blocked : SweepRule::empty;
  fs.enter_rule[i] = EnterRule::empty;
  fs.sweep_candidates.set(i, !fs.fire[i]);
  fs.sweep_empty.set(i, !fs.fire[i]);
  fs.batch_regular.reset(i);
  fs.batch_conditional.reset(i);
  fs.transit.set(i);
  fs.terminal.set(i);
  fs.empty_cells.set(i);
  fs.item_cells.reset(i);
  fs.potion_cells.reset(i);
  fs.fake_wall_cells.reset(i);
  fs.lava_cells.reset(i);
  fs.stairs.reset(i);
  fs.change_cells.reset(i);
  fs.walk_regions.update(i, true);
  fs.empty_regions.update(i, true);
}
void Game::replace(int f, Pos p, Block b, bool destruct, bool activate) {
  require(f >= world->first_floor && f < floors_count && p.valid(), "Invalid replacement");
  invalidate_hazard(f, block(f, p), b);
  auto &fs = edit_floor(f);
  auto &c = fs.cell[p.index()];
  if (destruct)
    c.source_destroyed = true;
  const auto &fd = world->floors[f];
  auto overlay = c.source_destroyed ? 0 : fd.source_payload[p.index()];
  auto rule = world->composition.at(overlay * world->entities.size() + b.entity);
  const auto &value = world->composed[rule - 1];
  if (!b.entity && value.trigger == Trigger::none) {
    remove(f, p, destruct);
    return;
  }
  bool existed = c.present;
  c.retained = b;
  c.present = true;
  c.changed = rule;
  c.payload = 0;
  c.base_payload = false;
  c.touch_precompiled = false;
  if (!existed) {
    fs.order.push_back(p);
    c.disabled = false;
    disabled_flag(f, p, false);
  } else if (destruct || activate) {
    c.disabled = false;
    disabled_flag(f, p, false);
  }
  sync_cell(f, p);
}
void Game::hide(int f, Pos p) {
  if (!present(f, p, true))
    return;
  invalidate_hazard(f, block(f, p), {});
  edit_floor(f).cell[p.index()].disabled = true;
  disabled_flag(f, p, true);
  sync_cell(f, p);
}
void Game::show(int f, Pos p) {
  if (!present(f, p, true) || !cell(f, p).disabled)
    return;
  invalidate_hazard(f, {}, cell(f, p).retained);
  edit_floor(f).cell[p.index()].disabled = false;
  disabled_flag(f, p, false);
  sync_cell(f, p);
}
void Game::erase_event(int f, Pos p) {
  invalidate_hazard(f, {}, {});
  auto &fs = edit_floor(f);
  auto &c = fs.cell[p.index()];
  c.event_entry = false;
  if (c.present) {
    c.changed = 0;
    c.payload = 0;
    c.base_payload = true;
    if (!c.retained.entity)
      remove(f, p);
  }
  ++fs.revision;
  mark_geometry(f, p);
}
void Game::layer(int f, Pos p, int tile, bool fg) {
  if (!p.valid())
    return;
  invalidate_hazard(f, {}, {});
  auto &fs = edit_floor(f);
  (fg ? fs.foreground : fs.background)[p.index()] = std::int16_t(tile);
  auto &changes = fg ? fs.foreground_changes : fs.background_changes;
  changes.erase(std::remove_if(changes.begin(), changes.end(), [&](const auto &r) { return r.pos == p; }),
                changes.end());
  changes.push_back({p, tile});
  (fg ? fs.foreground_record : fs.background_record) = true;
  ++fs.revision;
  mark_geometry(f, p);
}
const Actor &Game::enemy(Block b) const {
  if (b.entity >= world->actors.size())
    throw Error("Enemy ID outside compiled world");
  return actor_data[b.entity];
}
void Game::resolve_enemy(int id) {
  const auto &o = overrides->at(id);
  if (!resolved_enemies)
    resolved_enemies = std::make_shared<std::vector<Actor>>(world->actors);
  else if (resolved_enemies.use_count() != 1)
    resolved_enemies = std::make_shared<std::vector<Actor>>(*resolved_enemies);
  actor_data = resolved_enemies->data();
  Actor e = world->actors.at(id);
  for (int i = 0; i < 12; ++i)
    if (o.present[i]) {
      if (i < 9 && o.value[i].index() == 0)
        continue;
      auto n = integer(o.value[i]);
      switch (i) {
      case 0:
        e.hp = n;
        break;
      case 1:
        e.atk = n;
        break;
      case 2:
        e.def = n;
        break;
      case 3:
        e.money = n;
        break;
      case 4:
        e.n = n;
        break;
      case 5:
        e.def_value = n;
        break;
      case 6:
        e.atk_value = n;
        break;
      case 7:
        e.value = n;
        break;
      case 8:
        e.range = int(n);
        break;
      case 9:
        e.square = rule_truth(o.value[i]);
        break;
      case 10:
        e.special = specials(o.value[i]);
        break;
      case 11:
        e.exp = n;
        e.exp_present = true;
        break;
      }
    }
  resolved_enemies->at(id) = std::move(e);
}
void Game::change_enemy(int id, int property, Value v) {
  if (overrides.use_count() != 1)
    overrides = std::make_shared<OverrideTable>(*overrides);
  auto &o = overrides->edit(id);
  o.present.set(property);
  o.value[property] = std::move(v);
  resolve_enemy(id);
  has_enemy_info = true;
  ++hazard_context;
  if (property == 10)
    ++hazard_roles_epoch;
}
Number Game::damage(Block b) const {
  return damage_for(b, hero);
}
Number Game::damage_for(Block b, const Hero &combat_hero) const {
  const auto &e = enemy(b);
  require(e.kind == Kind::enemy, "Not an enemy");
  Number attack = combat_hero[atk];
  if (combat_hero[cross] && (e.features & EntityFeature::cross_enemy))
    attack *= 2;
  if (combat_hero[knife] && (e.features & EntityFeature::dragon))
    attack *= 2;
  auto hit = attack - e.def;
  if (hit <= 0)
    return infinite_damage;
  Number per = std::max(Number(0), e.atk - combat_hero[defense]);
  auto has = [&](int n) { return (e.special & (1ULL << n)) != 0; };
  if (has(2))
    per = e.atk;
  if (has(4))
    per *= 2;
  if (has(5))
    per *= 3;
  if (has(6))
    per *= e.n;
  Number initial = has(1) ? per : 0;
  if (has(7))
    initial += e.def_value * combat_hero[defense];
  if (has(9))
    initial += e.n * combat_hero[mdef];
  if (per == 0 && initial == 0 && (!has(8) || e.atk_value == 0))
    return 0;
  Number turns = (e.hp + hit - 1) / hit;
  return initial + (turns - 1) * per + turns * (has(8) ? e.atk_value * attack : 0);
}
const Hazard &Game::build_hazard(int f) const {
  ensure_geometry(f);
  constexpr auto relevant = (1ULL << 15) | (1ULL << 16) | (1ULL << 18) | (1ULL << 24) | (1ULL << 27);
  if (floors[f]->hazard_roles_epoch != hazard_roles_epoch) {
    auto &fs = const_cast<Game *>(this)->edit_floor(f);
    fs.hazard_order = {};
    fs.hazard_cells.reset();
    for (auto p : fs.order)
      if (fs.enemy_cells[p.index()] && (enemy(block(f, p)).special & relevant)) {
        fs.hazard_order.push_back(p);
        fs.hazard_cells.set(p.index());
      }
    fs.hazard_roles_epoch = hazard_roles_epoch;
  }
  const auto &fs = *floors[f];
  auto *out = &hazards.reset(f);
  out->revision = fs.hazard_revision;
  out->context = hazard_context;
  const auto &ids = world->ids;
  bool immune = flag_truth(ids.magic_immune), zone = !flag_truth(ids.no_zone),
       repulse = !flag_truth(ids.no_repulse), laser = !flag_truth(ids.no_laser),
       between = !flag_truth(ids.no_between), ambush = !flag_truth(ids.no_ambush) && !immune;
  static constexpr std::array<Pos, 8> around = {Pos{0, -1},  Pos{-1, 0}, Pos{0, 1},  Pos{1, 0},
                                                Pos{-1, -1}, Pos{-1, 1}, Pos{1, -1}, Pos{1, 1}};
  // Iterate retained order because the first pincer splits preceding and
  // following damage. Visit only the cells in each monster's actual stencil.
  for (auto q : fs.hazard_order) {
    auto b = block(f, q);
    auto special = enemy(b).special;
    if (!(special & relevant))
      continue;
    out->plain = false;
    const auto &e = enemy(b);
    if (special & ((1ULL << 18) | (1ULL << 27)))
      out->geometry_sensitive = true;
    auto add_damage = [&](Pos p) {
      int i = p.index();
      (out->pincer[i] ? out->after[i] : out->before[i]) += e.value;
    };
    if (!immune) {
      if ((special & (1ULL << 15)) && zone) {
        int range = e.range ? e.range : 1;
        for (int y = std::max(0, q.y - range); y <= std::min(side - 1, q.y + range); ++y) {
          int width = e.square ? range : range - std::abs(y - q.y);
          for (int x = std::max(0, q.x - width); x <= std::min(side - 1, q.x + width); ++x)
            if (x != q.x || y != q.y)
              add_damage({x, y});
        }
      }
      if ((special & (1ULL << 18)) && repulse)
        for (int d = 0; d < (e.square ? 8 : 4); ++d) {
          auto p = add(q, around[d]);
          if (p.valid())
            add_damage(p);
        }
      if ((special & (1ULL << 24)) && laser) {
        for (int x = 0; x < side; ++x)
          if (x != q.x)
            add_damage({x, q.y});
        for (int y = 0; y < side; ++y)
          if (y != q.y)
            add_damage({q.x, y});
      }
      if ((special & (1ULL << 16)) && between)
        for (auto d : {Pos{1, 0}, Pos{0, 1}}) {
          auto p = add(q, d), other = add(p, d);
          if (other.valid() && block(f, other).entity == b.entity)
            out->pincer.set(p.index());
        }
    }
    if (special & ((1ULL << 18) | (1ULL << 27)))
      for (int d = 0; d < (e.square ? 8 : 4); ++d) {
        auto p = add(q, around[d]);
        if (!p.valid())
          continue;
        if ((special & (1ULL << 27)) && ambush && (d >= 4 || can_move(f, q, p)))
          out->reactions.set(p.index());
        if ((special & (1ULL << 18)) && repulse) {
          Pos target{q.x - around[d].x, q.y - around[d].y};
          if (target.valid() && !block(f, target).entity && !event_enabled(f, target) &&
              (d >= 4 || can_move(f, q, target)))
            out->reactions.set(p.index());
        }
      }
  }
  if (!out->plain)
    for (int i = 0; i < cells; ++i)
      if (out->before[i] + out->after[i] > 0)
        out->positive.set(i);
  const_cast<Game *>(this)->hazard_builds++;
  return *out;
}
Number Game::map_damage(int f, Pos p) const {
  require(p.valid(), "Bad hazard position");
  const auto &h = hazard(f);
  Number result = h.before[p.index()];
  if (h.pincer[p.index()] && hero[hp] - result > 1)
    result += (hero[hp] - result) / 2;
  return result + h.after[p.index()];
}
void Game::invalidate_hazard(int f, Block before, Block after) {
  constexpr auto relevant = (1ULL << 15) | (1ULL << 16) | (1ULL << 18) | (1ULL << 24) | (1ULL << 27);
  bool changed = hazards[f] && hazards[f]->geometry_sensitive;
  for (auto b : {before, after})
    if (world->entities[b.entity].kind == Kind::enemy) {
      auto special = enemy(b).special;
      changed |= bool(special & relevant);
    }
  if (changed)
    ++edit_floor(f).hazard_revision;
}
void Game::mark_geometry(int f, Pos p) {
  auto &fs = edit_floor(f);
  fs.dirty_cells.set(p.index());
  geometry_pending |= std::uint64_t(1) << f;
  auto_dirty |= world->cell_dependents[f][p.index()];
}
void Game::ensure_geometry(int f) const {
  auto bit = std::uint64_t(1) << f;
  if (!(geometry_pending & bit))
    return;
  auto &self = *const_cast<Game *>(this);
  auto &fs = self.edit_floor(f);
  auto pending = fs.dirty_cells;
  fs.dirty_cells.reset();
  geometry_pending &= ~bit;
  ++geometry_flushes;
  geometry_cells += pending.count();
  for (; pending.any();)
    self.refresh_cell(f, cell_positions[pending.pop()]);
}
void Game::refresh_cell(int f, Pos p) {
  auto &fs = *floors[f];
  const int i = p.index();
  const auto &fd = world->floors[f];
  auto b = block(f, p);
  auto in = fd.in[i], out = fd.out[i];
  const auto &entity = world->entities[b.entity].props;
  in |= entity.in;
  out |= entity.out;
  for (int tile : {fs.background[i], fs.foreground[i]})
    if (tile) {
      auto found = world->tiles.find(tile);
      if (found != world->tiles.end()) {
        const auto &layer = world->entities[found->second.entity].props;
        in |= layer.in;
        out |= layer.out;
      }
    }
  bool edges_changed = in != fs.movement_in[i] || out != fs.movement_out[i] || fs.revision == 1;
  fs.movement_restrictions -= bool(fs.movement_in[i] | fs.movement_out[i]);
  fs.movement_in[i] = in;
  fs.movement_out[i] = out;
  fs.movement_restrictions += bool(in | out);
  fs.restricted.set(i, in | out);
  auto update = [&](Pos from) {
    if (!from.valid())
      return;
    for (int d = 0; d < 4; ++d) {
      auto to = add(from, directions[d]);
      bool ok = to.valid() && !(fs.movement_out[from.index()] & (1 << d)) &&
                !(fs.movement_in[to.index()] & (1 << (d ^ 1)));
      fs.edges[d].set(from.index(), ok);
    }
  };
  if (edges_changed) {
    update(p);
    for (auto d : directions)
      update(add(p, d));
  }
  bool pass = !event_enabled(f, p), terminal = pass;
  const auto &v = props(f, p);
  if (present(f, p)) {
    if (b.entity && (v.no_pass || v.script || v.event_truth))
      pass = false;
    if (v.trigger != Trigger::none) {
      terminal = false;
      if (v.trigger != Trigger::change_floor || !v.change || !world->instructions[v.change].keep)
        pass = false;
    }
  }
  fs.transit.set(i, pass);
  fs.terminal.set(i, pass && terminal);
  fs.fire.set(i, fs.foreground[i] == 103);
  fs.stairs.set(i, world->entities[b.entity].features & EntityFeature::stair);
  fs.change_cells.set(i, present(f, p) && fd.change[i] != 0);
  const auto &actor = world->entities[b.entity];
  fs.empty_cells.set(i, b.entity == 0);
  fs.item_cells.set(i, actor.kind == Kind::item);
  bool enemy = actor.kind == Kind::enemy;
  constexpr auto hazard_specials = (1ULL << 15) | (1ULL << 16) | (1ULL << 18) | (1ULL << 24) | (1ULL << 27);
  bool hazard_source = enemy && (this->enemy(b).special & hazard_specials);
  if (hazard_source != fs.hazard_cells[i]) {
    if (!hazard_source)
      fs.hazard_order.erase(p);
    else {
      int next = fs.order.next_index(p);
      while (next < cells && !fs.hazard_cells[next])
        next = fs.order.next_index({next % side, next / side});
      fs.hazard_order.insert_before_index(next, p);
    }
  }
  fs.hazard_cells.set(i, hazard_source);
  fs.enemy_cells.set(i, enemy);
  fs.potion_cells.set(i, actor.features & EntityFeature::potion);
  fs.fake_wall_cells.set(i, actor.features & EntityFeature::fake_wall);
  fs.lava_cells.set(i, actor.features & EntityFeature::lava);
  auto rule = SweepRule::blocked;
  if (!fs.fire[i]) {
    if (!present(f, p))
      rule = SweepRule::empty;
    else if (fd.change[i])
      rule = SweepRule::conditional;
    else if (!event_enabled(f, p)) {
      if (!b.entity)
        rule = SweepRule::empty;
      else if (actor.features & EntityFeature::fake_wall)
        rule = SweepRule::wall;
      else if (actor.features & EntityFeature::lava)
        rule = SweepRule::lava;
      else if (actor.kind == Kind::item)
        rule = actor.features & EntityFeature::potion ? SweepRule::potion : SweepRule::item;
      else if (actor.kind == Kind::enemy && !fd.auto_battle_blocked[i])
        rule = SweepRule::enemy;
    }
  }
  fs.sweep_rule[i] = rule;
  auto enter = EnterRule::generic;
  if (!present(f, p))
    enter = EnterRule::empty;
  else if (!v.touch && !event_enabled(f, p) && v.trigger != Trigger::action &&
           v.trigger != Trigger::change_floor) {
    if (actor.kind == Kind::enemy)
      enter = EnterRule::enemy;
    else if (actor.kind == Kind::item && !v.no_pass)
      enter = EnterRule::item;
    else if (actor.kind == Kind::door && v.door && v.no_pass && v.trigger == Trigger::door &&
             std::all_of(v.keys.begin(), v.keys.end(), [](const auto &key) { return key.item >= 0; }))
      enter = EnterRule::door;
  }
  fs.enter_rule[i] = enter;
  fs.sweep_candidates.set(i, rule != SweepRule::blocked);
  fs.sweep_empty.set(i, rule == SweepRule::empty);
  bool regular = !entity.in && !entity.out && b.tile != 121 && b.tile != 122;
  if (rule == SweepRule::item || rule == SweepRule::potion) {
    bool additive = actor.id == "redGem" || actor.id == "blueGem";
    regular &=
        (!(actor.features & EntityFeature::combat_item) || additive) && actor.inventory_stat != coin &&
        world->programs[fd.after_item[i]].code.empty() &&
        (actor.item_class != 0 || (actor.effect.supported && actor.effect.flag < 0 && !actor.effect.immune));
    for (int n = 0; n < actor.effect.count; ++n)
      regular &= actor.effect.amount[n] >= 0;
  } else if (rule == SweepRule::enemy)
    regular &= world->programs[fd.after_battle[i]].code.empty();
  else
    regular &= rule == SweepRule::wall || rule == SweepRule::lava;
  fs.batch_regular.set(i, regular);
  fs.batch_conditional.set(i, rule == SweepRule::conditional && b.tile == 0);
  fs.walk_regions.update(i, pass);
  fs.empty_regions.update(i, b.entity == 0);
}
bool Game::can_move(int f, Pos from, Pos to) const {
  if (!from.valid() || !to.valid() || distance(from, to) != 1)
    return false;
  ensure_geometry(f);
  int d = to.y < from.y ? 0 : to.y > from.y ? 1 : to.x < from.x ? 2 : 3;
  return floors[f]->edges[d][from.index()];
}
bool Game::direct_enabled() const {
  const auto &i = world->ids;
  return hero[hp] > 0 && allow_direct && !flag_truth(i.cannot_direct) &&
         !(flag_truth(i.feature) && flag_number(i.energy) > 0) && !world->floors[hero.floor].no_direct;
}
bool Game::can_connect(int f, Pos from, Pos to, bool direct) {
  if (f < world->first_floor || f >= floors_count || !from.valid() || !to.valid())
    return false;
  ensure_geometry(f);
  if (from == to)
    return true;
  const auto &h = hazard(f);
  if (map_damage(f, from) > 0 || h.reactions[from.index()])
    return false;
  if ((block(f, from).entity || event_enabled(f, from)) && props(f, from).trigger != Trigger::change_floor)
    return false;
  const auto &fs = *floors[f];
  if (!fs.terminal[to.index()])
    return false;
  auto blocked = h.positive | h.reactions;
  for (auto remaining = h.pincer; remaining.any();) {
    int i = remaining.pop();
    blocked.set(i, map_damage(f, {i % side, i / side}) > 0 || h.reactions[i]);
  }
  if (direct)
    blocked |= fs.fire;
  if (blocked[to.index()])
    return false;
  auto allowed = fs.transit & ~blocked;
  if ((fs.restricted & fs.transit).none() && (blocked & fs.transit).none()) {
    ++component_queries;
    auto connected = [&](const auto &regions) {
      int target = regions.root(to.index());
      if (regions.active[from.index()])
        return regions.root(from.index()) == target;
      for (auto d : directions) {
        auto p = add(from, d);
        if (p.valid() && can_move(f, from, p) && regions.active[p.index()] &&
            regions.root(p.index()) == target)
          return true;
      }
      return false;
    };
    if (fs.transit == fs.empty_cells) {
      if (fs.empty_regions.dirty)
        edit_floor(f).empty_regions.rebuild();
      return connected(floors[f]->empty_regions);
    }
    if (fs.walk_regions.dirty)
      edit_floor(f).walk_regions.rebuild();
    return connected(floors[f]->walk_regions);
  }
  Number health = h.pincer.any() ? hero[hp] : 0;
  const auto *known = reach[f * 2 + (direct ? 1 : 0)];
  if (known && known->revision == fs.revision && known->context == hazard_context && known->hp == health &&
      (known->from == from.index() || (known->component && known->reached[from.index()])))
    return known->reached[to.index()];
  ++bfs_count;
  CellMask seen, frontier;
  seen.set(from.index());
  frontier = seen;
  while (frontier.any()) {
    auto next = ((frontier & fs.edges[0]) >> side) | ((frontier & fs.edges[1]) << side) |
                ((frontier & fs.edges[2]) >> 1) | ((frontier & fs.edges[3]) << 1);
    frontier = next & allowed & ~seen;
    seen |= frontier;
  }
  ReachCache storage;
  auto *cache = &storage;
  cache->revision = fs.revision;
  cache->context = hazard_context;
  cache->hp = health;
  cache->from = from.index();
  cache->component = fs.movement_restrictions == 0 && allowed[from.index()];
  cache->reached = seen;
  reach.set(f * 2 + (direct ? 1 : 0), std::move(storage));
  return seen[to.index()];
}
bool Game::can_delete(int f, Pos p) const {
  const auto &i = world->ids;
  if (!flag_truth(i.feature) || flag_number(i.energy) <= 0)
    return false;
  auto b = block(f, p);
  if (!b.entity)
    return false;
  const auto &e = world->entities[b.entity];
  const auto &v = props(f, p);
  if (e.kind == Kind::enemy)
    return damage(b) >= infinite_damage || damage(b) + map_damage(f, p) >= hero[hp];
  if (!event_enabled(f, p) && v.trigger == Trigger::none)
    return true;
  for (auto key : v.keys)
    if (key.item < 0 || item_count(key.item) < key.count)
      return true;
  return false;
}
void Game::successful_move(int f, Pos from, Pos to) {
  hero.pos = to;
  facing = to.y < from.y   ? Direction::up
           : to.y > from.y ? Direction::down
           : to.x < from.x ? Direction::left
                           : Direction::right;
  int event = props(f, from).out_event;
  if (event)
    start(event, f, from);
  set_number_flag(world->ids.energy, flag_number(world->ids.energy) - 1);
  auto d = map_damage(f, to);
  if (d > 0) {
    hero[hp] -= d;
    hero[dmg] += d;
    require(hero[hp] > 0, "Map damage death");
  }
}
void Game::tile_event(int f, Pos p) {
  if (floors[f]->foreground[p.index()] != 103)
    return;
  layer(f, p, 0, true);
  for (auto d : auto_dirs) {
    auto q = add(p, d), r = add(q, d);
    if (q.x < 1 || q.y < 1 || q.x > 11 || q.y > 11 || r.x < 1 || r.y < 1 || r.x > 11 || r.y > 11)
      continue;
    auto b = block(f, q);
    if (world->entities[b.entity].id != "redWizard")
      continue;
    if (!present(f, r, true)) {
      remove(f, q);
      replace(f, r, b);
      if (floors[f]->foreground[r.index()] == 103) {
        auto beyond = add(r, d);
        if (!retained_props(cell(f, beyond)).has_data)
          layer(f, beyond, 103, true);
        layer(f, q, 103, true);
      }
    }
    layer(f, r, 0, true);
  }
}
void Game::step(Direction d, bool strict) {
  if (step_transaction(d, strict))
    return;
  facing = d;
  int f = hero.floor;
  Pos from = hero.pos, to = add(from, delta(d));
  require(hero[hp] > 0, "Cannot move at zero HP");
  require(to.x >= 1 && to.y >= 1 && to.x <= 11 && to.y <= 11 && can_move(f, from, to), "Movement blocked");
  auto b = block(f, to);
  const auto &e = world->entities[b.entity];
  const auto &v = props(f, to);
  auto serial = event_serial;
  bool blocked = (b.entity || event_enabled(f, to)) && v.no_pass;
  if (e.kind == Kind::enemy)
    blocked = damage(b) >= infinite_damage || damage(b) + map_damage(f, to) >= hero[hp];
  require(map_damage(f, to) < std::max(Number(1), hero[hp]), "Lethal hazard");
  if (!blocked || can_delete(f, to))
    if (hazard(f).reactions[to.index()])
      throw Unsupported("Directional repulse/ambush");
  if (blocked) {
    if (can_delete(f, to)) {
      set_number_flag(world->ids.energy, Number(0));
      remove(f, to);
      erase_event(f, to);
      successful_move(f, from, to);
      tile_event(f, to);
      auto_sweep(f);
      return;
    }
    if (e.kind == Kind::enemy)
      throw Error("Cannot win battle");
    if (trigger(f, to))
      return;
    if (e.kind == Kind::door || e.id == "fakeWall" || e.id == "fakeWall2") {
      require(v.door, "Block has no door interaction");
      for (auto k : v.keys)
        if (item_count(k.item) < k.count) {
          if (!strict)
            return;
          throw Error("Missing door key");
        }
      auto keys = v.keys;
      for (auto k : keys)
        if (!k.preserve)
          set_item(k.item, item_count(k.item) - k.count);
      if (e.id == "yellowDoor")
        ++hero[yd];
      if (e.id == "blueDoor")
        ++hero[bd];
      if (e.id == "redDoor")
        ++hero[rd];
      remove(f, to);
      after(2, f, to);
      return;
    }
    if (!strict && e.kind == Kind::wall && v.trigger == Trigger::none)
      return;
    throw Error("Obstacle cannot be entered");
  }
  successful_move(f, from, to);
  bool started = trigger(f, to);
  if (!started && b.entity) {
    if (e.kind == Kind::item || e.kind == Kind::enemy)
      apply_block(f, to, b);
    else if (e.id != "none" && e.id != "airwall" && e.id != "fire" && e.id != "flower")
      require(e.props.trigger == Trigger::none, "Unsupported terrain trigger");
  }
  if (hero.floor == f && hero.pos == to)
    tile_event(f, to);
  if (serial == event_serial)
    auto_sweep(hero.floor);
}
void Game::apply_item(int f, int id) {
  const auto &e = world->entities[id];
  if (!flag_value(world->ids.item_hint))
    set_flag(world->ids.item_hint, Opaque{-1, false});
  if (e.item_class != 0) {
    set_item(e.item_index, item_count(e.item_index) + 1, true);
    return;
  }
  const auto &effect = e.effect;
  require(effect.supported, "Unknown compiled item effect");
  Number ratio = effect.ratio ? world->floors[f].ratio : 1;
  for (int i = 0; i < effect.count; ++i)
    hero.values[effect.stat[i]] += effect.amount[i] * ratio;
  if (effect.flag >= 0)
    set_flag(effect.flag, e.id);
  if (effect.immune) {
    hero[magicImmune] = 1;
    set_flag(world->ids.magic_immune, true);
  }
}
void Game::fight(int f, Pos p, Block b, Number d) {
  const auto &e = enemy(b);
  require(d < infinite_damage && hero[hp] > d, "Battle death");
  if (flag_number(world->ids.addhp) == 1)
    d = -d;
  hero[hp] -= d;
  hero[dmg] += d;
  if (!flag_truth(world->ids.curse))
    hero[gold] += e.money * (hero[coin] ? 2 : 1);
  set_number_flag(world->ids.energy, Number(0));
  set_number_flag(world->ids.hatred, flag_number(world->ids.hatred) + world->hatred);
  has_exp = true;
  if (!e.exp_present || exp_null)
    exp_null = true;
  else
    exp += e.exp;

  remove(f, p);
  after(0, f, p);
  record_native_battle(f,p,b.entity);
}
void Game::apply_block(int f, Pos p, Block b) {
  const auto &e = enemy(b);
  if (e.kind == Kind::enemy) {
    fight(f, p, b, damage(b));
    return;
  } else if (e.kind == Kind::item)
    apply_item(f, b.entity);
  else if (e.kind != Kind::terrain)
    throw Unsupported("Non-native block interaction");
  if (e.kind != Kind::terrain || world->entities[b.entity].id == "fakeWall" ||
      world->entities[b.entity].id == "fakeWall2")
    remove(f, p);
  if (e.kind == Kind::enemy)
    after(0, f, p);
  else if (e.kind == Kind::item)
    after(1, f, p);
}
void Game::auto_sweep(int f) {
  if (f < 0)
    f = hero.floor;
  if (f != hero.floor || (!automatic.battle && !automatic.wall && !automatic.item) || !direct_enabled() ||
      choice || !frames.empty())
    return;
  ensure_geometry(f);
  auto here = block(f, hero.pos);
  if (here.entity && world->entities[here.entity].kind != Kind::item)
    return;
  if (world->floors[f].source_event[hero.pos.index()] || map_damage(f, hero.pos) > 0 ||
      hazard(f).reactions[hero.pos.index()])
    return;
  const bool pincer_dependent = hazard(f).pincer.any();
  auto eligibility = [&] {
    return std::array<Number, 7>{hero[atk],
                                 hero[defense],
                                 hero[mdef],
                                 hero[cross],
                                 hero[knife],
                                 hero[snow],
                                 pincer_dependent ? hero[hp] : 0};
  };
  const auto stats = eligibility();
  const auto revision = floors[f]->revision;
  const auto hazard_revision = floors[f]->hazard_revision;
  const auto context = hazard_context;
  const bool ignore = flag_truth(world->ids.ignore_floor);
  const auto &known = settled[f];
  bool matches = known && known->revision == revision && known->context == context && known->stats == stats &&
                 known->options == automatic && known->ignore == ignore &&
                 known->blocked_items == blocked_items[f];
  if (matches && known->settled[hero.pos.index()]) {
    if (!auto_active)
      drain();
    return;
  }
  // On a floor whose effects commute, solve the reachable component closure
  // once. The same proof handles both an empty frontier and bulk collection;
  // it avoids a separate candidate scan followed by the same traversal again.
  if (floors[f]->empty_regions.dirty)
    edit_floor(f).empty_regions.rebuild();
  if (use_closure_first && use_batches) {
    CellMask reached;
    if (collect_batch(f, reached)) {
      if (floors[f]->revision == revision)
        ++auto_no_work;
      if (!here.entity) {
        SettledCache cache;
        if (matches)
          cache = *known;
        cache.revision = floors[f]->revision;
        cache.context = hazard_context;
        cache.stats = eligibility();
        cache.options = automatic;
        cache.ignore = ignore;
        cache.blocked_items = blocked_items[f];
        cache.settled |= reached;
        settled.set(f, std::move(cache));
      }
      if (!auto_active)
        drain();
      return;
    }
  }
  // Prove that a collection sweep has no work using an overapproximation of
  // empty reachability. Ignoring events, fire and hazards can add candidates,
  // never hide one. When there is a candidate, retain the exact ordered BFS.
  static constexpr auto horizontal = [] {
    std::array<CellMask, 2> masks;
    for (int i = 0; i < cells; ++i) {
      if (i % side)
        masks[0].set(i);
      if (i % side + 1 < side)
        masks[1].set(i);
    }
    return masks;
  }();
  auto adjacent = [&](CellMask mask) {
    return (mask >> side) | (mask << side) | ((mask & horizontal[0]) >> 1) | ((mask & horizontal[1]) << 1);
  };
  if (floors[f]->empty_regions.dirty)
    edit_floor(f).empty_regions.rebuild();
  const auto &floor_state = *floors[f];
  CellMask empty_region = floor_state.empty_regions.region(hero.pos.index());
  if (empty_region.none()) {
    empty_region.set(hero.pos.index());
    for (auto d : directions) {
      auto p = add(hero.pos, d);
      if (p.valid())
        empty_region |= floor_state.empty_regions.region(p.index());
    }
  }
  const auto boundary = adjacent(empty_region) & ~empty_region;
  CellMask candidate;
  if (automatic.item)
    candidate |=
        (automatic.item == 2 ? floor_state.item_cells & ~floor_state.potion_cells : floor_state.item_cells) &
        ~blocked_items[f];
  if (automatic.wall) {
    candidate |= floor_state.fake_wall_cells;
    if (hero[snow])
      candidate |= floor_state.lava_cells;
  }
  constexpr std::uint64_t excluded = (1ULL << 12) | (1ULL << 13) | (1ULL << 14) | (1ULL << 19) |
                                     (1ULL << 21) | (1ULL << 26) | (1ULL << 27) | (1ULL << 28) | (1ULL << 29);
  auto possible = [&](CellMask edge) {
    auto found = candidate & edge;
    if (automatic.battle && found.count() < 2) {
      auto enemies = edge & floor_state.enemy_cells & ~world->floors[f].auto_battle_blocked;
      while (enemies.any()) {
        int i = enemies.pop();
        auto b = block(f, cell_positions[i]);
        if (!(enemy(b).special & excluded) && damage(b) == 0) {
          found.set(i);
          if (found.count() > 1)
            break;
        }
      }
    }
    return found;
  };
  const auto possible_actions = possible(boundary);
  bool work = possible_actions.any();
  if (!work) {
    ++auto_no_work;
    if (!here.entity) {
      SettledCache storage;
      auto *cache = &storage;
      if (matches)
        *cache = *known;
      cache->revision = revision;
      cache->context = context;
      cache->stats = stats;
      cache->options = automatic;
      cache->ignore = ignore;
      cache->blocked_items = blocked_items[f];
      cache->settled |= empty_region;
      settled.set(f, std::move(storage));
    }
    if (!auto_active)
      drain();
    return;
  }

  // An empty, unobstructed region with exactly one possible interaction has
  // no ordering ambiguity. Also prove that opening that cell cannot reveal
  // another interaction, and exclude items that change combat eligibility.
  // The action and its event scheduling still execute through normal rules.
  if (!here.entity && possible_actions.count() == 1) {
    const auto &h = hazard(f);
    auto barriers = ~floor_state.transit | floor_state.fire | floor_state.change_cells | h.positive |
                    h.reactions | h.pincer;
    int target = possible_actions.first();
    auto p = cell_positions[target];
    auto b = block(f, p);
    const auto &e = world->entities[b.entity];
    bool action_ok = e.kind == Kind::enemy ||
                     (e.kind == Kind::item && !(e.features & EntityFeature::combat_item)) ||
                     e.kind == Kind::terrain || (e.features & EntityFeature::lava);
    if (action_ok && (empty_region & barriers).none() && !floor_state.fire[target] &&
        !floor_state.change_cells[target] && !event_enabled(f, p) && !h.positive[target] &&
        !h.reactions[target] && !h.pincer[target]) {
      auto expanded = empty_region;
      expanded.set(target);
      for (auto next : auto_neighbors[target])
        if (next != 255)
          expanded |= floor_state.empty_regions.region(next);
      if (possible(adjacent(expanded) & ~expanded).none()) {
        ++auto_single;
        bool previous = auto_active;
        auto_active = true;
        try {
          if (e.kind == Kind::enemy)
            apply_block(f, p, b);
          else if (e.kind == Kind::item) {
            apply_item(f, b.entity);
            remove(f, p);
            after(1, f, p);
          } else
            remove(f, p);
          auto_active = previous;
        } catch (...) {
          auto_active = previous;
          throw;
        }
        if (!auto_active)
          drain();
        return;
      }
    }
  }

  bool previous = auto_active;
  auto_active = true;
  ++auto_bfs_count;
  try {
    batch_exhausted = false;
    auto reached = sweep_ordered(f);
    // All effects in this phase remove cells. Their scripted continuations
    // wait for drain(), so a sweep with unchanged eligibility and hazards
    // has exhausted its reachable region even when it collected objects.
    if (!here.entity && floors[f]->hazard_revision == hazard_revision && hazard_context == context &&
        (eligibility() == stats || batch_exhausted)) {
      SettledCache storage;
      auto *cache = &storage;
      if (matches)
        *cache = *known;
      cache->revision = floors[f]->revision;
      cache->context = context;
      cache->stats = eligibility();
      cache->options = automatic;
      cache->ignore = ignore;
      cache->blocked_items = blocked_items[f];
      cache->settled |= reached;
      settled.set(f, std::move(storage));
    }
    auto_active = previous;
  } catch (...) {
    auto_active = previous;
    throw;
  }
  if (!auto_active)
    drain();
}
bool Game::has_visited(int f) const {
  if (f < world->first_floor || f >= floors_count)
    return false;
  if (world->floors[f].hidden)
    return visited[f];
  for (int i = f; i < floors_count; ++i)
    if (visited[i])
      return true;
  return false;
}
void Game::transition(int f, Pos p, bool flying) {
  require(f >= world->first_floor && f < floors_count, "Unknown destination");
  if (!p.valid())
    p = f > hero.floor ? world->floors[f].down : world->floors[f].up;
  if (!p.valid() && f == 44)
    p = {1, 1};
  require(p.valid(), "Missing landing");
  bool first = !has_visited(f);
  if (!(flying && f == hero.floor)) {
    leave[hero.floor] = Leave{hero.pos, facing};
    has_leave = true;
  }
  auto_dirty |= world->auto_relative;
  hero.floor = f;
  hero.pos = p;
  if (first) {
    visited.set(f);
    visited_flags.set(f);
    has_visited_record = true;
  }
  set_number_flag(world->ids.energy, Number(0));
  arrive(f, first);
}
bool Game::can_fly(int f) {
  if (f < world->first_floor || f >= floors_count || !has_visited(f))
    return false;
  int from = hero.floor;
  ensure_geometry(from);
  if (!world->floors[from].fly_from || !world->floors[f].fly_to || item_count(world->item_ids.at("fly")) <= 0)
    return false;
  bool near = false;
  for (auto stairs = floors[from]->stairs; stairs.any();) {
    int i = stairs.pop();
    if (distance(hero.pos, {i % side, i / side}) <= 1) {
      near = true;
      break;
    }
  }
  if (!near && world->near_stair) {
    if (!flag_truth(world->ids.extra_functions) || !direct_enabled())
      return false;
    bool accessible = false;
    for (auto stairs = floors[from]->stairs; stairs.any() && !accessible;) {
      int i = stairs.pop();
      for (auto d : auto_dirs)
        if (can_connect(from, hero.pos, add({i % side, i / side}, d), true)) {
          accessible = true;
          break;
        }
    }
    if (!accessible)
      return false;
  }
  if (flag_truth(world->ids.fly) || world->floors[f].hidden)
    return true;
  if (f == from)
    return direct_enabled() && can_connect(from, hero.pos, world->floors[f].down, true);
  int step = f > from ? 1 : -1;
  for (int n = f - step; n != from; n -= step) {
    if (world->floors[n].hidden)
      return true;
    if (!can_connect(n, world->floors[n].up, world->floors[n].down))
      return false;
  }
  return from == 1 || from == 45 ||
         can_connect(from, hero.pos, step > 0 ? world->floors[from].up : world->floors[from].down);
}
void Game::fly(int f) {
  require(can_fly(f), "Illegal flight");
  auto p = f >= hero.floor ? world->floors[f].down : world->floors[f].up;
  require(p.valid(), "Missing fly landing");
  transition(f, p, true);
}
} // namespace engine
