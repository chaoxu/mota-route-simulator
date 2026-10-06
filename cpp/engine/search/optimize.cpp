// Candidate evaluation over original game inputs; no expected snapshots enter
// execution. Prefixes are copies of states produced by this same simulator.
#include "engine/runtime.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <numeric>
#include <set>
#include <thread>
#include <unordered_map>

using namespace engine;
using engine::io::Json;
using mota::checked_int;
using mota::read_json;
namespace {
using Clock = std::chrono::steady_clock;
using Tokens = std::vector<std::string>;
double seconds(Clock::time_point start) {
  return std::chrono::duration<double>(Clock::now() - start).count();
}
void save(const std::filesystem::path &path, const Json &value) {
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("Cannot write " + path.string());
  out << value.dump(2) << '\n';
  if (!out)
    throw std::runtime_error("Write failed: " + path.string());
}

Number item_count(const Game &g, const std::string &name) {
  auto it = g.world->item_ids.find(name);
  return it == g.world->item_ids.end() ? 0 : g.item_count(it->second);
}
Json summary(const Game &g, const engine::io::Compiler &compiler) {
  Json items = {{"tools", Json::object()}, {"constants", Json::object()}, {"equips", Json::object()}};
  for (std::size_t id = 0; id < g.world->item_names.size(); ++id)
    if (auto n = g.item_count(int(id))) {
      int cls = g.world->item_class[id];
      if (!cls)
        continue;
      items[cls == 1   ? "constants"
            : cls == 2 ? "tools"
            : cls == 3 ? "equips"
                       : "items"][g.world->item_names[id]] = n;
    }
  Json out = {{"floor", floor_name(g.hero.floor)},
              {"x", g.hero.pos.x},
              {"y", g.hero.pos.y},
              {"hp", g.hero[hp]},
              {"atk", g.hero[atk]},
              {"def", g.hero[defense]},
              {"gold", g.hero[gold]},
              {"items", items},
              {"direction", direction_name(g.facing)},
              {"dialogue_energy", g.flag_number(g.world->ids.energy)},
              {"shop_times", g.flag_number(g.world->ids.times)},
              {"event_serial", g.has_event_serial ? g.event_serial : 0},
              {"event_pending", native_events_pending(g)},
              {"healing_mode", g.flag_number(g.world->ids.addhp)},
              {"win_reason", g.has_win_reason ? Json(g.win_reason) : Json()},
              {"choice", nullptr}};
  if (g.choice) {
    const auto &choice = *g.choice;
    const auto &i = g.world->instructions[choice.instruction];
    Json options = Json::array();
    int index = 0;
    for (int n : choice.options) {
      const auto &option = compiler.sources[i.choices[n].source];
      options.push_back({{"index", index++},
                         {"text", option.value("text", Json())},
                         {"need", option.value("need", Json())}});
    }
    out["choice"] = {{"text", compiler.sources[i.source].value("text", Json(""))}, {"options", options}};
  }
  return out;
}

Json map_changes(const Game &before, const Game &after) {
  Json changes = Json::array();
  for (int f = before.world->first_floor; f < floors_count; ++f) {
    if (before.floors[f] == after.floors[f])
      continue;
    const auto &a = *before.floors[f], &b = *after.floors[f];
    for (int i = 0; i < cells; ++i) {
      auto p = cell_positions[i];
      auto x = before.block(f, p), y = after.block(f, p);
      if (x == y && a.foreground[i] == b.foreground[i] && a.background[i] == b.background[i])
        continue;
      Json c = {{"floor", floor_name(f)}, {"x", p.x}, {"y", p.y}};
      if (x != y) {
        c["from"] = before.world->entities[x.entity].id;
        c["to"] = after.world->entities[y.entity].id;
        c["tiles"] = {x.tile, y.tile};
      }
      if (a.foreground[i] != b.foreground[i])
        c["fg"] = {a.foreground[i], b.foreground[i]};
      if (a.background[i] != b.background[i])
        c["bg"] = {a.background[i], b.background[i]};
      changes.push_back(std::move(c));
    }
  }
  return changes;
}

bool victory(const Game &g) {
  return g.hero[won50] && g.win_reason == "特性版-TE（生命）" && g.flag_number(g.world->ids.addhp) != 1;
}
Json outcome(const Game &g) {
  return {{"win", victory(g)},
          {"hp", g.hero[hp]},
          {"atk", g.hero[atk]},
          {"def", g.hero[defense]},
          {"gold", g.hero[gold]},
          {"win_reason", (g.has_win_reason ? Json(g.win_reason) : Json())},
          {"normal_healing", g.flag_number(g.world->ids.addhp) != 1}};
}
struct Patch {
  std::size_t start = 0, remove = 0;
  Tokens insert;
};
struct Candidate {
  std::string id;
  std::vector<Patch> patches;
  Tokens tokens;
  std::vector<std::size_t> origins;
  std::size_t first = 0, suffix = 0;
};
struct CategoryViolation : Error {
  using Error::Error;
};

// Interpreter control flow must bypass catches for ordinary invalid game moves.
struct NativeBudgetStop : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct NativeObservationError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

class ScopedNativeBudget;
thread_local ScopedNativeBudget *native_budget = nullptr;
class ScopedNativeBudget {
  ScopedNativeBudget *previous_;
  std::uint64_t &calls_;
  const std::uint64_t &limit_;
  const Clock::time_point &deadline_;
  std::function<bool(const Game &)> category_;
public:
  // Bind the interpreter's existing counters; nested scopes restore their
  // predecessor and may bind the same shared budget without double counting.
  ScopedNativeBudget(std::uint64_t &calls, const std::uint64_t &limit,
                     const Clock::time_point &deadline,
                     std::function<bool(const Game &)> category = {})
      : previous_(native_budget), calls_(calls), limit_(limit), deadline_(deadline), category_(std::move(category)) {
    native_budget = this;
  }
  ScopedNativeBudget(const ScopedNativeBudget &) = delete;
  ScopedNativeBudget &operator=(const ScopedNativeBudget &) = delete;
  ~ScopedNativeBudget() { native_budget = previous_; }
  bool category_allowed(const Game &g) const { return !category_ || category_(g); }
  void attempt() {
    if (calls_ >= limit_) throw NativeBudgetStop("native_call_limit");
    if (Clock::now() >= deadline_) throw NativeBudgetStop("time_limit");
    ++calls_; // Includes speculative calls and calls rejected by token guards.
  }
};

class ScopedNativeWins;
thread_local ScopedNativeWins *native_wins = nullptr;
class ScopedNativeWins {
  struct Path {
    std::shared_ptr<const Path> parent;
    std::string token;
    std::size_t size;
  };
  using PathPtr = std::shared_ptr<const Path>;
  struct Reached { Game game; PathPtr path; };
  ScopedNativeWins *previous_;
  std::function<void(Game, Tokens)> observer_;
  std::unordered_map<std::uint64_t, std::vector<Reached>> reached_;
  PathPtr path_for(const Game &game) const {
    const auto bucket = reached_.find(game.hash());
    if (bucket != reached_.end())
      for (const auto &entry : bucket->second)
        if (entry.game == game) return entry.path;
    throw NativeObservationError("Observed token has no native path from the observer root");
  }
  void emit(const Game &game, PathPtr path) const {
    Tokens inputs(path ? path->size : 0);
    for (std::size_t i = inputs.size(); i; --i, path = path->parent)
      inputs[i - 1] = path->token;
    try {
      observer_(Game(game), std::move(inputs));
    } catch (...) {
      std::throw_with_nested(NativeObservationError("Native win observer failed"));
    }
  }
public:
  ScopedNativeWins(const Game &root, std::function<void(Game, Tokens)> observer)
      : previous_(native_wins), observer_(std::move(observer)) {
    if (!observer_) throw std::invalid_argument("Native win observer must be callable");
    reached_[root.hash()].push_back({root, {}});
    // Deliver a known initial win without fabricating or replaying any input.
    if (victory(root)) emit(root, {});
    native_wins = this;
  }
  ScopedNativeWins(const ScopedNativeWins &) = delete;
  ScopedNativeWins &operator=(const ScopedNativeWins &) = delete;
  ~ScopedNativeWins() { native_wins = previous_; }
  PathPtr before(const Game &game) const { return path_for(game); }
  void committed(const Game &game, const std::string &token, PathPtr parent) {
    auto path = std::make_shared<Path>(Path{parent, token, parent ? parent->size + 1 : 1});
    // Observe every legal winning input before exact-state or heuristic filters.
    // A later enclosing transaction may discard this branch; its root-relative
    // native path is nevertheless a complete, independently playable witness.
    if (victory(game)) emit(game, path);
    auto &bucket = reached_[game.hash()];
    for (auto &entry : bucket)
      if (entry.game == game) {
        if (!entry.path || entry.path->size <= path->size) return;
        entry.path = std::move(path);
        return;
      }
    bucket.push_back({game, std::move(path)});
  }
};

void checked_token(Game &g, const std::string &token, bool strict_steps = true) {
  if (native_budget) native_budget->attempt();
  if (native_budget && !native_budget->category_allowed(g))
    throw CategoryViolation("Candidate excludes required category");
  const auto observed_parent = native_wins ? native_wins->before(g) : nullptr;
  auto scope = g.transaction();
  if (g.hero[won50])
    throw Error("Input token occurs after victory");
  if (g.flag_number(g.world->ids.addhp) == 1)
    throw CategoryViolation("Candidate enabled healing-mode combat");
  try {
    g.token(decode_token(*g.world, token), strict_steps);
  } catch (const Unsupported &) {
    throw;
  } catch (const Error &e) {
    std::string message = e.what();
    if (message == "Illegal direct movement")
      message = "No legal direct-movement path";
    if (message == "Cannot win battle")
      message = "Cannot win target battle";
    if (message == "Lethal hazard")
      message = "Cannot enter lethal hazard";
    throw Error(message);
  }
  if (g.flag_number(g.world->ids.addhp) == 1)
    throw CategoryViolation("Candidate enabled healing-mode combat");
  if (native_budget && !native_budget->category_allowed(g))
    throw CategoryViolation("Candidate excludes required category");
  scope.commit();
  if (native_wins) native_wins->committed(g, token, observed_parent);
}

std::string artifact_id(const std::string &id) {
  auto key = id;
  for (char &c : key)
    if (c >= 'A' && c <= 'Z')
      c = char(c - 'A' + 'a');
  return key;
}
Candidate candidate(const Json &raw, const Tokens &base) {
  Candidate c;
  c.id = raw.at("id").get<std::string>();
  if (c.id.empty() || c.id.size() > 120 ||
      c.id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.") !=
          std::string::npos ||
      c.id == "." || c.id == "..")
    throw std::runtime_error("Unsafe candidate id");
  const auto &edits = raw.at("edits");
  if (!edits.is_array())
    throw std::runtime_error("Candidate edits must be an array: " + c.id);
  for (const auto &edit : edits) {
    auto index = checked_int(edit.at("start")), count = checked_int(edit.value("delete", Json(0)));
    if (index < 0 || count < 0 || std::size_t(index) > base.size() ||
        std::size_t(count) > base.size() - std::size_t(index))
      throw std::runtime_error("Patch outside baseline actions: " + c.id);
    c.patches.push_back({std::size_t(index), std::size_t(count), edit.value("insert", Tokens{})});
  }
  std::stable_sort(c.patches.begin(), c.patches.end(),
                   [](const auto &a, const auto &b) { return a.start < b.start; });
  std::size_t cursor = 0;
  c.first = c.patches.empty() ? base.size() : c.patches.front().start;
  auto append_original = [&](std::size_t until) {
    for (; cursor < until; ++cursor) {
      c.tokens.push_back(base[cursor]);
      c.origins.push_back(cursor);
    }
  };
  for (const auto &p : c.patches) {
    if (p.start < cursor)
      throw std::runtime_error("Overlapping candidate patches: " + c.id);
    append_original(p.start);
    for (const auto &token : p.insert) {
      c.tokens.push_back(token);
      c.origins.push_back(base.size() + 1);
    }
    cursor += p.remove;
    c.suffix = c.tokens.size();
  }
  append_original(base.size());
  return c;
}
void walk_instead_of_direct(Game &g, const std::string &token, Tokens &walk) {
  auto split = token.find(':', 5);
  if (split == std::string::npos)
    throw Error("Malformed direct move for repair");
  Pos target{std::stoi(token.substr(5, split - 5)), std::stoi(token.substr(split + 1))};
  int floor = g.hero.floor;
  auto path = g.path_to(floor, target);
  if (path.size() > cells)
    throw Error("Repair path exceeds floor size");
  for (auto p : path) {
    if (g.hero.pos == p)
      continue;
    for (int attempt = 0; attempt < 2 && g.hero.pos != p; ++attempt) {
      if (g.hero.floor != floor || native_events_pending(g))
        throw Error("Repair entered another floor or a menu");
      int dx = p.x - g.hero.pos.x, dy = p.y - g.hero.pos.y;
      if (std::abs(dx) + std::abs(dy) != 1)
        throw Error("Repair path lost adjacency");
      std::string direction = dx < 0 ? "left" : dx > 0 ? "right" : dy < 0 ? "up" : "down";
      checked_token(g, direction);
      walk.push_back(direction);
    }
    if (g.hero.floor != floor || g.hero.pos != p)
      throw Error("Repair step failed to reach its target");
  }
  if (g.hero.floor != floor || g.hero.pos != target)
    throw Error("Repair did not reach the direct-move destination");
}

void recorded_move(Game &g, Pos target, Tokens &route) {
  auto token = "move:" + std::to_string(target.x) + ":" + std::to_string(target.y);
  try {
    checked_token(g, token);
    route.push_back(token);
  } catch (const Unsupported &) {
    throw;
  } catch (const CategoryViolation &) {
    throw;
  } catch (const Error &) {
    walk_instead_of_direct(g, token, route);
  }
}

bool directional_input(const std::string &token) {
  return token == "up" || token == "down" || token == "left" || token == "right";
}
void visit_item(Game &g, int floor, Pos target, Tokens &route) {
  if (g.hero.floor != floor) {
    auto token = "fly:" + floor_name(floor);
    checked_token(g, token);
    route.push_back(token);
  }
  recorded_move(g, target, route);
}
void return_and_retry(Game &g, int floor, Pos origin, const std::string &direction, Tokens &route) {
  visit_item(g, floor, origin, route);
  checked_token(g, direction);
  route.push_back(direction);
}
Number keys_spent(const Game &before, const Game &after, const std::string &except = {}) {
  Number count = 0;
  for (const auto &key : {"yellowKey", "blueKey", "redKey"})
    if (key != except)
      count += std::max(Number(0), item_count(before, key) - item_count(after, key));
  return count;
}
bool fetch_missing_key(Game &g, const std::string &direction, Tokens &route) {
  Pos delta;
  if (direction == "up")
    delta = {0, -1};
  else if (direction == "down")
    delta = {0, 1};
  else if (direction == "left")
    delta = {-1, 0};
  else if (direction == "right")
    delta = {1, 0};
  else
    return false;
  const auto origin = g.hero.pos;
  const int origin_floor = g.hero.floor;

  const auto &door = g.props(origin_floor, {origin.x + delta.x, origin.y + delta.y});
  if (!door.door)
    return false;
  std::string needed;
  Number amount = 0;
  for (const auto &key : door.keys)
    if (g.item_count(key.item) < key.count) {
      needed = g.world->item_names.at(key.item);
      amount = key.count;
      break;
    }

  if (needed.empty())
    return false;
  std::unique_ptr<Game> best;
  Tokens best_route;
  Number best_damage = infinite_damage, best_other_keys = infinite_damage;
  for (int floor = g.world->first_floor; floor < floors_count; ++floor) {
    if (floor != origin_floor && (!g.has_visited(floor) || !g.can_fly(floor)))
      continue;
    for (int index = 0; index < cells; ++index) {
      Pos target{index % side, index / side};
      if (g.world->entities[g.block(floor, target).entity].id != needed)
        continue;
      auto trial = g;
      Tokens proposed;
      try {
        visit_item(trial, floor, target, proposed);
        if (item_count(trial, needed) < amount)
          continue;
        return_and_retry(trial, origin_floor, origin, direction, proposed);
        Number damage = trial.hero[dmg] - g.hero[dmg];
        Number other_keys = keys_spent(g, trial, needed);
        if (!best || other_keys < best_other_keys ||
            (other_keys == best_other_keys &&
             (damage < best_damage || (damage == best_damage && proposed.size() < best_route.size())))) {
          best = std::make_unique<Game>(std::move(trial));
          best_route = std::move(proposed);
          best_damage = damage;
          best_other_keys = other_keys;
        }
      } catch (const Error &) {
      }
    }
  }
  if (!best)
    return false;
  g = std::move(*best);
  route = std::move(best_route);
  return true;
}
bool fetch_survival_potion(Game &g, const std::string &direction, Tokens &route, Json &detail) {
  if (!directional_input(direction))
    return false;
  constexpr std::size_t max_candidates = 128;
  const int origin_floor = g.hero.floor;
  const auto origin = g.hero.pos;
  // Prefer current-floor candidates when the explicit finite budget is reached.
  std::vector<int> floors{origin_floor};
  for (int floor = g.world->first_floor; floor < floors_count; ++floor)
    if (floor != origin_floor && g.has_visited(floor) && g.can_fly(floor))
      floors.push_back(floor);
  std::unique_ptr<Game> best;
  Tokens best_route;
  Json selected;
  Number best_keys = infinite_damage, best_hp = infinite_damage;
  std::size_t attempted = 0;
  for (int floor : floors) {
    for (int index = 0; index < cells && attempted < max_candidates; ++index) {
      Pos target{index % side, index / side};
      const auto &id = g.world->entities[g.block(floor, target).entity].id;
      // These are immediate HP pickups. Holy water remains an inventory tool.
      if (id != "redPotion" && id != "bluePotion" && id != "greenPotion" && id != "yellowPotion")
        continue;
      ++attempted;
      auto trial = g;
      Tokens proposed;
      try {
        visit_item(trial, floor, target, proposed);
        visit_item(trial, origin_floor, origin, proposed);
        Number ready_hp = trial.hero[hp];
        if (ready_hp <= g.hero[hp])
          continue;
        checked_token(trial, direction);
        proposed.push_back(direction);
        auto lost_keys = keys_spent(g, trial);
        if (!best || lost_keys < best_keys ||
            (lost_keys == best_keys &&
             (ready_hp < best_hp || (ready_hp == best_hp && proposed.size() < best_route.size())))) {
          selected = {{"floor", floor_name(floor)},  {"x", target.x},        {"y", target.y}, {"item", id},
                      {"hp_before_retry", ready_hp}, {"key_loss", lost_keys}};
          best = std::make_unique<Game>(std::move(trial));
          best_route = std::move(proposed);
          best_keys = lost_keys;
          best_hp = ready_hp;
        }
      } catch (const Error &) {
      } // Includes category violations; speculative branch is discarded.
    }
    if (attempted == max_candidates)
      break;
  }
  if (!best)
    return false;
  detail = {{"selected", selected}, {"candidates_tried", attempted}, {"candidate_limit", max_candidates}};
  g = std::move(*best);
  route = std::move(best_route);
  return true;
}
Json evaluate(const Candidate &c, const std::vector<Game> &prefix, std::size_t base_size, bool repair_direct,
              bool repair_keys, Tokens &realized, bool repair_survival = false, bool strict_steps = true,
              Game *ending = nullptr, bool adapt_open_doors = false) {
  auto started = Clock::now();
  Game g = prefix.at(c.first);
  realized.assign(c.tokens.begin(), c.tokens.begin() + c.first);
  Json result = {{"id", c.id},
                 {"prefix_actions", c.first},
                 {"action_count", c.tokens.size()},
                 {"executed_actions", 0},
                 {"complete", false},
                 {"reused_suffix", false},
                 {"movement_repairs", Json::array()}};
  std::size_t i = c.first, executed = 0;
  for (; i < c.tokens.size(); ++i) {
    if (g.hero[won50]) {
      result["classification"] = "rejected";
      result["error"] = "Input token occurs after victory";
      break;
    }
    if (i >= c.suffix && c.origins[i] < base_size && g == prefix[c.origins[i]]) {
      realized.insert(realized.end(), c.tokens.begin() + i, c.tokens.end());
      g = prefix.back();
      result["reused_suffix"] = true;
      i = c.tokens.size();
      break;
    }
    try {
      if (adapt_open_doors && directional_input(c.tokens[i]) && c.origins[i] < base_size) {
        const auto origin = c.origins[i];
        const auto &before = prefix[origin], &after = prefix[origin + 1];
        auto target = before.hero.pos;
        if (c.tokens[i] == "up") --target.y;
        if (c.tokens[i] == "down") ++target.y;
        if (c.tokens[i] == "left") --target.x;
        if (c.tokens[i] == "right") ++target.x;
        if (target.valid() && before.hero.floor == after.hero.floor && before.hero.pos == after.hero.pos &&
            g.hero.floor == before.hero.floor && g.hero.pos == before.hero.pos &&
            before.world->entities[before.block(before.hero.floor, target).entity].kind == Kind::door &&
            after.world->entities[after.block(after.hero.floor, target).entity].kind == Kind::empty &&
            g.world->entities[g.block(g.hero.floor, target).entity].kind == Kind::empty) {
          result["movement_repairs"].push_back({{"proposed_index", i}, {"origin", origin},
            {"kind", "already_open_door"}, {"replaced", c.tokens[i]}, {"inserted", Tokens{}}});
          continue;
        }
      }
      bool check_door_key = false;
      if (repair_keys && directional_input(c.tokens[i])) {
        auto target = g.hero.pos;
        if (c.tokens[i] == "up") --target.y;
        if (c.tokens[i] == "down") ++target.y;
        if (c.tokens[i] == "left") --target.x;
        if (c.tokens[i] == "right") ++target.x;
        check_door_key = target.valid() && g.world->entities[g.block(g.hero.floor, target).entity].kind == Kind::door;
      }
      auto proposed = g.transaction();
      Tokens replacement;
      Json repair;
      try {
        checked_token(g, c.tokens[i], strict_steps || check_door_key);
        replacement.push_back(c.tokens[i]);
      } catch (const Error &error) {
        bool direct = repair_direct && c.tokens[i].starts_with("move:") &&
                      std::string(error.what()) == "No legal direct-movement path";
        bool key = repair_keys && std::string(error.what()) == "Missing door key";
        bool survival = repair_survival && directional_input(c.tokens[i]) &&
                        (std::string(error.what()) == "Cannot win target battle" ||
                         std::string(error.what()) == "Cannot enter lethal hazard");
        if (!direct && !key && !survival)
          throw;

        Json survival_detail;
        if (direct)
          walk_instead_of_direct(g, c.tokens[i], replacement);
        else if (key) {
          if (!fetch_missing_key(g, c.tokens[i], replacement))
            throw;
        } else if (!fetch_survival_potion(g, c.tokens[i], replacement, survival_detail))
          throw;
        repair = {{"proposed_index", i},
                  {"origin", c.origins[i]},
                  {"kind", direct ? "walking"
                           : key  ? "key_detour"
                                  : "survival_detour"},
                  {"replaced", c.tokens[i]},
                  {"inserted", replacement}};
        if (survival)
          repair["survival"] = std::move(survival_detail);
      }
      // A proposed input (including its whole replacement) commits atomically.
      // Failed speculative paths must never affect score or the recorded prefix.
      proposed.commit();
      realized.insert(realized.end(), replacement.begin(), replacement.end());
      executed += replacement.size();
      if (!repair.is_null())
        result["movement_repairs"].push_back(std::move(repair));
    } catch (const CategoryViolation &e) {
      result["classification"] = "wrong_category";
      result["error"] = e.what();
      break;
    } catch (const Unsupported &e) {
      result["classification"] = "unsupported";
      result["error"] = e.what();
      break;
    } catch (const Error &e) {
      result["classification"] = "rejected";
      result["error"] = e.what();
      break;
    } catch (const NativeBudgetStop &) {
      throw;
    } catch (const NativeObservationError &) {
      throw;
    } catch (const std::exception &e) {
      result["classification"] = "error";
      result["error"] = e.what();
      break;
    }
  }
  result["complete"] = i == c.tokens.size();
  result["applied_actions"] = realized.size();
  result["next_proposed_index"] = i;
  result["action_count"] = realized.size() + c.tokens.size() - i;
  result["outcome_boundary"] = "committed_prefix";
  result["proposed_action_count"] = c.tokens.size();
  if (i == c.tokens.size()) {
    result["action_count"] = realized.size();
    result["applied_actions"] = realized.size();
  }
  result["executed_actions"] = executed;
  if (i < c.tokens.size()) {
    result["failure_token"] = c.tokens[i];
    result["failure_origin"] = c.origins[i];
  }
  result.update(outcome(g));
  if (!result["complete"].get<bool>())
    result["win"] = false;
  if (ending) *ending = g;
  result["wall_seconds"] = seconds(started);
  return result;
}
// Cash proposes a purchase count. Every shop input must still replay legally.
[[maybe_unused]] Json try_late_hp_purchases(const std::string &id, const Tokens &base,
    const std::vector<Game> &prefix, const Game &ending, bool repair_direct, Json &result, Tokens &realized) {
  if (!result.at("win").get<bool>()) return nullptr;
  int k = int(ending.flag_number(ending.world->ids.times)), extra = 0;
  Number cash = ending.hero[gold] - 500;
  Tokens shopping = {"fly:MT46", "move:6:2", "up"};
  while (k < 64 && cash >= 20 + 10 * k * (k + 1)) {
    cash -= 20 + 10 * k * (k + 1); ++k; ++extra; shopping.push_back("choices:0");
  }
  if (!extra) return nullptr;
  shopping.push_back("choices:3");
  const auto at = std::find_if(realized.rbegin(), realized.rend(), [](const auto &t) {return t.starts_with("fly:");});
  if (at == realized.rend()) return nullptr;
  const auto position = realized.size() - 1 - std::size_t(at - realized.rbegin());
  Candidate augmented; augmented.id = id + "-hp"; augmented.tokens = realized;
  augmented.tokens.insert(augmented.tokens.begin() + position, shopping.begin(), shopping.end());
  augmented.origins.assign(augmented.tokens.size(), base.size() + 1); augmented.suffix = augmented.tokens.size();
  while (augmented.first < base.size() && augmented.first < augmented.tokens.size() && base[augmented.first] == augmented.tokens[augmented.first]) ++augmented.first;
  Tokens concrete;
  auto r = evaluate(augmented, prefix, base.size(), repair_direct, false, concrete, false, false);
  Json attempt = {{"proposed_purchases", extra}, {"before_realized_index", position}, {"tokens", shopping}, {"result", r}};
  if (r.at("win").get<bool>() && r.at("hp").get<Number>() > result.at("hp").get<Number>()) {
    result = std::move(r); realized = std::move(concrete);
  }
  return attempt;
}
Tokens concrete_route(const Candidate &c, const Tokens &realized, const Json &result) {
  auto route = realized;
  auto next = result.at("next_proposed_index").get<std::size_t>();
  route.insert(route.end(), c.tokens.begin() + next, c.tokens.end());
  return route;
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 4 && argc != 5) {
      std::cerr << "Usage: mota-native-optimize DATA_DIRECTORY INPUT_JSON OUTPUT_DIRECTORY [BATCH_JSON]\n";
      return 2;
    }
    auto input = read_json(argv[2]);
    if (input.value("schema", std::string()) != "mota.cpp.native-replay-input.v1" ||
        input.at("initial").value("model_mode", std::string()) != "native-corrected")
      throw std::runtime_error("Expected one native initial state plus original input tokens");
    const auto tokens = input.at("actions").get<Tokens>();
    auto output = std::filesystem::absolute(argv[3]);
    std::filesystem::create_directories(output / "routes");
    auto load_start = Clock::now();
    auto source_world = mota::World::load(argv[1]);
    engine::io::Compiler compiler(source_world);
    auto world = compiler.world;
    const auto &nativeFloors = input.at("initial").at("floors");
    for (int floor = world->first_floor; floor < floors_count; ++floor) {
      const auto id = floor_name(floor);
      if (!nativeFloors.contains(id) || !nativeFloors.at(id).contains("retained_blocks") ||
          !nativeFloors.at(id).contains("retained_block_order"))
        throw std::runtime_error("Native replay/search requires complete retained block state; regenerate "
                                 "input from the original initial capture");
    }
    auto reference_initial = mota::Game::from_reference(source_world, input.at("initial"));
    Game initial = make_native_game(compiler, input.at("initial"));
    double load_seconds = seconds(load_start);
    std::vector<Game> prefix;
    prefix.reserve(tokens.size() + 1);
    prefix.push_back(initial);
    auto run_start = Clock::now();
    for (std::size_t i = 0; i < tokens.size(); ++i) {
      auto g = prefix.back();
      try {
        checked_token(g, tokens[i], false);
      } catch (const std::exception &e) {
        throw std::runtime_error("Baseline token " + std::to_string(i) + " " + tokens[i] + ": " + e.what());
      }
      prefix.push_back(std::move(g));
    }
    double replay_seconds = seconds(run_start);
    Json benchmark = outcome(prefix.back());
    benchmark.update({{"schema", "mota.native-benchmark.v1"},
                      {"actions", tokens.size()},
                      {"load_seconds", load_seconds},
                      {"replay_with_prefix_clones_seconds", replay_seconds},
                      {"tokens_per_second", tokens.size() / replay_seconds},
                      {"full_map_exports", 0},
                      {"simulator", native_runtime_info(initial)},
                      {"baseline_input", std::filesystem::absolute(argv[2]).string()}});
    save(output / "baseline.json", benchmark);
    std::cout << benchmark.dump() << '\n' << std::flush;
    auto trace_start = Clock::now();
    std::ofstream trace(output / "trace.jsonl");
    if (!trace)
      throw std::runtime_error("Cannot create compact trace");
    auto previous = summary(prefix.front(), compiler);
    trace << Json{{"applied", 0}, {"state", previous}}.dump() << '\n';
    for (std::size_t i = 0; i < tokens.size(); ++i) {
      auto state = summary(prefix[i + 1], compiler);
      trace << Json{{"applied", i + 1},
                    {"token", tokens[i]},
                    {"state", state},
                    {"changes", Json::diff(previous, state)},
                    {"map_changes", map_changes(prefix[i], prefix[i + 1])}}
                   .dump()
            << '\n';
      previous = std::move(state);
    }
    trace.close();
    if (!trace)
      throw std::runtime_error("Trace write failed");
    double trace_seconds = seconds(trace_start);
    if (argc == 5) {
      auto batch = read_json(argv[4]);
      if (batch.value("schema", std::string()) != "mota.native-candidates.v1")
        throw std::runtime_error("Unsupported candidate schema");
      std::ofstream results(output / "results.jsonl");
      if (!results)
        throw std::runtime_error("Cannot create results");
      Json best = outcome(prefix.back());
      best["id"] = "baseline";
      std::size_t count = 0;
      auto batch_start = Clock::now();
      int workers = checked_int(batch.value("workers", Json(1)));
      if (workers < 1 || workers > 64)
        throw std::runtime_error("workers must be between1 and64");
      if (!batch.at("candidates").is_array())
        throw std::runtime_error("Batch candidates must be an array");
      std::vector<Candidate> candidates;
      std::set<std::string> ids;
      for (const auto &raw : batch.at("candidates")) {
        auto c = candidate(raw, tokens);
        if (!ids.insert(artifact_id(c.id)).second)
          throw std::runtime_error("Candidate ids collide as artifact filenames: " + c.id);
        candidates.push_back(std::move(c));
      }
      std::atomic<std::size_t> next{0};
      std::atomic<bool> stop{false};
      std::mutex output_mutex;
      std::exception_ptr failure;
      auto work = [&]() {
        try {
          while (!stop.load()) {
            auto i = next.fetch_add(1);
            if (i >= candidates.size())
              break;
            const auto &c = candidates[i];
            Tokens realized;
            auto result = evaluate(c, prefix, tokens.size(), batch.value("repair_direct_moves", false),
                                   batch.value("repair_missing_keys", false), realized,
                                   batch.value("repair_survival", false), batch.value("strict_steps", true),
                                   nullptr, batch.value("adapt_open_doors", false));
            save(output / "routes" / (c.id + ".json"),
                 Json{{"schema", "mota.native-candidate-route.v1"},
                      {"baseline_input", std::filesystem::absolute(argv[2]).string()},
                      {"edits", batch.at("candidates")[i].at("edits")},
                      {"actions", concrete_route(c, realized, result)},
                      {"proposed_actions", c.tokens},
                      {"result", result}});
            std::lock_guard lock(output_mutex);
            results << result.dump() << '\n';
            results.flush();
            std::cout << result.dump() << '\n' << std::flush;
            if (!results)
              throw std::runtime_error("Candidate result write failed");
            if (result["win"].get<bool>() &&
                (!best["win"].get<bool>() || result["hp"].get<Number>() > best["hp"].get<Number>()))
              best = result;
            ++count;
          }
        } catch (...) {
          stop.store(true);
          std::lock_guard lock(output_mutex);
          if (!failure)
            failure = std::current_exception();
        }
      };
      {
        std::vector<std::jthread> threads;
        for (int i = 0; i < workers; ++i)
          threads.emplace_back(work);
      }
      if (failure)
        std::rethrow_exception(failure);
      save(output / "batch-summary.json", Json{{"candidates", count},
                                               {"wall_seconds", seconds(batch_start)},
                                               {"workers", workers},
                                               {"trace_seconds", trace_seconds},
                                               {"best", best}});
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
