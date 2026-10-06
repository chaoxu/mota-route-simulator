#pragma once
#include "mota/simulator.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <exception>
#include <string_view>
#include <unordered_map>

#define MOTA_NATIVE_EXPRESSION_PROGRAM_CACHE 1

namespace mota {
using NativeResolver = std::function<Json(const std::string &, const std::vector<Json> &)>;

// Expressions in the tower's event data. No JavaScript statements or eval are
// accepted. Runtime references and game functions are supplied by the event engine.
class NativeExpression {
  std::string_view source;
  const NativeResolver &resolve;
  std::size_t cursor = 0;
  struct Token { std::string text; bool name = false, number = false, string = false; } token;
  enum class Operation { literal, reference, call, member, unary, binary, conditional, error };
  struct Node {
    Operation operation;
    std::size_t cursor;
    std::string text;
    Json value;
    std::vector<int> children;
    std::exception_ptr error;
  };
  struct Program { std::vector<Node> nodes; int root = 0; };
  Program *compiling = nullptr;
  int compiled_result = 0;
  int emit(Operation operation, std::string text = {}, Json value = {}, std::vector<int> children = {}) {
    compiling->nodes.push_back({operation,cursor,std::move(text),std::move(value),std::move(children),{}});
    return compiled_result = static_cast<int>(compiling->nodes.size()) - 1;
  }
  struct SourceHash {
    using is_transparent = void;
    std::size_t operator()(std::string_view value) const { return std::hash<std::string_view>{}(value); }
  };
  static auto &programs() {
    static thread_local std::unordered_map<std::string,std::shared_ptr<const Program>,SourceHash,std::equal_to<>> cache;
    return cache;
  }
  [[noreturn]] void fail(const std::string &what) const {
    throw Unsupported("Native expression " + what + " at " + std::to_string(cursor) +
                      ": " + std::string(source));
  }
  static bool name_start(unsigned char c) {
    return std::isalpha(c) || c == '_' || c == '$' || c >= 128;
  }
  static bool name_char(unsigned char c) { return name_start(c) || std::isdigit(c); }
  static bool reference_prefix(const std::string &s) {
    return s == "flag" || s == "status" || s == "item" || s == "temp" ||
           s == "switch" || s == "blockId" || s == "blockCls";
  }
  void next() {
    while (cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
    token = {};
    if (cursor == source.size()) return;
    auto start = cursor;
    unsigned char c = source[cursor++];
    if (name_start(c)) {
      while (cursor < source.size() && name_char(source[cursor])) ++cursor;
      token.text = std::string(source.substr(start, cursor - start));
      if (cursor < source.size() && source[cursor] == ':' && reference_prefix(token.text)) {
        bool coordinate = token.text == "blockId" || token.text == "blockCls";
        ++cursor;
        auto key = cursor;
        while (cursor < source.size() && (name_char(source[cursor]) ||
               (coordinate && source[cursor] == ','))) ++cursor;
        if (cursor == key) fail("has an empty reference");
        token.text = std::string(source.substr(start, cursor - start));
      }
      token.name = true;
      return;
    }
    if (std::isdigit(c) || (c == '.' && cursor < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor])))) {
      while (cursor < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor]))) ++cursor;
      if (cursor < source.size() && source[cursor] == '.') {
        ++cursor;
        while (cursor < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor]))) ++cursor;
      }
      if (cursor < source.size() && (source[cursor] == 'e' || source[cursor] == 'E')) {
        ++cursor;
        if (cursor < source.size() && (source[cursor] == '+' || source[cursor] == '-')) ++cursor;
        auto digits = cursor;
        while (cursor < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor]))) ++cursor;
        if (digits == cursor) fail("has an invalid exponent");
      }
      token.text = std::string(source.substr(start, cursor - start)); token.number = true;
      return;
    }
    if (c == '\'' || c == '"') {
      const char quote = c;
      std::string value;
      bool closed = false;
      while (cursor < source.size()) {
        char ch = source[cursor++];
        if (ch == quote) { closed = true; break; }
        if (ch != '\\') { value += ch; continue; }
        if (cursor == source.size()) fail("has an unfinished escape");
        ch = source[cursor++];
        switch (ch) {
        case 'n': value += '\n'; break;
        case 'r': value += '\r'; break;
        case 't': value += '\t'; break;
        case 'b': value += '\b'; break;
        case 'f': value += '\f'; break;
        case 'v': value += '\v'; break;
        case '\\': case '\'': case '"': case '/': value += ch; break;
        case 'u': {
          if (cursor + 4 > source.size()) fail("has an invalid Unicode escape");
          std::string encoded = "\"\\u" + std::string(source.substr(cursor, 4)); cursor += 4;
          // JSON validates surrogate pairs and emits UTF-8 for us.
          if (cursor + 6 <= source.size() && source.substr(cursor, 2) == "\\u") {
            encoded += std::string(source.substr(cursor, 6)); cursor += 6;
          }
          encoded += '"';
          try { value += Json::parse(encoded).get<std::string>(); }
          catch (const Json::exception &) { fail("has an invalid Unicode escape"); }
          break;
        }
        default: fail("has an unsupported string escape");
        }
      }
      if (!closed) fail("has an unterminated string");
      token.text = std::move(value); token.string = true; return;
    }
    --cursor;
    for (std::string_view op : {"===", "!==", "&&", "||", "==", "!=", "<=", ">="}) {
      if (source.substr(cursor, op.size()) == op) {
        token.text = op; cursor += op.size(); return;
      }
    }
    if (std::string_view("+-*/%<>()!?:,.[ ]").find(c) == std::string_view::npos)
      fail("has an unsupported token");
    token.text = char(c); ++cursor;
  }
  bool take(std::string_view text) {
    if (token.name || token.number || token.string || token.text != text) return false;
    next(); return true;
  }
  void expect(std::string_view text) { if (!take(text)) fail("expected " + std::string(text)); }
  static Json numeric(double x) {
    if (!std::isfinite(x)) throw RouteError("Non-finite native expression result");
    // JS arithmetic is IEEE double. Preserve integer JSON for exact state comparison.
    if (std::trunc(x) == x && x >= -9007199254740991.0 && x <= 9007199254740991.0)
      return static_cast<Number>(x);
    return x;
  }
  static bool equal(const Json &a, const Json &b, bool strict) {
    if (a.is_number() && b.is_number()) return a.get<double>() == b.get<double>();
    if (a.is_object() || a.is_array() || b.is_object() || b.is_array()) {
      if (a.is_null() || b.is_null()) return false;
      throw Unsupported("Object identity in native equality");
    }
    if (a.type() == b.type()) return a == b;
    if (strict || a.is_null() || b.is_null()) return false;
    if ((a.is_number() || a.is_boolean() || a.is_string()) &&
        (b.is_number() || b.is_boolean() || b.is_string())) {
      try { return number(a) == number(b); } catch (const RouteError &) { return false; }
    }
    throw Unsupported("Object coercion in native equality");
  }
  Json call(const std::string &name, const std::vector<Json> &args) {
    auto one = [&]() { if (args.size() != 1) fail("has invalid function arity"); return number(args[0]); };
    if (name == "Math.round") return numeric(std::floor(one() + .5));
    if (name == "Math.floor") return numeric(std::floor(one()));
    if (name == "Math.ceil") return numeric(std::ceil(one()));
    if (name == "Math.abs") return numeric(std::abs(one()));
    if (name == "Math.min" || name == "Math.max") {
      if (args.empty()) fail("has invalid function arity");
      double result = number(args[0]);
      for (std::size_t i = 1; i < args.size(); ++i)
        result = name == "Math.min" ? std::min(result, number(args[i])) : std::max(result, number(args[i]));
      return numeric(result);
    }
    return resolve(name, args);
  }
  Json atom(bool evaluate) {
    Json value;
    if (take("(")) { value = conditional(evaluate); expect(")"); }
    else if (token.string) {
      if (evaluate) value = token.text;
      if (compiling) emit(Operation::literal,{},token.text);
      next();
    }
    else if (token.number) {
      if (evaluate) {
        try { value = numeric(std::stod(token.text)); }
        catch (const std::invalid_argument &) { fail("has an invalid number"); }
        catch (const std::out_of_range &) { fail("has a number out of range"); }
      }
      if (compiling) {
        // Parse literals once, but defer their errors until the branch executes.
        emit(Operation::literal);
        try {
          try { compiling->nodes.back().value = numeric(std::stod(token.text)); }
          catch (const std::invalid_argument &) { fail("has an invalid number"); }
          catch (const std::out_of_range &) { fail("has a number out of range"); }
        } catch (const RouteError &) {
          compiling->nodes.back().operation = Operation::error;
          compiling->nodes.back().error = std::current_exception();
        }
      }
      next();
    } else if (token.name) {
      std::string name = token.text; next();
      while (take(".")) {
        if (!token.name) fail("expected member name");
        name += '.' + token.text; next();
      }
      if (take("(")) {
        std::vector<Json> args;
        std::vector<int> compiled_args;
        if (!take(")")) {
          do {
            args.push_back(conditional(evaluate));
            if (compiling) compiled_args.push_back(compiled_result);
          } while (take(","));
          expect(")");
        }
        if (evaluate) value = call(name, args);
        if (compiling) emit(Operation::call,name,{},std::move(compiled_args));
      } else if (evaluate) {
        if (name == "true") value = true;
        else if (name == "false") value = false;
        else if (name == "null") value = nullptr;
        else value = resolve(name, {});
      } else if (compiling) {
        if (name == "true") emit(Operation::literal,{},true);
        else if (name == "false") emit(Operation::literal,{},false);
        else if (name == "null") emit(Operation::literal,{},nullptr);
        else emit(Operation::reference,name);
      }
    } else fail("expected an operand");
    for (;;) {
      Json property;
      const int base_node = compiled_result;
      if (take(".")) {
        if (!token.name) fail("expected member name");
        property = token.text;
        if (compiling) emit(Operation::literal,{},token.text);
        next();
      } else if (take("[")) { property = conditional(evaluate); expect("]"); }
      else break;
      if (compiling) emit(Operation::member,{},{},{base_node,compiled_result});
      if (!evaluate) continue;
      value = member(std::move(value),property);
    }
    return value;
  }
  Json member(Json value, const Json &property) {
      if (property == Json("length") && (value.is_string() || value.is_array())) {
        if (value.is_array()) value = value.size();
        else {
          std::size_t units = 0;
          for (unsigned char c : value.get_ref<const std::string &>())
            if ((c & 0xc0) != 0x80) units += c >= 0xf0 ? 2 : 1;
          value = units; // JavaScript string length counts UTF-16 code units.
        }
      } else if (value.is_object() && property.is_string()) {
        // Use a temporary: the member may alias the value being replaced.
        const auto key = property.get<std::string>();
        if (!value.contains(key)) fail("accessed an undefined property");
        Json member = value.at(key); value = std::move(member);
      } else if (value.is_array() && property.is_number_integer()) {
        auto i = property.get<Number>();
        if (i < 0 || static_cast<std::size_t>(i) >= value.size()) fail("accessed an undefined index");
        Json member = value[i];
        value = std::move(member);
      } else fail("has unsupported member access");
    return value;
  }
  Json unary(bool evaluate) {
    if (take("!")) { auto x = unary(evaluate); if(compiling)emit(Operation::unary,"!",{},{compiled_result}); return evaluate ? Json(!truth(x)) : Json(); }
    if (take("-")) { auto x = unary(evaluate); if(compiling)emit(Operation::unary,"-",{},{compiled_result}); return evaluate ? numeric(-number(x)) : Json(); }
    if (take("+")) { auto x = unary(evaluate); if(compiling)emit(Operation::unary,"+",{},{compiled_result}); return evaluate ? numeric(number(x)) : Json(); }
    return atom(evaluate);
  }
  static int precedence(const Token &t) {
    if (t.name || t.number || t.string) return 0;
    if (t.text == "||") return 1;
    if (t.text == "&&") return 2;
    if (t.text == "==" || t.text == "!=" || t.text == "===" || t.text == "!==") return 3;
    if (t.text == "<" || t.text == ">" || t.text == "<=" || t.text == ">=") return 4;
    if (t.text == "+" || t.text == "-") return 5;
    if (t.text == "*" || t.text == "/" || t.text == "%") return 6;
    return 0;
  }
  Json binary(int minimum, bool evaluate) {
    Json left = unary(evaluate);
    int left_node = compiled_result;
    while (precedence(token) >= minimum && precedence(token) != 0) {
      auto op = token.text; auto level = precedence(token); next();
      bool rightLive = evaluate && !(op == "&&" && !truth(left)) && !(op == "||" && truth(left));
      Json right = binary(level + 1, rightLive);
      if (compiling) left_node = emit(Operation::binary,op,{},{left_node,compiled_result});
      if (!evaluate) continue;
      if (op == "&&" || op == "||") { if (rightLive) left = std::move(right); continue; }
      left = combine(op,std::move(left),right);
    }
    return left;
  }
  Json combine(const std::string &op, Json left, const Json &right) {
      if (op == "==" || op == "!=" || op == "===" || op == "!==") {
        bool eq = equal(left, right, op.size() == 3); left = op[0] == '!' ? !eq : eq;
      } else if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        bool less, same;
        if (left.is_string() && right.is_string()) { less = left < right; same = left == right; }
        else { less = number(left) < number(right); same = number(left) == number(right); }
        left = op == "<" ? less : op == "<=" ? less || same : op == ">" ? !less && !same : !less;
      } else if (op == "+" && (left.is_string() || right.is_string())) {
        left = string(left) + string(right);
      } else {
        double a = number(left), b = number(right);
        if ((op == "/" || op == "%") && b == 0) fail("divides by zero");
        left = numeric(op == "+" ? a + b : op == "-" ? a - b : op == "*" ? a * b :
                       op == "/" ? a / b : std::fmod(a, b));
      }
    return left;
  }
  Json conditional(bool evaluate) {
    Json value = binary(1, evaluate);
    const int condition_node = compiled_result;
    if (!take("?")) return value;
    bool yes = evaluate && truth(value);
    Json left = conditional(yes); expect(":");
    const int left_node = compiled_result;
    Json right = conditional(evaluate && !yes);
    if (compiling) emit(Operation::conditional,{},{},{condition_node,left_node,compiled_result});
    return evaluate ? (yes ? left : right) : Json();
  }
  Json execute(const Program &program, int index) {
    const auto &node = program.nodes[index];
    switch (node.operation) {
    case Operation::literal: return node.value;
    case Operation::reference: return resolve(node.text,{});
    case Operation::error: std::rethrow_exception(node.error);
    case Operation::call: {
      std::vector<Json> args;args.reserve(node.children.size());
      for(int child:node.children)args.push_back(execute(program,child));
      cursor=node.cursor;return call(node.text,args);
    }
    case Operation::member: {
      auto base=execute(program,node.children[0]);auto property=execute(program,node.children[1]);
      cursor=node.cursor;return member(std::move(base),property);
    }
    case Operation::unary: {
      auto value=execute(program,node.children[0]);
      if(node.text=="!")return !truth(value);
      return numeric(node.text=="-"?-number(value):number(value));
    }
    case Operation::binary: {
      auto left=execute(program,node.children[0]);
      if(node.text=="&&")return truth(left)?execute(program,node.children[1]):left;
      if(node.text=="||")return truth(left)?left:execute(program,node.children[1]);
      auto right=execute(program,node.children[1]);cursor=node.cursor;
      return combine(node.text,std::move(left),right);
    }
    case Operation::conditional:
      return execute(program,node.children[truth(execute(program,node.children[0]))?1:2]);
    }
    throw Unsupported("Invalid compiled expression operation");
  }
  NativeExpression(std::string_view input, const NativeResolver &resolver, bool tokenize)
      : source(input), resolve(resolver) { if(tokenize)next(); }
public:
  static void clear_cache() { programs().clear(); }
  static Json cached(std::string_view input, const NativeResolver &resolver) {
    auto &cache=programs();auto found=cache.find(input);
    std::shared_ptr<const Program> program;
    if(found!=cache.end())program=found->second;
    else {
      try {
        auto compiled=std::make_shared<Program>();NativeExpression parser(input,resolver);
        parser.compiling=compiled.get();parser.conditional(false);
        if(!parser.token.text.empty()||parser.token.string)parser.fail("has trailing input");
        compiled->root=parser.compiled_result;program=std::move(compiled);
      } catch(const Unsupported &) {
        // Invalid syntax must preserve the interpreter's resolver/error order.
      }
      if(cache.size()>=1024)cache.clear();
      cache.emplace(std::string(input),program);
    }
    if(!program)return NativeExpression(input,resolver).run();
    NativeExpression evaluator(input,resolver,false);
    return evaluator.execute(*program,program->root);
  }
  static bool truth(const Json &j) {
    if (j.is_null()) return false;
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number()) return j.get<double>() != 0;
    if (j.is_string()) return !j.get_ref<const std::string &>().empty();
    return true;
  }
  static double number(const Json &j) {
    if (j.is_null()) return 0;
    if (j.is_boolean()) return j.get<bool>() ? 1 : 0;
    if (j.is_number()) return j.get<double>();
    if (j.is_string()) {
      const auto &s = j.get_ref<const std::string &>();
      auto begin = s.find_first_not_of(" \t\r\n");
      if (begin == std::string::npos) return 0;
      try {
        std::size_t used = 0; double n = std::stod(s.substr(begin), &used);
        if (s.find_first_not_of(" \t\r\n", begin + used) == std::string::npos && std::isfinite(n)) return n;
      } catch (const std::exception &) {}
    }
    throw Unsupported("Unsupported native numeric conversion: " + j.dump());
  }
  static std::string string(const Json &j) {
    if (j.is_string()) return j.get<std::string>();
    if (j.is_object() || j.is_array()) throw Unsupported("Native object string coercion");
    return j.dump();
  }
  NativeExpression(std::string_view input, const NativeResolver &resolver) : source(input), resolve(resolver) { next(); }
  Json run() {
    Json result = conditional(true);
    if (!token.text.empty() || token.string) fail("has trailing input");
    return result;
  }
};
inline Json native_eval(std::string_view source, const NativeResolver &resolver) {
  return NativeExpression::cached(source,resolver);
}
} // namespace mota
