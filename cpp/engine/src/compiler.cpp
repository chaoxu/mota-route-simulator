#include "engine/aot.hpp"
#include "engine/bridge.hpp"
#include <algorithm>
#include <regex>

namespace engine::io {
namespace {
#include "precompile.inc"
Json actions(Json v) {
  if (v.is_null())
    return Json::array();
  if (v.is_array())
    return v;
  if (v.is_object() && v.contains("data") && !v.contains("type"))
    return v.at("data");
  return Json::array({v});
}
std::uint64_t source_hash(const std::string &s) {
  std::uint64_t h = 14695981039346656037ULL;
  for (unsigned char c : s) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  return h;
}
Assign assign_op(const std::string &s) {
  if (s == "=")
    return Assign::set;
  if (s == "+=")
    return Assign::add;
  if (s == "-=")
    return Assign::sub;
  if (s == "*=")
    return Assign::mul;
  if (s == "/=")
    return Assign::div;
  if (s == "%=")
    return Assign::mod;
  throw Unsupported("Unknown assignment " + s);
}
std::uint8_t movement(const Json &j) {
  std::uint8_t out = 0;
  if (j.is_array())
    for (const auto &v : j) {
      auto d = direction(v.get<std::string>());
      if (int(d) < 4)
        out |= 1 << int(d);
    }
  return out;
}
} // namespace
Json compiled_source(Json source) {
  precompile_event(source);
  return source;
}
int Compiler::save(const Json &j) {
  sources.push_back(j);
  return int(sources.size() - 1);
}
std::uint32_t Compiler::identity_id(const Json &j) {
  auto key = j.dump();
  auto [it, inserted] = identity_ids.try_emplace(std::move(key), std::uint32_t(identity_ids.size() + 1));
  return it->second;
}
Value Compiler::value(const Json &j) {
  if (j.is_null())
    return {};
  if (j.is_boolean())
    return j.get<bool>();
  if (j.is_number_integer())
    return j.get<Number>();
  if (j.is_number_float())
    return j.get<double>();
  if (j.is_string())
    return j.get<std::string>();
  if (j.is_array() && std::all_of(j.begin(), j.end(), [](const auto &v) { return v.is_number_integer(); }))
    return NumberArray{j.get<std::vector<Number>>()};
  opaque.push_back(j);
  world->opaque_identity.resize(opaque.size());
  world->opaque_identity.back() = identity_id(j);
  return Opaque{int(opaque.size() - 1), !j.empty()};
}
int Compiler::expression(const Json &j) {
  if (j.is_string())
    return compile_expression(*world, j.get_ref<const std::string &>());
  Expr e;
  e.literal = value(j);
  world->expressions.push_back(std::move(e));
  return int(world->expressions.size() - 1);
}
int Compiler::program(const Json &raw, bool compile) {
  auto data = actions(raw);
  if (compile)
    precompile_event(data);
  const auto key = data.dump();
  auto found = program_cache.find(key);
  if (found != program_cache.end())
    return found->second;
  int id = int(world->programs.size());
  world->programs.push_back({});
  program_sources.push_back(data);
  program_cache.emplace(key, id);
  std::vector<int> code;
  if (data.is_array())
    for (const auto &entry : data)
      code.push_back(instruction(entry));
  else
    code.push_back(instruction(data));
  world->programs[id].code = std::move(code);
  return id;
}
int Compiler::instruction(const Json &a) try {
  Instruction i;
  i.source = save(a);
  if (a.is_null() || a.is_string()) {
    world->instructions.push_back(std::move(i));
    return int(world->instructions.size() - 1);
  }
  if (!a.is_object())
    throw Unsupported("Nonobject event command");
  if (a.value("_disabled", false)) {
    world->instructions.push_back(std::move(i));
    return int(world->instructions.size() - 1);
  }
  auto type = a.value("type", std::string("text"));
  static const std::unordered_map<std::string, Op> operations = {{"setValue", Op::assign},
                                                                 {"if", Op::if_else},
                                                                 {"switch", Op::switch_case},
                                                                 {"choices", Op::choices},
                                                                 {"while", Op::while_loop},
                                                                 {"for", Op::for_loop},
                                                                 {"break", Op::break_loop},
                                                                 {"exit", Op::exit},
                                                                 {"insert", Op::insert},
                                                                 {"function", Op::function},
                                                                 {"hide", Op::hide},
                                                                 {"show", Op::show},
                                                                 {"setBlock", Op::set_block},
                                                                 {"setBgFgBlock", Op::set_layer},
                                                                 {"openDoor", Op::open_door},
                                                                 {"closeDoor", Op::close_door},
                                                                 {"moveHero", Op::move_hero},
                                                                 {"move", Op::move_block},
                                                                 {"generateMove", Op::generate_move},
                                                                 {"changeFloor", Op::change_floor},
                                                                 {"setEnemy", Op::set_enemy},
                                                                 {"battle", Op::battle},
                                                                 {"win", Op::win},
                                                                 {"playBgm", Op::bgm}};
  static const std::set<std::string> presentation = {
      "comment",   "text",         "tip",          "playSound",    "stopSound",  "sleep",
      "waitAsync", "setText",      "setCurtain",   "setAttribute", "update",     "previewUI",
      "fillRect",  "fillBoldText", "drawSelector", "clearMap",     "strokeRect", "animate"};
  auto operation = operations.find(type);
  if (operation == operations.end()) {
    i.op = presentation.contains(type) ? Op::noop : Op::unsupported;
    i.name = "Unsupported source command " + type;
    world->instructions.push_back(std::move(i));
    return int(world->instructions.size() - 1);
  }
  i.op = operation->second;
  if (a.contains("floorId") && !a["floorId"].is_null() && a["floorId"] != "") {
    auto s = a["floorId"].get<std::string>();
    i.floor = s == ":next" ? -2 : s == ":before" ? -3 : floor_number(s);
  }
  if (a.contains("loc")) {
    i.has_location = true;
    auto loc = a.at("loc");
    if (!loc.is_array())
      throw Unsupported("Invalid event location");
    if (loc.size() == 2 && !loc[0].is_array())
      i.locations.push_back({expression(loc[0]), expression(loc[1])});
    else
      for (const auto &p : loc)
        i.locations.push_back({expression(p.at(0)), expression(p.at(1))});
  }
  i.remove = a.value("remove", false);
  i.destruct = a.value("destruct", false);
  i.keep = a.value("keep", false);
  i.need_key = a.value("needKey", false);
  i.animate = a.value("time", 0) > 0;
  auto block = [&](const Json &j) {
    if (j.is_number_integer()) {
      int tile = j.get<int>();
      return tile == 0 ? Block{} : world->tiles.at(tile);
    }
    auto s = j.get<std::string>();
    if (s == "0")
      return Block{};
    if (!s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); }))
      return world->tiles.at(std::stoi(s));
    return world->entities.at(world->entity_ids.at(s)).block;
  };
  auto common = [&](const std::string &name) {
    auto it = world->common.find(name);
    if (it != world->common.end())
      return it->second;
    const auto &all = reference_world->native_data.at("commonEvent");
    if (!all.contains(name))
      throw Unsupported("Missing common event " + name);
    int id = program(all[name]);
    world->common[name] = id;
    return id;
  };
  switch (i.op) {
  case Op::assign:
    i.target = bind_target(*world, a.at("name").get<std::string>());
    i.value = expression(a.at("value"));
    i.assign = assign_op(a.value("operator", std::string("=")));
    break;
  case Op::if_else:
    i.condition = expression(a.at("condition"));
    i.yes = program(a.value("true", Json::array()));
    i.no = program(a.value("false", Json::array()));
    break;
  case Op::switch_case:
    i.condition = expression(a.at("condition"));
    for (const auto &c : a.at("caseList"))
      if (!c.value("_disabled", false)) {
        bool def = c.at("case") == "default";
        i.cases.push_back({def ? 0 : expression(c.at("case")), program(c.value("action", Json::array())), def,
                           c.value("nobreak", false)});
      }
    break;
  case Op::choices:
    for (const auto &c : a.at("choices")) {
      if (c.value("_disabled", false))
        continue;
      Choice v;
      v.source = save(c);
      if (c.contains("condition") && !c["condition"].is_null() && c["condition"] != "")
        v.condition = expression(c["condition"]);
      if (c.contains("need") && !c["need"].is_null() && c["need"] != "")
        v.need = expression(c["need"]);
      v.program = program(c.value("action", Json::array()));
      i.choices.push_back(v);
    }
    break;
  case Op::while_loop:
    i.condition = expression(a.at("condition"));
    i.program = program(a.at("data"), false);
    break;
  case Op::for_loop: {
    auto name = a.at("name").get<std::string>();
    if (name.size() != 6 || !name.starts_with("temp:") || name[5] < 'A' || name[5] > 'Z') {
      i.op = Op::noop;
      break;
    }
    i.target = bind_target(*world, name);
    i.from = expression(a.at("from"));
    i.to = expression(a.at("to"));
    i.step = expression(a.at("step"));
    i.program = program(a.at("data"), false);
    i.number = intern_flag(*world, "@temp@for-to@" + name.substr(5));
    i.entity = intern_flag(*world, "@temp@for-step@" + name.substr(5));
    i.arguments = {value(a.at("to")), value(a.at("step"))};
    break;
  }
  case Op::break_loop:
    i.number = a.value("n", 1);
    break;
  case Op::insert:
    if (!a.contains("name")) {
      i.op = Op::unsupported;
      i.name = "Coordinate insert is unsupported";
      break;
    }
    i.name = a.at("name").get<std::string>();
    i.program = common(i.name);
    if (a.contains("args"))
      for (const auto &v : a["args"])
        i.arguments.push_back(value(v));
    for (std::size_t n = 0; n < i.arguments.size(); ++n)
      intern_flag(*world, "arg" + std::to_string(n + 1));
    break;
  case Op::function:
    i.function = source_hash(a.at("function").get<std::string>());
    break;
  case Op::set_block:
  case Op::set_layer: {
    auto b = block(a.at("number"));
    i.entity = b.entity;
    i.number = b.tile;
    if (i.op == Op::set_layer) {
      auto name = a.at("name").get<std::string>();
      if (name != "fg" && name != "bg")
        throw Unsupported("Invalid map layer");
      i.foreground = name == "fg";
    }
    break;
  }
  case Op::close_door: {
    auto it = world->entity_ids.find(a.value("id", std::string()));
    i.entity = it == world->entity_ids.end() ? 0 : it->second;
    break;
  }
  case Op::generate_move: {
    auto b = block(a.at("id"));
    i.entity = b.entity;
    [[fallthrough]];
  }
  case Op::move_block:
  case Op::move_hero:
    for (const auto &step : a.value("steps", Json::array())) {
      auto s = step.get<std::string>();
      auto colon = s.find(':');
      auto name = s.substr(0, colon);
      if (name == "speed")
        continue;
      i.motions.push_back({direction(name), colon == std::string::npos ? 1 : std::stoi(s.substr(colon + 1))});
    }
    break;
  case Op::change_floor:
    i.name = a.value("stair", std::string());
    i.number = a.contains("direction") ? int(direction(a.at("direction").get<std::string>())) : -1;
    i.keep = a.value("ignoreChangeFloor", world->ignore_stairs);
    i.ignore_present = a.contains("ignoreChangeFloor");
    break;
  case Op::set_enemy: {
    i.entity = world->entity_ids.at(a.at("id").get<std::string>());
    auto name = a.at("name").get<std::string>();
    i.number = -1;
    for (int n = 0; n < 12; ++n)
      if (name == enemy_fields[n])
        i.number = n;
    if (i.number < 0)
      throw Unsupported("Unknown enemy property " + name);
    i.value = expression(a.at("value"));
    i.assign = assign_op(a.value("operator", std::string("=")));
    break;
  }
  case Op::battle:
    if (a.contains("id") && !a["id"].is_null() && a["id"] != "") {
      i.has_id = true;
      i.entity = world->entity_ids.at(a["id"].get<std::string>());
    }
    break;
  case Op::win: {
    i.reason = a.value("reason", std::string());
    std::size_t cursor = 0;
    while ((cursor = i.reason.find("${", cursor)) != std::string::npos) {
      auto end = i.reason.find('}', cursor + 2);
      if (end == std::string::npos)
        throw Unsupported("Invalid reason template");
      i.locations.push_back({compile_expression(*world, i.reason.substr(cursor + 2, end - cursor - 2)), 0});
      cursor = end + 1;
    }
    break;
  }
  case Op::bgm:
    i.name = a.value("name", std::string());
    break;
  default:
    break;
  }
  world->instructions.push_back(std::move(i));
  return int(world->instructions.size() - 1);
} catch (const std::exception &e) {
  throw Error("Compiling event " + a.dump() + ": " + e.what());
}
Props Compiler::properties(const Json &j) {
  Props p;
  if (!j.is_object())
    return p;
  p.no_pass = rule_truth(value(j.value("noPass", Json(true))));
  if (j.contains("noPass") && !j["noPass"].is_null())
    p.specified |= 1;
  if (j.contains("trigger") && !j["trigger"].is_null()) {
    p.specified |= 2;
    auto t = j.value("trigger", std::string());
    p.trigger = t.empty()            ? Trigger::none
                : t == "action"      ? Trigger::action
                : t == "battle"      ? Trigger::battle
                : t == "getItem"     ? Trigger::item
                : t == "openDoor"    ? Trigger::door
                : t == "changeFloor" ? Trigger::change_floor
                                     : Trigger::other;
  }
  if (j.contains("data")) {
    p.specified |= 4;
    p.has_data = true;
    if (p.trigger == Trigger::change_floor) {
      auto a = j.at("data");
      a["type"] = "changeFloor";
      if (a.contains("loc"))
        a.erase("stair");
      p.change = instruction(a);
    } else
      p.action = program(j.at("data"));
  }
  if (j.contains("event")) {
    p.specified |= 8;
    p.event_truth = rule_truth(value(j.at("event")));
    if (truth(value(j.at("event"))))
      p.touch = program(j.at("event"));
  }
  if (j.contains("outEvent")) {
    p.specified |= 16;
    p.out_event = program(j.at("outEvent"));
  }
  if (j.contains("cannotIn")) {
    p.specified |= 32;
    p.in = movement(j.at("cannotIn"));
  }
  if (j.contains("cannotOut")) {
    p.specified |= 64;
    p.out = movement(j.at("cannotOut"));
  }
  if (j.contains("script")) {
    p.specified |= 128;
    p.script = rule_truth(value(j["script"]));
  }
  if (j.contains("doorInfo") && !j["doorInfo"].is_null()) {
    p.specified |= 256;
    p.door = true;
    auto keys = j["doorInfo"].value("keys", Json::object());
    for (auto it = keys.begin(); it != keys.end(); ++it) {
      auto name = it.key();
      bool preserve = name.ends_with(":o");
      if (preserve)
        name.resize(name.size() - 2);
      auto item = world->item_ids.find(name);
      p.keys.push_back(
          {item == world->item_ids.end() ? -1 : item->second, it.value().get<Number>(), preserve});
    }
  }
  if (j.contains("canBreak")) {
    p.specified |= 512;
    p.can_break = rule_truth(value(j.value("canBreak", Json(false))));
  }
  if (j.contains("cls")) {
    p.specified |= 1024;
    p.cls = j.value("cls", std::string("terrains"));
    p.door_class = p.cls == "animates" || p.cls == "npc48";
  }
  if (j.contains("id")) {
    p.specified |= 2048;
    p.id = j.value("id", std::string("none"));
  }
  return p;
}
static bool compile_guard(const World &w, int id, std::vector<Guard> &out) {
  const auto &e = w.expressions[id];
  if (e.op == ExprOp::logical_and)
    return compile_guard(w, e.args[0], out) && compile_guard(w, e.args[1], out);
  if (e.op == ExprOp::reference && e.target.kind == Ref::flag) {
    out.push_back({GuardOp::flag, e.target.index});
    return true;
  }
  if (e.op != ExprOp::seq && e.op != ExprOp::sne && e.op != ExprOp::eq && e.op != ExprOp::ne)
    return false;
  const Expr *a = &w.expressions[e.args[0]], *b = &w.expressions[e.args[1]];
  if (a->op == ExprOp::literal)
    std::swap(a, b);
  if (a->op != ExprOp::call || !a->bound || b->op != ExprOp::literal)
    return false;
  if (a->function != Function::block_id && a->function != Function::block_cls &&
      a->function != Function::block)
    return false;
  Guard g;
  g.floor = a->bound_floor;
  g.pos = a->bound_pos;
  g.negate = e.op == ExprOp::sne || e.op == ExprOp::ne;
  if (b->literal.index() == 0)
    g.op = GuardOp::is_null;
  else if (auto *s = std::get_if<std::string>(&b->literal)) {
    if (a->function == Function::block)
      return false;
    g.op = a->function == Function::block_id ? GuardOp::block_id : GuardOp::block_class;
    g.value = *s;
  } else
    return false;
  out.push_back(std::move(g));
  return true;
}
Compiler::Compiler(std::shared_ptr<const mota::World> reference, bool corridors)
    : reference_world(std::move(reference)), build_corridors(corridors) {
  auto &w = *world;
  w.first_floor = reference_world->first_floor;
  w.item_stat.fill(-1);
  w.item_class.fill(0);
  w.payloads.push_back({});
  const auto &native = reference_world->native_data;
  w.near_stair = native.at("flags").value("flyNearStair", true);
  w.direct_enabled = native.at("flags").value("enableMoveDirectly", true);
  w.gentle = native.at("flags").value("enableGentleClick", false);
  w.ignore_stairs = native.at("flags").value("ignoreChangeFloor", false);
  const auto &values = native.at("values");
  w.hatred = values.value("hatred", Number(0));
  w.green_potion = values.value("greenPotion", Number(0));
  w.yellow_potion = values.value("yellowPotion", Number(0));
  w.green_gem = values.value("greenGem", Number(0));
  for (auto it = native.at("items").begin(); it != native.at("items").end(); ++it) {
    int id = int(w.item_names.size());
    if (id >= 256)
      throw Unsupported("Too many source items");
    w.item_names.push_back(it.key());
    w.item_ids[it.key()] = id;
    auto cls = it.value().value("cls", std::string("tools"));
    w.item_class[id] = cls == "items" ? 0 : cls == "constants" ? 1 : cls == "tools" ? 2 : 3;
    for (int s = 0; s < stat_count; ++s)
      if (it.key() == stat_names[s])
        w.item_stat[id] = s;
    if (it.key() == "yellowKey")
      w.item_stat[id] = yk;
    if (it.key() == "blueKey")
      w.item_stat[id] = bk;
    if (it.key() == "redKey")
      w.item_stat[id] = rk;
  }
  for (std::size_t id = 0; id < reference_world->entities.size(); ++id) {
    const auto &s = reference_world->entities[id];
    Entity e;
    e.id = s.id;
    e.kind = Kind(s.kind);
    if (e.id == "flower")
      e.features |= EntityFeature::flower;
    if (e.id == "lava")
      e.features |= EntityFeature::lava;
    if (e.id == "fakeWall")
      e.features |= EntityFeature::fake_wall;
    if (e.id == "redPotion" || e.id == "bluePotion" || e.id == "greenPotion" || e.id == "yellowPotion" ||
        e.id == "superPotion")
      e.features |= EntityFeature::potion;
    if (e.id == "upFloor" || e.id == "downFloor" || e.id == "portal")
      e.features |= EntityFeature::stair;
    if (e.id == "redGem" || e.id == "blueGem" || e.id == "greenGem" || e.id == "yellowGem" ||
        e.id.starts_with("sword") || e.id.starts_with("shield") || e.id == "cross" || e.id == "knife" ||
        e.id == "snow")
      e.features |= EntityFeature::combat_item;
    if (e.id == "zombie" || e.id == "zombieKnight" || e.id == "vampire")
      e.features |= EntityFeature::cross_enemy;
    if (e.id == "magicDragon")
      e.features |= EntityFeature::dragon;
    e.hp = s.hp;
    e.atk = s.atk;
    e.def = s.def;
    e.money = s.money;
    e.n = s.n;
    e.def_value = s.def_value;
    e.atk_value = s.atk_value;
    e.value = s.value;
    e.range = s.range;
    e.special = s.special;
    e.square = s.square;
    e.block.entity = std::uint16_t(id);
    w.entity_ids[e.id] = int(id);
    auto item = w.item_ids.find(e.id);
    if (item != w.item_ids.end()) {
      e.item_index = item->second;
      e.item_class = w.item_class[item->second];
      e.inventory_stat = w.item_stat[item->second];
    }
    if (native.at("enemies").contains(e.id)) {
      const auto &j = native.at("enemies").at(e.id);
      e.exp_present = j.contains("exp");
      e.exp = j.contains("exp") && j["exp"].is_number() ? j["exp"].get<Number>() : 0;
    }
    w.entities.push_back(std::move(e));
  }
  for (const auto &[tile, b] : reference_world->tiles) {
    w.tiles[tile] = {b.entity, b.tile};
    if (b.entity && w.entities[b.entity].block.tile == 0)
      w.entities[b.entity].block.tile = b.tile;
  }
  for (int f = 0; f < floors_count; ++f) {
    w.dialogue_flags[f][0] = intern_flag(w, "对话" + std::to_string(f));
    w.dialogue_flags[f][1] = intern_flag(w, "对话" + std::to_string(f) + "02");
    for (int c = 0; c < cells; ++c)
      w.floors[f].switch_slot[c] = intern_flag(w, floor_name(f) + "@" + std::to_string(c % side) + "@" +
                                                      std::to_string(c / side) + "@A");
  }
  for (int r = 0; r < 6; ++r) {
    auto prefix = std::to_string(r) + "区购买";
    w.purchase_atk[r] = intern_flag(w, prefix + "atk");
    w.purchase_def[r] = intern_flag(w, prefix + "def");
    w.purchase_hp[r] = intern_flag(w, prefix + "hp");
    w.purchase_atk_count[r] = intern_flag(w, prefix + "atk次数");
    w.purchase_def_count[r] = intern_flag(w, prefix + "def次数");
    w.purchase_hp_count[r] = intern_flag(w, prefix + "hp次数");
  }
  for (std::size_t id = 0; id < w.entities.size(); ++id) {
    auto raw = reference_world->entities[id].native;
    if (id == 0)
      raw = {{"cls", "terrains"}, {"id", "none"}, {"noPass", false}};
    w.entities[id].props = properties(raw);
    auto name = w.entities[id].id;
    if (native.at("enemies").contains(name))
      w.entities[id].after_battle = program(native.at("enemies").at(name).value("afterBattle", Json()));
  }
  for (int f = w.first_floor; f < floors_count; ++f) {
    const auto &src = reference_world->floors[f];
    const auto &m = src.metadata;
    auto &fd = w.floors[f];
    fd.ratio = src.ratio;
    fd.auto_battle_blocked = src.after_battle;
    fd.up = {src.up.x, src.up.y};
    fd.down = {src.down.x, src.down.y};
    fd.hidden = m.value("isHide", false);
    fd.fly_from = m.value("canFlyFrom", true);
    fd.fly_to = m.value("canFlyTo", true);
    fd.no_direct = m.value("cannotMoveDirectly", false);
    fd.first_arrive = program(m.value("firstArrive", Json::array()));
    fd.each_arrive = program(m.value("eachArrive", Json::array()));
    for (int c = 0; c < cells; ++c) {
      auto key = std::to_string(c % side) + "," + std::to_string(c / side);
      for (auto [field, array] : {std::pair{"afterBattle", &fd.after_battle},
                                  {"afterGetItem", &fd.after_item},
                                  {"afterOpenDoor", &fd.after_door}})
        if (m.contains(field) && m[field].contains(key))
          (*array)[c] = program(m[field][key]);
      for (auto [field, array] : {std::pair{"cannotMove", &fd.out}, {"cannotMoveIn", &fd.in}})
        if (m.contains(field) && m[field].contains(key))
          (*array)[c] = movement(m[field][key]);
      Json source = src.events.value(key, Json());
      if (!source.is_null())
        fd.source_event.set(c);
      if (source.is_string())
        source = Json{{"data", Json::array({source})}};
      else if (source.is_array())
        source = Json{{"data", source}};
      if (!source.is_object())
        source = Json::object();
      if (!source.empty() && (!source.contains("data") || source["data"].is_null()))
        source["data"] = Json::array();
      if (src.change_floor.contains(key)) {
        source["trigger"] = "changeFloor";
        source["data"] = src.change_floor.at(key);
      }
      if (!source.empty()) {
        auto props = properties(source);
        fd.source_payload[c] = int(w.payloads.size());
        w.payloads.push_back(std::move(props));
        payload_sources.push_back(source);
        if (w.payloads.back().trigger == Trigger::change_floor)
          fd.change[c] = w.payloads.back().change;
      }
    }
    if (m.contains("autoEvent"))
      for (auto loc = m["autoEvent"].begin(); loc != m["autoEvent"].end(); ++loc)
        for (auto event = loc.value().begin(); event != loc.value().end(); ++event) {
          if (!event->is_object())
            continue;
          auto comma = loc.key().find(',');
          int x = std::stoi(loc.key().substr(0, comma)), y = std::stoi(loc.key().substr(comma + 1));
          const auto &e = event.value();
          w.automatic.push_back(
              {f, y * side + x, expression(e.at("condition")), program(e.value("data", Json::array())),
               save(e), e.value("multiExecute", false), e.value("currentFloor", true),
               floor_name(f) + "@" + std::to_string(x) + "@" + std::to_string(y) + "@" + event.key()});
        }
  }
  for (auto &e : w.automatic) {
    e.guards_native = compile_guard(w, e.condition, e.guards);
    if (!e.guards_native)
      e.guards.clear();
  }
  for (int f = w.first_floor; f < floors_count; ++f)
    for (std::size_t i = 0; i < w.automatic.size(); ++i)
      if (!w.automatic[i].current_floor || w.automatic[i].floor == f)
        w.floors[f].automatic.push_back(int(i));
  w.lower();
  const auto &common = native.at("commonEvent");
  for (auto it = common.begin(); it != common.end(); ++it)
    if (!w.common.contains(it.key()))
      w.common[it.key()] = program(it.value());
  w.dialogue = w.common.at("对话");
  w.trader = w.common.at("商人");
  w.help = program(Json::array({{{"type", "insert"}, {"name", "游戏说明"}}}));
  constexpr std::array<Pos, 13> positions = {Pos{5, 4}, Pos{4, 4},  Pos{3, 4}, Pos{7, 4}, Pos{8, 4},
                                             Pos{9, 4}, Pos{4, 2},  Pos{3, 2}, Pos{2, 2}, Pos{8, 2},
                                             Pos{9, 2}, Pos{10, 2}, Pos{6, 1}};
  for (int i = 0; i < 13; ++i) {
    auto q = positions[i];
    Json steps = Json::array();
    int dx = 6 - q.x, dy = 7 - q.y;
    if (dx)
      steps.push_back(std::string(dx > 0 ? "right:" : "left:") + std::to_string(std::abs(dx)));
    if (dy)
      steps.push_back(std::string(dy > 0 ? "down:" : "up:") + std::to_string(std::abs(dy)));
    w.march_programs[i] =
        program(Json::array({{{"type", "move"}, {"loc", {q.x, q.y}}, {"keep", true}, {"steps", steps}},
                             {{"type", "battle"}, {"loc", {6, 7}}}}));
  }
  w.march_tail =
      program(Json::array({{{"type", "setBlock"}, {"number", "upFloor"}, {"loc", {{6, 1}}}},
                           {{"type", "setBlock"}, {"number", "yellowKnight"}, {"loc", {{6, 7}}}},
                           {{"type", "move"}, {"loc", {6, 7}}, {"keep", false}, {"steps", {"up:6"}}}}));
  // Match compiled handlers to immutable source and its resolved ID layout.
  w.source_tag = 14695981039346656037ULL;
  auto hash = [&](std::string_view s) {
    for (unsigned char c : s)
      w.source_tag = (w.source_tag ^ c) * 1099511628211ULL;
    w.source_tag = (w.source_tag ^ 255) * 1099511628211ULL;
  };
  hash(reference_world->native_data.dump());
  for (int f = w.first_floor; f < floors_count; ++f)
    hash(reference_world->floors[f].metadata.dump());
  for (const auto &s : w.flag_names)
    hash(s);
  for (const auto &e : w.entities) {
    hash(e.id);
    hash(std::to_string(e.block.tile));
  }
  for (const auto &program : w.programs)
    for (int id : program.code)
      hash(std::to_string(id));
  install_aot(w);
}
} // namespace engine::io
