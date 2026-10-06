#include "engine/bridge.hpp"
#include "engine/graph.hpp"
#include <algorithm>

namespace engine::io {
static Pos position(const std::string &s) {
  auto c = s.find(',');
  return {std::stoi(s.substr(0, c)), std::stoi(s.substr(c + 1))};
}
static Json composed_payload(const mota::World &w, int f, mota::Pos p, mota::Block b, bool destroyed) {
  Json out =
      b.entity ? w.entities[b.entity].native : Json{{"cls", "terrains"}, {"id", "none"}, {"noPass", false}};
  out.erase("canPass");
  const auto &e = w.entities[b.entity];
  if (e.kind == mota::Kind::enemy) {
    const auto &enemies = w.native_data.at("enemies");
    if (enemies.contains(e.id) && enemies[e.id].contains("faceIds") && !enemies[e.id]["faceIds"].is_null())
      out["faceIds"] = enemies[e.id]["faceIds"];
  }
  auto merge = [&](Json v) {
    if (v.is_null())
      return;
    if (v.is_string())
      v = {{"data", Json::array({v})}};
    else if (v.is_array())
      v = {{"data", v}};
    if (!v.is_object())
      return;
    if (!v.contains("data") || v["data"].is_null())
      v["data"] = Json::array();
    for (auto it = v.begin(); it != v.end(); ++it)
      if (it.key() != "enable" && it.key() != "animate" && it.key() != "opacity" && it.key() != "filter" &&
          !it->is_null())
        out[it.key()] = it.value();
    if (!out.contains("trigger") || out["trigger"].is_null() || out["trigger"] == "")
      out["trigger"] = "action";
  };
  const auto key = std::to_string(p.x) + "," + std::to_string(p.y);
  if (!destroyed) {
    merge(w.floors[f].events.value(key, Json()));
    if (w.floors[f].change_floor.contains(key))
      merge(Json{{"trigger", "changeFloor"}, {"data", w.floors[f].change_floor[key]}});
  }
  return out;
}
void Compiler::seal_identities() {
  auto &w = *world;
  auto pair = [&](Json raw) {
    auto normal = identity_id(raw);
    if (raw.is_object() && raw.contains("event"))
      raw["event"] = compiled_source(raw["event"]);
    return std::array<std::uint32_t, 2>{normal, identity_id(raw)};
  };
  for (std::size_t i = w.payload_identity.size(); i < payload_sources.size(); ++i)
    w.payload_identity.push_back(pair(payload_sources[i]));
  if (w.base_identity.empty()) {
    for (std::size_t e = 0; e < w.entities.size(); ++e)
      w.base_identity.push_back(pair(e ? reference_world->entities[e].native
                                       : Json{{"cls", "terrains"}, {"id", "none"}, {"noPass", false}}));
    std::vector<std::pair<int, Pos>> origins(w.composition.size() / w.entities.size(),
                                             {w.first_floor, {0, 0}});
    for (int f = w.first_floor; f < floors_count; ++f)
      for (int i = 0; i < cells; ++i)
        if (int overlay = w.floors[f].source_payload[i])
          origins[overlay] = {f, cell_positions[i]};
    w.composed_identity.resize(w.composed.size());
    for (std::size_t overlay = 0; overlay < origins.size(); ++overlay)
      for (std::size_t e = 0; e < w.entities.size(); ++e) {
        auto [f, p] = origins[overlay];
        mota::Block b{std::uint16_t(e), 0};
        w.composed_identity[w.composition[overlay * w.entities.size() + e] - 1] =
            pair(composed_payload(*reference_world, f, {p.x, p.y}, b, overlay == 0));
      }
  }
  for (std::size_t i = w.source_identity.size(); i < sources.size(); ++i)
    w.source_identity.push_back(identity_id(sources[i]));
  for (std::size_t i = w.menu_part_identity.size(); i < sources.size(); ++i) {
    const auto &raw = sources[i];
    w.menu_part_identity.push_back(
        identity_id(Json{{"text", raw.is_object() ? raw.value("text", Json()) : Json()},
                         {"need", raw.is_object() ? raw.value("need", Json()) : Json()}}));
  }
  for (std::size_t i = w.choice_text_identity.size(); i < w.instructions.size(); ++i) {
    const auto &raw = sources[w.instructions[i].source];
    w.choice_text_identity.push_back(identity_id(raw.is_object() ? raw.value("text", Json("")) : Json("")));
  }
  w.identities_ready = true;
}
Game Compiler::import(const mota::Game &src) {
  auto &w = *world;
  for (auto it = src.flags->value.begin(); it != src.flags->value.end(); ++it)
    intern_flag(w, it.key());
  Game out(world);
  out.flags->scalar.resize(w.flag_names.size());
  out.flags->extra.resize(w.extra_names.size());
  out.hero.values = src.hero.values;
  out.hero.floor = src.hero.floor;
  out.hero.pos = {src.hero.pos.x, src.hero.pos.y};
  out.mdef_present = src.mdef_present;
  out.facing = direction(src.extra_state->value.value("direction", std::string("down")));
  out.automatic = {src.automatic.battle, src.automatic.wall, src.automatic.item};
  out.visited = src.visited;
  const auto &flags = src.flags->value;
  for (auto it = flags.begin(); it != flags.end(); ++it)
    out.store_flag(w.flag_ids.at(it.key()), value(it.value()));
  for (std::size_t i = 0; i < w.extra_names.size(); ++i) {
    auto it = src.extra_state->value.find(w.extra_names[i]);
    if (it != src.extra_state->value.end())
      out.flags->extra[i] = value(*it);
  }
  for (std::size_t i = 0; i < w.item_names.size(); ++i)
    if (w.item_stat[i] < 0)
      out.flags->inventory[i] = src.item_count(w.item_names[i]);
  out.has_auto = flags.contains("__auto__");
  out.has_aed = flags.contains("__aed__");
  out.has_aei = flags.contains("__aei__");
  out.has_visited_record = flags.contains("__visited__");
  out.has_leave = flags.contains("__leaveLoc__");
  out.has_enemy_info = flags.contains("enemyInfo");
  auto read_auto = [&](const char *name, auto &dest) {
    if (!flags.contains(name))
      return;
    for (const auto &s : flags.at(name)) {
      auto it = std::find_if(w.automatic.begin(), w.automatic.end(),
                             [&](const auto &e) { return e.symbol == s.get<std::string>(); });
      if (it == w.automatic.end())
        throw Unsupported("Unknown automatic-event symbol in checkpoint");
      dest.push_back(int(it - w.automatic.begin()));
    }
  };
  read_auto("__aed__", out.auto_executed);
  read_auto("__aei__", out.auto_executing);
  if (flags.contains("__visited__"))
    for (auto it = flags["__visited__"].begin(); it != flags["__visited__"].end(); ++it)
      if (it.value() == true)
        out.visited_flags.set(floor_number(it.key()));
  if (flags.contains("__leaveLoc__"))
    for (auto it = flags["__leaveLoc__"].begin(); it != flags["__leaveLoc__"].end(); ++it) {
      const auto &j = it.value();
      out.leave[floor_number(it.key())] =
          Leave{{j.at("x"), j.at("y")}, direction(j.at("direction").get<std::string>())};
    }
  for (int f = w.first_floor; f < floors_count; ++f) {
    out.floors[f] = std::make_shared<FloorState>();
    auto &fs = *out.floors[f];
    const auto &from = *src.floors[f];
    fs.foreground = from.foreground;
    fs.background = from.background;
    for (auto p : from.block_order)
      fs.order.push_back({p.x, p.y});
    if (flags.contains("__disabled__") && flags["__disabled__"].contains(floor_name(f))) {
      const auto &lists = flags["__disabled__"][floor_name(f)];
      fs.disabled_record = true;
      auto seed = std::make_shared<ObservationLog::Orders>();
      for (int i : lists.at(0))
        seed->disabled.push_back(i);
      for (int i : lists.at(1))
        seed->enabled.push_back(i);
      fs.observations.seed = std::move(seed);
    }
    for (bool foreground : {false, true}) {
      const char *key = foreground ? "__fgv__" : "__bgv__";
      if (flags.contains(key) && flags[key].contains(floor_name(f))) {
        (foreground ? fs.foreground_record : fs.background_record) = true;
        for (const auto &r : flags[key][floor_name(f)])
          (foreground ? fs.foreground_changes : fs.background_changes)
              .push_back({{r.at(0), r.at(1)}, r.at(2)});
      }
    }
    for (int i = 0; i < cells; ++i) {
      auto &c = fs.cell[i];
      c.retained = {from.retained[i].entity, from.retained[i].tile};
      c.present = from.retained_present[i];
      c.disabled = from.disabled[i];
      c.event_entry = from.event_entries[i];
      auto key = std::to_string(i % side) + "," + std::to_string(i / side);
      c.source_destroyed = src.flag_truth("R" + floor_name(f) + "," + key);
      if (c.present) {
        auto raw = src.retained_block_event(f, {i % side, i / side});
        c.payload = int(w.payloads.size());
        w.payloads.push_back(properties(raw));
        payload_sources.push_back(std::move(raw));
      }
    }
  }
  const auto &extra = src.extra_state->value;
  const auto globals = extra.value("game_flags", reference_world->native_data.at("flags"));
  out.allow_direct = rule_truth(value(globals.value("enableMoveDirectly", Json(true))));
  out.gentle_click = truth(value(globals.value("enableGentleClick", Json(false))));
  for (int marker : src.blocked_items)
    out.blocked_items.at(marker / cells).set(marker % cells);
  out.has_event_serial = extra.contains("native_event_serial");
  out.event_serial = extra.value("native_event_serial", Number(0));
  out.has_win_reason = extra.contains("native_win_reason");
  out.win_reason = extra.value("native_win_reason", std::string());
  out.has_exp = extra.contains("exp");
  out.exp_null = out.has_exp && !extra["exp"].is_number();
  out.exp = out.has_exp && !out.exp_null ? extra["exp"].get<Number>() : 0;
  if (extra.contains("enemy_overrides"))
    for (auto it = extra["enemy_overrides"].begin(); it != extra["enemy_overrides"].end(); ++it) {
      auto &o = out.overrides->edit(w.entity_ids.at(it.key()));
      for (auto field = it.value().begin(); field != it.value().end(); ++field) {
        int n = -1;
        for (int i = 0; i < 12; ++i)
          if (field.key() == enemy_fields[i])
            n = i;
        if (n < 0)
          throw Unsupported("Unknown enemy override");
        o.present.set(n);
        o.value[n] = value(field.value());
      }
      out.resolve_enemy(w.entity_ids.at(it.key()));
    }
  if (extra.contains("native_event_state")) {
    const auto &state = extra["native_event_state"];
    out.initialized = state.value("initialized", false);
    for (const auto &v : state.value("frames", Json::array())) {
      int loop = 0;
      if (v.contains("loop") && !v["loop"].is_null())
        loop = instruction(v["loop"]);
      out.push(program(v.at("todo"), false), v.at("floor"), {v.at("x"), v.at("y")}, loop);
      if (v.contains("auto_symbol")) {
        auto it = std::find_if(w.automatic.begin(), w.automatic.end(), [&](const auto &e) {
          return e.symbol == v["auto_symbol"].get<std::string>();
        });
        if (it == w.automatic.end())
          throw Unsupported("Unknown frame automatic symbol");
        out.frames.back().automatic = int(it - w.automatic.begin());
      }
    }
    if (state.contains("choice")) {
      const auto &c = state["choice"];
      auto raw = Json{{"type", "choices"}, {"choices", c.at("options")}, {"text", c.value("text", Json())}};
      PendingChoice pending;
      pending.instruction = instruction(raw);
      pending.floor = c.at("floor");
      pending.pos = {c.at("x"), c.at("y")};
      for (std::size_t i = 0; i < world->instructions[pending.instruction].choices.size(); ++i)
        pending.options.push_back(int(i));
      out.choice = std::move(pending);
    }
  }
  // Import compilation can discover extra flags in retained, checkpoint-owned
  // event programs. Extend storage before the immutable world is used by Game.
  out.flags->scalar.resize(w.flag_names.size());
  out.flags->extra.resize(w.extra_names.size());
  for (int f = w.first_floor; f < floors_count; ++f)
    for (int i = 0; i < cells; ++i)
      out.refresh_cell(f, {i % side, i / side});
  w.lower_events();
  for (int f = w.first_floor; f < floors_count; ++f)
    if (build_corridors && !w.corridors[f]) {
      CellMask open;
      for (int i = 0; i < cells; ++i)
        if (out.floors[f]->sweep_rule[i] == SweepRule::empty)
          open.set(i);
      w.corridors[f] = std::make_shared<const CorridorGraph>(open);
    }
  seal_identities();
  return out;
}
Json Compiler::scalar(const Value &v) const {
  return std::visit(
      [&](const auto &x) -> Json {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return nullptr;
        else if constexpr (std::is_same_v<T, Opaque>)
          return x.id < 0 ? Json::array() : opaque.at(x.id);
        else if constexpr (std::is_same_v<T, NumberArray>)
          return x.values;
        else if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string>)
          return x;
        else
          throw Unsupported("Transient expression object cannot be exported as scalar flag");
      },
      v);
}
Json Compiler::remaining(const Frame &f) const {
  auto out = Json::array();
  for (auto it = f.segments.rbegin(); it != f.segments.rend(); ++it) {
    const auto &program = program_sources.at(it->program);
    for (std::size_t n = it->cursor; n < program.size(); ++n)
      out.push_back(program[n]);
  }
  return out;
}
Json Compiler::event_state(const Game &g) const {
  Json state = {{"frames", Json::array()}, {"initialized", g.initialized}};
  for (const auto &f : g.frames) {
    Json loop = f.loop ? sources.at(world->instructions[f.loop].source) : Json();
    Json frame = {{"floor", f.floor}, {"x", f.pos.x}, {"y", f.pos.y}, {"loop", loop}, {"todo", remaining(f)}};
    if (f.automatic >= 0)
      frame["auto_symbol"] = world->automatic[f.automatic].symbol;
    state["frames"].push_back(std::move(frame));
  }
  if (g.choice) {
    const auto &c = *g.choice;
    const auto &i = world->instructions[c.instruction];
    Json options = Json::array();
    for (int n : c.options)
      options.push_back(sources.at(i.choices[n].source));
    state["choice"] = {{"options", options},
                       {"floor", c.floor},
                       {"x", c.pos.x},
                       {"y", c.pos.y},
                       {"text", sources.at(i.source).value("text", Json(""))}};
  }
  return state;
}
mota::Game Compiler::project(const Game &g, const mota::Game &initial) const {
  auto out = initial;
  out.hero.values = g.hero.values;
  out.hero.floor = g.hero.floor;
  out.hero.pos = {g.hero.pos.x, g.hero.pos.y};
  out.visited = g.visited;
  out.automatic = {g.automatic.battle, g.automatic.wall, g.automatic.item};
  out.mdef_present = g.mdef_present;
  out.native_continuation.reset();
  out.blocked_items.clear();
  for (int f = 0; f < floors_count; ++f)
    for (auto remaining = g.blocked_items[f]; remaining.any();)
      out.blocked_items.insert(f * cells + remaining.pop());
  out.flags = std::make_shared<mota::JsonState>();
  auto &flags = out.flags->value;
  for (std::size_t i = 0; i < g.flags->scalar.size(); ++i)
    if (g.flag_value(int(i)))
      flags[world->flag_names[i]] = scalar(*g.flag_value(int(i)));
  if (g.has_auto)
    flags["__auto__"] = {{"battle", g.automatic.battle},
                         {"wall", g.automatic.wall},
                         {"item", g.automatic.item == 2 ? Json("nopotion") : Json(g.automatic.item == 1)}};
  if (g.has_aed) {
    flags["__aed__"] = Json::array();
    for (int index : g.auto_executed)
      flags["__aed__"].push_back(world->automatic[index].symbol);
  }
  if (g.has_aei) {
    flags["__aei__"] = Json::array();
    for (int index : g.auto_executing)
      flags["__aei__"].push_back(world->automatic[index].symbol);
  }
  if (g.has_visited_record) {
    flags["__visited__"] = Json::object();
    for (int f = 0; f < floors_count; ++f)
      if (g.visited_flags[f])
        flags["__visited__"][floor_name(f)] = true;
  }
  if (g.has_leave) {
    flags["__leaveLoc__"] = Json::object();
    for (int f = 0; f < floors_count; ++f)
      if (g.leave[f])
        flags["__leaveLoc__"][floor_name(f)] = {{"x", g.leave[f]->pos.x},
                                                {"y", g.leave[f]->pos.y},
                                                {"direction", direction_name(g.leave[f]->direction)}};
  }
  for (int f = world->first_floor; f < floors_count; ++f) {
    const auto &fs = *g.floors[f];
    auto fid = floor_name(f);
    if (fs.disabled_record) {
      auto orders = fs.observations.materialize();
      flags["__disabled__"][fid] = {std::vector<int>(orders.disabled.begin(), orders.disabled.end()),
                                    std::vector<int>(orders.enabled.begin(), orders.enabled.end())};
    }
    for (bool fg : {false, true})
      if (fg ? fs.foreground_record : fs.background_record) {
        auto &rows = flags[fg ? "__fgv__" : "__bgv__"][fid];
        rows = Json::array();
        for (const auto &r : fg ? fs.foreground_changes : fs.background_changes)
          rows.push_back({r.pos.x, r.pos.y, r.tile});
      }
    for (int i = 0; i < cells; ++i)
      if (fs.cell[i].source_destroyed)
        flags["R" + fid + "," + std::to_string(i % side) + "," + std::to_string(i / side)] = true;
  }
  out.items = std::make_shared<mota::JsonState>(*initial.items);
  for (std::size_t i = 0; i < world->item_names.size(); ++i)
    if (world->item_stat[i] < 0 && world->item_class[i] > 0) {
      auto cls = world->item_class[i] == 1   ? "constants"
                 : world->item_class[i] == 2 ? "tools"
                 : world->item_class[i] == 3 ? "equips"
                                             : "items";
      auto &section = out.items->value[cls];
      if (!section.is_object())
        section = Json::object();
      if (g.flags->inventory[i])
        section[world->item_names[i]] = g.flags->inventory[i];
      else
        section.erase(world->item_names[i]);
    }
  out.items->hash = mota::hash_json(out.items->value);
  out.extra_state = std::make_shared<mota::JsonState>(*initial.extra_state);
  auto &extra = out.extra_state->value;
  extra["direction"] = direction_name(g.facing);
  for (std::size_t i = 0; i < g.flags->extra.size(); ++i)
    if (g.flags->extra[i])
      extra[world->extra_names[i]] = scalar(*g.flags->extra[i]);
  if (g.initialized || !g.frames.empty() || g.choice)
    extra["native_event_state"] = event_state(g);
  if (g.has_event_serial)
    extra["native_event_serial"] = g.event_serial;
  if (g.has_win_reason)
    extra["native_win_reason"] = g.win_reason;
  if (g.has_exp)
    extra["exp"] = g.exp_null ? Json() : Json(g.exp);
  Json overrides = Json::object();
  for (std::size_t e = 0; e < g.overrides->size(); ++e) {
    const auto &o = g.overrides->at(e);
    for (int i = 0; i < 12; ++i)
      if (o.present[i])
        overrides[world->entities[e].id][enemy_fields[i]] = scalar(o.value[i]);
  }
  if (!overrides.empty() || initial.extra_state->value.contains("enemy_overrides"))
    extra["enemy_overrides"] = overrides;
  if (g.has_enemy_info)
    flags["enemyInfo"] = overrides;
  out.flags->hash = mota::hash_json(flags);
  out.extra_state->hash = mota::hash_json(extra);
  for (int f = world->first_floor; f < floors_count; ++f) {
    if (projected_inputs[f] == g.floors[f]) {
      out.floors[f] = projected_floors[f];
      continue;
    }
    const auto &fs = *g.floors[f];
    auto result = std::make_shared<mota::FloorState>(*initial.floors[f]);
    result->retained_events = std::make_shared<mota::RetainedEvents>();
    result->events = std::make_shared<Json>(Json::object());
    result->block_order.clear();
    result->spell_enemies.clear();
    result->spell_cells.reset();
    result->foreground = fs.foreground;
    result->background = fs.background;
    result->retained_present.reset();
    result->disabled.reset();
    result->event_entries.reset();
    for (auto p : fs.order)
      result->block_order.push_back({p.x, p.y});
    out.floors[f] = result;
    for (int i = 0; i < cells; ++i) {
      const auto &c = fs.cell[i];
      mota::Pos p{i % side, i / side};
      result->retained[i] = {c.retained.entity, c.retained.tile};
      result->retained_present[i] = c.present;
      result->disabled[i] = c.disabled;
      result->blocks[i] = c.present && !c.disabled ? result->retained[i] : mota::Block{};
      result->grid[i] = result->blocks[i].tile;
      if (c.present) {
        Json payload;
        if (c.payload)
          payload = payload_sources.at(c.payload);
        else if (c.base_payload)
          payload = c.retained.entity ? reference_world->entities[c.retained.entity].native
                                      : Json{{"cls", "terrains"}, {"id", "none"}, {"noPass", false}};
        else
          payload = composed_payload(*reference_world, f, p, result->retained[i], c.source_destroyed);
        if (c.touch_precompiled && payload.contains("event"))
          payload["event"] = compiled_source(payload["event"]);
        result->retained_events->set(p, payload);
        if (c.event_entry) {
          auto key = std::to_string(p.x) + "," + std::to_string(p.y);
          auto source = reference_world->floors[f].events.value(key, Json());
          if (source.is_object()) {
            source["data"] = payload.at("data");
            source["enable"] = true;
            (*result->events)[key] = source;
          } else
            (*result->events)[key] = payload.at("data");
          result->event_entries.set(i);
        }
      }
      const auto &e = reference_world->entities[result->blocks[i].entity];
      if (e.kind == mota::Kind::enemy && (e.special & (1ULL << 15))) {
        result->spell_cells.set(i);
        result->spell_enemies.push_back(p);
      }
    }
    result->movement_source.reset();
    result->navigation.store(nullptr);
    result->hazards.store(nullptr);
    result->automatic_cache.store(nullptr);
    result->hash.invalidate();
    projected_inputs[f] = g.floors[f];
    projected_floors[f] = out.floors[f];
  }
  return out;
}
} // namespace engine::io
