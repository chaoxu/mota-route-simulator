#include "engine/core.hpp"
#include <algorithm>
namespace engine {
static Props merged(Props base, const Props &over) {
  auto mask = over.specified;
  base.specified |= mask;
  if (mask & 1)
    base.no_pass = over.no_pass;
  if (mask & 2)
    base.trigger = over.trigger;
  if (mask & 4) {
    base.has_data = true;
    base.action = over.action;
    base.change = over.change;
  }
  if (mask & 8) {
    base.touch = over.touch;
    base.event_truth = over.event_truth;
  }
  if (mask & 16)
    base.out_event = over.out_event;
  if (mask & 32)
    base.in = over.in;
  if (mask & 64)
    base.out = over.out;
  if (mask & 128)
    base.script = over.script;
  if (mask & 256) {
    base.door = over.door;
    base.keys = over.keys;
  }
  if (mask & 512)
    base.can_break = over.can_break;
  if (mask & 1024) {
    base.cls = over.cls;
    base.door_class = over.door_class;
  }
  if (mask & 2048)
    base.id = over.id;
  if (base.trigger == Trigger::none)
    base.trigger = Trigger::action;
  return base;
}

void World::lower() {
  indexed_automatic = automatic.size() <= 64;
  if (indexed_automatic) {
    flag_dependents.resize(flag_names.size());
    for (std::size_t index = 0; index < automatic.size(); ++index) {
      const auto &event = automatic[index];
      auto bit = std::uint64_t(1) << index;
      if (!event.guards_native || event.multiple) {
        auto_dynamic |= bit;
        continue;
      }
      for (const auto &guard : event.guards) {
        if (guard.op == GuardOp::flag)
          flag_dependents.at(guard.index) |= bit;
        else if (guard.floor < 0) {
          auto_relative |= bit;
          for (int f = first_floor; f < floors_count; ++f)
            cell_dependents[f][guard.pos.index()] |= bit;
        } else
          cell_dependents.at(guard.floor).at(guard.pos.index()) |= bit;
      }
    }
  }
  actors.assign(entities.begin(), entities.end());
  int max_overlay = 0;
  for (const auto &f : floors)
    for (int overlay : f.source_payload)
      max_overlay = std::max(max_overlay, overlay);
  composition.resize((max_overlay + 1) * entities.size());
  for (int overlay = 0; overlay <= max_overlay; ++overlay) {
    for (std::size_t e = 0; e < entities.size(); ++e) {
      composed.push_back(overlay ? merged(entities[e].props, payloads[overlay]) : entities[e].props);
      composition[overlay * entities.size() + e] = std::uint32_t(composed.size());
    }
  }
  for (auto &e : entities) {
    auto &v = e.effect;
    auto add = [&](Stat s, Number n) {
      v.stat[v.count] = s;
      v.amount[v.count++] = n;
      v.supported = true;
    };
    const auto &s = e.id;
    if (s == "yellowKey")
      add(yk, 1);
    else if (s == "blueKey")
      add(bk, 1);
    else if (s == "redKey")
      add(rk, 1);
    else if (s == "redPotion") {
      add(hp, 50);
      v.ratio = true;
    } else if (s == "bluePotion") {
      add(hp, 200);
      v.ratio = true;
    } else if (s == "greenPotion") {
      add(hp, green_potion);
      v.ratio = true;
    } else if (s == "yellowPotion") {
      add(hp, yellow_potion);
      v.ratio = true;
    } else if (s == "greenGem") {
      add(mdef, green_gem);
      v.ratio = true;
    } else if (s == "yellowGem") {
      add(hp, 1000);
      add(atk, 6);
      add(defense, 6);
      add(mdef, 10);
    } else if (s == "pack")
      add(gold, 500);
    else if (s == "redGem") {
      add(atk, 1);
      v.ratio = true;
    } else if (s == "blueGem") {
      add(defense, 1);
      v.ratio = true;
    } else if (s.starts_with("sword") && s.back() >= '1' && s.back() <= '5') {
      add(atk, std::array<int, 5>{10, 20, 40, 50, 100}[s.back() - '1']);
      v.flag = ids.weapon;
    } else if (s.starts_with("shield") && s.back() >= '1' && s.back() <= '5') {
      add(defense, std::array<int, 5>{10, 20, 40, 50, 100}[s.back() - '1']);
      v.flag = ids.shield;
      v.immune = s.back() == '5';
    } else if (s == "centerFly3")
      add(centerFly, 3);
  }
}
void World::lower_events() {
  for (auto &program : programs)
    if (program.next_effect.empty()) {
      auto n = program.code.size();
      program.next_effect.resize(n + 1);
      program.next_effect[n] = int(n);
      for (std::size_t i = n; i-- > 0;)
        program.next_effect[i] =
            instructions[program.code[i]].op == Op::noop ? program.next_effect[i + 1] : int(i);
    }
  for (std::size_t id = micro.size(); id < instructions.size(); ++id) {
    const auto &i = instructions[id];
    MicroOp m;
    m.assign = i.assign;
    if (i.op == Op::noop)
      m.op = FastOp::noop;
    else if (i.op == Op::function) {
      m.op = FastOp::function;
      m.immediate = std::bit_cast<Number>(i.function);
    } else if (i.op == Op::assign && expressions[i.value].op == ExprOp::literal) {
      m.op = FastOp::literal_assign;
      if (i.target.kind == Ref::stat) {
        m.op = FastOp::stat_constant;
        m.index = i.target.index;
        m.immediate = integer(expressions[i.value].literal);
      }
    } else if ((i.op == Op::hide || i.op == Op::show) && i.floor >= -1 && i.locations.size() == 1) {
      const auto &location = i.locations[0];
      const auto &x = expressions[location.x], &y = expressions[location.y];
      if (x.op == ExprOp::literal && y.op == ExprOp::literal && integer(x.literal) >= 0 &&
          integer(x.literal) < side && integer(y.literal) >= 0 && integer(y.literal) < side) {
        m.op = i.op == Op::hide ? FastOp::fixed_hide : FastOp::fixed_show;
        m.floor = std::int16_t(i.floor);
        m.x = integer(x.literal);
        m.y = integer(y.literal);
        m.remove = i.remove;
        m.destruct = i.destruct;
      }
    }
    micro.push_back(m);
  }
}
} // namespace engine
