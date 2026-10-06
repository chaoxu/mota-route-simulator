// Goal-level route representation: interactions are kept, walking and ordinary
// door openings are re-planned at execution time with gold-aware costs.
#define main native_optimizer_cli_main
#include "../engine/search/optimize.cpp"
#undef main
#include <deque>
#include <map>
#include <numeric>
#include <queue>
#include <random>

namespace goal {
// Scope to one interpreter realization root. All checked_token calls, including
// nested repairs and discarded speculative branches, contribute actual paths.
// Example:
//   ScopedNativeBudget budget(calls, max_calls, deadline);
//   goal::ScopedWinObserver wins(start, [&](Game win, Tokens local) { save(win, local); });
//   auto result = goal::realize(start, program, pc, costs, &states);
// The observer receives owned snapshots and root-relative inputs. It performs
// no simulation; append the interpreter's already verified prefix when saving.
// NativeBudgetStop and NativeObservationError propagate through realize().
using ScopedWinObserver = ScopedNativeWins;
constexpr std::array<const char *, 4> dir_names = {"up", "down", "left", "right"};
constexpr std::array<Pos, 4> dir_delta = {Pos{0, -1}, Pos{0, 1}, Pos{-1, 0}, Pos{1, 0}};
int dir_index(const std::string &t) {
  for (int d = 0; d < 4; ++d)
    if (t == dir_names[d]) return d;
  return -1;
}
const Entity &entity_at(const Game &g, int f, Pos p) { return g.world->entities[g.block(f, p).entity]; }

enum class Kind2 { Fly, Bump, Raw, RawAt, MoveTo, Collect, PrepareHp };
bool barrier_static(const Game &g, int f, Pos p) {
  const auto &e = entity_at(g, f, p);
  if (e.kind == Kind::wall || e.kind == Kind::door || e.kind == Kind::npc) return true;
  if (e.kind == Kind::terrain && (e.id == "airwall" || e.id == "blueShop-left" || e.id == "blueShop-right")) return true;
  return false;
}
std::array<std::array<int, cells>, floors_count> room_labels(const Game &initial) {
  std::array<std::array<int, cells>, floors_count> labels{};
  for (int f = initial.world->first_floor; f < floors_count; ++f) {
    auto &lab = labels[f];
    lab.fill(0);
    int n = 0;
    for (int i = 0; i < cells; ++i) {
      auto p = cell_positions[i];
      if (lab[i] || barrier_static(initial, f, p)) continue;
      ++n;
      std::vector<int> stack{i};
      lab[i] = n;
      while (!stack.empty()) {
        auto c = cell_positions[stack.back()];
        stack.pop_back();
        for (auto d : dir_delta) {
          auto q = add(c, d);
          if (!q.valid() || lab[q.index()] || barrier_static(initial, f, q)) continue;
          lab[q.index()] = n;
          stack.push_back(q.index());
        }
      }
    }
  }
  return labels;
}
struct Step {
  Kind2 kind;
  int floor = 0;
  Pos approach;   // hero position required before the input (Bump, RawAt)
  int dir = -1;   // Bump direction
  std::string token;
  std::size_t origin = 0;
  std::string target; // entity id at the bumped cell in the source route
  bool floor_change = false;
};
struct FloorKeyCosts {
  std::optional<double> yellow, blue;
};
using FloorKeyCostTable = std::map<int, FloorKeyCosts>;
struct Costs {
  double yellow = 66, blue = 132, damage = 1.0, step = 0.01;
  bool allow_potions = false;
  bool repair_survival = false;
  bool adaptive_access = false; // optional native wall clearing and flight approaches
  double pretrap_damage = 1.0; // optional price before first visiting MT41
  std::bitset<64> no_transit_fights; // floors where walking never fights an enemy
  std::set<std::pair<int, int>> no_doors; // (floor, cell index) doors the walker never opens
  std::set<std::pair<int, int>> forbidden_transit; // (floor, cell index), including targets and exempt cells
  FloorKeyCostTable floor_key_costs; // optional per-color overrides of the global prices
};

FloorKeyCostTable floor_key_costs_from(const Json &table) {
  if (!table.is_object()) throw std::runtime_error("floor_key_costs must be an object");
  FloorKeyCostTable result;
  for (const auto &[id, prices] : table.items()) {
    int floor = -1;
    for (int f = 0; f < floors_count; ++f) if (id == floor_name(f)) { floor = f; break; }
    if (floor < 0) throw std::runtime_error("Unknown floor_key_costs floor: " + id);
    if (!prices.is_object()) throw std::runtime_error("floor_key_costs." + id + " must be an object");
    FloorKeyCosts entry;
    for (const auto &[color, value] : prices.items()) {
      if (color != "yellow" && color != "blue")
        throw std::runtime_error("Unknown floor_key_costs color: " + color);
      if (!value.is_number() || !std::isfinite(value.get<double>()) || value.get<double>() < 0)
        throw std::runtime_error("Invalid nonnegative floor_key_costs price: " + id + "." + color);
      (color == "yellow" ? entry.yellow : entry.blue) = value.get<double>();
    }
    result.emplace(floor, entry);
  }
  return result;
}
Json floor_key_costs_json(const Costs &c) {
  Json result = Json::object();
  for (const auto &[floor, prices] : c.floor_key_costs) {
    Json entry = Json::object();
    if (prices.yellow) entry["yellow"] = *prices.yellow;
    if (prices.blue) entry["blue"] = *prices.blue;
    result[floor_name(floor)] = std::move(entry);
  }
  return result;
}
std::pair<double, double> key_costs_at(const Costs &c, int floor) {
  const auto entry = c.floor_key_costs.find(floor);
  if (entry == c.floor_key_costs.end()) return {c.yellow, c.blue};
  return {entry->second.yellow.value_or(c.yellow), entry->second.blue.value_or(c.blue)};
}
Json with_floor_key_costs(Json metadata, const Costs &c) {
  if (!c.floor_key_costs.empty()) metadata["floor_key_costs"] = floor_key_costs_json(c);
  return metadata;
}

// Extract a goal-level route from a concrete token route and its prefix states.
std::vector<Step> extract(const Tokens &T, const std::vector<Game> &P, bool drop_doors) {
  std::vector<Step> steps;
  for (std::size_t i = 0; i < T.size(); ++i) {
    const auto &t = T[i];
    const auto &a = P[i], &b = P[i + 1];
    if (t.starts_with("fly:")) {
      steps.push_back({Kind2::Fly, a.hero.floor, a.hero.pos, -1, t, i, {}, true});
      continue;
    }
    const int d = dir_index(t);
    const bool floor_change = a.hero.floor != b.hero.floor;
    bool effect = floor_change || native_events_pending(b) || a.hero.values != b.hero.values ||
                  a.event_serial != b.event_serial;
    if (!effect)
      for (int f = a.world->first_floor; f < floors_count && !effect; ++f)
        if (a.floors[f] != b.floors[f])
          for (int k = 0; k < cells; ++k) {
            auto q = cell_positions[k];
            if (a.block(f, q) != b.block(f, q)) { effect = true; break; }
          }
    if (t.starts_with("move:")) {
      // Walking that triggered an automatic sweep keeps its destination.
      if (effect) steps.push_back({Kind2::MoveTo, a.hero.floor, b.hero.pos, -1, t, i, {}, floor_change});
      continue;
    }
    if (d >= 0) {
      const Pos cell = add(a.hero.pos, dir_delta[d]);
      if (!effect) continue; // pure walking
      const auto &e = cell.valid() ? entity_at(a, a.hero.floor, cell) : a.world->entities[0];
      const bool plain_door = e.kind == Kind::door && (e.id == "yellowDoor" || e.id == "blueDoor") &&
                              a.hero.pos == b.hero.pos && !floor_change && !native_events_pending(b);
      if (drop_doors && plain_door) continue;
      steps.push_back({Kind2::Bump, a.hero.floor, a.hero.pos, d, t, i, e.id, floor_change});
      continue;
    }
    if (t.starts_with("item:") || t.starts_with("key:") || t == "turn")
      steps.push_back({Kind2::RawAt, a.hero.floor, a.hero.pos, -1, t, i, {}, a.hero.floor != b.hero.floor});
    else
      steps.push_back({Kind2::Raw, a.hero.floor, a.hero.pos, -1, t, i, {}, a.hero.floor != b.hero.floor});
  }
  return steps;
}

// Heuristic path to `goal`: keep one resource label per cell and use the
// current stats for edge costs. Exact replay checks the resulting walk.
// Doors are priced by key value; potions are avoided unless allowed.
std::optional<std::vector<Pos>> plan(const Game &g, Pos goal, const Costs &c, Pos exempt = {-1, -1}) {
  const int f = g.hero.floor;
  const auto [yellow_cost, blue_cost] = key_costs_at(c, f);
  if (g.hero.pos == goal) return std::vector<Pos>{};
  std::array<double, cells> dist;
  dist.fill(1e18);
  std::array<int, cells> par;
  par.fill(-1);
  std::array<Number, cells> spent{};
  std::array<Number, cells> ykeys{}, bkeys{};
  using E = std::pair<double, int>;
  std::priority_queue<E, std::vector<E>, std::greater<E>> pq;
  const int s = g.hero.pos.index();
  dist[s] = 0;
  ykeys[s] = g.hero[yk];
  bkeys[s] = g.hero[bk];
  pq.push({0, s});
  while (!pq.empty()) {
    auto [w, i] = pq.top();
    pq.pop();
    if (w != dist[i]) continue;
    auto p = cell_positions[i];
    if (p == goal) break;
    for (auto dd : dir_delta) {
      auto q = add(p, dd);
      if (!q.valid() || q.x < 1 || q.y < 1 || q.x > 11 || q.y > 11 ||
          c.forbidden_transit.contains({f,q.index()}) || !g.can_move(f, p, q)) continue;
      const auto blk = g.block(f, q);
      const auto &e = g.world->entities[blk.entity];
      const auto &v = g.props(f, q);
      // MT40's revealed stairs use an action script, not changeFloor as their
      // trigger. They must still be excluded from same-floor transit paths.
      if (q != goal && (v.trigger == Trigger::change_floor || e.id == "upFloor" || e.id == "downFloor")) continue;
      double cost = c.step;
      Number damage = 0;
      Number yk2 = ykeys[i], bk2 = bkeys[i];
      if (blk.entity || g.event_enabled(f, q)) {
        if (c.adaptive_access && g.automatic.wall && !g.event_enabled(f, q) &&
            ((e.features & EntityFeature::fake_wall) || ((e.features & EntityFeature::lava) && g.hero[snow]))) {
          // A preceding native movement/opening can expose this cell to the
          // automatic collector. Execution must still clear it legally.
        } else if (e.kind == Kind::enemy) {
          if (q != goal && c.no_transit_fights[f]) continue;
          damage = g.damage(blk);
          if (damage >= infinite_damage) continue;
        } else if (e.kind == Kind::item) {
          if (!c.allow_potions && q != goal && q != exempt && (e.id == "redPotion" || e.id == "bluePotion" || e.id == "greenPotion" || e.id == "yellowPotion"))
            continue;
        } else if (e.kind == Kind::door) {
          if (c.no_doors.count({f, q.index()})) continue;
          if (e.id == "yellowDoor" && yk2 > 0) { cost += yellow_cost; --yk2; }
          else if (e.id == "blueDoor" && bk2 > 0) { cost += blue_cost; --bk2; }
          else continue;
        } else if (e.kind == Kind::terrain) {
          if (v.no_pass) continue;
        } else if (e.kind == Kind::empty) {
          if (v.no_pass) continue;
        } else
          continue;
      }
      damage += g.map_damage(f, q);
      if (spent[i] + damage >= g.hero[hp]) continue;
      cost += c.damage * (g.has_visited(41) ? 1.0 : c.pretrap_damage) * double(damage);
      int j = q.index();
      if (w + cost < dist[j]) {
        dist[j] = w + cost;
        par[j] = i;
        spent[j] = spent[i] + damage;
        ykeys[j] = yk2;
        bkeys[j] = bk2;
        pq.push({dist[j], j});
      }
    }
  }
  if (par[goal.index()] < 0) return std::nullopt;
  std::vector<Pos> path;
  for (int i = goal.index(); i != s; i = par[i]) path.push_back(cell_positions[i]);
  std::reverse(path.begin(), path.end());
  return path;
}

// Walk along a planned path; a door costs one input to open and one to enter.
void walk(Game &g, Pos goal, const Costs &c, Tokens &out, Pos exempt = {-1, -1}) {
  if (g.hero.pos == goal) return;
  auto path = plan(g, goal, c, exempt);
  if (!path) {
    const auto &e = entity_at(g, g.hero.floor, goal);
    throw Error("No planned path to " + std::to_string(goal.x) + "," + std::to_string(goal.y) + " (" + e.id + ") hp=" +
                std::to_string(g.hero[hp]) + " yk=" + std::to_string(g.hero[yk]) + " dmg=" + std::to_string(g.map_damage(g.hero.floor, goal)));
  }
  const int f = g.hero.floor;
  for (auto p : *path) {
    for (int attempt = 0; attempt < 3 && g.hero.pos != p; ++attempt) {
      if (g.hero.floor != f) throw Error("Walk left the floor");
      if (native_events_pending(g)) throw Error("Walk opened a menu");
      int dx = p.x - g.hero.pos.x, dy = p.y - g.hero.pos.y;
      if (std::abs(dx) + std::abs(dy) != 1) throw Error("Walk lost adjacency");
      std::string d = dx < 0 ? "left" : dx > 0 ? "right" : dy < 0 ? "up" : "down";
      checked_token(g, d, false);
      out.push_back(d);
    }
    if (g.hero.pos != p) {
      // An automatic sweep may relocate nothing; a blocked cell means failure.
      throw Error("Walk step failed");
    }
  }
}

struct Result {
  int floor = 0;
  Pos pos;
  bool win = false;
  Number hp = 0, gold = 0, shops = 0, keys = 0;
  std::string error;
  std::size_t fail_step = 0;
  Tokens tokens;
};
bool rescue_step(Game &, const Step &, const Costs &, Tokens &);
Result realize(const Game &start, const std::vector<Step> &steps, std::size_t from, const Costs &c,
               std::vector<Game> *states_out = nullptr, std::vector<Tokens> *tokens_out = nullptr) {
  Result r;
  Game g = start;
  std::size_t k = from;
  for (; k < steps.size(); ++k) {
    const auto &s = steps[k];
    if (states_out) states_out->push_back(g);
    try {
      auto scope = g.transaction();
      Tokens local;
      switch (s.kind) {
      case Kind2::Fly:
        try {
          auto probe = g.transaction();
          checked_token(g, s.token, false);
          probe.commit();
        } catch (const Unsupported &) {
          throw;
        } catch (const Error &) {
          // Flight legality depends on position: retry from the source position.
          if (!c.adaptive_access) {
            if (g.hero.floor != s.floor) throw;
            walk(g, s.approach, c, local);
            checked_token(g, s.token, false);
          } else {
            std::vector<Pos> approaches;
            if (g.hero.floor == s.floor) approaches.push_back(s.approach);
            const auto &fd = g.world->floors[g.hero.floor];
            approaches.push_back(fd.down); approaches.push_back(fd.up);
            std::optional<Game> best;
            Tokens best_tokens;
            double best_cost = 1e18;
            const auto [yellow_cost, blue_cost] = key_costs_at(c, g.hero.floor);
            for (auto p : approaches) {
              if (!p.valid()) continue;
              auto probe = g;
              Tokens route;
              try {
                walk(probe, p, c, route);
                checked_token(probe, s.token, false);
                const double price = c.damage * double(g.hero[hp] - probe.hero[hp]) +
                    yellow_cost * double(g.hero[yk] - probe.hero[yk]) + blue_cost * double(g.hero[bk] - probe.hero[bk]) + c.step * route.size();
                if (price < best_cost) { best = std::move(probe); best_tokens = std::move(route); best_cost = price; }
              } catch (const Unsupported &) { throw; }
              catch (const Error &) {}
            }
            if (!best) throw;
            g = std::move(*best);
            local = std::move(best_tokens);
          }
        }
        local.push_back(s.token);
        break;
      case Kind2::Raw:
        checked_token(g, s.token, false);
        local.push_back(s.token);
        break;
      case Kind2::MoveTo:
        if (g.hero.floor != s.floor) throw Error("Wrong floor for sweep walk");
        if (c.no_doors.count({s.floor, s.approach.index()})) break;
        walk(g, s.approach, c, local);
        break;
      case Kind2::Collect:
        // The original automatic collection may already have satisfied this
        // requirement. A changed purchase must earn any missing effect through
        // actual game inputs, never by restoring the reference state.
        if (entity_at(g, s.floor, s.approach).id != s.target) break;
        if (g.hero.floor != s.floor) throw Error("Wrong floor for collection requirement");
        walk(g, s.approach, c, local);
        if (entity_at(g, s.floor, s.approach).id == s.target)
          throw Error("Collection requirement was not satisfied");
        break;
      case Kind2::PrepareHp: {
        const Number minimum = std::stoll(s.token);
        for (int trip = 0; g.hero[hp] < minimum && trip < 16; ++trip) {
          const Step stay{Kind2::MoveTo, g.hero.floor, g.hero.pos, -1, "", s.origin, "", false};
          Tokens healing;
          if (!rescue_step(g, stay, c, healing)) break;
          local.insert(local.end(), healing.begin(), healing.end());
        }
        if (g.hero[hp] < minimum) throw Error("Could not prepare requested HP");
        break;
      }
      case Kind2::RawAt:
        if (g.hero.floor != s.floor) throw Error("Wrong floor for positioned input");
        walk(g, s.approach, c, local);
        checked_token(g, s.token, false);
        local.push_back(s.token);
        break;
      case Kind2::Bump: {
        if (g.hero.floor != s.floor) throw Error("Wrong floor for interaction");
        const Pos cell = add(s.approach, dir_delta[s.dir]);
        // A door the route has decided to keep closed is never opened, entered or left through.
        if ((c.no_doors.count({s.floor, cell.index()}) && entity_at(g, s.floor, cell).kind == Kind::door) ||
            c.no_doors.count({s.floor, s.approach.index()}))
          break;
        const auto &now = entity_at(g, s.floor, cell);
        const bool cleared = now.kind == Kind::empty && !s.floor_change && !g.event_enabled(s.floor, cell) &&
                             (s.target == "yellowDoor" || s.target == "blueDoor" || s.target == "redDoor" ||
                              g.world->entity_ids.count(s.target));
        // Items and enemies already collected elsewhere need no input.
        if (cleared && s.target != "" && now.id != s.target) {
          const auto &orig = g.world->entities[g.world->entity_ids.at(s.target)];
          if (orig.kind == Kind::item || orig.kind == Kind::enemy || orig.kind == Kind::door) break;
        }
        walk(g, s.approach, c, local, cell);
        const bool enter_after_open = s.target.empty() && entity_at(g, s.floor, cell).kind == Kind::door;
        checked_token(g, s.token, false);
        local.push_back(s.token);
        if (enter_after_open && g.hero.floor == s.floor && g.hero.pos == s.approach &&
            entity_at(g, s.floor, cell).kind == Kind::empty && !native_events_pending(g)) {
          // This source input entered an empty cell. A changed route may leave
          // its door closed; opening alone would silently omit the room entry
          // and the automatic pickups that made this instruction significant.
          // The first native input also handles red keys and NPC deletion;
          // those legal actions need not be supported by the path proposal.
          checked_token(g, s.token, false);
          local.push_back(s.token);
        }
        if ((enter_after_open || s.target == "yellowDoor" || s.target == "blueDoor" || s.target == "redDoor") &&
            entity_at(g, s.floor, cell).kind == Kind::door)
          throw Error("Door did not open");
        break;
      }
      }
      scope.commit();
      r.tokens.insert(r.tokens.end(), local.begin(), local.end());
      if (tokens_out) tokens_out->push_back(local);
    } catch (const Unsupported &) {
      throw;
    } catch (const Error &e) {
      const std::string error = e.what();
      if (c.repair_survival && !native_events_pending(g) &&
          (error == "Cannot win target battle" || error == "Cannot enter lethal hazard" || error == "Fatal event battle" || error.starts_with("No planned path"))) {
        Tokens repair;
        if (rescue_step(g, s, c, repair)) {
          r.tokens.insert(r.tokens.end(), repair.begin(), repair.end());
          if (tokens_out) tokens_out->push_back(repair);
          continue;
        }
      }
      r.error = e.what();
      break;
    }
  }
  r.fail_step = k;
  if (states_out && k == steps.size()) states_out->push_back(g);
  r.floor = g.hero.floor;
  r.pos = g.hero.pos;
  r.hp = g.hero[hp];
  r.gold = g.hero[gold];
  r.shops = g.hero[times1];
  r.keys = g.hero[yk];
  r.win = k == steps.size() && victory(g);
  if (!r.win && r.error.empty()) r.error = "incomplete";
  return r;
}

bool rescue_step(Game &g, const Step &step, const Costs &costs, Tokens &route) {
  Costs plain = costs; plain.repair_survival = false;
  const int origin_floor = g.hero.floor;
  const Pos origin = g.hero.pos;
  std::vector<int> floors{origin_floor};
  for (int f = g.world->first_floor; f < floors_count; ++f)
    if (f != origin_floor && g.has_visited(f) && g.can_fly(f)) floors.push_back(f);
  std::optional<Game> best;
  Tokens best_tokens;
  Number best_keys = infinite_damage, best_ready_hp = infinite_damage;
  int tried = 0;
  for (int f : floors) for (auto p : cell_positions) {
    const auto &id = entity_at(g, f, p).id;
    if (id != "redPotion" && id != "bluePotion" && id != "greenPotion" && id != "yellowPotion") continue;
    if (++tried > 128) break;
    auto trial = g;
    Tokens inputs;
    try {
      auto visit = [&](int floor, Pos target) {
        if (!plain.no_transit_fights[floor]) {
          visit_item(trial, floor, target, inputs);
          return;
        }
        // The generic transport repair may fight through a blocked corridor.
        // Use the constrained walker on floors that prohibit transit fights.
        if (trial.hero.floor != floor) {
          const auto token = "fly:" + floor_name(floor);
          checked_token(trial, token);
          inputs.push_back(token);
        }
        walk(trial, target, plain, inputs);
      };
      visit(f, p);
      visit(origin_floor, origin);
      const Number ready_hp = trial.hero[hp];
      if (ready_hp <= g.hero[hp]) continue;
      std::vector<Game> states;
      const auto result = realize(trial, {step}, 0, plain, &states);
      if (result.fail_step != 1 || states.size() != 2) continue;
      bool forbidden = false;
      for (const auto &[closed_floor, cell] : costs.no_doors)
        if (entity_at(g, closed_floor, cell_positions[cell]).kind == Kind::door &&
            entity_at(states.back(), closed_floor, cell_positions[cell]).kind != Kind::door) forbidden = true;
      if (forbidden) continue;
      const Number lost_keys = keys_spent(g, states.back());
      inputs.insert(inputs.end(), result.tokens.begin(), result.tokens.end());
      if (!best || lost_keys < best_keys || (lost_keys == best_keys && ready_hp < best_ready_hp) ||
          (lost_keys == best_keys && ready_hp == best_ready_hp && inputs.size() < best_tokens.size())) {
        best = std::move(states.back()); best_tokens = std::move(inputs);
        best_keys = lost_keys; best_ready_hp = ready_hp;
      }
    } catch (const Unsupported &) { throw; }
    catch (const Error &) {} // Speculative state is discarded.
  }
  if (!best) return false;
  g = std::move(*best); route = std::move(best_tokens); return true;
}
} // namespace goal

namespace goal {
Json steps_json(const std::vector<Step> &steps) {
  Json out = Json::array();
  for (const auto &s : steps)
    out.push_back({{"k", int(s.kind)}, {"f", s.floor}, {"x", s.approach.x}, {"y", s.approach.y}, {"d", s.dir}, {"t", s.token},
                   {"o", s.origin}, {"g", s.target}, {"c", s.floor_change}});
  return out;
}
std::vector<Step> steps_from(const Json &j) {
  if (!j.is_array() || j.empty()) throw std::runtime_error("steps must be a nonempty array");
  std::vector<Step> out;
  for (const auto &s : j) {
    const int kind = checked_int(s.at("k")), floor = checked_int(s.at("f")), dir = checked_int(s.at("d"));
    const Pos pos{checked_int(s.at("x")), checked_int(s.at("y"))};
    if (kind < int(Kind2::Fly) || kind > int(Kind2::PrepareHp)) throw std::runtime_error("Invalid step kind");
    if (kind == int(Kind2::PrepareHp)) {
      const auto text = s.at("t").get<std::string>();
      if (text.empty() || text.size() > 7 || !std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; }) || std::stoll(text) <= 0)
        throw std::runtime_error("Invalid HP preparation target");
    }
    if (kind == int(Kind2::Bump) || kind == int(Kind2::RawAt) || kind == int(Kind2::MoveTo) || kind == int(Kind2::Collect) || (kind == int(Kind2::Fly) && floor != -1)) {
      if (floor < 0 || floor >= floors_count || !pos.valid()) throw std::runtime_error("Invalid step location");
    }
    if (kind == int(Kind2::Bump) && (dir < 0 || dir >= 4 || !add(pos, dir_delta[dir]).valid() || s.at("t") != dir_names[dir]))
      throw std::runtime_error("Invalid bump direction");
    out.push_back({Kind2(s.at("k").get<int>()), s.at("f").get<int>(), {s.at("x").get<int>(), s.at("y").get<int>()}, s.at("d").get<int>(),
                   s.at("t").get<std::string>(), s.at("o").get<std::size_t>(), s.at("g").get<std::string>(), s.at("c").get<bool>()});
  }
  return out;
}

// Propagate internal/unsupported failures after joining workers. They are not
// ordinary illegal proposals, and must not silently look like search exhaustion.
template <typename Work> void run_workers(int count, Work work) {
  std::exception_ptr failure;
  std::mutex lock;
  {
    std::vector<std::jthread> pool;
    for (int i = 0; i < count; ++i) pool.emplace_back([&, i] {
      try { work(i); }
      catch (...) { std::lock_guard guard(lock); if (!failure) failure = std::current_exception(); }
    });
  }
  if (failure) std::rethrow_exception(failure);
}
struct Incumbent {
  std::vector<Step> steps;
  std::vector<Game> states;        // state before each step, plus the final state
  std::vector<Tokens> step_tokens; // realized inputs for each step
  Number hp = 0, gold = 0, shops = 0;
  double score = 0;
};
// Objective: linear gold value, or a shortfall penalty below a purchase threshold.
struct Objective {
  bool threshold = false;
  Number target_gold = 2920, target_shops = 16;
  double lambda = 1.0, rate_above = 0.0;
  double key_value = 0.0; // gold credited per unsold yellow key at the end
} objective;
double score_of(Number hp_, Number gold_, Number shops_, double rate, Number keys_ = 0) {
  gold_ += Number(objective.key_value * double(keys_));
  if (!objective.threshold) return double(hp_) + rate * double(gold_ - 500);
  if (shops_ >= objective.target_shops) return double(hp_) + objective.rate_above * double(gold_ - 500);
  return double(hp_) - objective.lambda * double(std::max<Number>(0, objective.target_gold - gold_));
}
// Realize every step, recording states for later partial re-evaluation.
std::optional<Incumbent> build(const Game &initial, std::vector<Step> steps, const Costs &c, double rate, std::string &error) {
  Incumbent inc;
  inc.steps = std::move(steps);
  Game g = initial;
  inc.states.reserve(inc.steps.size() + 1);
  for (std::size_t k = 0; k < inc.steps.size(); ++k) {
    inc.states.push_back(g);
    std::vector<Step> one{inc.steps[k]};
    auto r = realize(g, one, 0, c);
    // realize() returns tokens and error; re-run to capture the state.
    Game next = g;
    for (const auto &t : r.tokens) checked_token(next, t, false);
    if (!r.error.empty() && r.error != "incomplete") {
      const auto &st = inc.steps[k];
      error = r.error + " at step " + std::to_string(k) + " origin " + std::to_string(st.origin) + " " + floor_name(std::max(st.floor, 1)) +
              " (" + std::to_string(st.approach.x) + "," + std::to_string(st.approach.y) + ") " + st.token + " target=" + st.target +
              " hero hp=" + std::to_string(g.hero[hp]) + " atk=" + std::to_string(g.hero[atk]) + " def=" + std::to_string(g.hero[defense]) +
              " yk=" + std::to_string(g.hero[yk]) + " bk=" + std::to_string(g.hero[bk]) + " rk=" + std::to_string(g.hero[rk]) + " gold=" + std::to_string(g.hero[gold]) + " at " + floor_name(g.hero.floor) + "(" +
              std::to_string(g.hero.pos.x) + "," + std::to_string(g.hero.pos.y) + ")";
      return std::nullopt;
    }
    inc.step_tokens.push_back(r.tokens);
    g = std::move(next);
  }
  inc.states.push_back(g);
  if (!victory(g)) { error = "no victory"; return std::nullopt; }
  inc.hp = g.hero[hp];
  inc.gold = g.hero[gold];
  inc.shops = g.hero[times1];
  inc.score = score_of(inc.hp, inc.gold, inc.shops, rate, g.hero[yk]);
  return inc;
}

std::vector<Step> collection_dependencies(const Incumbent &source, bool include_enemies) {
  std::vector<Step> result;
  std::size_t origin = 0;
  for (const auto &s : source.steps) origin = std::max(origin, s.origin + 1);
  for (std::size_t k = 0; k < source.steps.size(); ++k) {
    result.push_back(source.steps[k]);
    const auto &before = source.states[k], &after = source.states[k + 1];
    const int f = after.hero.floor;
    std::map<std::string, int> appeared;
    for (auto p : cell_positions) {
      const auto a = before.block(f, p), b = after.block(f, p);
      if (a.entity != b.entity && after.world->entities[b.entity].kind == Kind::enemy)
        ++appeared[after.world->entities[b.entity].id];
    }
    for (auto p : cell_positions) {
      const auto a = before.block(f, p), b = after.block(f, p);
      if (!a.entity || a.entity == b.entity) continue;
      const auto &e = before.world->entities[a.entity];
      if (e.kind == Kind::enemy) {
        if (appeared[e.id] > 0) { --appeared[e.id]; continue; } // Moving enemy, not a pickup.
        if (!include_enemies || before.damage(a) != 0) continue;
      } else if (e.kind != Kind::item) continue;
      result.push_back({Kind2::Collect, f, p, -1, "", origin++, e.id, false});
    }
  }
  return result;
}
struct Move {
  std::string id, kind;
  std::vector<Step> steps;
  std::size_t first = 0;
  std::vector<std::pair<int, int>> closed{}; // doors this move forbids the walker to open
};

// A proposed removal/restoration belongs to the proposal, not to the live
// chain. Commit it only after accepting and successfully rebuilding the route.
struct PoolEdit {
  std::optional<Step> dropped;
  std::optional<std::size_t> restored;
  void commit(std::vector<Step> &pool) const {
    if (restored) pool.erase(pool.begin() + *restored);
    if (dropped) pool.push_back(*dropped);
  }
};
PoolEdit propose_drop(std::vector<Step> &proposal, const std::vector<Step> &source, std::size_t at) {
  proposal = source;
  proposal.erase(proposal.begin() + at);
  return {source.at(at), {}};
}
PoolEdit propose_restore(std::vector<Step> &proposal, const std::vector<Step> &source,
                         const std::vector<Step> &pool, std::size_t index, std::size_t at) {
  const auto &step = pool.at(index);
  proposal = source;
  const std::vector<Step> block{{Kind2::Fly, -1, {}, -1, "fly:" + floor_name(step.floor), step.origin, {}, true}, step};
  proposal.insert(proposal.begin() + at, block.begin(), block.end());
  return {{}, index};
}

// Weighted acceptance controls exploration; it must not filter HP witnesses.
struct HpRecord {
  Number hp, gold;
  double score;
  template <typename Write> void observe(const Result &result, double candidate_score, Write write) {
    if (!result.win) return;
    score = std::max(score, candidate_score);
    if (result.hp > hp || (result.hp == hp && result.gold > gold)) {
      write();
      hp = result.hp;
      gold = result.gold;
    }
  }
};

Tokens complete_tokens(const std::vector<Tokens> &prefix, std::size_t first, const Result &suffix) {
  if (!suffix.win || first > prefix.size()) throw std::runtime_error("Invalid successful candidate prefix");
  Tokens all;
  for (std::size_t k = 0; k < first; ++k) all.insert(all.end(), prefix[k].begin(), prefix[k].end());
  all.insert(all.end(), suffix.tokens.begin(), suffix.tokens.end());
  return all;
}

void validate_shop_inserts(const Json &cfg) {
  for (const auto &shop : cfg.value("shop_inserts", Json::array()))
    if (shop.contains("window"))
      throw std::runtime_error("shop_inserts.window is unsupported; use inclusive from/to step-origin bounds");
}
} // namespace goal

#ifndef MOTA_GOAL_ROUTE_LIBRARY
int main(int argc, char **argv) {
  try {
    using namespace goal;
    if (argc == 2 && std::string(argv[1]) == "--help") {
      std::cout << "Usage: mota-goal-route WORLD INPUT CONFIG NEW_OUTPUT\nModes: replay, lns, ruin, beam, anneal. Long searches require supervised compute.\n";
      return 0;
    }
    if (argc != 5) throw std::runtime_error("Usage: mota-goal-route WORLD INPUT CONFIG NEW_OUTPUT");
    const auto input = read_json(argv[2]), cfg = read_json(argv[3]);
    const auto out = std::filesystem::absolute(argv[4]);
    if (cfg.value("schema", std::string()) != "mota.goal-route.v1" || std::filesystem::exists(out))
      throw std::runtime_error("Invalid config or existing output");
    const auto mode = cfg.value("mode", std::string("lns"));
    if (mode != "replay" && mode != "lns" && mode != "ruin" && mode != "beam" && mode != "anneal")
      throw std::runtime_error("Unknown mode: " + mode);
    validate_shop_inserts(cfg);
    const int workers = checked_int(cfg.value("workers", Json(8)));
    const int rounds = checked_int(cfg.value("rounds", Json(20)));
    const int defer_horizon = checked_int(cfg.value("defer_horizon", Json(30)));
    const double limit = cfg.value("max_seconds", 600.0);
    if (workers < 1 || workers > 64 || rounds < 0 || defer_horizon < 0 || !std::isfinite(limit) || limit <= 0)
      throw std::runtime_error("Invalid workers, rounds, horizon or time limit");
    for (const auto *key : {"yellow_cost", "blue_cost", "damage_weight", "gold_rate", "key_value", "pretrap_damage"})
      if (cfg.contains(key) && (!std::isfinite(cfg.at(key).get<double>()) || cfg.at(key).get<double>() < 0))
        throw std::runtime_error(std::string("Invalid nonnegative cost: ") + key);
    const auto floor_prices = floor_key_costs_from(cfg.value("floor_key_costs", Json::object()));
    for (const auto *key : {"iterations", "ruin_max", "beam_width", "order_window", "floor_window", "window"})
      if (cfg.contains(key) && checked_int(cfg.at(key)) <= 0) throw std::runtime_error(std::string("Invalid positive limit: ") + key);
    if (mode == "anneal") {
      const int chains = checked_int(cfg.value("chains", Json(workers)));
      if (chains < 1 || chains > workers) throw std::runtime_error("chains must be between 1 and workers");
      for (const auto *key : {"t0", "t1"})
        if (cfg.contains(key) && (!std::isfinite(cfg.at(key).get<double>()) || cfg.at(key).get<double>() <= 0))
          throw std::runtime_error(std::string("Invalid positive temperature: ") + key);
      if (cfg.contains("cooling_seconds") && (!std::isfinite(cfg.at("cooling_seconds").get<double>()) ||
          cfg.at("cooling_seconds").get<double>() <= 0 || cfg.at("cooling_seconds").get<double>() > limit))
        throw std::runtime_error("cooling_seconds must be positive and no greater than max_seconds");
    }
    if (input.value("schema", std::string()) != "mota.cpp.native-replay-input.v1") throw std::runtime_error("Invalid input schema");
    std::filesystem::create_directories(out / "rounds");
    save(out / "config.json", cfg);
    engine::io::Compiler compiler(mota::World::load(argv[1]));
    auto initial = make_native_game(compiler, input.at("initial"));
    const auto tokens = input.at("actions").get<Tokens>();
    std::vector<Game> prefix{initial};
    // A steps file replaces extraction, so the input's own actions need no replay.
    if (!cfg.contains("steps_file")) for (const auto &t : tokens) {
      auto next = prefix.back();
      checked_token(next, t, false);
      prefix.push_back(std::move(next));
    }
    Costs costs;
    costs.yellow = cfg.value("yellow_cost", 66.0);
    costs.blue = cfg.value("blue_cost", 132.0);
    costs.damage = cfg.value("damage_weight", 1.0);
    costs.allow_potions = cfg.value("allow_potions", false);
    costs.repair_survival = cfg.value("repair_survival", false);
    costs.adaptive_access = cfg.value("adaptive_access", false);
    costs.pretrap_damage = cfg.value("pretrap_damage", 1.0);
    costs.floor_key_costs = floor_prices;
    objective.key_value = cfg.value("key_value", 0.0);
    for (const auto &fj : cfg.value("no_transit_fights", Json::array())) costs.no_transit_fights.set(floor_number(fj.get<std::string>()));
    auto add_closed_doors = [&](const Json &doors) {
      for (const auto &dj : doors) {
        const int f = floor_number(dj.at(0).get<std::string>());
        const Pos p{checked_int(dj.at(1)), checked_int(dj.at(2))};
        if (f < 0 || f >= floors_count || !p.valid()) throw std::runtime_error("Invalid closed-door location");
        costs.no_doors.insert({f, p.index()});
      }
    };
    add_closed_doors(cfg.value("no_doors", Json::array()));
    const double rate = cfg.value("gold_rate", 1600.0 / 2420.0);
    if (cfg.contains("threshold")) {
      const auto &tj = cfg.at("threshold");
      objective.threshold = true;
      objective.target_gold = tj.value("gold", Number(2920));
      objective.target_shops = tj.value("shops", Number(16));
      objective.lambda = tj.value("lambda", 1.0);
      objective.rate_above = tj.value("rate_above", 0.0);
    }
    std::string err;
    auto base_steps = cfg.contains("steps_file") ? steps_from(read_json(cfg.at("steps_file").get<std::string>()).at("steps"))
                                                 : extract(tokens, prefix, cfg.value("drop_doors", false));
    if (cfg.contains("steps_file")) {
      const auto sj = read_json(cfg.at("steps_file").get<std::string>());
      add_closed_doors(sj.value("no_doors", Json::array()));
      // An explicit table, including {}, replaces a saved route's policy.
      if (!cfg.contains("floor_key_costs"))
        costs.floor_key_costs = floor_key_costs_from(sj.value("floor_key_costs", Json::object()));
    }
    if (!costs.floor_key_costs.empty()) save(out / "config.json", with_floor_key_costs(cfg, costs));
    if (cfg.value("explicit_collection", false)) {
      auto source = build(initial, base_steps, costs, rate, err);
      if (!source) throw std::runtime_error("Collection source route failed: " + err);
      base_steps = collection_dependencies(*source, cfg.value("collect_zero_damage_enemies", true));
      save(out / "collection-source.json", Json{{"hp", source->hp}, {"gold", source->gold},
        {"original_steps", source->steps.size()}, {"expanded_steps", base_steps.size()},
        {"requirements", base_steps.size() - source->steps.size()}, {"states_restored", false}});
    }
    if (cfg.value("dump_steps", false)) {
      save(out / "steps.json", with_floor_key_costs(Json{{"steps", steps_json(base_steps)}}, costs));
    }
    // Scripted edits: delete steps by origin, move steps (by origin) after an anchor origin.
    for (const auto &ed : cfg.value("edits", Json::array())) {
      auto find = [&](std::size_t origin) {
        for (std::size_t k = 0; k < base_steps.size(); ++k)
          if (base_steps[k].origin == origin) return k;
        throw std::runtime_error("No step with origin " + std::to_string(origin));
      };
      const auto op = ed.at("op").get<std::string>();
      if (op == "delete") {
        for (auto o : ed.at("origins").get<std::vector<std::size_t>>()) base_steps.erase(base_steps.begin() + find(o));
      } else if (op == "move_after") {
        std::vector<Step> block;
        for (auto o : ed.at("origins").get<std::vector<std::size_t>>()) {
          auto k = find(o);
          block.push_back(base_steps[k]);
          base_steps.erase(base_steps.begin() + k);
        }
        auto at = find(ed.at("anchor").get<std::size_t>()) + 1;
        if (ed.contains("fly")) block.insert(block.begin(), Step{Kind2::Fly, -1, {}, -1, ed.at("fly").get<std::string>(), 0, {}, true});
        base_steps.insert(base_steps.begin() + at, block.begin(), block.end());
      } else if (op == "insert_bump_after") {
        const auto &bj = ed.at("bump");
        Step st{Kind2::Bump, bj.at("floor").get<int>(), {bj.at("x").get<int>(), bj.at("y").get<int>()}, dir_index(bj.at("dir").get<std::string>()),
                bj.at("dir").get<std::string>(), 0, bj.value("target", std::string()), false};
        auto at = find(ed.at("anchor").get<std::size_t>()) + 1;
        base_steps.insert(base_steps.begin() + at, st);
      } else if (op == "insert_fly_after") {
        Step st{Kind2::Fly, ed.at("floor").get<int>(), {ed.at("x").get<int>(), ed.at("y").get<int>()}, -1, ed.at("token").get<std::string>(), 0, {}, true};
        auto at = find(ed.at("anchor").get<std::size_t>()) + 1;
        base_steps.insert(base_steps.begin() + at, st);
      } else if (op == "insert_collect_after") {
        const auto &item = ed.at("item");
        Step st{Kind2::Collect, checked_int(item.at("floor")), {checked_int(item.at("x")), checked_int(item.at("y"))},
          -1, "", 0, item.at("id").get<std::string>(), false};
        auto at = find(ed.at("anchor").get<std::size_t>()) + 1;
        base_steps.insert(base_steps.begin() + at, st);
      } else if (op == "prepare_hp_after") {
        Step st{Kind2::PrepareHp, -1, {}, -1, std::to_string(checked_int(ed.at("minimum_hp"))), 0, "", false};
        auto at = find(ed.at("anchor").get<std::size_t>()) + 1;
        base_steps.insert(base_steps.begin() + at, st);
      } else if (op == "insert_after") {
        std::vector<Step> block;
        for (const auto &t : ed.at("tokens")) block.push_back(Step{Kind2::Raw, -1, {}, -1, t.get<std::string>(), 0, {}, false});
        auto at = find(ed.at("anchor").get<std::size_t>()) + 1;
        base_steps.insert(base_steps.begin() + at, block.begin(), block.end());
      } else if (op == "replace_token") {
        auto &st = base_steps[find(ed.at("origin").get<std::size_t>())];
        if (st.kind != Kind2::Raw) throw std::runtime_error("replace_token requires a raw menu input");
        st.token = ed.at("token").get<std::string>();
      } else
        throw std::runtime_error("Unknown edit op " + op);
    }
    // Validate scripted edits as well as externally supplied steps before execution.
    base_steps = steps_from(steps_json(base_steps));
    for (const auto &s : base_steps) if (s.kind == Kind2::Collect) {
      const auto entity = initial.world->entity_ids.find(s.target);
      if (entity == initial.world->entity_ids.end() ||
          (initial.world->entities[entity->second].kind != Kind::item &&
           initial.world->entities[entity->second].kind != Kind::enemy))
        throw std::runtime_error("Invalid collection target: " + s.target);
    }
    auto inc_opt = build(initial, base_steps, costs, rate, err);
    if (!inc_opt && cfg.value("dump_realized", false)) {
      // Write the realized prefix up to the failing step for diagnosis.
      std::vector<Game> st;
      std::vector<Tokens> tk;
      realize(initial, base_steps, 0, costs, &st, &tk);
      Json rows = Json::array();
      for (std::size_t k = 0; k < st.size() && k < base_steps.size(); ++k) {
        const auto &g = st[k];
        rows.push_back({{"k", k}, {"o", base_steps[k].origin}, {"t", base_steps[k].token}, {"g", base_steps[k].target},
                        {"floor", floor_name(g.hero.floor)}, {"x", g.hero.pos.x}, {"y", g.hero.pos.y}, {"hp", g.hero[hp]},
                        {"atk", g.hero[atk]}, {"def", g.hero[defense]}, {"gold", g.hero[gold]}, {"yk", g.hero[yk]}, {"bk", g.hero[bk]},
                        {"tokens", k < tk.size() ? Json(tk[k]) : Json::array()}});
      }
      save(out / "realized-prefix.json", rows);
    }
    if (!inc_opt) throw std::runtime_error("Baseline goal route failed: " + err);
    Incumbent cur = std::move(*inc_opt);
    if (cfg.contains("expected_hp") && cur.hp != cfg.at("expected_hp").get<Number>())
      throw std::runtime_error("Realized baseline differs from expected_hp");
    const auto runtime = native_runtime_info(initial);
    auto write_witness = [&](const std::string &name, const std::vector<Step> &steps, const Tokens &all,
                             Number hp_, Number gold_, double score_, const Costs &route_costs) {
      Json closed = Json::array();
      for (const auto &[f, i] : route_costs.no_doors) closed.push_back({floor_name(f), i % side, i / side});
      save(out / (name + "-input.json"), Json{{"schema", input.at("schema")}, {"initial", input.at("initial")}, {"actions", all}, {"checkpoints", {0, all.size()}}});
      save(out / (name + "-steps.json"), with_floor_key_costs(Json{{"steps", steps_json(steps)}, {"hp", hp_}, {"gold", gold_}, {"score", score_}, {"no_doors", closed}}, route_costs));
    };
    auto write_route = [&](const std::string &name, const Incumbent &route) {
      Tokens all;
      for (const auto &part : route.step_tokens) all.insert(all.end(), part.begin(), part.end());
      write_witness(name, route.steps, all, route.hp, route.gold, route.score, costs);
    };
    const Number baseline_hp = cur.hp, baseline_gold = cur.gold;
    HpRecord saved{cur.hp, cur.gold, cur.score};
    auto retain_hp = [&](const Incumbent &route) {
      Result result; result.win = true; result.hp = route.hp; result.gold = route.gold;
      saved.observe(result, route.score, [&] { write_route("best-hp", route); });
    };
    auto retain_evaluated = [&](const Incumbent &base, const std::vector<Step> &steps, std::size_t first,
                                const Result &result, double score, const Costs &route_costs) {
      saved.observe(result, score, [&] {
        write_witness("best-hp", steps, complete_tokens(base.step_tokens, first, result),
                      result.hp, result.gold, score, route_costs);
      });
    };
    write_route("best", cur);
    write_route("best-hp", cur);
    save(out / "baseline.json", Json{{"hp", cur.hp}, {"gold", cur.gold}, {"score", cur.score}, {"simulator", runtime}, {"win", true}});
    auto finish = [&](double elapsed) {
      save(out / "summary.json", Json{{"passed", true}, {"mode", mode}, {"baseline_hp", baseline_hp}, {"baseline_gold", baseline_gold},
        {"best_hp", saved.hp}, {"best_hp_gold", saved.gold}, {"best_score", saved.score}, {"seconds", elapsed},
        {"simulator", runtime}, {"original_game_verified", false}, {"optimality_proved", false},
        {"hp_scope", mode == "anneal" || mode == "lns"
          ? "Starting route and every evaluated successful candidate; HP retention is independent of weighted acceptance"
          : "Starting route and retained winners; weighted search need not accept every HP improvement"}});
    };
    if (mode == "replay") { finish(0); return 0; }
    if (cfg.value("dump_realized", false)) {
      Json rows = Json::array();
      for (std::size_t k = 0; k < cur.steps.size(); ++k) {
        const auto &g = cur.states[k];
        rows.push_back({{"k", k}, {"o", cur.steps[k].origin}, {"t", cur.steps[k].token}, {"g", cur.steps[k].target},
                        {"floor", floor_name(g.hero.floor)}, {"x", g.hero.pos.x}, {"y", g.hero.pos.y}, {"hp", g.hero[hp]},
                        {"gold", g.hero[gold]}, {"yk", g.hero[yk]}, {"tokens", cur.step_tokens[k]}});
      }
      save(out / "realized-steps.json", rows);
    }
    std::cout << Json{{"steps", cur.steps.size()}, {"hp", cur.hp}, {"gold", cur.gold}, {"score", cur.score}}.dump() << '\n' << std::flush;
    if (cfg.value("mode", std::string("lns")) == "ruin") {
      // Ruin and recreate: remove a set of interactions, then reinsert each at its
      // best position (or skip it) by exact realization; keep improvements.
      const long iterations = cfg.value("iterations", 1000L);
      const int ruin_max = checked_int(cfg.value("ruin_max", Json(8)));
      const auto seed = cfg.value("seed", std::uint64_t(1));
      const double accept_worse = cfg.value("accept_worse", 0.0);
      const auto began = Clock::now();
      std::mt19937_64 rng(seed);
      auto uni = [&](std::size_t n_) { return std::size_t(rng() % std::max<std::size_t>(1, n_)); };
      std::ofstream progress(out / "progress.jsonl");
      double best_score = cur.score;
      for (long it = 0; it < iterations && seconds(began) < limit; ++it) {
        const auto &S = cur.steps;
        const std::size_t n = S.size();
        auto removable = [&](std::size_t k) {
          const auto &x = S[k];
          return (x.kind == Kind2::Bump || x.kind == Kind2::MoveTo || (cfg.value("move_collections", false) && x.kind == Kind2::Collect)) && !x.floor_change && !(k + 1 < n && S[k + 1].kind == Kind2::Raw) &&
                 x.target != "redDoor" && x.target != "oldman" && x.target != "trader" && x.target != "thief";
        };
        // Choose a ruin: a floor and a window of its interactions.
        std::vector<std::size_t> cand;
        for (std::size_t k = 0; k < n; ++k) if (removable(k)) cand.push_back(k);
        if (cand.empty()) break;
        const auto pivot = cand[uni(cand.size())];
        const int f = S[pivot].floor;
        const int mode_r = int(uni(3));
        std::vector<std::size_t> ruin;
        for (auto k : cand) {
          if (S[k].floor != f) continue;
          const long dist = long(k) - long(pivot);
          if (mode_r == 0 && std::labs(dist) > 60) continue;       // local window on the floor
          if (mode_r == 1 && std::labs(dist) > 250) continue;      // wider window
          ruin.push_back(k);
        }
        std::shuffle(ruin.begin(), ruin.end(), rng);
        if (int(ruin.size()) > ruin_max) ruin.resize(ruin_max);
        std::sort(ruin.begin(), ruin.end());
        std::vector<Step> removed;
        std::vector<Step> base;
        {
          std::set<std::size_t> rs(ruin.begin(), ruin.end());
          for (std::size_t k = 0; k < n; ++k) {
            if (rs.count(k)) removed.push_back(S[k]);
            else base.push_back(S[k]);
          }
        }
        std::shuffle(removed.begin(), removed.end(), rng);
        // Reinsert greedily. Positions: right after a flight to floor f, at the end of such a visit, or as a new visit before any flight.
        std::vector<Step> work = base;
        bool ok = true;
        double work_score = -1e18;
        std::deque<Step> pending(removed.begin(), removed.end());
        int stall = 0;
        while (!pending.empty()) {
          const Step t = pending.front();
          pending.pop_front();
          // Record the states of the current work route for cached evaluation.
          std::vector<Game> ws;
          std::vector<Tokens> wt;
          auto wr = realize(initial, work, 0, costs, &ws, &wt);
          std::vector<std::pair<std::size_t, std::vector<Step>>> options;
          const std::size_t m = work.size();
          for (std::size_t k = 0; k < m; ++k) {
            if (work[k].kind != Kind2::Fly) continue;
            std::size_t e = k + 1;
            while (e < m && work[e].kind != Kind2::Fly) ++e;
            bool same = false;
            for (std::size_t q = k + 1; q < e; ++q) if (work[q].kind != Kind2::Raw) { same = work[q].floor == t.floor; break; }
            if (same) {
              options.push_back({k + 1, {t}});
              if (e > k + 1) options.push_back({e, {t}});
            }
            options.push_back({k, {Step{Kind2::Fly, -1, {}, -1, "fly:" + floor_name(t.floor), t.origin, {}, true}, t}});
          }
          std::vector<double> scores(options.size() + 1, -1e18);
          if (wr.win) scores.back() = score_of(wr.hp, wr.gold, wr.shops, rate, wr.keys);
          std::atomic<std::size_t> next{0};
          auto job = [&] {
            for (;;) {
              auto o = next.fetch_add(1);
              if (o >= options.size()) break;
              const auto at = options[o].first;
              if (at >= ws.size()) continue; // the work route failed before this point
              auto ns = work;
              ns.insert(ns.begin() + at, options[o].second.begin(), options[o].second.end());
              auto r = realize(ws[at], ns, at, costs);
              if (r.win) scores[o] = score_of(r.hp, r.gold, r.shops, rate, r.keys);
            }
          };
          run_workers(workers, [&](int) { job(); });
          std::size_t bi = scores.size() - 1;
          for (std::size_t o = 0; o < scores.size(); ++o) if (scores[o] > scores[bi]) bi = o;
          if (scores[bi] <= -1e17) {
            // Maybe it depends on another pending interaction: retry later.
            pending.push_back(t);
            if (++stall > int(pending.size()) + 1) { ok = false; break; }
            // Insert it at its original relative position as a feasibility fallback is not tracked; continue.
            continue;
          }
          stall = 0;
          if (bi < options.size()) work.insert(work.begin() + options[bi].first, options[bi].second.begin(), options[bi].second.end());
          work_score = scores[bi];
        }
        Json row = {{"it", it}, {"floor", floor_name(f)}, {"ruined", ruin.size()}, {"ok", ok}, {"score", ok ? Json(work_score) : Json()},
                    {"incumbent", cur.score}};
        if (ok && (work_score > cur.score + 1e-9 || (accept_worse > 0 && work_score > cur.score - accept_worse))) {
          auto rebuilt = build(initial, work, costs, rate, err);
          if (rebuilt) {
            row["accepted"] = true;
            cur = std::move(*rebuilt);
            retain_hp(cur);
            if (cur.score > best_score + 1e-9) {
              best_score = cur.score;
              write_route("best", cur);
            }
          }
        }
        progress << row.dump() << '\n' << std::flush;
        if (row.value("accepted", false) || it % 20 == 0)
          std::cout << Json{{"it", it}, {"score", cur.score}, {"hp", cur.hp}, {"gold", cur.gold}, {"best", best_score},
                            {"seconds", seconds(began)}, {"last", row}}.dump() << '\n' << std::flush;
      }
      finish(seconds(began));
      return 0;
    }
    if (cfg.value("mode", std::string("lns")) == "beam") { // mode == beam
      // Rebuild the opening from its interactions. Door openings and flights are left to the walker, and each
      // partial order is scored by completing it with its remaining interactions and the untouched rest of the route.
      const auto &S = cur.steps;
      const std::size_t split_origin = cfg.at("split_origin").get<std::size_t>();
      std::size_t K = 0;
      while (K < S.size() && S[K].origin != split_origin) ++K;
      if (K == S.size()) throw std::runtime_error("split_origin not found");
      struct Unit { std::vector<Step> steps; int floor = 0, end_floor = 0; bool npc = false, door = false; };
      std::vector<Unit> units;
      auto is_npc = [](const std::string &t) {
        return t == "oldman" || t == "trader"; // ids 121/122 grant dialogue energy when they vanish
      };
      auto next_floor = [&](std::size_t k) { // floor of the first located step after k
        for (std::size_t q = k + 1; q < S.size(); ++q) if (S[q].floor > 0 && S[q].kind != Kind2::Fly) return S[q].floor;
        return 0;
      };
      int energy = 0; // non-Raw steps still attached to the previous NPC unit (dialogue energy window)
      for (std::size_t k = 0; k < K; ++k) {
        const auto &st = S[k];
        const bool door = st.kind == Kind2::Bump && !st.floor_change && (st.target == "yellowDoor" || st.target == "blueDoor");
        if (st.kind == Kind2::Fly) { energy = 0; continue; }
        bool attach = !units.empty() && !door && (st.kind == Kind2::Raw || (energy > 0 && st.floor == units.back().floor && !st.floor_change));
        if (units.empty() && st.kind == Kind2::Raw) attach = false;
        if (attach) {
          units.back().steps.push_back(st);
          if (st.kind != Kind2::Raw) --energy;
        } else {
          Unit u;
          u.steps.push_back(st);
          u.floor = st.floor;
          u.npc = is_npc(st.target);
          u.door = door;
          units.push_back(std::move(u));
          // A vanishing NPC grants one step or item before a wall/door deletion: keep a following wall bump attached.
          energy = 0;
          if (units.back().npc)
            for (std::size_t q = k + 1, seen = 0; q < K && seen < 2; ++q) {
              if (S[q].kind == Kind2::Raw) continue;
              ++seen;
              if (S[q].target.find("Wall") != std::string::npos || S[q].target.empty()) energy = int(seen);
            }
        }
        units.back().end_floor = st.floor_change ? next_floor(k) : (units.back().end_floor ? units.back().end_floor : units.back().floor);
        if (st.floor_change) units.back().end_floor = next_floor(k);
      }
      for (auto &u : units) if (u.floor <= 0) u.floor = u.end_floor;
      {
        Json uj = Json::array();
        for (const auto &u : units) uj.push_back({{"floor", u.floor}, {"end_floor", u.end_floor}, {"door", u.door}, {"npc", u.npc}, {"steps", steps_json(u.steps)}});
        save(out / "units.json", uj);
      }
      // Fly to floor f from the hero's current position, else from one of the current floor's stair landings.
      auto try_fly = [&](Game &g, int f, std::vector<Step> &out) {
        const auto &fd = g.world->floors[g.hero.floor];
        for (Pos ap : {Pos{-1, -1}, fd.down, fd.up}) {
          if (ap.x >= 0 && !ap.valid()) continue;
          Step st{Kind2::Fly, ap.x >= 0 ? g.hero.floor : -1, ap, -1, "fly:" + floor_name(f), 0, {}, true};
          std::vector<Game> states;
          auto r = realize(g, std::vector<Step>{st}, 0, costs, &states);
          if (r.fail_step != 1 || states.size() != 2) continue;
          g = std::move(states.back());
          out.push_back(st);
          return true;
        }
        return false;
      };
      // Finish a partial opening: remaining units in their original order, flying wherever the hero actually is,
      // then the untouched rest of the route. Returns the final result and the steps it used.
      auto completion = [&](Game g, const std::vector<char> &used, std::vector<Step> &out) {
        Result r;
        for (std::size_t u = 0; u < units.size(); ++u) {
          if (used[u]) continue;
          if (units[u].floor > 0 && units[u].floor != g.hero.floor && !try_fly(g, units[u].floor, out)) { r.win = false; return r; }
          std::vector<Step> part(units[u].steps.begin(), units[u].steps.end());
          std::vector<Game> st;
          r = realize(g, part, 0, costs, &st);
          if (r.fail_step != part.size() || st.size() != part.size() + 1) { r.win = false; return r; }
          g = std::move(st.back());
          out.insert(out.end(), part.begin(), part.end());
        }
        if (S[K].kind != Kind2::Fly && S[K].floor > 0 && S[K].floor != g.hero.floor && !try_fly(g, S[K].floor, out)) { r.win = false; return r; }
        std::vector<Step> part(S.begin() + K, S.end());
        r = realize(g, part, 0, costs);
        out.insert(out.end(), part.begin(), part.end());
        return r;
      };
      const int width = checked_int(cfg.value("beam_width", Json(12)));
      const int order_window = checked_int(cfg.value("order_window", Json(6)));
      const int floor_window = checked_int(cfg.value("floor_window", Json(20)));
      const bool skip_doors = cfg.value("skip_doors", true);
      const bool dedup_used_only = cfg.value("dedup_used_only", false);
      const auto beam_seed = cfg.value("seed", std::uint64_t(1));
      struct BState { Game g; std::vector<char> used; std::vector<Step> seq; double score = -1e18; };
      std::vector<BState> beam(1, BState{initial, std::vector<char>(units.size(), 0), {}, -1e18});
      std::ofstream progress(out / "progress.jsonl");
      double best_score = cur.score;
      Number best_hp = cur.hp;
      auto save_best = [&](const std::string &name, const std::vector<Step> &steps) {
        std::vector<Game> st;
        std::vector<Tokens> tk;
        auto r = realize(initial, steps, 0, costs, &st, &tk);
        if (!r.win) throw std::runtime_error("Beam winner failed continuous reconstruction");
        Tokens all;
        for (const auto &t : tk) all.insert(all.end(), t.begin(), t.end());
        save(out / (name + "-steps.json"), with_floor_key_costs(Json{{"steps", steps_json(steps)}, {"hp", r.hp}, {"gold", r.gold}, {"keys", r.keys},
                                                {"score", score_of(r.hp, r.gold, r.shops, rate, r.keys)}}, costs));
        save(out / (name + "-input.json"), Json{{"schema", input.at("schema")}, {"initial", input.at("initial")}, {"actions", all},
                                                {"checkpoints", {0, all.size()}}});
        saved.score = std::max(saved.score, score_of(r.hp, r.gold, r.shops, rate, r.keys));
        if (name == "best-hp") { saved.hp = r.hp; saved.gold = r.gold; }
      };
      {
        std::vector<Step> comp;
        auto r = completion(initial, beam[0].used, comp);
        std::cout << Json{{"units", units.size()}, {"split", K}, {"identity_win", r.win}, {"identity_hp", r.hp}, {"identity_gold", r.gold},
                          {"identity_keys", r.keys}, {"identity_error", r.error}}.dump() << '\n' << std::flush;
      }
      const auto began = Clock::now();
      for (int level = 0; seconds(began) < limit; ++level) {
        struct Cand { std::size_t state, unit; bool skip; std::optional<Game> g; std::vector<Step> added; double score = -1e18; Result r; std::vector<Step> comp; };
        std::vector<Cand> cands;
        for (std::size_t si = 0; si < beam.size(); ++si) {
          const auto &b = beam[si];
          int taken = 0, scanned = 0;
          for (std::size_t u = 0; u < units.size() && scanned < floor_window; ++u) {
            if (b.used[u]) continue;
            ++scanned;
            if (taken < order_window || units[u].floor == b.g.hero.floor) {
              cands.push_back({si, u, false, {}, {}, -1e18, {}, {}});
              // A door can also be skipped for good: the walker then opens it only if a later path needs it.
              if (units[u].door && skip_doors) cands.push_back({si, u, true, {}, {}, -1e18, {}, {}});
              ++taken;
            }
          }
        }
        if (cands.empty()) break;
        std::atomic<std::size_t> next{0};
        auto job = [&] {
          for (;;) {
            auto ci = next.fetch_add(1);
            if (ci >= cands.size()) break;
            auto &c = cands[ci];
            const auto &b = beam[c.state];
            const auto &u = units[c.unit];
            if (c.skip) c.g = b.g;
            else {
              Game g = b.g;
              if (u.floor > 0 && u.floor != g.hero.floor && !try_fly(g, u.floor, c.added)) continue;
              std::vector<Game> st;
              auto r = realize(g, u.steps, 0, costs, &st);
              if (r.fail_step != u.steps.size() || st.size() != u.steps.size() + 1) continue;
              c.added.insert(c.added.end(), u.steps.begin(), u.steps.end());
              c.g = std::move(st.back());
            }
            auto used = b.used;
            used[c.unit] = 1;
            c.r = completion(*c.g, used, c.comp);
            if (c.r.win) c.score = score_of(c.r.hp, c.r.gold, c.r.shops, rate, c.r.keys);
          }
        };
        run_workers(workers, [&](int) { job(); });
        std::vector<std::size_t> order(cands.size());
        std::iota(order.begin(), order.end(), 0);
        std::vector<std::uint64_t> tie(cands.size());
        {
          std::mt19937_64 tie_rng(beam_seed * 1000003ULL + std::uint64_t(level));
          for (auto &t : tie) t = tie_rng();
        }
        std::sort(order.begin(), order.end(), [&](auto a, auto b) {
          return cands[a].score != cands[b].score ? cands[a].score > cands[b].score : tie[a] < tie[b];
        });
        std::vector<BState> next_beam;
        std::set<std::string> seen;
        int valid = 0;
        for (auto ci : order) {
          auto &c = cands[ci];
          if (c.score <= -1e17) break;
          ++valid;
          const auto &b = beam[c.state];
          std::vector<Step> full = b.seq;
          full.insert(full.end(), c.added.begin(), c.added.end());
          if (c.score > best_score + 1e-9 || c.r.hp > best_hp) {
            std::vector<Step> route = full;
            route.insert(route.end(), c.comp.begin(), c.comp.end());
            if (c.score > best_score + 1e-9) { best_score = c.score; save_best("best", route); }
            if (c.r.hp > best_hp) { best_hp = c.r.hp; save_best("best-hp", route); }
          }
          if (int(next_beam.size()) >= width) continue;
          std::string key(b.used.begin(), b.used.end());
          key[c.unit] = 1;
          if (!dedup_used_only) key += "|" + std::to_string(c.g->hero.floor) + "," + std::to_string(c.g->hero.pos.index());
          if (!seen.insert(key).second) continue;
          auto used = b.used;
          used[c.unit] = 1;
          next_beam.push_back(BState{std::move(*c.g), std::move(used), std::move(full), c.score});
        }
        progress << Json{{"level", level}, {"candidates", cands.size()}, {"valid", valid}, {"beam", next_beam.size()},
                         {"level_best", next_beam.empty() ? Json() : Json(next_beam.front().score)}, {"best", best_score}, {"best_hp", best_hp},
                         {"seconds", seconds(began)}}.dump() << '\n' << std::flush;
        if (level % 10 == 0)
          std::cout << Json{{"level", level}, {"valid", valid}, {"best", best_score}, {"best_hp", best_hp}, {"seconds", seconds(began)}}.dump()
                    << '\n' << std::flush;
        if (next_beam.empty()) break;
        beam = std::move(next_beam);
      }
      std::cout << Json{{"done", true}, {"best", best_score}, {"best_hp", best_hp}, {"seconds", seconds(began)}}.dump() << '\n' << std::flush;
      finish(seconds(began));
      return 0;
    }
    if (cfg.value("mode", std::string("lns")) == "anneal") {
      const int chains = checked_int(cfg.value("chains", Json(workers)));
      const long iterations = cfg.value("iterations", 100000L);
      const double t0 = cfg.value("t0", 20.0), t1 = cfg.value("t1", 0.5);
      const double cooling_seconds = cfg.value("cooling_seconds", 0.0);
      const int window = checked_int(cfg.value("window", Json(25)));
      const auto seed = cfg.value("seed", std::uint64_t(1));
      std::mutex io;
      double global_best = cur.score;
      std::ofstream progress(out / "progress.jsonl");
      const auto began = Clock::now();
      auto run_chain = [&](int id) {
        Incumbent c = cur;
        std::mt19937_64 rng(seed * 1000003ULL + id);
        auto uni = [&](std::size_t n_) { return std::size_t(rng() % std::max<std::size_t>(1, n_)); };
        std::vector<Step> pool;
        double best = c.score;
        long accepted = 0, wins = 0;
        long attempted = 0;
        long evaluated = 0;
        double last_temperature = t0;
        const auto chain_began = Clock::now();
        for (long it = 0; it < iterations && seconds(chain_began) < limit; ++it) {
          attempted = it + 1;
          const double frac = cooling_seconds > 0 ? std::min(1.0, seconds(chain_began) / cooling_seconds) : double(it) / double(iterations);
          const double temp = t0 * std::pow(t1 / t0, frac);
          const auto &S = c.steps;
          const std::size_t n = S.size();
          std::vector<std::size_t> flies;
          for (std::size_t k = 0; k < n; ++k) if (S[k].kind == Kind2::Fly) flies.push_back(k);
          auto movable = [&](std::size_t k) {
            return (S[k].kind == Kind2::Bump || S[k].kind == Kind2::MoveTo || (cfg.value("move_collections", false) && S[k].kind == Kind2::Collect)) && !S[k].floor_change && !(k + 1 < n && S[k + 1].kind == Kind2::Raw);
          };
          std::vector<Step> ns;
          PoolEdit pool_edit;
          std::size_t first = n;
          const int family = int(uni(7));
          if (family == 0 && flies.size() > 3) { // visit move
            auto v = uni(flies.size() - 1);
            auto b = flies[v], e = flies[v + 1];
            long wv = long(v) + long(uni(2 * window + 1)) - window;
            if (wv < 0 || wv >= long(flies.size()) || wv == long(v) || wv == long(v) + 1) continue;
            auto at = flies[wv];
            if (at < b) { ns.assign(S.begin(), S.begin() + at); ns.insert(ns.end(), S.begin() + b, S.begin() + e); ns.insert(ns.end(), S.begin() + at, S.begin() + b); ns.insert(ns.end(), S.begin() + e, S.end()); first = at; }
            else { ns.assign(S.begin(), S.begin() + b); ns.insert(ns.end(), S.begin() + e, S.begin() + at); ns.insert(ns.end(), S.begin() + b, S.begin() + e); ns.insert(ns.end(), S.begin() + at, S.end()); first = b; }
          } else if (family == 1 || family == 2) { // step move / relocate into a new mini visit
            auto k = uni(n);
            if (!movable(k) || (S[k].kind != Kind2::Bump && S[k].kind != Kind2::Collect)) continue;
            if (S[k].target == "yellowDoor" || S[k].target == "blueDoor" || S[k].target == "redDoor") continue;
            std::size_t fk = 0;
            while (fk < flies.size() && flies[fk] < k) ++fk;
            long wv = long(fk) + long(uni(2 * window + 1)) - window;
            if (wv < 0 || wv >= long(flies.size())) continue;
            auto at = flies[wv];
            if (at == k || at == k + 1) continue;
            std::vector<Step> block;
            if (family == 2) block.push_back(Step{Kind2::Fly, -1, {}, -1, "fly:" + floor_name(S[k].floor), S[k].origin, {}, true});
            else {
              // Insert right after the flight only if that visit is on the same floor.
              if (at + 1 >= n || S[at + 1].floor != S[k].floor) continue;
              at = at + 1;
            }
            block.push_back(S[k]);
            ns = S;
            if (at > k) { ns.insert(ns.begin() + at, block.begin(), block.end()); ns.erase(ns.begin() + k); }
            else { ns.erase(ns.begin() + k); ns.insert(ns.begin() + at, block.begin(), block.end()); }
            first = std::min(at, k);
          } else if (family == 3) { // defer the rest of a visit from a door
            auto k = uni(n);
            if (S[k].kind != Kind2::Bump || !(S[k].target == "yellowDoor" || S[k].target == "blueDoor")) continue;
            std::size_t e = k;
            while (e < n && S[e].kind != Kind2::Fly && !S[e].floor_change) ++e;
            if (e >= n || S[e].kind != Kind2::Fly) continue;
            std::size_t fe = 0;
            while (fe < flies.size() && flies[fe] < e) ++fe;
            auto wv = fe + 1 + uni(window);
            if (wv >= flies.size()) continue;
            auto j = flies[wv];
            ns.assign(S.begin(), S.begin() + k);
            ns.insert(ns.end(), S.begin() + e, S.begin() + j);
            ns.push_back(Step{Kind2::Fly, -1, {}, -1, "fly:" + floor_name(S[k].floor), S[k].origin, {}, true});
            ns.insert(ns.end(), S.begin() + k, S.begin() + e);
            ns.insert(ns.end(), S.begin() + j, S.end());
            first = k;
          } else if (family == 4) { // drop an interaction into the pool
            auto k = uni(n);
            if (!movable(k) || (S[k].kind != Kind2::Bump && S[k].kind != Kind2::Collect)) continue;
            pool_edit = propose_drop(ns, S, k);
            first = k;
          } else if (family == 5) { // restore a dropped interaction as a new mini visit
            if (pool.empty() || flies.empty()) continue;
            auto pi = uni(pool.size());
            auto at = flies[uni(flies.size())];
            pool_edit = propose_restore(ns, S, pool, pi, at);
            first = at;
          } else { // altar choice
            auto k = uni(n);
            if (S[k].kind != Kind2::Raw || !(S[k].token == "choices:0" || S[k].token == "choices:1" || S[k].token == "choices:2")) continue;
            std::size_t b = k;
            while (b > 0 && S[b - 1].kind == Kind2::Raw) --b;
            if (b == 0 || S[b - 1].kind != Kind2::Bump || S[b - 1].target != "blueShop") continue;
            ns = S;
            std::string alt;
            do alt = "choices:" + std::to_string(uni(3)); while (alt == S[k].token);
            ns[k].token = alt;
            first = k;
          }
          if (first >= n || ns.empty()) continue;
          ++evaluated;
          last_temperature = temp;
          auto r = realize(c.states[first], ns, first, costs);
          if (!r.win) continue;
          ++wins;
          const double sc = score_of(r.hp, r.gold, r.shops, rate, r.keys);
          { std::lock_guard lock(io); retain_evaluated(c, ns, first, r, sc, costs); }
          const double d = sc - c.score;
          const double u = double(rng() >> 11) / 9007199254740992.0;
          if (!(d >= 0 || (temp > 0 && u < std::exp(d / temp)))) continue;
          // Accept: rebuild caches from the first changed step.
          std::vector<Game> st;
          std::vector<Tokens> tk;
          auto rr = realize(c.states[first], ns, first, costs, &st, &tk);
          if (!rr.win) continue;
          c.states.erase(c.states.begin() + first, c.states.end());
          c.states.insert(c.states.end(), st.begin(), st.end());
          c.step_tokens.erase(c.step_tokens.begin() + first, c.step_tokens.end());
          c.step_tokens.insert(c.step_tokens.end(), tk.begin(), tk.end());
          c.steps = std::move(ns);
          c.hp = rr.hp;
          c.gold = rr.gold;
          c.shops = rr.shops;
          c.score = sc;
          pool_edit.commit(pool);
          ++accepted;
          { std::lock_guard lock(io); retain_hp(c); }
          if (sc > best + 1e-9) {
            best = sc;
            std::lock_guard lock(io);
            if (sc > global_best + 1e-9) {
              global_best = sc;
              write_route("best", c);
            }
            progress << Json{{"chain", id}, {"it", it}, {"hp", c.hp}, {"gold", c.gold}, {"score", sc}, {"global", global_best},
                             {"seconds", seconds(chain_began)}}.dump() << '\n' << std::flush;
          }
          if (it % 2000 == 0) {
            std::lock_guard lock(io);
            std::cout << Json{{"chain", id}, {"it", it}, {"temp", temp}, {"score", c.score}, {"best", best}, {"accepted", accepted},
                              {"wins", wins}}.dump() << '\n' << std::flush;
          }
        }
        std::lock_guard lock(io);
        const double elapsed = seconds(chain_began);
        const Json chain_report{{"chain", id}, {"done", true}, {"best", best}, {"accepted", accepted}, {"wins", wins},
          {"iterations", attempted}, {"evaluated", evaluated}, {"seconds", elapsed}, {"final_temperature", last_temperature},
          {"cooling_complete", evaluated > 0 && last_temperature <= t1 * (1.0 + 1e-9)}};
        save(out / ("chain-" + std::to_string(id) + ".json"), chain_report);
        std::cout << chain_report.dump() << '\n' << std::flush;
      };
      run_workers(chains, run_chain);
      finish(seconds(began));
      return 0;
    }
    const auto began = Clock::now();
    Json history = Json::array();
    for (int round = 0; round < rounds && seconds(began) < limit; ++round) {
      std::vector<Move> moves;
      const auto &S = cur.steps;
      const std::size_t n = S.size();
      std::vector<std::size_t> flies;
      for (std::size_t k = 0; k < n; ++k)
        if (S[k].kind == Kind2::Fly) flies.push_back(k);
      for (std::size_t k = 0; k < n; ++k) {
        const auto &st = S[k];
        if (st.kind != Kind2::Bump || st.floor_change) continue;
        const bool door = st.target == "yellowDoor" || st.target == "blueDoor";
        const bool next_raw = k + 1 < n && S[k + 1].kind == Kind2::Raw;
        if (door) {
          auto ns = S;
          ns.erase(ns.begin() + k);
          moves.push_back({"dropdoor-" + std::to_string(st.origin), "drop_door", std::move(ns), k});
        } else if (!next_raw && cfg.value("drop_targets", false)) {
          auto ns = S;
          ns.erase(ns.begin() + k);
          moves.push_back({"droptarget-" + std::to_string(st.origin), "drop_target", std::move(ns), k});
        }
        // Defer the rest of this visit (from k to the next flight) to a later flight.
        if (door) {
          std::size_t e = k;
          while (e < n && S[e].kind != Kind2::Fly && !S[e].floor_change) ++e;
          if (e < n && S[e].kind == Kind2::Fly && e > k) {
            int count = 0;
            for (auto j : flies) {
              if (j <= e) continue;
              if (++count > defer_horizon) break;
              std::vector<Step> ns;
              ns.insert(ns.end(), S.begin(), S.begin() + k);
              ns.insert(ns.end(), S.begin() + e, S.begin() + j);
              Step fly{Kind2::Fly, -1, {}, -1, "fly:" + floor_name(st.floor), st.origin, {}, true};
              ns.push_back(fly);
              ns.insert(ns.end(), S.begin() + k, S.begin() + e);
              ns.insert(ns.end(), S.begin() + j, S.end());
              moves.push_back({"defer-" + std::to_string(st.origin) + "-to-" + std::to_string(S[j].origin), "defer", std::move(ns), k});
            }
          }
        }
      }
      // Let the planner choose all door openings within one visit, or on one floor.
      if (cfg.value("free_doors", true)) {
        for (std::size_t v = 0; v < flies.size(); ++v) {
          const auto b = flies[v], e = v + 1 < flies.size() ? flies[v + 1] : n;
          std::vector<Step> ns(S.begin(), S.begin() + b);
          int dropped = 0;
          std::size_t first = n;
          for (std::size_t k = b; k < e; ++k) {
            const bool door = S[k].kind == Kind2::Bump && !S[k].floor_change && (S[k].target == "yellowDoor" || S[k].target == "blueDoor");
            if (door) { ++dropped; first = std::min(first, k); continue; }
            ns.push_back(S[k]);
          }
          if (dropped < 2) continue;
          ns.insert(ns.end(), S.begin() + e, S.end());
          moves.push_back({"freedoors-visit-" + std::to_string(S[b].origin), "free_doors_visit", std::move(ns), first});
        }
        std::map<int, std::vector<std::size_t>> by_floor;
        for (std::size_t k = 0; k < n; ++k)
          if (S[k].kind == Kind2::Bump && !S[k].floor_change && (S[k].target == "yellowDoor" || S[k].target == "blueDoor"))
            by_floor[S[k].floor].push_back(k);
        for (auto &[f, ks] : by_floor) {
          if (ks.size() < 2) continue;
          std::vector<Step> ns;
          std::set<std::size_t> drop(ks.begin(), ks.end());
          for (std::size_t k = 0; k < n; ++k) if (!drop.count(k)) ns.push_back(S[k]);
          moves.push_back({"freedoors-floor-" + floor_name(f), "free_doors_floor", std::move(ns), ks.front()});
        }
      }
      // Relocate whole flight visits within a window.
      const int visit_window = checked_int(cfg.value("visit_window", Json(0)));
      if (visit_window > 0) {
        for (std::size_t v = 0; v + 1 < flies.size(); ++v) {
          const auto b = flies[v], e = flies[v + 1];
          for (std::size_t w = (v > std::size_t(visit_window) ? v - visit_window : 0); w < std::min(flies.size(), v + visit_window + 2); ++w) {
            if (w == v || w == v + 1) continue;
            const auto at = flies[w];
            std::vector<Step> ns;
            if (at < b) {
              ns.insert(ns.end(), S.begin(), S.begin() + at);
              ns.insert(ns.end(), S.begin() + b, S.begin() + e);
              ns.insert(ns.end(), S.begin() + at, S.begin() + b);
              ns.insert(ns.end(), S.begin() + e, S.end());
            } else {
              ns.insert(ns.end(), S.begin(), S.begin() + b);
              ns.insert(ns.end(), S.begin() + e, S.begin() + at);
              ns.insert(ns.end(), S.begin() + b, S.begin() + e);
              ns.insert(ns.end(), S.begin() + at, S.end());
            }
            moves.push_back({"visit-" + std::to_string(S[b].origin) + "-to-" + std::to_string(S[at].origin), "visit_move", std::move(ns), std::min(at, b)});
          }
        }
      }
      // Move single interactions into other visits to the same floor.
      if (cfg.value("step_moves", false)) {
        // Visit spans on each floor: [after fly, next fly).
        std::vector<std::pair<std::size_t, std::size_t>> spans;
        for (std::size_t v = 0; v < flies.size(); ++v) spans.push_back({flies[v] + 1, v + 1 < flies.size() ? flies[v + 1] : n});
        for (std::size_t k = 0; k < n; ++k) {
          const auto &st = S[k];
          if ((st.kind != Kind2::Bump && st.kind != Kind2::MoveTo && !(cfg.value("move_collections", false) && st.kind == Kind2::Collect)) || st.floor_change) continue;
          if (k + 1 < n && S[k + 1].kind == Kind2::Raw) continue;
          for (auto [sb, se] : spans) {
            if (sb > se || (k >= sb && k < se)) continue;
            if (sb == 0 || S[sb - 1].kind != Kind2::Fly) continue;
            // The visit must be on the same floor: check the recorded floor of its first step.
            bool same = false;
            for (std::size_t q = sb; q < se; ++q) if (S[q].kind != Kind2::Raw) { same = S[q].floor == st.floor; break; }
            if (!same) continue;
            for (auto at : {sb, se}) {
              std::vector<Step> ns = S;
              Step moved = st;
              if (at > k) {
                ns.insert(ns.begin() + at, moved);
                ns.erase(ns.begin() + k);
              } else {
                ns.erase(ns.begin() + k);
                ns.insert(ns.begin() + at, moved);
              }
              moves.push_back({"step-" + std::to_string(st.origin) + "-to-" + std::to_string(at), "step_move", std::move(ns), std::min(at, k)});
            }
          }
        }
      }
      // Skip a whole room: the door and every interaction inside its static room.
      if (cfg.value("room_moves", false)) {
        static const auto labels = room_labels(initial);
        for (std::size_t k = 0; k < n; ++k) {
          const auto &st = S[k];
          if (st.kind != Kind2::Bump || !(st.target == "yellowDoor" || st.target == "blueDoor")) continue;
          const Pos door = add(st.approach, dir_delta[st.dir]);
          const Pos far = add(door, dir_delta[st.dir]);
          if (!far.valid()) continue;
          const int room = labels[st.floor][far.index()];
          if (!room || room == labels[st.floor][st.approach.index()]) continue;
          std::vector<Step> ns;
          int dropped = 0;
          bool unsafe = false;
          for (std::size_t q = 0; q < n; ++q) {
            const auto &x = S[q];
            bool inside = false;
            if (q == k) inside = true;
            else if ((x.kind == Kind2::Bump || x.kind == Kind2::MoveTo || x.kind == Kind2::RawAt) && x.floor == st.floor && q > k) {
              const Pos cell = x.kind == Kind2::Bump ? add(x.approach, dir_delta[x.dir]) : x.approach;
              inside = cell.valid() && labels[st.floor][cell.index()] == room;
            }
            if (inside) {
              if (x.floor_change || (q + 1 < n && S[q + 1].kind == Kind2::Raw)) { unsafe = true; break; }
              ++dropped;
              continue;
            }
            ns.push_back(x);
          }
          if (unsafe || dropped < 2) continue;
          moves.push_back({"room-" + std::to_string(st.origin), "drop_room", std::move(ns), k, {{st.floor, door.index()}}});
        }
      }
      // Keep one door closed for good: drop its opening and every interaction that enters or leaves through it.
      if (cfg.value("door_skips", false)) {
        for (std::size_t k = 0; k < n; ++k) {
          const auto &st = S[k];
          if (st.kind != Kind2::Bump || st.floor_change || !(st.target == "yellowDoor" || st.target == "blueDoor")) continue;
          const Pos door = add(st.approach, dir_delta[st.dir]);
          std::vector<Step> ns;
          for (std::size_t q = 0; q < n; ++q) {
            const auto &x = S[q];
            const bool through = x.floor == st.floor &&
                                 ((x.kind == Kind2::Bump && add(x.approach, dir_delta[x.dir]) == door) ||
                                  ((x.kind == Kind2::Bump || x.kind == Kind2::MoveTo) && x.approach == door));
            if (q == k || through) continue;
            ns.push_back(x);
          }
          moves.push_back({"skipdoor-" + std::to_string(st.origin) + "-" + floor_name(st.floor) + "-" + std::to_string(door.x) + "-" +
                               std::to_string(door.y),
                           "door_skip", std::move(ns), k, {{st.floor, door.index()}}});
        }
      }
      // Keep one door closed for good and defer the rest of its visit to a later flight, when another way in may be open.
      if (cfg.value("skip_defers", false)) {
        std::vector<int> offsets;
        for (const auto &oj : cfg.value("skip_defer_offsets", Json::array({1, 2, 4, 8, 16, 32, 64, 128, 256}))) offsets.push_back(oj.get<int>());
        for (std::size_t k = 0; k < n; ++k) {
          const auto &st = S[k];
          if (st.kind != Kind2::Bump || st.floor_change || st.target != "yellowDoor") continue;
          const Pos door = add(st.approach, dir_delta[st.dir]);
          std::size_t e = k;
          while (e < n && S[e].kind != Kind2::Fly && !S[e].floor_change) ++e;
          if (e >= n || S[e].kind != Kind2::Fly) continue;
          std::size_t fe = 0;
          while (fe < flies.size() && flies[fe] < e) ++fe;
          std::set<std::size_t> targets;
          for (int o : offsets) if (fe + std::size_t(o) < flies.size()) targets.insert(fe + std::size_t(o));
          if (flies.size() > 2) targets.insert(flies.size() - 2);
          for (auto wv : targets) {
            if (wv <= fe) continue;
            const auto j = flies[wv];
            std::vector<Step> ns(S.begin(), S.begin() + k);
            ns.insert(ns.end(), S.begin() + e, S.begin() + j);
            ns.push_back(Step{Kind2::Fly, -1, {}, -1, "fly:" + floor_name(st.floor), st.origin, {}, true});
            ns.insert(ns.end(), S.begin() + k + 1, S.begin() + e);
            ns.insert(ns.end(), S.begin() + j, S.end());
            moves.push_back({"skipdefer-" + std::to_string(st.origin) + "-" + floor_name(st.floor) + "-" + std::to_string(door.x) + "-" +
                                 std::to_string(door.y) + "-to-" + std::to_string(S[j].origin),
                             "skip_defer", std::move(ns), k, {{st.floor, door.index()}}});
          }
        }
      }
      // Swap a door opening for another closed door on the same floor.
      if (cfg.value("door_swaps", false)) {
        const auto &final_state = cur.states.back();
        for (std::size_t k = 0; k < n; ++k) {
          const auto &st = S[k];
          if (st.kind != Kind2::Bump || !(st.target == "yellowDoor" || st.target == "blueDoor")) continue;
          const int f = st.floor;
          for (int i = 0; i < cells; ++i) {
            const Pos d2 = cell_positions[i];
            const auto &e = entity_at(final_state, f, d2);
            if (e.kind != Kind::door || !(e.id == "yellowDoor" || e.id == "blueDoor")) continue;
            for (int dd = 0; dd < 4; ++dd) {
              const Pos ap = add(d2, dir_delta[dd]);
              if (!ap.valid() || ap.x < 1 || ap.y < 1 || ap.x > 11 || ap.y > 11) continue;
              const auto &ae = entity_at(initial, f, ap);
              if (ae.kind == Kind::wall || ae.kind == Kind::door || ae.kind == Kind::npc) continue;
              int back = dd == 0 ? 1 : dd == 1 ? 0 : dd == 2 ? 3 : 2;
              Step nb{Kind2::Bump, f, ap, back, dir_names[back], st.origin, e.id, false};
              auto ns = S;
              ns[k] = nb;
              moves.push_back({"swap-" + std::to_string(st.origin) + "-" + std::to_string(d2.x) + "-" + std::to_string(d2.y) + "-" + dir_names[back],
                               "door_swap", std::move(ns), k});
              // Also: add the other door in addition (keep the original).
              auto ns2 = S;
              ns2.insert(ns2.begin() + k, nb);
              moves.push_back({"adddoor-" + std::to_string(st.origin) + "-" + std::to_string(d2.x) + "-" + std::to_string(d2.y) + "-" + dir_names[back],
                               "door_add", std::move(ns2), k});
            }
          }
        }
      }
      // Relocate one interaction into a new flight visit before another flight.
      const int relocate_window = checked_int(cfg.value("relocate_window", Json(0)));
      if (relocate_window > 0) {
        for (std::size_t k = 0; k < n; ++k) {
          const auto &st = S[k];
          if ((st.kind != Kind2::Bump && !(cfg.value("move_collections", false) && st.kind == Kind2::Collect)) || st.floor_change) continue;
          if (k + 1 < n && S[k + 1].kind == Kind2::Raw) continue;
          if (st.target == "yellowDoor" || st.target == "blueDoor" || st.target == "redDoor" || st.target.empty()) continue;
          // Position of k among flights.
          std::size_t fk = 0;
          while (fk < flies.size() && flies[fk] < k) ++fk;
          for (std::size_t w = (fk > std::size_t(relocate_window) ? fk - relocate_window : 0); w < std::min(flies.size(), fk + relocate_window); ++w) {
            const auto at = flies[w];
            if (at == k || at == k + 1) continue;
            std::vector<Step> ns = S;
            Step fly{Kind2::Fly, -1, {}, -1, "fly:" + floor_name(st.floor), st.origin, {}, true};
            std::vector<Step> block{fly, st};
            if (at > k) {
              ns.insert(ns.begin() + at, block.begin(), block.end());
              ns.erase(ns.begin() + k);
            } else {
              ns.erase(ns.begin() + k);
              ns.insert(ns.begin() + at, block.begin(), block.end());
            }
            moves.push_back({"reloc-" + std::to_string(st.origin) + "-to-" + std::to_string(S[at].origin), "relocate", std::move(ns), std::min(at, k)});
          }
        }
      }
      // Change altar purchase choices (HP / attack / defense).
      if (cfg.value("purchase_moves", false)) {
        for (std::size_t k = 1; k < n; ++k) {
          const auto &st = S[k];
          if (st.kind != Kind2::Raw || !(st.token == "choices:0" || st.token == "choices:1" || st.token == "choices:2")) continue;
          // Only altar menus: the menu-opening bump targets a blueShop.
          std::size_t b = k;
          while (b > 0 && S[b - 1].kind == Kind2::Raw) --b;
          if (b == 0 || S[b - 1].kind != Kind2::Bump || S[b - 1].target != "blueShop") continue;
          for (auto alt : {"choices:0", "choices:1", "choices:2"}) {
            if (st.token == alt) continue;
            auto ns = S;
            ns[k].token = alt;
            moves.push_back({"shop-" + std::to_string(st.origin) + "-" + alt, "purchase", std::move(ns), k});
          }
        }
      }
      for (const auto &macro : cfg.value("macros", Json::array())) {
        // Macro steps are raw tokens executed before the named flight.
        const auto anchor = macro.at("before_token").get<std::string>();
        for (std::size_t k = n; k-- > 0;)
          if (S[k].kind == Kind2::Fly && S[k].token == anchor) {
            auto ns = S;
            std::vector<Step> add;
            for (const auto &t : macro.at("tokens")) add.push_back({Kind2::Raw, -1, {}, -1, t.get<std::string>(), S[k].origin, {}, false});
            ns.insert(ns.begin() + k, add.begin(), add.end());
            moves.push_back({"macro-" + macro.at("id").get<std::string>(), "macro", std::move(ns), k});
            break;
          }
      }
      // Dialogue wall passing: after a vanishing NPC, one optional step, then touch a neighbour.
      static const std::array<const char *, 5> first_actions = {"", "up", "down", "left", "right"};
      auto energy_variants = [&](std::vector<Step> base, std::size_t at, const std::string &id, std::size_t first) {
        for (const auto *a1 : first_actions)
          for (int d = 0; d < 4; ++d) {
            auto ns = base;
            std::vector<Step> add;
            if (*a1) add.push_back({Kind2::Raw, -1, {}, -1, a1, 0, {}, false});
            add.push_back({Kind2::Raw, -1, {}, -1, dir_names[d], 0, {}, false});
            ns.insert(ns.begin() + at, add.begin(), add.end());
            moves.push_back({id + "-" + (*a1 ? a1 : "stay") + "-" + dir_names[d], "npc_energy", std::move(ns), first});
          }
      };
      if (cfg.value("npc_energy", false)) {
        for (std::size_t k = 0; k < n; ++k) {
          const auto &st = S[k];
          if (st.kind != Kind2::Bump || !(st.target == "oldman" || st.target == "trader")) continue;
          std::size_t at = k + 1;
          while (at < n && S[at].kind == Kind2::Raw && S[at].token.starts_with("choices")) ++at;
          energy_variants(S, at, "energy-" + std::to_string(st.origin), k);
        }
      }
      // Talk to an oldman the route never visits, inside an existing visit to his floor.
      if (cfg.value("oldman_talks", false)) {
        const auto &fin = cur.states.back();
        for (int f = initial.world->first_floor; f < floors_count; ++f)
          for (int i = 0; i < cells; ++i) {
            const Pos p = cell_positions[i];
            if (entity_at(initial, f, p).id != "oldman" || entity_at(fin, f, p).id != "oldman") continue;
            for (std::size_t v = 0; v < flies.size(); ++v) {
              const auto b = flies[v], e = v + 1 < flies.size() ? flies[v + 1] : n;
              if (b + 1 >= e || S[b + 1].floor != f) continue;
              for (int d = 0; d < 4; ++d) {
                const Pos ap = add(p, dir_delta[d]);
                if (!ap.valid() || ap.x < 1 || ap.y < 1 || ap.x > 11 || ap.y > 11) continue;
                const auto &ae = entity_at(initial, f, ap);
                if (ae.kind == Kind::wall || ae.kind == Kind::npc) continue;
                const int back = d == 0 ? 1 : d == 1 ? 0 : d == 2 ? 3 : 2;
                for (auto at : {b + 1, e}) {
                  for (int with_choice = 0; with_choice < 2; ++with_choice) {
                    auto ns = S;
                    std::vector<Step> add{{Kind2::Bump, f, ap, back, dir_names[back], 0, "oldman", false}};
                    if (with_choice) add.push_back({Kind2::Raw, -1, {}, -1, "choices:0", 0, {}, false});
                    ns.insert(ns.begin() + at, add.begin(), add.end());
                    const std::string id = "talk-" + floor_name(f) + "-" + std::to_string(ap.x) + "-" + std::to_string(ap.y) + "-at-" +
                                           std::to_string(at) + (with_choice ? "-c" : "");
                    moves.push_back({id, "oldman_talk", ns, std::min<std::size_t>(at, n - 1)});
                    energy_variants(ns, at + add.size(), id, std::min<std::size_t>(at, n - 1));
                  }
                }
              }
            }
          }
      }
      // Sell yellow keys at MT28 before a flight.
      for (const auto &kj : cfg.value("key_sales", Json::array())) {
        // Either a count, or {count, floor, x, y, choices:[...]} to spend the proceeds at an altar.
        const int count = kj.is_number() ? kj.get<int>() : kj.at("count").get<int>();
        const int mt28 = 28;
        for (std::size_t w = 0; w < flies.size(); ++w) {
          const auto at = flies[w];
          if (!cur.states[at].has_visited(mt28)) continue;
          if (cur.states[at].hero[yk] < count) continue;
          auto ns = S;
          std::vector<Step> add{{Kind2::Fly, -1, {}, -1, "fly:MT28", 0, {}, true}, {Kind2::Bump, mt28, {8, 5}, 0, "up", 0, "specialTrader", false}};
          for (int c = 0; c < count; ++c) add.push_back({Kind2::Raw, -1, {}, -1, "choices:0", 0, {}, false});
          add.push_back({Kind2::Raw, -1, {}, -1, "choices:3", 0, {}, false});
          std::string id = "sell" + std::to_string(count);
          if (kj.is_object()) {
            const int f = floor_number(kj.at("floor").get<std::string>());
            if (!cur.states[at].has_visited(f)) continue;
            add.push_back({Kind2::Fly, -1, {}, -1, "fly:" + floor_name(f), 0, {}, true});
            add.push_back({Kind2::Bump, f, {kj.at("x").get<int>(), kj.at("y").get<int>()}, 0, "up", 0, "blueShop", false});
            for (const auto &c : kj.at("choices")) {
              add.push_back({Kind2::Raw, -1, {}, -1, c.get<std::string>(), 0, {}, false});
              id += "-" + c.get<std::string>();
            }
            add.push_back({Kind2::Raw, -1, {}, -1, "choices:3", 0, {}, false});
            id += "-at-" + floor_name(f);
          }
          ns.insert(ns.begin() + at, add.begin(), add.end());
          moves.push_back({id + "-before-" + std::to_string(S[at].origin), "key_sale", std::move(ns), at});
        }
      }
      // Insert an extra altar purchase before a flight.
      for (const auto &sj : cfg.value("shop_inserts", Json::array())) {
        const int f = floor_number(sj.at("floor").get<std::string>());
        const Pos ap{sj.at("x").get<int>(), sj.at("y").get<int>()};
        const auto choice = sj.at("choice").get<std::string>();
        for (std::size_t w = 0; w < flies.size(); ++w) {
          const auto at = flies[w];
          if (!cur.states[at].has_visited(f)) continue;
          if (sj.contains("from") && S[at].origin < sj.at("from").get<std::size_t>()) continue;
          if (sj.contains("to") && S[at].origin > sj.at("to").get<std::size_t>()) continue;
          auto ns = S;
          std::vector<Step> add{{Kind2::Fly, -1, {}, -1, "fly:" + floor_name(f), 0, {}, true}, {Kind2::Bump, f, ap, 0, "up", 0, "blueShop", false},
                                {Kind2::Raw, -1, {}, -1, choice, 0, {}, false}, {Kind2::Raw, -1, {}, -1, "choices:3", 0, {}, false}};
          ns.insert(ns.begin() + at, add.begin(), add.end());
          moves.push_back({"shopins-" + floor_name(f) + "-" + choice + "-before-" + std::to_string(S[at].origin), "shop_insert", std::move(ns), at});
        }
      }
      // Remove one altar purchase.
      if (cfg.value("shop_removes", false)) {
        for (std::size_t k = 1; k < n; ++k) {
          const auto &st = S[k];
          if (st.kind != Kind2::Raw || !(st.token == "choices:0" || st.token == "choices:1" || st.token == "choices:2")) continue;
          std::size_t b = k;
          while (b > 0 && S[b - 1].kind == Kind2::Raw) --b;
          if (b == 0 || S[b - 1].kind != Kind2::Bump || S[b - 1].target != "blueShop") continue;
          auto ns = S;
          ns.erase(ns.begin() + k);
          moves.push_back({"shoprm-" + std::to_string(st.origin), "shop_remove", std::move(ns), k});
        }
      }
      std::vector<Result> results(moves.size());
      std::atomic<std::size_t> next{0};
      auto work = [&] {
        for (;;) {
          auto m = next.fetch_add(1);
          if (m >= moves.size()) break;
          if (moves[m].closed.empty()) {
            results[m] = realize(cur.states[moves[m].first], moves[m].steps, moves[m].first, costs);
          } else {
            Costs local = costs;
            for (const auto &d : moves[m].closed) local.no_doors.insert(d);
            results[m] = realize(cur.states[moves[m].first], moves[m].steps, moves[m].first, local);
          }
        }
      };
      run_workers(workers, [&](int) { work(); });
      std::ofstream log(out / "rounds" / ("round-" + std::to_string(round) + ".jsonl"));
      int best = -1;
      double best_score = cur.score + 1e-9;
      std::map<std::string, int> wins, tried;
      for (std::size_t m = 0; m < moves.size(); ++m) {
        const auto &r = results[m];
        ++tried[moves[m].kind];
        double sc = r.win ? score_of(r.hp, r.gold, r.shops, rate, r.keys) : -1e18;
        log << Json{{"id", moves[m].id}, {"kind", moves[m].kind}, {"win", r.win}, {"hp", r.hp}, {"gold", r.gold}, {"keys", r.keys},
                    {"delta", r.win ? Json(sc - cur.score) : Json()}, {"error", r.error}, {"fail_step", r.fail_step}}.dump() << '\n';
        if (r.win) {
          ++wins[moves[m].kind];
          Costs local = costs;
          for (const auto &d : moves[m].closed) local.no_doors.insert(d);
          retain_evaluated(cur, moves[m].steps, moves[m].first, r, sc, local);
        }
        if (r.win && sc > best_score) { best_score = sc; best = int(m); }
      }
      Json summary = {{"round", round}, {"moves", moves.size()}, {"tried", tried}, {"wins", wins}, {"hp", cur.hp},
                      {"gold", cur.gold}, {"score", cur.score}, {"seconds", seconds(began)}};
      if (best < 0) {
        summary["stop"] = "no improving move";
        std::cout << summary.dump() << '\n' << std::flush;
        history.push_back(summary);
        break;
      }
      for (const auto &d : moves[best].closed) costs.no_doors.insert(d);
      auto rebuilt = build(initial, moves[best].steps, costs, rate, err);
      if (!rebuilt) throw std::runtime_error("Accepted move failed to rebuild: " + err);
      summary["accepted"] = {{"id", moves[best].id}, {"kind", moves[best].kind}, {"hp", rebuilt->hp}, {"gold", rebuilt->gold}, {"score", rebuilt->score}};
      std::cout << summary.dump() << '\n' << std::flush;
      history.push_back(summary);
      cur = std::move(*rebuilt);
      retain_hp(cur);
      write_route("best", cur);
      Tokens all;
      for (const auto &t : cur.step_tokens) all.insert(all.end(), t.begin(), t.end());
      save(out / "current-input.json", Json{{"schema", input.at("schema")}, {"initial", input.at("initial")}, {"actions", all},
                                             {"checkpoints", {0, all.size()}}});
      save(out / "history.json", history);
      Json closed = Json::array();
      for (const auto &[f, i] : costs.no_doors) closed.push_back({floor_name(f), i % side, i / side});
      save(out / "current-steps.json", with_floor_key_costs(Json{{"steps", steps_json(cur.steps)}, {"hp", cur.hp}, {"gold", cur.gold}, {"score", cur.score},
                                             {"no_doors", closed}}, costs));
    }
    save(out / "history.json", history);
    finish(seconds(began));
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
#endif
