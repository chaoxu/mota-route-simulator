#include "engine/core.hpp"
#include <bit>
#include <cmath>
#include <cstring>

namespace engine {
namespace {
struct Encode {
  std::string bytes;
  template <class T> void pod(T value) {
    static_assert(std::is_arithmetic_v<T>);
    bytes.append(reinterpret_cast<const char *>(&value), sizeof(value));
  }
  void byte(unsigned v) { pod(std::uint8_t(v)); }
  void word(std::uint64_t v) { pod(v); }
  void id(std::uint32_t v) { pod(v); }
  void string(std::string_view v) {
    word(v.size());
    bytes.append(v);
  }
  void pos(Pos p) {
    pod(p.x);
    pod(p.y);
  }
  void mask(CellMask v) {
    for (auto word : v.words)
      this->word(word);
  }
  StateIdentity finish() && {
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : bytes) {
      h ^= c;
      h *= 1099511628211ULL;
    }
    return {h, std::move(bytes)};
  }
};
void value(Encode &out, const World &world, const Value &v) {
  if (auto *opaque = std::get_if<Opaque>(&v); opaque && opaque->id < 0 && !opaque->truth) {
    out.byte(9);
    out.word(0);
    return; // Native empty-array literal.
  }
  out.byte(v.index());
  std::visit(
      [&](const auto &x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
        } else if constexpr (std::is_same_v<T, bool>)
          out.byte(x);
        else if constexpr (std::is_same_v<T, Number>)
          out.pod(x);
        else if constexpr (std::is_same_v<T, double>) {
          if (!std::isfinite(x))
            throw Unsupported("Nonfinite scalar in search state");
          out.pod(x == 0 ? 0.0 : x);
        } else if constexpr (std::is_same_v<T, std::string>)
          out.string(x);
        else if constexpr (std::is_same_v<T, EnemyRef>)
          out.pod(x.entity);
        else if constexpr (std::is_same_v<T, BlockRef>) {
          out.pod(x.floor);
          out.pod(x.index);
        } else if constexpr (std::is_same_v<T, SearchRef>) {
          out.pod(x.floor);
          out.pod(x.entity);
        } else if constexpr (std::is_same_v<T, Opaque>) {
          out.id(x.id < 0                        ? 0
                 : world.opaque_identity.empty() ? std::uint32_t(x.id)
                                                 : world.opaque_identity.at(x.id));
          out.byte(x.truth);
        } else if constexpr (std::is_same_v<T, NumberArray>) {
          out.word(x.values.size());
          for (auto n : x.values)
            out.pod(n);
        }
      },
      v);
}
std::shared_ptr<const StateIdentity> flags_key(const Game &g) {
  return g.flags->identity.get([&] {
    Encode out;
    g.flags->scalar.for_each([&](std::size_t id, const Value &v) {
      out.word(id);
      value(out, *g.world, v);
    });
    out.word(UINT64_MAX);
    out.word(g.flags->extra.size());
    for (const auto &v : g.flags->extra) {
      out.byte(bool(v));
      if (v)
        value(out, *g.world, *v);
    }
    for (auto n : g.flags->inventory)
      out.pod(n);
    return std::move(out).finish();
  });
}
std::shared_ptr<const StateIdentity> overrides_key(const Game &g) {
  return g.overrides->identity.get([&] {
    Encode out;
    for (std::size_t id = 0; id < g.overrides->size(); ++id) {
      const auto &entry = g.overrides->at(id);
      if (entry.present.none())
        continue;
      out.word(id);
      out.word(entry.present.to_ullong());
      for (int p = 0; p < 12; ++p)
        if (entry.present[p])
          value(out, *g.world, entry.value[p]);
    }
    return std::move(out).finish();
  });
}
std::shared_ptr<const StateIdentity> floor_key(const Game &g, int f) {
  const auto &fs = *g.floors[f];
  return fs.identity.get([&] {
    Encode out;
    const auto &w = *g.world;
    for (const auto &c : fs.cell) {
      out.pod(c.retained.entity);
      out.pod(c.retained.tile);
      out.byte(c.present | (c.disabled << 1) | (c.event_entry << 2) | (c.source_destroyed << 3));
      if (c.present) {
        if (w.identities_ready) {
          const auto &ids = c.payload        ? w.payload_identity.at(c.payload)
                            : c.base_payload ? w.base_identity.at(c.retained.entity)
                                             : w.composed_identity.at(c.changed - 1);
          out.id(ids[c.touch_precompiled]);
        } else {
          out.byte(c.base_payload);
          out.byte(c.touch_precompiled);
          out.pod(c.payload);
          out.id(c.changed);
        }
      }
    }
    for (auto n : fs.foreground)
      out.pod(n);
    for (auto n : fs.background)
      out.pod(n);
    out.word(fs.order.size());
    for (auto p : fs.order)
      out.byte(p.index());
    out.byte(fs.disabled_record);
    if (fs.disabled_record) {
      auto orders = fs.observations.materialize();
      for (const auto *list : {&orders.disabled, &orders.enabled}) {
        out.word(list->size());
        for (int i : *list)
          out.byte(i);
      }
    }
    for (bool fg : {false, true}) {
      bool present = fg ? fs.foreground_record : fs.background_record;
      out.byte(present);
      if (present) {
        const auto &changes = fg ? fs.foreground_changes : fs.background_changes;
        out.word(changes.size());
        for (const auto &change : changes) {
          out.pos(change.pos);
          out.pod(change.tile);
        }
      }
    }
    return std::move(out).finish();
  });
}
std::uint32_t source_id(const World &world, int id) {
  return world.identities_ready ? world.source_identity.at(id) : std::uint32_t(id);
}
StateIdentity state_key(const Game &g) {
  Encode out;
  out.bytes.reserve(2048);
  out.word(g.world->source_tag);
  for (auto n : g.hero.values)
    out.pod(n);
  out.pod(g.hero.floor);
  out.pos(g.hero.pos);
  out.byte(unsigned(g.facing));
  for (int i = 0; i < FlagIds::count; ++i) {
    out.byte(g.builtin_types[i] != 255);
    if (auto v = g.flag_value(i))
      value(out, *g.world, *v);
  }
  out.byte(g.automatic.battle);
  out.byte(g.automatic.wall);
  out.pod(g.automatic.item);
  out.byte(g.allow_direct);
  out.byte(g.gentle_click);
  for (auto mask : g.blocked_items)
    out.mask(mask);
  out.word(g.visited.to_ullong());
  out.word(g.visited_flags.to_ullong());
  for (const auto &leave : g.leave) {
    out.byte(bool(leave));
    if (leave) {
      out.pos(leave->pos);
      out.byte(unsigned(leave->direction));
    }
  }
  for (const auto *list : {&g.auto_executed, &g.auto_executing}) {
    out.word(list->size());
    for (int i : *list)
      out.pod(i);
  }
  for (bool b : {g.initialized, g.has_auto, g.has_visited_record, g.has_leave, g.has_aed, g.has_aei,
                 g.has_enemy_info, g.auto_active, g.executing, g.mdef_present})
    out.byte(b);
  out.byte(g.has_event_serial);
  if (g.has_event_serial)
    out.pod(g.event_serial);
  out.byte(g.has_win_reason);
  if (g.has_win_reason)
    out.string(g.win_reason);
  out.byte(g.has_exp);
  if (g.has_exp) {
    out.byte(g.exp_null);
    if (!g.exp_null)
      out.pod(g.exp);
  }
  out.word(g.frames.size());
  for (const auto &frame : g.frames) {
    out.pod(frame.floor);
    out.pos(frame.pos);
    out.pod(frame.automatic);
    out.byte(frame.loop != 0);
    if (frame.loop)
      out.id(source_id(*g.world, g.world->instructions.at(frame.loop).source));
    std::size_t count = 0;
    for (const auto &segment : frame.segments)
      count += g.world->programs.at(segment.program).code.size() - segment.cursor;
    out.word(count);
    for (auto segment = frame.segments.rbegin(); segment != frame.segments.rend(); ++segment) {
      const auto &code = g.world->programs.at(segment->program).code;
      for (std::size_t i = segment->cursor; i < code.size(); ++i)
        out.id(source_id(*g.world, g.world->instructions.at(code[i]).source));
    }
  }
  out.byte(bool(g.choice));
  if (g.choice) {
    const auto &c = *g.choice;
    const auto &instruction = g.world->instructions.at(c.instruction);
    out.pod(c.floor);
    out.pos(c.pos);
    out.id(g.world->identities_ready ? g.world->choice_text_identity.at(c.instruction)
                                     : std::uint32_t(c.instruction));
    out.word(c.options.size());
    for (int i : c.options)
      out.id(source_id(*g.world, instruction.choices.at(i).source));
  }
  out.word(flags_key(g)->hash);
  out.word(overrides_key(g)->hash);
  for (int f = g.world->first_floor; f < floors_count; ++f)
    out.word(floor_key(g, f)->hash);
  return std::move(out).finish();
}
bool same(const std::shared_ptr<const StateIdentity> &a, const std::shared_ptr<const StateIdentity> &b) {
  return a == b || (a->hash == b->hash && a->bytes == b->bytes);
}
} // namespace
std::uint64_t Game::hash() const {
  return state_key(*this).hash;
}
bool Game::operator==(const Game &other) const {
  if (this == &other)
    return true;
  if (world != other.world || hero != other.hero)
    return false;
  if (state_key(*this).bytes != state_key(other).bytes)
    return false;
  if (!same(flags_key(*this), flags_key(other)) || !same(overrides_key(*this), overrides_key(other)))
    return false;
  for (int f = world->first_floor; f < floors_count; ++f)
    if (floors[f] != other.floors[f] && !same(floor_key(*this, f), floor_key(other, f)))
      return false;
  return true;
}
} // namespace engine
