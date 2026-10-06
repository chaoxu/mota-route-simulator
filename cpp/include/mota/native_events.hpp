#pragma once
#include "mota/simulator.hpp"
#include <unordered_map>

namespace mota {
struct PreparedNativeWorld {
  struct MovementMask {std::uint8_t in=0,out=0;};
  struct AutoEvent {
    int floor;Pos pos;std::string symbol;
    bool multi_execute,current_floor;
    const Json *source;
  };
  std::weak_ptr<const World> source;
  std::array<std::array<std::uint8_t,cells>,floor_count> floor_in{},floor_out{};
  std::array<std::bitset<cells>,floor_count> source_events,change_floors;
  std::vector<MovementMask> entities,tiles;
  std::vector<AutoEvent> auto_events;
  std::array<std::vector<std::size_t>,floor_count> auto_for_floor;
  // Static source programs are normalized once. Keys point into the immutable
  // source World; callers must check matches before looking up an address.
  std::unordered_map<const Json *,std::shared_ptr<const Json>> event_programs;
  bool matches(const std::shared_ptr<const World> &world) const {
    return !source.owner_before(world)&&!world.owner_before(source);
  }
};
std::shared_ptr<const PreparedNativeWorld> prepare_native_world(const std::shared_ptr<const World> &world);
// Replay controls are interpreted against the pinned tower source. This runtime
// is internal state, never a native-engine checkpoint or a score correction.
Json native_event_state_json(const Game &game);
bool native_events_pending(const Game &game);
void native_events_arrive(Game &game, int floor, bool first);
void native_events_after(Game &game, const std::string &kind, int floor, Pos position);
void native_events_check_auto(Game &game);
} // namespace mota
