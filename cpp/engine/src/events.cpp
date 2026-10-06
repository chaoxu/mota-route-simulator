#include "engine/core.hpp"
#include <algorithm>
#include <cmath>

namespace engine {
static void require(bool ok, const char *message) {
  if (!ok)
    throw Error(message);
}
void Game::push(int program, int f, Pos p, int loop) {
  Frame frame;
  frame.floor = f;
  frame.pos = p;
  frame.loop = loop;
  if (!world->programs.at(program).code.empty())
    frame.segments.push_back({program, 0});
  frames.push_back(std::move(frame));
}
void Game::prepend(int program) {
  require(!frames.empty(), "Prepend without event frame");
  if (!world->programs.at(program).code.empty())
    frames.back().segments.push_back({program, 0});
}
void Game::start(int program, int f, Pos p) {
  start_sequence(std::span<const int>(&program, 1), f, p);
}
void Game::start_sequence(std::span<const int> programs, int f, Pos p) {
  bool any = false;
  for (int id : programs)
    any |= !world->programs.at(id).code.empty();
  if (!any)
    return;
  has_event_serial = true;
  ++event_serial;
  push(0, f, p);
  for (auto it = programs.rbegin(); it != programs.rend(); ++it)
    prepend(*it);
  if (!executing)
    drain();
}
void Game::drain() {
  if (auto_active || executing || choice)
    return;
  if (frames.empty()) {
    clear_temporaries();
    return;
  }
  executing = true;
  std::size_t operations = 0;
  try {
    while (!frames.empty() && !choice) {
      if (++operations > 200000)
        throw Error("Event operation limit");
      auto &frame = frames.back();
      int f = frame.floor;
      Pos p = frame.pos;
      if (frame.segments.empty()) {
        if (frame.loop) {
          const auto &i = world->instructions[frame.loop];
          bool repeat = false;
          if (i.op == Op::for_loop) {
            auto end = eval(i.to, f, p), step = eval(i.step, f, p);
            auto n = integer(step);
            if (n == 0)
              repeat = true;
            else {
              auto value = integer(read(i.target, f, p)) + n;
              assign(i.target, value, Assign::set, f, p);
              repeat = n > 0 ? value <= integer(end) : value >= integer(end);
            }
          } else
            repeat = truth(eval(i.condition, f, p));
          if (repeat) {
            prepend(i.program);
            continue;
          }
        }
        if (frame.automatic >= 0) {
          auto_executing.erase(std::remove(auto_executing.begin(), auto_executing.end(), frame.automatic),
                               auto_executing.end());
          has_aei = true;
        }
        frames.pop_back();
        continue;
      }
      auto &segment = frame.segments.back();
      const auto &program = world->programs[segment.program];
      const auto &code = program.code;
      if (use_native && std::size_t(segment.cursor) < program.native_blocks.size()) {
        if (auto run = program.native_blocks[segment.cursor]) {
          ++native_block_count;
          run(*this, f, p, operations);
          continue;
        }
      }
      if (!program.next_effect.empty()) {
        int next = program.next_effect[segment.cursor];
        int skipped = next - segment.cursor;
        operations += skipped - (next == int(code.size()) ? 1 : 0);
        if (operations > 200000)
          throw Error("Event operation limit");
        segment.cursor = next;
        if (next == int(code.size())) {
          frame.segments.pop_back();
          continue;
        }
      }
      int instruction = code[segment.cursor++];
      if (segment.cursor == int(code.size()))
        frame.segments.pop_back();
      execute(instruction, f, p);
    }
    if (frames.empty() && !choice)
      clear_temporaries();
    executing = false;
  } catch (...) {
    executing = false;
    throw;
  }
}
SmallVector<Pos, 4> Game::locations(const Instruction &i, int f, Pos p) const {
  if (!i.has_location)
    return {p};
  SmallVector<Pos, 4> out;
  out.reserve(i.locations.size());
  for (auto loc : i.locations)
    out.push_back({int(integer(eval(loc.x, f, p))), int(integer(eval(loc.y, f, p)))});
  return out;
}
int Game::event_floor(const Instruction &i) const {
  if (i.floor == -1)
    return hero.floor;
  if (i.floor >= 0)
    return i.floor;
  int step = i.floor == -2 ? 1 : -1;
  for (int f = hero.floor + step; f >= world->first_floor && f < floors_count; f += step)
    if (!world->floors[f].hidden)
      return f;
  throw Error("Relative event floor missing");
}
void Game::execute(int id, int f, Pos p) {
  if (use_native && std::size_t(id) < world->native_steps.size()) {
    if (auto run = world->native_steps[id]) {
      ++native_step_count;
      run(*this, f, p);
      return;
    }
  }
  ++slow_step_count;
  execute_slow(id, f, p);
}
void Game::execute_slow(int id, int f, Pos p) {
  if (std::size_t(id) < world->micro.size()) {
    const auto &m = world->micro[id];
    if (m.op != FastOp::generic)
      ++compiled_steps;
    switch (m.op) {
    case FastOp::noop:
      return;
    case FastOp::stat_constant: {
      auto &v = hero.values.at(m.index);
      switch (m.assign) {
      case Assign::set:
        v = m.immediate;
        return;
      case Assign::add:
        v += m.immediate;
        return;
      case Assign::sub:
        v -= m.immediate;
        return;
      case Assign::mul:
        v *= m.immediate;
        return;
      case Assign::div:
        require(m.immediate != 0, "Division zero");
        v /= m.immediate;
        return;
      case Assign::mod:
        require(m.immediate != 0, "Modulo zero");
        v %= m.immediate;
        return;
      }
      break;
    }
    case FastOp::literal_assign: {
      const auto &i = world->instructions[id];
      assign(i.target, world->expressions[i.value].literal, i.assign, f, p);
      return;
    }
    case FastOp::fixed_hide: {
      int floor = m.floor < 0 ? hero.floor : m.floor;
      Pos q{m.x, m.y};
      if (m.remove || m.destruct)
        remove(floor, q, m.destruct);
      else
        hide(floor, q);
      return;
    }
    case FastOp::fixed_show:
      show(m.floor < 0 ? hero.floor : m.floor, {m.x, m.y});
      return;
    case FastOp::function:
      function(std::bit_cast<std::uint64_t>(m.immediate), f, p);
      return;
    case FastOp::generic:
      break;
    }
  }
  ++generic_steps;
  const auto &i = world->instructions.at(id);
  switch (i.op) {
  case Op::noop:
    return;
  case Op::unsupported:
    throw Unsupported(i.name);
  case Op::assign:
    assign(i.target, eval(i.value, f, p), i.assign, f, p);
    return;
  case Op::if_else:
    prepend(truth(eval(i.condition, f, p)) ? i.yes : i.no);
    return;
  case Op::switch_case: {
    auto key = eval(i.condition, f, p);
    SmallVector<int, 4> parts;
    for (const auto &c : i.cases)
      if (c.is_default || equal_value(eval(c.condition, f, p), key, true)) {
        parts.push_back(c.program);
        if (!c.fallthrough)
          break;
      }
    for (auto it = parts.rbegin(); it != parts.rend(); ++it)
      prepend(*it);
    return;
  }
  case Op::choices: {
    PendingChoice next;
    next.instruction = id;
    next.floor = f;
    next.pos = p;
    for (std::size_t n = 0; n < i.choices.size(); ++n)
      if (!i.choices[n].condition || truth(eval(i.choices[n].condition, f, p)))
        next.options.push_back(int(n));
    if (!next.options.empty())
      choice = std::move(next);
    return;
  }
  case Op::while_loop:
    if (truth(eval(i.condition, f, p)))
      push(i.program, f, p, id);
    return;
  case Op::for_loop: {
    auto begin = eval(i.from, f, p), end = eval(i.to, f, p), step = eval(i.step, f, p);
    auto first = integer(begin), last = integer(end), increment = integer(step);
    if ((increment > 0 && first > last) || (increment < 0 && first < last))
      return;
    assign(i.target, begin, Assign::set, f, p);
    if (i.arguments.size() == 2) {
      set_flag(i.number, i.arguments[0]);
      set_flag(i.entity, i.arguments[1]);
    }
    push(i.program, f, p, id);
    return;
  }
  case Op::break_loop: {
    int n = i.number;
    while (n-- && frames.size() > 1)
      frames.pop_back();
    return;
  }
  case Op::exit:
    frames.clear();
    choice.reset();
    return;
  case Op::insert:
    for (std::size_t n = 0; n < i.arguments.size(); ++n)
      if (i.arguments[n].index() != 0)
        set_flag(world->flag_ids.at("arg" + std::to_string(n + 1)), i.arguments[n]);
    set_flag(world->ids.arg0, i.name);
    push(i.program, f, p);
    return;
  case Op::function:
    function(i.function, f, p);
    return;
  case Op::hide:
    for (auto q : locations(i, f, p)) {
      int floor = event_floor(i);
      if (i.remove || i.destruct)
        remove(floor, q, i.destruct);
      else
        hide(floor, q);
    }
    return;
  case Op::show:
    for (auto q : locations(i, f, p))
      show(event_floor(i), q);
    return;
  case Op::set_block: {
    auto b = world->entities.at(i.entity).block;
    if (i.number)
      b.tile = std::int16_t(i.number);
    for (auto q : locations(i, f, p)) {
      int floor = event_floor(i);
      const auto &c = cell(floor, q);
      bool zero =
          !b.entity && !i.destruct && !c.source_destroyed && world->floors[floor].source_event[q.index()];
      bool animate = c.retained.tile != 0 ? !c.disabled && (b.entity || zero) : bool(b.entity);
      replace(floor, q, b, i.destruct, i.animate && floor == hero.floor && animate);
    }
    return;
  }
  case Op::set_layer:
    for (auto q : locations(i, f, p))
      layer(event_floor(i), q, i.number, i.foreground);
    return;
  case Op::open_door:
    for (auto q : locations(i, f, p)) {
      int floor = event_floor(i);
      if (floor != hero.floor) {
        remove(floor, q);
        continue;
      }
      auto b = block(floor, q);
      auto v = props(floor, q);
      if ((!b.entity && !event_enabled(floor, q)) || !v.door_class || !v.door)
        continue;
      if (i.need_key) {
        bool enough = true;
        for (auto k : v.keys)
          if (item_count(k.item) < k.count)
            enough = false;
        if (!enough)
          continue;
        for (auto k : v.keys)
          if (!k.preserve)
            set_item(k.item, item_count(k.item) - k.count);
        const auto &name = world->entities[b.entity].id;
        if (name == "yellowDoor")
          ++hero[yd];
        if (name == "blueDoor")
          ++hero[bd];
        if (name == "redDoor")
          ++hero[rd];
      }
      remove(floor, q);
      int event = world->floors[floor].after_door[q.index()];
      start(event, floor, q);
    }
    return;
  case Op::close_door:
    if (i.entity && world->entities[i.entity].props.door)
      for (auto q : locations(i, f, p))
        if (!present(hero.floor, q)) {
          replace(hero.floor, q, world->entities[i.entity].block);
          show(hero.floor, q);
        }
    return;
  case Op::move_hero:
    for (auto motion : i.motions) {
      auto dir = motion.direction;
      int sign = dir == Direction::backward ? -1 : 1;
      if (dir == Direction::forward || dir == Direction::backward)
        dir = facing;
      auto d = delta(dir);
      hero.pos.x += sign * d.x * std::max(motion.count, 0);
      hero.pos.y += sign * d.y * std::max(motion.count, 0);
      facing = d.x < 0 ? Direction::left : d.x > 0 ? Direction::right : dir;
    }
    return;
  case Op::move_block:
  case Op::generate_move: {
    int floor = event_floor(i);
    auto begin = locations(i, f, p).at(0), end = begin;
    if (i.op == Op::move_block && (!present(floor, begin) || !block(floor, begin).entity))
      return;
    auto b = i.op == Op::generate_move ? world->entities.at(i.entity).block : block(floor, begin);
    if (i.op == Op::move_block) {
      remove(floor, begin);
      erase_event(floor, begin);
    }
    for (auto motion : i.motions) {
      auto d = delta(motion.direction);
      end.x += d.x * motion.count;
      end.y += d.y * motion.count;
    }
    if (i.keep && b.entity) {
      require(end.valid(), "Moving block leaves map");
      replace(floor, end, b);
      show(floor, end);
    }
    return;
  }
  case Op::change_floor: {
    int floor = event_floor(i);
    Pos landing = i.has_location          ? locations(i, f, p).at(0)
                  : i.name == "downFloor" ? world->floors[floor].down
                  : i.name == "upFloor"   ? world->floors[floor].up
                                          : Pos{-1, -1};
    transition(floor, landing);
    if (i.number >= 0)
      facing = Direction(i.number);
    return;
  }
  case Op::set_enemy: {
    auto v = eval(i.value, f, p);
    const auto &e = enemy(world->entities.at(i.entity).block);
    Number old = i.number == 0   ? e.hp
                 : i.number == 1 ? e.atk
                 : i.number == 2 ? e.def
                 : i.number == 3 ? e.money
                                 : 0;
    if (i.assign == Assign::div) {
      require(integer(v) != 0, "Enemy divide by zero");
      v = old / integer(v);
    } else if (i.assign == Assign::add)
      v = old + integer(v);
    else if (i.assign != Assign::set)
      throw Unsupported("Enemy assignment operator");
    change_enemy(i.entity, i.number, v);
    return;
  }
  case Op::battle: {
    int floor = event_floor(i);
    if (!i.has_id && floor != hero.floor)
      return;
    auto q = i.has_id ? Pos{-1, -1} : locations(i, f, p).at(0);
    auto b = i.has_id ? world->entities[i.entity].block : block(floor, q);
    if (!b.entity)
      return;
    const auto &e = enemy(b);
    require(e.kind == Kind::enemy, "Event battle nonenemy");
    auto d = damage(b);
    if (flag_number(world->ids.addhp) == 1 && d < infinite_damage)
      d = -d;
    require(d < hero[hp], "Fatal event battle");
    hero[hp] -= d;
    hero[dmg] += d;
    if (!flag_truth(world->ids.curse))
      hero[gold] += e.money * (hero[coin] ? 2 : 1);
    set_number_flag(world->ids.hatred, flag_number(world->ids.hatred) + world->hatred);
    set_number_flag(world->ids.energy, Number(0));
    if (!i.has_id) {
      remove(floor, q);
      erase_event(floor, q);
      start(world->floors[floor].after_battle[q.index()], floor, q);
    }
    return;
  }
  case Op::win: {
    std::string reason;
    std::size_t cursor = 0, index = 0;
    for (;;) {
      auto begin = i.reason.find("${", cursor);
      if (begin == std::string::npos) {
        reason += i.reason.substr(cursor);
        break;
      }
      reason += i.reason.substr(cursor, begin - cursor);
      auto end = i.reason.find('}', begin + 2);
      require(end != std::string::npos, "Invalid ending template");
      reason += text(eval(i.locations.at(index++).x, f, p));
      cursor = end + 1;
    }
    hero[won50] = 1;
    win_reason = std::move(reason);
    has_win_reason = true;
    frames.clear();
    return;
  }
  case Op::bgm:
    set_flag(world->ids.bgm, i.keep ? Value(i.name) : Value{});
    return;
  }
}
void Game::choose(int index) {
  require(bool(choice), "Choice without pending menu");
  auto pending = *choice;
  const auto &i = world->instructions[pending.instruction];
  int size = int(pending.options.size());
  if (index < 0)
    index += size;
  if (index % 100 > 50)
    index += size;
  require(index >= 0 && index % 100 < size, "Choice out of range");
  set_flag(world->ids.timeout, Number(index / 100));
  index %= 100;
  choice.reset();
  const auto &selected = i.choices[pending.options[index]];
  if (selected.need && !truth(eval(selected.need, pending.floor, pending.pos)))
    throw Error("Choice requirement failed");
  require(!frames.empty(), "Choice without frame");
  prepend(selected.program);
  drain();
}
bool Game::trigger(int f, Pos p) {
  if (!present(f, p))
    return false;
  const auto &v = props(f, p);
  if (v.touch) {
    int program = v.touch;
    edit_floor(f).cell[p.index()].touch_precompiled = true;
    sync_cell(f, p);
    start(program, f, p);
    return true;
  }
  if (v.trigger == Trigger::action && v.action && !world->programs[v.action].code.empty()) {
    start(v.action, f, p);
    return true;
  }
  const auto &id = world->entities[block(f, p).entity].id;
  if (id == "oldman" || id == "trader") {
    bool used = flag_truth(world->floors[f].switch_slot[p.index()]);
    bool dialogue = id == "oldman" || used;
    set_flag(world->ids.arg1, Number(f));
    set_flag(world->ids.arg2, Number(p.x));
    set_flag(world->ids.arg3, Number(p.y));
    if (dialogue)
      set_flag(world->ids.arg4, Number(id == "trader" && used ? 1 : 0));
    set_flag(world->ids.arg0, std::string(dialogue ? "对话" : "商人"));
    has_event_serial = true;
    ++event_serial;
    push(0, f, p);
    push(dialogue ? world->dialogue : world->trader, f, p);
    drain();
    return true;
  }
  if (v.trigger == Trigger::change_floor && v.change) {
    const int instruction = v.change;
    has_event_serial = true;
    ++event_serial;
    push(0, f, p);
    execute(instruction, f, p);
    drain();
    return true;
  }
  return false;
}
void Game::after(int kind, int f, Pos p) {
  const auto &fd = world->floors[f];
  start(kind == 0   ? fd.after_battle[p.index()]
        : kind == 1 ? fd.after_item[p.index()]
                    : fd.after_door[p.index()],
        f, p);
  check_auto();
}
void Game::arrive(int f, bool first) {
  std::array<int, 2> programs = {first ? world->floors[f].first_arrive : 0, world->floors[f].each_arrive};
  start_sequence(programs, f, hero.pos);
  check_auto();
}
bool Game::auto_condition(const AutoEvent &e) const {
  if (!e.guards_native)
    return truth(eval(e.condition, e.floor, {e.index % side, e.index / side}));
  for (const auto &g : e.guards) {
    bool value;
    if (g.op == GuardOp::flag) {
      const auto &v = flag_value(g.index);
      value = v && truth(*v);
    } else {
      int f = g.floor < 0 ? hero.floor : g.floor;
      bool exists = present(f, g.pos);
      if (g.op == GuardOp::is_null)
        value = !exists;
      else
        value = exists && (g.op == GuardOp::block_id ? props(f, g.pos).id : props(f, g.pos).cls) == g.value;
    }
    if (g.negate)
      value = !value;
    if (!value)
      return false;
  }
  return true;
}
void Game::check_auto() {
  if (flag_truth(world->ids.no_auto_check))
    return;
  const auto &candidates = world->floors[hero.floor].automatic;
  for (int index : candidates) {
    const auto &e = world->automatic[index];
    if (!e.multiple && std::find(auto_executed.begin(), auto_executed.end(), index) != auto_executed.end())
      continue;
    if (std::find(auto_executing.begin(), auto_executing.end(), index) != auto_executing.end())
      continue;
    const auto bit = world->indexed_automatic ? (std::uint64_t(1) << index) : 0;
    if (world->indexed_automatic && !((auto_dirty | world->auto_dynamic) & bit)) {
      ++auto_skips;
      continue;
    }
    auto_dirty &= ~bit;
    ++auto_tests;
    if (auto_condition(e)) {
      auto_executed.erase(std::remove(auto_executed.begin(), auto_executed.end(), index),
                          auto_executed.end());
      auto_executed.push_back(index);
      auto_executing.push_back(index);
      has_aed = has_aei = true;
      has_event_serial = true;
      ++event_serial;
      push(e.program, e.floor, {e.index % side, e.index / side});
      frames.back().automatic = index;
    }
  }
  drain();
}
void Game::function(std::uint64_t hash, int f, Pos p) {
  auto set_auto = [&](int what, int value) {
    has_auto = true;
    if (what == 0)
      automatic.battle = value;
    if (what == 1)
      automatic.wall = value;
    if (what == 2)
      automatic.item = value;
  };
  switch (hash) {
  case 0x8802bf5fe2330650ULL:
  case 0x160dd6d3d88aabb2ULL:
  case 0x46ef99a17b25b78fULL:
  case 0x9f5c8e50510968b8ULL:
  case 0x85e862bbac92a720ULL:
  case 0xab6c4e65cbaff7b4ULL:
  case 0x3cc98e3a89a1c842ULL:
  case 0x6b8c0e900f4f21aaULL:
    return;
  case 0xc750c83ef68625b8ULL:
    if (!has_auto) {
      automatic = {};
      has_auto = true;
    }
    return;
  case 0x78b0e0091f3f8207ULL:
    set_auto(0, 1);
    return;
  case 0xd9ea223352e2468cULL:
    set_auto(0, 0);
    return;
  case 0x0d3691d0167e555aULL:
    set_auto(2, 1);
    return;
  case 0xc6f8d5ff735d1167ULL:
    set_auto(2, 0);
    return;
  case 0x34730c5a5a8654adULL:
    set_auto(1, 1);
    return;
  case 0xef85bf376f2d0f0aULL:
    set_auto(1, 0);
    return;
  case 0xca6871a82e7465acULL:
    set_auto(2, 2);
    return;
  case 0x39acc41fb1ed0b5cULL:
    set_flag(world->dialogue_flags.at(flag_number(world->ids.arg1))[flag_truth(world->ids.arg4)], true);
    return;
  case 0x3588d884c60a7ab9ULL: {
    Pos q{int(flag_number(world->ids.arg2)), int(flag_number(world->ids.arg3))};
    set_flag(world->floors[hero.floor].switch_slot[q.index()], Number(1));
    return;
  }
  case 0x1844aa6b1fa3dce0ULL:
  case 0x0ea43c4abcf06457ULL:
  case 0x4739fd881e242301ULL: {
    int ratio = flag_value(world->ids.ratio) ? int(flag_number(world->ids.ratio)) : 1;
    require(ratio >= 0 && ratio < 6, "Purchase ratio outside pinned tower");
    bool health = hash == 0x1844aa6b1fa3dce0ULL, attack = hash == 0x0ea43c4abcf06457ULL;
    Number quantity = health ? 100 * (flag_number(world->ids.times) + 1) : (attack ? 2 : 4) * ratio;
    int total = health   ? world->purchase_hp[ratio]
                : attack ? world->purchase_atk[ratio]
                         : world->purchase_def[ratio];
    int count = health   ? world->purchase_hp_count[ratio]
                : attack ? world->purchase_atk_count[ratio]
                         : world->purchase_def_count[ratio];
    set_flag(total, flag_number(total) + quantity);
    set_flag(count, flag_number(count) + 1);
    return;
  }
  case 0xa47bf1e400ec1804ULL: {
    Number count = 0;
    for (int floor = world->first_floor; floor < floors_count; ++floor)
      for (int i = 0; i < cells; ++i)
        count += world->entities[block(floor, {i % side, i / side}).entity].kind == Kind::enemy;
    set_flag(world->ids.t, count);
    return;
  }
  case 0xe7f5ac81276b2f94ULL: {
    Number count = 0;
    for (std::size_t i = 0; i < world->item_names.size(); ++i) {
      auto &name = world->item_names[i];
      if (world->item_class[i] > 0 && name != "yellowKey" && name != "blueKey" && name != "redKey")
        count += item_count(int(i));
    }
    hero[hp] = count - 1;
    return;
  }
  case 0x856889e68d3801e2ULL: {
    hero[hp] = 0;
    const auto &e = enemy(world->entities[world->entity_ids.at("yellowKnight")].block);
    if (!flag_truth(world->ids.curse))
      hero[gold] += e.money * (hero[coin] ? 2 : 1);
    set_number_flag(world->ids.hatred, flag_number(world->ids.hatred) + world->hatred);
    set_number_flag(world->ids.energy, Number(0));
    return;
  }
  case 0xe4603e09e8824571ULL: {
    constexpr std::array<Pos, 13> positions = {Pos{5, 4}, Pos{4, 4},  Pos{3, 4}, Pos{7, 4}, Pos{8, 4},
                                               Pos{9, 4}, Pos{4, 2},  Pos{3, 2}, Pos{2, 2}, Pos{8, 2},
                                               Pos{9, 2}, Pos{10, 2}, Pos{6, 1}};
    prepend(world->march_tail);
    for (int n = 12; n >= 0; --n)
      if (block(hero.floor, positions[n]).entity)
        prepend(world->march_programs[n]);
    return;
  }
  default:
    throw Unsupported("Unknown source function hash " + std::to_string(hash));
  }
}
void Game::tool(int id, bool decrement) {
  const auto &name = world->item_names.at(id);
  auto energy = [&] {
    if (decrement)
      set_number_flag(world->ids.energy, flag_number(world->ids.energy) - 1);
  };
  require(item_count(id) > 0, "Missing tool");
  int f = hero.floor;
  if (name == "centerFly") {
    Pos p{12 - hero.pos.x, 12 - hero.pos.y};
    auto b = block(f, p);
    const auto &s = world->entities[b.entity].id;
    require(!b.entity || s == "none" || s == "airwall", "Blocked center fly");
    --hero[centerFly];
    hero.pos = p;
    set_flag(world->ids.talking, Number(0));
    energy();
    return;
  }
  if (name == "upFly" || name == "downFly") {
    int to = f + (name == "upFly" ? 1 : -1);
    if (name == "upFly")
      require(f < 49, "Up fly unavailable");
    require(to >= world->first_floor && to < floors_count && !present(to, hero.pos), "Blocked floor tool");
    --hero[name == "upFly" ? upFly : downFly];
    transition(to, hero.pos);
    return;
  }
  if (name == "bigKey") {
    --hero[bigKey];
    for (int i = 0; i < cells; ++i)
      if (world->entities[block(f, {i % side, i / side}).entity].id == "yellowDoor")
        remove(f, {i % side, i / side});
    energy();
    return;
  }
  if (name == "earthquake") {
    --hero[earthquake];
    for (int i = 0; i < cells; ++i)
      if (world->entities[block(f, {i % side, i / side}).entity].props.can_break)
        remove(f, {i % side, i / side});
    energy();
    return;
  }
  if (name == "pickaxe") {
    --hero[pickaxe];
    for (auto d : auto_dirs) {
      auto p = add(hero.pos, d);
      if (world->entities[block(f, p).entity].props.can_break) {
        remove(f, p);
        after(2, f, p);
      }
    }
    energy();
    return;
  }
  if (name == "bomb") {
    SmallVector<Pos, 4> targets;
    SmallVector<int, 8> programs;
    Number reward = 0;
    for (auto d : auto_dirs) {
      auto p = add(hero.pos, d);
      auto b = block(f, p);
      const auto &e = enemy(b);
      if (e.kind != Kind::enemy || e.hp >= 500 || props(f, p).trigger != Trigger::battle)
        continue;
      targets.push_back(p);
      reward += e.money;
      programs.push_back(world->floors[f].after_battle[p.index()]);
      programs.push_back(e.after_battle);
    }
    auto order = floors[f]->order;
    for (auto it = order.rbegin(); it != order.rend(); ++it)
      if (std::find(targets.begin(), targets.end(), *it) != targets.end()) {
        remove(f, *it);
        erase_event(f, *it);
      }
    hero[gold] += reward;
    start_sequence(std::span<const int>(programs.data(), programs.size()), f, hero.pos);
    --hero[bomb];
    energy();
    return;
  }
  if (name == "snow") {
    int n = 0;
    for (auto d : directions) {
      auto p = add(hero.pos, d);
      if (world->entities[block(f, p).entity].id == "lava") {
        remove(f, p);
        ++n;
      }
    }
    if (!n)
      set_item(id, item_count(id) + 1, true);
    energy();
    return;
  }
  if (name == "superPotion") {
    --hero[superPotion];
    hero[hp] += Number(std::floor(double(hero[atk] + hero[defense]) * .74 + .5)) * 10;
    energy();
    return;
  }
  if (name == "book" || name == "I300" || name == "I333") {
    energy();
    return;
  }
  throw Unsupported("Unsupported native tool " + name);
}
Token decode_token(const World &w, std::string_view token) {
  if (token == "up" || token == "down" || token == "left" || token == "right")
    return {TokenOp::step, int(direction(token))};
  if (token.starts_with("choices:"))
    return {TokenOp::choice, std::stoi(std::string(token.substr(8)))};
  if (token.starts_with("fly:"))
    return {TokenOp::fly, floor_number(token.substr(4))};
  if (token.starts_with("move:")) {
    auto s = token.substr(5);
    auto c = s.find(':');
    if (c == std::string_view::npos)
      throw Error("Invalid move token");
    Pos p{std::stoi(std::string(s.substr(0, c))), std::stoi(std::string(s.substr(c + 1)))};
    require(p.valid(), "Invalid direct destination");
    return {TokenOp::move, 0, p};
  }
  if (token == "turn")
    return {TokenOp::turn, -1};
  if (token.starts_with("turn:"))
    return {TokenOp::turn, int(direction(token.substr(5)))};
  if (token.starts_with("item:")) {
    auto it = w.item_ids.find(std::string(token.substr(5)));
    if (it == w.item_ids.end())
      throw Unsupported("Unknown item token");
    return {TokenOp::item, it->second};
  }
  if (token.starts_with("key:"))
    return {TokenOp::key, std::stoi(std::string(token.substr(4)))};
  if (token == "help")
    return {TokenOp::help};
  if (token == "getNext")
    return {TokenOp::get_next};
  throw Unsupported("Unknown native token");
}
void Game::token(const Token &t, bool strict) {
  require(!hero[won50], "Input after victory");
  if (!initialized) {
    initialized = true;
    if (hero.floor == 1 && !flag_value(world->ids.feature))
      start(world->floors[1].first_arrive, 1, hero.pos);
  }
  if (t.op == TokenOp::choice) {
    choose(t.argument);
    return;
  }
  require(frames.empty() && !choice, "Input while event pending");
  switch (t.op) {
  case TokenOp::step:
    step(Direction(t.argument), strict);
    break;
  case TokenOp::move:
    require(direct_enabled() && can_connect(hero.floor, hero.pos, t.pos, true), "Illegal direct movement");
    hero.pos = t.pos;
    auto_sweep();
    break;
  case TokenOp::fly:
    fly(t.argument);
    break;
  case TokenOp::turn:
    if (t.argument >= 0)
      facing = Direction(t.argument);
    else
      facing = facing == Direction::up      ? Direction::right
               : facing == Direction::right ? Direction::down
               : facing == Direction::down  ? Direction::left
                                            : Direction::up;
    break;
  case TokenOp::item:
    tool(t.argument);
    break;
  case TokenOp::key:
    if (t.argument == 49)
      tool(world->item_ids.at("pickaxe"), false);
    else if (t.argument == 50)
      tool(world->item_ids.at("bomb"), false);
    else if (t.argument == 52) {
      bool used = false;
      for (const char *name : {"icePickaxe", "freezeBadge", "earthquake", "upFly", "downFly", "jumpShoes",
                               "lifeWand", "poisonWine", "weakWine", "curseWine", "superWine"}) {
        auto it = world->item_ids.find(name);
        if (it == world->item_ids.end() || item_count(it->second) <= 0)
          continue;
        auto trial = transaction();
        try {
          tool(it->second, false);
          trial.commit();
          used = true;
          break;
        } catch (const Unsupported &) {
          throw;
        } catch (const Error &) {
        }
      }
      require(used, "No usable shortcut item");
    } else
      throw Unsupported("Unknown shortcut key");
    break;
  case TokenOp::help:
    start(world->help, hero.floor, hero.pos);
    return;
  case TokenOp::get_next:
    if (gentle_click) {
      std::array<Direction, 5> dirs = {facing, Direction::up, Direction::down, Direction::left,
                                       Direction::right};
      for (auto d : dirs) {
        auto p = add(hero.pos, delta(d));
        auto b = block(hero.floor, p);
        if (!b.entity || !present(hero.floor, p) || !can_move(hero.floor, hero.pos, p))
          continue;
        const auto &v = props(hero.floor, p);
        if (v.trigger != Trigger::item || v.script || v.event_truth)
          continue;
        apply_item(hero.floor, b.entity);
        remove(hero.floor, p);
        after(1, hero.floor, p);
        return;
      }
    }
    return;
  case TokenOp::choice:
    break;
  }
  check_auto();
}
} // namespace engine
