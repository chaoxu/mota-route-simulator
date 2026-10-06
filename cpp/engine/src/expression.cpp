#include "engine/core.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace engine {
bool truth(const Value &v) {
  return std::visit(
      [](const auto &x) -> bool {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return false;
        else if constexpr (std::is_same_v<T, std::string>)
          return !x.empty();
        else if constexpr (std::is_arithmetic_v<T>)
          return x != 0;
        else
          return true;
      },
      v);
}
bool rule_truth(const Value &v) {
  if (auto *n = std::get_if<double>(&v))
    return Number(*n) != 0;
  if (auto *a = std::get_if<NumberArray>(&v))
    return !a->values.empty();
  if (auto *o = std::get_if<Opaque>(&v))
    return o->truth;
  return truth(v);
}
double numeric(const Value &v) {
  return std::visit(
      [](const auto &x) -> double {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return 0;
        else if constexpr (std::is_arithmetic_v<T>)
          return double(x);
        else if constexpr (std::is_same_v<T, std::string>) {
          if (x.find_first_not_of(" \r\n\t") == std::string::npos)
            return 0;
          char *end;
          double n = std::strtod(x.c_str(), &end);
          while (*end && std::isspace(static_cast<unsigned char>(*end)))
            ++end;
          if (*end || end == x.c_str())
            throw Error("Invalid numeric string");
          return n;
        } else
          throw Unsupported("Object numeric coercion");
      },
      v);
}
Number integer(const Value &v) {
  if (std::holds_alternative<std::monostate>(v))
    return 0;
  if (const auto *n = std::get_if<Number>(&v))
    return *n;
  if (const auto *n = std::get_if<double>(&v))
    return Number(*n);
  if (const auto *b = std::get_if<bool>(&v))
    return *b;
  return 0;
}
std::string text(const Value &v) {
  if (const auto *s = std::get_if<std::string>(&v))
    return *s;
  if (const auto *n = std::get_if<Number>(&v))
    return std::to_string(*n);
  if (const auto *n = std::get_if<double>(&v))
    return std::to_string(*n);
  if (const auto *b = std::get_if<bool>(&v))
    return *b ? "true" : "false";
  if (std::holds_alternative<std::monostate>(v))
    return "null";
  throw Unsupported("Object string coercion");
}
Value number_value(double n) {
  if (!std::isfinite(n))
    throw Error("Non-finite expression");
  if (std::trunc(n) == n && std::abs(n) <= 9007199254740991.0)
    return Number(n);
  return n;
}
bool equal_value(const Value &a, const Value &b, bool strict) {
  bool na = std::holds_alternative<Number>(a) || std::holds_alternative<double>(a),
       nb = std::holds_alternative<Number>(b) || std::holds_alternative<double>(b);
  if (na && nb)
    return numeric(a) == numeric(b);
  if (a.index() > 4 || b.index() > 4) {
    if (a.index() == 0 || b.index() == 0)
      return false;
    throw Unsupported("Object equality");
  }
  if (a.index() == b.index())
    return a == b;
  if (strict || a.index() == 0 || b.index() == 0)
    return false;
  try {
    return numeric(a) == numeric(b);
  } catch (const Error &) {
    return false;
  }
}
std::uint64_t specials(const Value &v) {
  std::uint64_t mask = 0;
  auto add = [&](Number n) {
    if (n >= 0 && n < 64)
      mask |= 1ULL << n;
  };
  if (auto *a = std::get_if<NumberArray>(&v)) {
    for (auto n : a->values)
      add(n);
  } else if (std::holds_alternative<Number>(v) || std::holds_alternative<double>(v))
    add(integer(v));
  return mask;
}
int intern_flag(World &w, std::string_view name) {
  auto key = std::string(name);
  auto it = w.flag_ids.find(key);
  if (it != w.flag_ids.end())
    return it->second;
  int id = int(w.flag_names.size());
  w.flag_names.push_back(key);
  w.flag_ids.emplace(key, id);
  bool temporary =
      name.starts_with("@temp@") ||
      (name.size() > 3 && name.starts_with("arg") &&
       std::all_of(name.begin() + 3, name.end(), [](unsigned char c) { return std::isdigit(c); }));
  w.is_temporary.push_back(temporary);
  if (temporary)
    w.temporary_flags.push_back(id);
  return id;
}
World::World() {
  for (auto name : builtin_flag_names)
    intern_flag(*this, name);
}
static int stat(std::string_view name) {
  if (name == "money")
    return gold;
  for (int i = 0; i < stat_count; ++i)
    if (name == stat_names[i])
      return i;
  return -1;
}
Target bind_target(World &w, std::string_view name) {
  if (name.starts_with("flag:"))
    return {Ref::flag, intern_flag(w, name.substr(5))};
  if (name.starts_with("temp:"))
    return {Ref::flag, intern_flag(w, "@temp@" + std::string(name.substr(5)))};
  if (name.starts_with("switch:")) {
    if (name != "switch:A")
      throw Unsupported("Only source switch A is available");
    return {Ref::switch_flag, 0};
  }
  if (name.starts_with("item:")) {
    auto it = w.item_ids.find(std::string(name.substr(5)));
    if (it == w.item_ids.end())
      throw Unsupported("Unknown item " + std::string(name));
    return {Ref::item, it->second};
  }
  if (name.starts_with("status:")) {
    auto key = name.substr(7);
    int i = stat(key);
    if (i >= 0)
      return {Ref::stat, i};
    if (key == "x")
      return {Ref::status_x};
    if (key == "y")
      return {Ref::status_y};
    if (key == "direction")
      return {Ref::status_direction};
    auto [it, inserted] = w.extra_ids.emplace(std::string(key), int(w.extra_names.size()));
    if (inserted)
      w.extra_names.emplace_back(key);
    return {Ref::extra, it->second};
  }
  if (name == "core.status.hero.flags.__auto__.battle")
    return {Ref::auto_battle};
  if (name == "core.status.hero.flags.__auto__.wall")
    return {Ref::auto_wall};
  if (name == "core.status.hero.flags.__auto__.item")
    return {Ref::auto_item};
  if (name == "core.status.floorId")
    return {Ref::floor};
  if (name == "main.levelChoose.length")
    return {Ref::zero};
  throw Unsupported("Unknown expression reference " + std::string(name));
}
namespace {
struct Parser {
  World &w;
  std::string_view input;
  std::size_t cursor = 0;
  std::string token;
  enum { symbol, name, number, string, end } kind = end;
  int emit(Expr e) {
    if (e.op == ExprOp::call && !e.args.empty()) {
      auto literal = [&](int i) {
        return std::size_t(i) < e.args.size() && w.expressions[e.args[i]].op == ExprOp::literal;
      };
      const auto *name = literal(0) ? std::get_if<std::string>(&w.expressions[e.args[0]].literal) : nullptr;
      if (name && e.args.size() <= 2 &&
          (e.function == Function::get_flag || e.function == Function::has_flag)) {
        e.bound = true;
        e.bound_index = intern_flag(w, *name);
      }
      if (name && e.args.size() == 1) {
        if (e.function == Function::item_count || e.function == Function::has_item) {
          auto it = w.item_ids.find(*name);
          e.bound = true;
          e.bound_index = it == w.item_ids.end() ? -1 : it->second;
        }
        if (e.function == Function::get_status) {
          e.bound = true;
          int index = stat(*name);
          e.target = index >= 0             ? Target{Ref::stat, index}
                     : *name == "x"         ? Target{Ref::status_x}
                     : *name == "y"         ? Target{Ref::status_y}
                     : *name == "direction" ? Target{Ref::status_direction}
                                            : Target{Ref::zero};
        }
        if (e.function == Function::hero_loc) {
          e.bound = true;
          e.target = *name == "x"   ? Target{Ref::status_x}
                     : *name == "y" ? Target{Ref::status_y}
                                    : Target{Ref::status_direction};
        }
        if (e.function == Function::visited)
          try {
            e.bound_index = floor_number(*name);
            e.bound = true;
          } catch (const Error &) {
          }
        if (e.function == Function::can_battle || e.function == Function::enemy_info) {
          auto it = w.entity_ids.find(*name);
          if (it != w.entity_ids.end()) {
            e.bound = true;
            e.bound_index = it->second;
          }
        }
      }
      if ((e.function == Function::block_id || e.function == Function::block_cls ||
           e.function == Function::block) &&
          e.args.size() >= 2 && e.args.size() <= 3 && literal(0) && literal(1) &&
          (e.args.size() == 2 || literal(2))) {
        bool floor_ok = true;
        if (e.args.size() == 3) {
          const auto &arg = w.expressions[e.args[2]].literal;
          if (arg.index() != 0) {
            if (auto *s = std::get_if<std::string>(&arg))
              try {
                e.bound_floor = floor_number(*s);
              } catch (const Error &) {
                floor_ok = false;
              }
            else
              floor_ok = false;
          }
        }
        if (floor_ok) {
          e.bound = true;
          e.bound_pos = {int(integer(w.expressions[e.args[0]].literal)),
                         int(integer(w.expressions[e.args[1]].literal))};
        }
      }
    }
    w.expressions.push_back(std::move(e));
    return int(w.expressions.size() - 1);
  }
  [[noreturn]] void fail() {
    throw Unsupported("Invalid native expression near " + std::string(input.substr(cursor)) + " in " +
                      std::string(input));
  }
  static bool first(unsigned char c) { return std::isalpha(c) || c == '_' || c == '$' || c >= 128; }
  static bool rest(unsigned char c) { return first(c) || std::isdigit(c); }
  void next() {
    while (cursor < input.size() && std::isspace(static_cast<unsigned char>(input[cursor])))
      ++cursor;
    token.clear();
    if (cursor == input.size()) {
      kind = end;
      return;
    }
    char c = input[cursor++];
    kind = symbol;
    if (first(c)) {
      kind = name;
      token = c;
      while (cursor < input.size() && rest(input[cursor]))
        token += input[cursor++];
      if (cursor < input.size() && input[cursor] == ':' &&
          (token == "flag" || token == "status" || token == "item" || token == "temp" || token == "switch" ||
           token == "blockId" || token == "blockCls")) {
        bool coordinate = token == "blockId" || token == "blockCls";
        token += input[cursor++];
        while (cursor < input.size() && (rest(input[cursor]) || (coordinate && input[cursor] == ',')))
          token += input[cursor++];
      }
      return;
    }
    if (std::isdigit(static_cast<unsigned char>(c)) ||
        (c == '.' && cursor < input.size() && std::isdigit(static_cast<unsigned char>(input[cursor])))) {
      kind = number;
      token = c;
      while (cursor < input.size() &&
             (std::isdigit(static_cast<unsigned char>(input[cursor])) || input[cursor] == '.'))
        token += input[cursor++];
      if (cursor < input.size() && (input[cursor] == 'e' || input[cursor] == 'E')) {
        token += input[cursor++];
        if (cursor < input.size() && (input[cursor] == '+' || input[cursor] == '-'))
          token += input[cursor++];
        while (cursor < input.size() && std::isdigit(static_cast<unsigned char>(input[cursor])))
          token += input[cursor++];
      }
      return;
    }
    if (c == '\'' || c == '"') {
      kind = string;
      char quote = c;
      while (cursor < input.size()) {
        c = input[cursor++];
        if (c == quote)
          return;
        if (c != '\\') {
          token += c;
          continue;
        }
        if (cursor == input.size())
          fail();
        c = input[cursor++];
        switch (c) {
        case 'n':
          token += '\n';
          break;
        case 'r':
          token += '\r';
          break;
        case 't':
          token += '\t';
          break;
        case 'b':
          token += '\b';
          break;
        case 'f':
          token += '\f';
          break;
        case 'v':
          token += '\v';
          break;
        case 'u': {
          auto unit = [&]() {
            if (cursor + 4 > input.size())
              fail();
            unsigned n = 0;
            for (int i = 0; i < 4; ++i) {
              char h = input[cursor++];
              n *= 16;
              if (h >= '0' && h <= '9')
                n += h - '0';
              else if (h >= 'a' && h <= 'f')
                n += h - 'a' + 10;
              else if (h >= 'A' && h <= 'F')
                n += h - 'A' + 10;
              else
                fail();
            }
            return n;
          };
          unsigned code = unit();
          if (code >= 0xd800 && code <= 0xdbff) {
            if (input.substr(cursor, 2) != "\\u")
              fail();
            cursor += 2;
            unsigned lo = unit();
            if (lo < 0xdc00 || lo > 0xdfff)
              fail();
            code = 0x10000 + ((code - 0xd800) << 10) + (lo - 0xdc00);
          } else if (code >= 0xdc00 && code <= 0xdfff)
            fail();
          if (code < 128)
            token += char(code);
          else if (code < 2048) {
            token += char(192 | (code >> 6));
            token += char(128 | (code & 63));
          } else if (code < 65536) {
            token += char(224 | (code >> 12));
            token += char(128 | ((code >> 6) & 63));
            token += char(128 | (code & 63));
          } else {
            token += char(240 | (code >> 18));
            token += char(128 | ((code >> 12) & 63));
            token += char(128 | ((code >> 6) & 63));
            token += char(128 | (code & 63));
          }
          break;
        }
        default:
          if (c != '\\' && c != '\'' && c != '"' && c != '/')
            fail();
          token += c;
        }
      }
      fail();
    }
    token = c;
    for (std::string_view op : {"===", "!==", "&&", "||", "==", "!=", "<=", ">="})
      if (input.substr(cursor - 1, op.size()) == op) {
        token = op;
        cursor += op.size() - 1;
        return;
      }
  }
  bool take(std::string_view s) {
    if (kind != symbol || token != s)
      return false;
    next();
    return true;
  }
  void need(std::string_view s) {
    if (!take(s))
      fail();
  }
  int atom() {
    int id = 0;
    if (take("(")) {
      id = conditional();
      need(")");
    } else if (kind == string) {
      Expr e;
      e.literal = token;
      id = emit(std::move(e));
      next();
    } else if (kind == number) {
      Expr e;
      try {
        e.literal = number_value(std::stod(token));
      } catch (...) {
        e.op = ExprOp::error;
        e.name = "Invalid number literal";
      }
      id = emit(std::move(e));
      next();
    } else if (kind == name) {
      std::string n = token;
      next();
      while (take(".")) {
        if (kind != name)
          fail();
        n += '.' + token;
        next();
      }
      Expr e;
      if (take("(")) {
        e.op = ExprOp::call;
        e.name = n;
        if (!take(")")) {
          do {
            e.args.push_back(conditional());
          } while (take(","));
          need(")");
        }
        static const std::unordered_map<std::string, Function> functions = {
            {"Math.round", Function::round},
            {"Math.floor", Function::floor},
            {"Math.ceil", Function::ceil},
            {"Math.abs", Function::abs},
            {"Math.min", Function::min},
            {"Math.max", Function::max},
            {"core.getFlag", Function::get_flag},
            {"core.hasFlag", Function::has_flag},
            {"core.itemCount", Function::item_count},
            {"core.hasItem", Function::has_item},
            {"core.getStatus", Function::get_status},
            {"core.hasVisitedFloor", Function::visited},
            {"core.isReplaying", Function::replaying},
            {"core.getHeroLoc", Function::hero_loc},
            {"core.getBlockId", Function::block_id},
            {"core.getBlockCls", Function::block_cls},
            {"core.getBlock", Function::block},
            {"core.searchBlock", Function::search},
            {"core.canBattle", Function::can_battle},
            {"core.getEnemyInfo", Function::enemy_info},
            {"core.calValue", Function::cal_value}};
        auto it = functions.find(n);
        if (it == functions.end()) {
          e.op = ExprOp::error;
          e.name = "Unknown function " + n;
        } else
          e.function = it->second;
        if ((e.function == Function::get_flag || e.function == Function::has_flag) && !e.args.empty())
          if (auto *s = std::get_if<std::string>(&w.expressions[e.args[0]].literal))
            intern_flag(w, *s);
      } else if (n == "true" || n == "false")
        e.literal = n == "true";
      else if (n == "null" || n == "undefined")
        e.literal = std::monostate{};
      else if (n.starts_with("blockId:") || n.starts_with("blockCls:")) {
        auto colon = n.find(':'), comma = n.find(',', colon);
        if (comma == std::string::npos)
          fail();
        Expr x, y;
        x.literal = Number(std::stoi(n.substr(colon + 1, comma - colon - 1)));
        y.literal = Number(std::stoi(n.substr(comma + 1)));
        e.op = ExprOp::call;
        e.function = n.starts_with("blockId:") ? Function::block_id : Function::block_cls;
        e.args = {emit(std::move(x)), emit(std::move(y))};
      } else {
        try {
          e.op = ExprOp::reference;
          e.target = bind_target(w, n);
        } catch (const Unsupported &err) {
          e.op = ExprOp::error;
          e.name = err.what();
        }
      }
      id = emit(std::move(e));
    } else
      fail();
    while (take(".")) {
      if (kind != name)
        fail();
      Expr e;
      e.op = ExprOp::member;
      e.name = token;
      e.args = {id};
      id = emit(std::move(e));
      next();
    }
    return id;
  }
  int unary() {
    for (auto [op, kind] : {std::pair{"!", ExprOp::logical_not}, {"-", ExprOp::neg}, {"+", ExprOp::positive}})
      if (take(op)) {
        Expr e;
        e.op = kind;
        e.args = {unary()};
        return emit(std::move(e));
      }
    return atom();
  }
  static int precedence(std::string_view s) {
    if (s == "||")
      return 1;
    if (s == "&&")
      return 2;
    if (s == "==" || s == "!=" || s == "===" || s == "!==")
      return 3;
    if (s == "<" || s == "<=" || s == ">" || s == ">=")
      return 4;
    if (s == "+" || s == "-")
      return 5;
    if (s == "*" || s == "/" || s == "%")
      return 6;
    return 0;
  }
  int binary(int level = 1) {
    int left = unary();
    while (kind == symbol && precedence(token) >= level) {
      auto op = token;
      int p = precedence(op);
      next();
      int right = binary(p + 1);
      static const std::unordered_map<std::string, ExprOp> ops = {
          {"||", ExprOp::logical_or}, {"&&", ExprOp::logical_and}, {"==", ExprOp::eq}, {"!=", ExprOp::ne},
          {"===", ExprOp::seq},       {"!==", ExprOp::sne},        {"<", ExprOp::lt},  {"<=", ExprOp::le},
          {">", ExprOp::gt},          {">=", ExprOp::ge},          {"+", ExprOp::add}, {"-", ExprOp::sub},
          {"*", ExprOp::mul},         {"/", ExprOp::div},          {"%", ExprOp::mod}};
      Expr e;
      e.op = ops.at(op);
      e.args = {left, right};
      left = emit(std::move(e));
    }
    return left;
  }
  int conditional() {
    int id = binary();
    if (take("?")) {
      int yes = conditional();
      need(":");
      int no = conditional();
      Expr e;
      e.op = ExprOp::conditional;
      e.args = {id, yes, no};
      id = emit(std::move(e));
    }
    return id;
  }
};
} // namespace
int compile_expression(World &w, std::string_view source) {
  Parser parser{w, source};
  parser.next();
  int id = parser.conditional();
  if (parser.kind != Parser::end)
    parser.fail();
  return id;
}
Value Game::read(Target t, int f, Pos p) const {
  switch (t.kind) {
  case Ref::flag:
    return flag(t.index);
  case Ref::stat:
    return hero.values.at(t.index);
  case Ref::item:
    return item_count(t.index);
  case Ref::switch_flag:
    return flag(world->floors[f].switch_slot[p.index()]);
  case Ref::status_x:
    return Number(hero.pos.x);
  case Ref::status_y:
    return Number(hero.pos.y);
  case Ref::status_direction:
    return std::string(direction_name(facing));
  case Ref::auto_battle:
    return automatic.battle;
  case Ref::auto_wall:
    return automatic.wall;
  case Ref::auto_item:
    if (automatic.item == 2)
      return std::string("nopotion");
    return automatic.item == 1;
  case Ref::floor:
    return floor_name(hero.floor);
  case Ref::zero:
    return Number(0);
  case Ref::extra:
    if (std::size_t(t.index) < flags->extra.size() && flags->extra[t.index])
      return *flags->extra[t.index];
    return Number(0);
  }
  throw Unsupported("Bad reference");
}
Value Game::eval(int id, int f, Pos p) const {
  const auto &e = world->expressions.at(id);
  auto child = [&](int i) { return eval(e.args.at(i), f, p); };
  switch (e.op) {
  case ExprOp::literal:
    return e.literal;
  case ExprOp::reference:
    return read(e.target, f, p);
  case ExprOp::error:
    throw Unsupported(e.name);
  case ExprOp::logical_not:
    return !truth(child(0));
  case ExprOp::neg:
    return number_value(-numeric(child(0)));
  case ExprOp::positive:
    return number_value(numeric(child(0)));
  case ExprOp::logical_and: {
    auto a = child(0);
    return truth(a) ? child(1) : a;
  }
  case ExprOp::logical_or: {
    auto a = child(0);
    return truth(a) ? a : child(1);
  }
  case ExprOp::conditional:
    return child(truth(child(0)) ? 1 : 2);
  case ExprOp::member: {
    auto a = child(0);
    if (auto *r = std::get_if<EnemyRef>(&a)) {
      const auto &v = enemy(world->entities[r->entity].block);
      if (e.name == "hp")
        return v.hp;
      if (e.name == "atk")
        return v.atk;
      if (e.name == "def")
        return v.def;
      if (e.name == "money")
        return v.money;
    }
    if (auto *r = std::get_if<SearchRef>(&a)) {
      if (e.name == "length") {
        Number n = 0;
        for (int i = 0; i < cells; ++i)
          n += block(r->floor, {i % side, i / side}).entity == r->entity;
        return n;
      }
    }
    if (auto *s = std::get_if<std::string>(&a))
      if (e.name == "length")
        return Number(s->size());
    throw Unsupported("Unknown native member " + e.name);
  }
  case ExprOp::call: {
    if (e.bound) {
      switch (e.function) {
      case Function::get_flag:
      case Function::has_flag: {
        auto fallback = e.args.size() > 1 ? child(1) : Value(Number(0));
        return e.function == Function::has_flag ? Value(flag_truth(e.bound_index))
                                                : flag(e.bound_index, std::move(fallback));
      }
      case Function::item_count:
      case Function::has_item: {
        Number n = item_count(e.bound_index);
        return e.function == Function::has_item ? Value(n > 0) : Value(n);
      }
      case Function::get_status:
      case Function::hero_loc:
        return read(e.target, f, p);
      case Function::visited:
        return has_visited(e.bound_index);
      case Function::can_battle:
        return damage(world->entities[e.bound_index].block) < hero[hp];
      case Function::enemy_info:
        return EnemyRef{e.bound_index};
      case Function::block_id:
      case Function::block_cls:
      case Function::block: {
        int floor = e.bound_floor < 0 ? hero.floor : e.bound_floor;
        auto q = e.bound_pos;
        if (!present(floor, q))
          return {};
        if (e.function == Function::block)
          return BlockRef{floor, q.index()};
        const auto &v = props(floor, q);
        return e.function == Function::block_id ? v.id : v.cls;
      }
      default:
        throw Unsupported("Invalid bound expression");
      }
    }
    if (e.args.size() > 8)
      throw Unsupported("Too many expression arguments");
    std::array<Value, 8> a;
    for (std::size_t i = 0; i < e.args.size(); ++i)
      a[i] = child(i);
    auto s = [&](int i) { return text(a.at(i)); };
    auto n = [&](int i) { return integer(a.at(i)); };
    auto ent = [&](int i) { return world->entity_ids.at(s(i)); };
    switch (e.function) {
    case Function::round:
      return number_value(std::floor(numeric(a[0]) + .5));
    case Function::floor:
      return number_value(std::floor(numeric(a[0])));
    case Function::ceil:
      return number_value(std::ceil(numeric(a[0])));
    case Function::abs:
      return number_value(std::abs(numeric(a[0])));
    case Function::min:
    case Function::max: {
      double v = numeric(a[0]);
      for (std::size_t i = 1; i < e.args.size(); ++i)
        v = e.function == Function::min ? std::min(v, numeric(a[i])) : std::max(v, numeric(a[i]));
      return number_value(v);
    }
    case Function::get_flag:
    case Function::has_flag: {
      auto it = world->flag_ids.find(s(0));
      if (e.function == Function::has_flag)
        return it != world->flag_ids.end() && flag_truth(it->second);
      auto v = it == world->flag_ids.end() ? (e.args.size() > 1 ? a[1] : Value(Number(0)))
                                           : flag(it->second, e.args.size() > 1 ? a[1] : Value(Number(0)));
      return v;
    }
    case Function::item_count:
    case Function::has_item: {
      auto it = world->item_ids.find(s(0));
      Number count = it == world->item_ids.end() ? 0 : item_count(it->second);
      return e.function == Function::has_item ? Value(count > 0) : Value(count);
    }
    case Function::get_status: {
      auto name = s(0);
      int index = stat(name);
      if (index >= 0)
        return hero.values[index];
      if (name == "x")
        return Number(hero.pos.x);
      if (name == "y")
        return Number(hero.pos.y);
      if (name == "direction")
        return std::string(direction_name(facing));
      return Number(0);
    }
    case Function::visited:
      return has_visited(floor_number(s(0)));
    case Function::replaying:
      return true;
    case Function::hero_loc:
      if (s(0) == "x")
        return Number(hero.pos.x);
      if (s(0) == "y")
        return Number(hero.pos.y);
      return std::string(direction_name(facing));
    case Function::block_id:
    case Function::block_cls:
    case Function::block: {
      int floor = e.args.size() > 2 && a[2].index() != 0 ? floor_number(s(2)) : hero.floor;
      Pos q{int(n(0)), int(n(1))};
      if (!present(floor, q))
        return {};
      if (e.function == Function::block)
        return BlockRef{floor, q.index()};
      const auto &v = props(floor, q);
      return e.function == Function::block_id ? v.id : v.cls;
    }
    case Function::search:
      return SearchRef{e.args.size() > 1 ? floor_number(s(1)) : hero.floor, ent(0)};
    case Function::can_battle:
      return damage(world->entities[ent(0)].block) < hero[hp];
    case Function::enemy_info:
      return EnemyRef{ent(0)};
    case Function::cal_value:
      throw Unsupported("Dynamic expression compilation is forbidden in native execution");
    }
    throw Unsupported("Unknown expression function");
  }
  default: {
    auto a = child(0), b = child(1);
    switch (e.op) {
    case ExprOp::eq:
      return equal_value(a, b, false);
    case ExprOp::ne:
      return !equal_value(a, b, false);
    case ExprOp::seq:
      return equal_value(a, b, true);
    case ExprOp::sne:
      return !equal_value(a, b, true);
    case ExprOp::add:
      if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b))
        return text(a) + text(b);
      return number_value(numeric(a) + numeric(b));
    case ExprOp::sub:
      return number_value(numeric(a) - numeric(b));
    case ExprOp::mul:
      return number_value(numeric(a) * numeric(b));
    case ExprOp::div:
      if (numeric(b) == 0)
        throw Error("Division by zero");
      return number_value(numeric(a) / numeric(b));
    case ExprOp::mod:
      if (numeric(b) == 0)
        throw Error("Modulo zero");
      return number_value(std::fmod(numeric(a), numeric(b)));
    case ExprOp::lt:
    case ExprOp::le:
    case ExprOp::gt:
    case ExprOp::ge: {
      int order = 0;
      if (auto *x = std::get_if<std::string>(&a); x && std::holds_alternative<std::string>(b))
        order = x->compare(std::get<std::string>(b));
      else {
        auto left = numeric(a), right = numeric(b);
        order = left < right ? -1 : left > right ? 1 : 0;
      }
      return e.op == ExprOp::lt   ? order < 0
             : e.op == ExprOp::le ? order <= 0
             : e.op == ExprOp::gt ? order > 0
                                  : order >= 0;
    }
    default:
      throw Unsupported("Unknown expression operator");
    }
  }
  }
}
} // namespace engine
