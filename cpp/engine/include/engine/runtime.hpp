#pragma once
#include "engine/bridge.hpp"

// JSON and the reference engine are confined to input, output and validation.
namespace engine {
using io::Json;
inline Json native_runtime_info(const Game &game) {
  return {{"implementation", "typed-native"}, {"compiled_events", game.use_native},
          {"batched_collection", game.use_batches}, {"atomic_inputs", true}};
}
inline Game make_native_game(io::Compiler &compiler, const Json &initial) {
  if (initial.value("model_mode", std::string()) != "native-corrected")
    throw Error("Expected native replay input with native-corrected initial state");
  const auto &floors = initial.at("floors");
  for (int f = compiler.world->first_floor; f < floors_count; ++f) {
    const auto id = floor_name(f);
    if (!floors.contains(id)) throw Error("Native search requires complete retained block state");
    const auto &floor = floors.at(id);
    if (!floor.contains("retained_blocks") || !floor.contains("retained_block_order") ||
        !floor.at("retained_blocks").is_object() || !floor.at("retained_block_order").is_array())
      throw Error("Native search requires complete retained block state");
  }
  return compiler.import(mota::Game::from_reference(compiler.reference_world, initial));
}
inline bool native_events_pending(const Game &g) {return !g.frames.empty() || bool(g.choice);}
inline Json inventory_json(const Game &g) {
  Json items = {{"tools", Json::object()}, {"constants", Json::object()}, {"equips", Json::object()}};
  for (std::size_t id = 0; id < g.world->item_names.size(); ++id) {
    const auto n = g.item_count(int(id)); const int cls = g.world->item_class[id];
    if (n && cls) items[cls == 1 ? "constants" : cls == 2 ? "tools" : cls == 3 ? "equips" : "items"][g.world->item_names[id]] = n;
  }
  return items;
}
inline Pos json_pos(const Json &x, const Json &y) {return {mota::checked_int(x), mota::checked_int(y)};}
inline std::string location(Pos p) {return std::to_string(p.x) + "," + std::to_string(p.y);}
} // namespace engine
