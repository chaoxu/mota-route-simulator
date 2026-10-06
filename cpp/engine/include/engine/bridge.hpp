#pragma once
#include "engine/core.hpp"
#include "mota/simulator.hpp"

namespace engine::io {
using Json = mota::Json;
Json compiled_source(Json);
struct Compiler {
  std::shared_ptr<World> world = std::make_shared<World>();
  std::shared_ptr<const mota::World> reference_world;
  bool build_corridors = false;
  std::vector<Json> sources{Json()}, opaque{Json()};
  std::vector<Json> program_sources{Json::array()}, payload_sources{Json::object()};
  std::unordered_map<std::string, int> program_cache;
  std::unordered_map<std::string, std::uint32_t> identity_ids;
  std::uint32_t identity_id(const Json &);
  void seal_identities();
  mutable std::array<std::shared_ptr<const FloorState>, floors_count> projected_inputs;
  mutable std::array<std::shared_ptr<mota::FloorState>, floors_count> projected_floors;
  int save(const Json &);
  Value value(const Json &);
  int expression(const Json &);
  int program(const Json &, bool precompile = true);
  int instruction(const Json &);
  Props properties(const Json &);
  explicit Compiler(std::shared_ptr<const mota::World>, bool corridors = false);
  Game import(const mota::Game &);
  mota::Game project(const Game &, const mota::Game &) const;
  Json scalar(const Value &) const;
  Json event_state(const Game &) const;
  Json remaining(const Frame &) const;
};
} // namespace engine::io
