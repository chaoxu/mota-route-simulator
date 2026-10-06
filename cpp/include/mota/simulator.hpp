#pragma once
#include <array>
#include <atomic>
#include <bitset>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <limits>
#include <nlohmann/json.hpp>
#include <boost/container/flat_map.hpp>
#include <algorithm>
#include <compare>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace mota {
template<class Key,class Value,class Compare,class Allocator>
struct FlatObject : boost::container::flat_map<Key,Value,Compare,std::allocator<std::pair<Key,Value>>> {
  using Base=boost::container::flat_map<Key,Value,Compare,std::allocator<std::pair<Key,Value>>>;
  using Base::Base;
  using Base::operator=;
  // Boost.Container supplies the sorted storage. Preserve std::map's C++20
  // lexicographic comparisons, including unordered nested JSON numbers.
  friend bool operator==(const FlatObject &a,const FlatObject &b) {
    return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin());
  }
  friend std::partial_ordering operator<=>(const FlatObject &a,const FlatObject &b) {
    return std::lexicographical_compare_three_way(a.begin(),a.end(),b.begin(),b.end(),[](const auto &x,const auto &y)->std::partial_ordering {
      if(auto order=x.first<=>y.first;order!=0)return order;
      return x.second<=>y.second;
    });
  }
};
using Json = nlohmann::basic_json<FlatObject>;
using Number = std::int64_t;
constexpr int side = 13, cells = side * side, floor_count = 51;
constexpr Number infinite_damage = INT64_MAX / 4;
struct RouteError : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct Unsupported : RouteError {
  using RouteError::RouteError;
};
inline int checked_int(const Json &value) {
  if (value.is_number_unsigned()) {
    auto number = value.get<std::uint64_t>();
    if (number > std::uint64_t(std::numeric_limits<int>::max()))
      throw RouteError("Integer input out of range");
    return int(number);
  }
  if (!value.is_number_integer())
    throw RouteError("Expected an integer input");
  auto number = value.get<std::int64_t>();
  if (number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
    throw RouteError("Integer input out of range");
  return int(number);
}
struct Pos {
  int x = 0, y = 0;
  auto operator<=>(const Pos &) const = default;
  int index() const {
    return y * side + x;
  }
  bool valid() const {
    return x >= 0 && y >= 0 && x < side && y < side;
  }
};
inline Pos json_pos(const Json &x, const Json &y) {
  return {checked_int(x), checked_int(y)};
}
inline int floor_number(const std::string &fid) {
  if (!fid.starts_with("MT") || fid.size() < 3 || fid.size() > 4)
    throw RouteError("Invalid floor ID: " + fid);
  int n = 0;
  for (std::size_t i = 2; i < fid.size(); ++i) {
    if (fid[i] < '0' || fid[i] > '9')
      throw RouteError("Invalid floor ID: " + fid);
    n = 10 * n + (fid[i] - '0');
  }
  if (n < 0 || n > 50)
    throw RouteError("Floor ID out of range: " + fid);
  return n;
}
inline std::string floor_id(int n) {
  return "MT" + std::to_string(n);
}
inline std::string location(Pos p) {
  return std::to_string(p.x) + "," + std::to_string(p.y);
}
enum class Kind : std::uint8_t { empty, wall, enemy, door, item, terrain, npc };
struct Entity {
  Kind kind = Kind::empty;
  std::string id;
  Number hp = 0, atk = 0, def = 0, money = 0, n = 4, def_value = 0, atk_value = 0, value = 0;
  int range = 1;
  std::uint64_t special = 0;
  bool square = false;
  Json native = Json::object();
};
struct Block {
  std::uint16_t entity = 0;
  std::int16_t tile = 0;
  auto operator<=>(const Block &) const = default;
};
struct FloorData {
  int ratio = 1;
  std::array<Block, cells> initial{};
  Json events = Json::object(), change_floor = Json::object();
  std::bitset<cells> after_battle, after_battle_auto;
  Pos up{-1, -1}, down{-1, -1};
  Json metadata = Json::object();
};
template<class T> struct SharedCacheHandle {
  mutable std::shared_ptr<T> value;
  SharedCacheHandle() = default;
  SharedCacheHandle(const SharedCacheHandle &other):value(other.load()) {}
  SharedCacheHandle &operator=(const SharedCacheHandle &other) {store(other.load());return *this;}
  std::shared_ptr<T> load() const {return std::atomic_load(&value);}
  void store(std::shared_ptr<T> next) const {std::atomic_store(&value,std::move(next));}
};
struct PreparedNativeWorld;
struct NativeContinuation;
struct World {
  int first_floor = 1;
  std::array<FloorData, floor_count> floors;
  std::vector<Entity> entities{Entity{}};
  std::map<std::string, std::uint16_t> by_id;
  std::map<int, Block> tiles;
  Json native_data = Json::object();
  SharedCacheHandle<const PreparedNativeWorld> prepared;
  static std::shared_ptr<const World> load(const std::filesystem::path &data_directory);
  Block make_block(const std::string &id, int tile = 0) const;
  const Entity &entity(Block b) const {
    return entities.at(b.entity);
  }
};
enum Stat : std::size_t {
  hp,
  atk,
  defense,
  gold,
  yk,
  bk,
  rk,
  dmg,
  yd,
  bd,
  rd,
  times1,
  centerFly,
  upFly,
  downFly,
  earthquake,
  bomb,
  bigKey,
  pickaxe,
  superPotion,
  snow,
  knife,
  earthquakeBought,
  coin,
  cross,
  magicImmune,
  won49,
  won50,
  mdef,
  stat_count
};
extern const std::array<std::string, stat_count> stat_names;
struct Hero {
  std::array<Number, stat_count> values{};
  int floor = 1;
  Pos pos{};
  auto operator<=>(const Hero &) const = default;
  Number &operator[](Stat s) {
    return values[s];
  }
  Number operator[](Stat s) const {
    return values[s];
  }
};
// Copy-on-write payloads are immutable while shared. Hashes may be requested
// concurrently by search workers, so only the derived cache is atomic.
class StateHash {
  mutable std::atomic<bool> dirty{false};
  mutable std::atomic<std::uint64_t> value{0};
public:
  StateHash() = default;
  StateHash(const StateHash &other)
    : dirty(other.dirty.load(std::memory_order_acquire)),value(other.value.load(std::memory_order_relaxed)) {}
  StateHash &operator=(const StateHash &other) {
    const bool pending=other.dirty.load(std::memory_order_acquire);
    value.store(other.value.load(std::memory_order_relaxed),std::memory_order_relaxed);
    dirty.store(pending,std::memory_order_release);return *this;
  }
  StateHash &operator=(std::uint64_t computed) {
    value.store(computed,std::memory_order_relaxed);dirty.store(false,std::memory_order_release);return *this;
  }
  void invalidate() {dirty.store(true,std::memory_order_relaxed);}
  template<class Compute> std::uint64_t get(Compute compute) const {
    if(dirty.load(std::memory_order_acquire)) {
      const auto computed=compute();value.store(computed,std::memory_order_relaxed);
      dirty.store(false,std::memory_order_release);return computed;
    }
    return value.load(std::memory_order_relaxed);
  }
};
struct NavigationCache;
struct HazardCache;
struct HazardReactions;
struct AutomaticCache;
struct RetainedEventPayload {
  Json value;
  const bool self_equal;
  mutable std::once_flag serialized_once;
  mutable std::string serialized;
  explicit RetainedEventPayload(Json data):value(std::move(data)),self_equal(value==value) {}
  const std::string &serialization() const;
};
// Payloads are immutable. A floor branch copies cell handles rather than the
// complete JSON trees for every retained block on that floor.
struct RetainedEvents {
  std::array<std::shared_ptr<const RetainedEventPayload>,cells> entries;
  // Keep the old map behavior for unusual direct API coordinates. Normal
  // imported/gameplay positions are validated and stay in the cell array.
  std::map<std::string,std::shared_ptr<const RetainedEventPayload>> overflow;
  const std::shared_ptr<const RetainedEventPayload> &lookup(Pos) const;
  const Json &view(Pos) const;
  Json value(Pos,Json fallback) const;
  void set(Pos,Json);
  void erase(Pos);
  std::uint64_t hash() const;
  bool operator==(const RetainedEvents &) const;
};
struct FloorState {
  std::array<Block, cells> blocks{};
  std::array<Block, cells> retained{};
  std::bitset<cells> retained_present, disabled;
  std::shared_ptr<RetainedEvents> retained_events = std::make_shared<RetainedEvents>();
  std::array<std::int16_t, cells> grid{};
  std::array<std::int16_t, cells> foreground{};
  std::array<std::int16_t, cells> background{};
  std::vector<Pos> spell_enemies;
  std::vector<Pos> block_order;
  std::shared_ptr<Json> events = std::make_shared<Json>(Json::object());
  StateHash hash;
  SharedCacheHandle<NavigationCache> navigation;
  SharedCacheHandle<const HazardCache> hazards;
  SharedCacheHandle<AutomaticCache> automatic_cache;
  std::shared_ptr<const PreparedNativeWorld> movement_source;
  std::array<std::uint8_t,cells> movement_in{},movement_out{};
  std::bitset<cells> spell_cells;
  std::bitset<cells> event_entries;
  bool operator==(const FloorState &other) const {
    return blocks == other.blocks && retained == other.retained && retained_present == other.retained_present && disabled == other.disabled &&
           (retained_events == other.retained_events || *retained_events == *other.retained_events) &&
           grid == other.grid && foreground == other.foreground && background == other.background && spell_enemies == other.spell_enemies &&
           (events == other.events || *events == *other.events);
  }
};
enum class NativeFlag : std::size_t { flag_0, flag_1, flag_2, flag_3, flag_4, flag_5, flag_6, flag_7, do_not_check_auto_events, addhp, arg1, arg2, arg3, arg4, cannotMoveDirectly, curse, earthquakeBought, fly, hatred, ignoreChangeFloor, no_ambush, no_betweenAttack, no_laser, no_repulse, no_zone, ratio, times1, energy, feature, flag_29, magic_immune };
inline const std::array<std::string,31> native_flag_names={"14zombieKnightGuards","2blueGuards","41","47wizardMoved","49cardinal","49sealed","50final","50thief","__doNotCheckAutoEvents__","addhp","arg1","arg2","arg3","arg4","cannotMoveDirectly","curse","earthquakeBought","fly","hatred","ignoreChangeFloor","no_ambush","no_betweenAttack","no_laser","no_repulse","no_zone","ratio","times1","对话能量","开启特性","额外功能开关","魔法免疫"};
struct NativeFlagSlots {
  std::array<std::atomic<std::size_t>,31> slots{};
  NativeFlagSlots()=default;
  NativeFlagSlots(const NativeFlagSlots &other){for(std::size_t i=0;i<slots.size();++i)slots[i].store(other.slots[i].load(std::memory_order_relaxed),std::memory_order_relaxed);}
  NativeFlagSlots &operator=(const NativeFlagSlots &other){for(std::size_t i=0;i<slots.size();++i)slots[i].store(other.slots[i].load(std::memory_order_relaxed),std::memory_order_relaxed);return *this;}
};
struct JsonState {
  NativeFlagSlots native_slots;
  Json value = Json::object();
  StateHash hash;
};
struct Effect {
  int floor = 0;
  Pos pos{};
  Block block{};
  auto operator<=>(const Effect &) const = default;
};
struct Auto {
  bool battle = false, wall = false;
  int item = 0; // 0 off, 1 on, 2 nopotion
  auto operator<=>(const Auto &) const = default;
};
class Game {
public:
  std::shared_ptr<const World> world;
  std::shared_ptr<const PreparedNativeWorld> prepared_native;
  // Derived execution representation of extra_state.native_event_state.
  std::shared_ptr<const NativeContinuation> native_continuation;
  Hero hero;
  bool strict = true;
  bool reference_model = false;
  bool mdef_present = false; // Import mode preserves the Python oracle's static hazard cache.
  std::bitset<floor_count> present;
  Json pending_records = Json::object();
  Auto automatic;
  std::bitset<floor_count> visited;
  std::array<std::shared_ptr<FloorState>, floor_count> floors;
  std::shared_ptr<JsonState> flags = std::make_shared<JsonState>();
  std::shared_ptr<JsonState> extra_state = std::make_shared<JsonState>();
  std::shared_ptr<JsonState> items = std::make_shared<JsonState>();
  std::set<int> blocked_items, retreat_wizards, no_auto_farm;
  bool defer_after = false, capturing = false, pending_auto = false;
  // Transient synchronous BFS scope; false at every replay/export boundary.
  bool native_auto_active = false;
  // Conservative until the next completed cleanup; non-temporary flag writes
  // do not require scanning the flag map again.
  bool native_temporaries_possible = true;
  std::uint64_t navigation_context = 0; // Derived-cache identity; not gameplay state.
  std::vector<Effect> deferred, captured;

  Game(std::shared_ptr<const World>, const Json &snapshot);
  static Game from_reference(std::shared_ptr<const World>, const Json &reference);
  // Explicit fast mode keeps continuation data authoritative until observed.
  // extra_state is then backing storage; use extra_json/state_json/export_json
  // for the logical state, or disable this mode to materialize backing storage.
  void set_deferred_native_serialization(bool);
  bool defers_native_serialization() const {return deferred_native_serialization_;}
  Json extra_json() const;
  Json state_json() const;
  Json export_json(bool include_floors = true) const;
  std::uint64_t hash() const;
  bool operator==(const Game &) const;
  Block block(int f, Pos p) const;
  Block retained_block(int f, Pos p) const;
  bool block_present(int f, Pos p, bool include_disabled = false) const;
  bool block_disabled(int f, Pos p) const;
  Json retained_block_event(int f, Pos p) const;
  void hide_block(int f, Pos p);
  void show_block(int f, Pos p);
  void remove_block(int f, Pos p, bool destruct = false);
  void replace_block(int f, Pos p, Block b, bool destruct = false, bool activate = false);
  const Entity &entity(Block b) const {
    return world->entity(b);
  }
  Entity effective_enemy(Block) const;
  void set_block(int f, Pos p, Block b);
  void set_block(int f, Pos p, const std::string &id, int tile = 0);
  void ground(int f, Pos p) {
    set_block(f, p, Block{});
  }
  void erase_event(int f, Pos p);
  void set_event(int f, Pos p, const Json &value);
  Number flag(const std::string &) const;
  Number flag(NativeFlag) const;
  bool flag_truth(NativeFlag) const;
  const Json *native_flag_value(NativeFlag) const;
  bool flag_truth(const std::string &) const;
  void set_flag(const std::string &, Json);
  void set_extra(const std::string &, Json);
  Json inventory_json() const;
  Number item_count(const std::string &) const;
  void add_inventory(const std::string &, Number count = 1);
  void set_inventory(const std::string &, Number count);
  void set_foreground(int, Pos, int);
  void set_background(int, Pos, int);
  int foreground(int, Pos) const;
  bool can_connect(int, Pos from, Pos to, bool direct = false) const;
  bool can_fly(int) const;
  bool has_visited(int) const;
  bool event_enabled(int, Pos) const;
  Json effective_block_event(int, Pos) const;
  Json hazard_reactions(int) const;
  std::shared_ptr<const HazardReactions> native_hazard_reactions(int) const;
  void step(const std::string &direction, bool allow_inert_bump = false);
  void move_directly(Pos);
  void talk_npc(int, Pos, int choice = 0);
  void native_token(const std::string &, bool strict_steps = false);
  void native_choose(int);
  void native_event_start(const Json &, int, Pos);
  bool native_event_trigger(int, Pos);
  void native_event_drain();
  void native_event_after(const std::string &, int, Pos);
  void native_event_arrive(int, bool);
  void set_enemy(const std::string &, const std::string &, Json);
  Number enemy_damage(const std::string &id, std::int64_t special_override = -1) const;
  Number enemy_damage(Block, std::int64_t special_override = -1) const;
  Number map_damage(int f, Pos) const;
  std::shared_ptr<const HazardCache> native_hazards(int f) const;
  void apply_map_damage(int f, Pos);
  bool is_wall(int f, Pos) const;
  bool can_enter(int f, Pos, Pos target, bool strict_click = false) const;
  Number path_cost(int f, Pos) const;
  std::vector<Pos> path_to(int f, Pos, bool strict_click = false) const;
  void go_to(int f, Pos);
  void go_to_decomposed(int f, Pos, int limit = 32);
  void auto_sweep(int f = -1, const std::set<int> &exclude = {});
  void apply_item(int f, const std::string &);
  void apply_block(int f, Pos, Block);
  void after_action(int f, Pos, Block, bool automatic_action = false);
  void trigger_tile_event(int f, Pos);
  void flush_deferred();
  void fly(int f);
  void transition(int f, Pos landing = {-1, -1}, bool flying = false);
  void center_fly();
  void up_fly();
  void down_fly();
  void use_big_key();
  void use_earthquake();
  void use_bomb();
  void use_pickaxe(int f, Pos);
  void use_snow();
  void use_super_potion();
  void move_adjacent(int f, Pos);
  void talk_trader(int f, Pos);
  void buy_shop(const std::string &kind, int count = 1);
  void set_auto(const Json &);
  void talk_mt2_bless();
  void talk_mt16_oldman();
  void talk_mt26_princess();
  void trigger_mt24_to_mt50();
  void trigger_mt50_final();
  void event_mt32_knight();
  void clear_mt10_boss_dyn();
  void
  action(const Json &); // Dispatch ordinary public Game methods; throws for unsupported actions.
private:
  bool deferred_native_serialization_ = false;
  void set_flag_member(const std::string &,const std::string &,Json);
  FloorState &mutable_floor(int f);
  void rehash_floor(int f);
  void rebuild_movement_floor(int f);
  void refresh_movement_cell(FloorState &,int f,Pos);
  std::uint64_t floor_hash(int f) const;
  bool enter_flower(int f, Pos from, Pos to) const;
  bool auto_can_battle(Block) const;
  bool trapped_wizard(int f, Pos) const;
  void after_action_impl(int f, Pos, Block, bool automatic_action);
  void successful_move(int, Pos from, Pos to);
  bool native_can_move(int, Pos from, Pos to) const;
  bool direct_movement_enabled() const;
  bool dialogue_can_delete(int, Pos) const;
  bool trigger_native_event(int, Pos);
  void consume_tool_energy();
  Json initialized_block_event(int f, Pos p, Block b) const;
  void set_block_disabled_flag(int f, Pos p, bool disabled);
  const Json &native_block_event_view(int f, Pos p) const;
  void sync_retained_cell(int f, Pos p);
};
Json read_json(const std::filesystem::path &);
std::uint64_t hash_json(const Json &);
} // namespace mota
