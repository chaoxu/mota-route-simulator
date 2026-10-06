#pragma once
#include "engine/bits.hpp"
#include "engine/cache.hpp"
#include "engine/identity.hpp"
#include "engine/observations.hpp"
#include "engine/order-log.hpp"
#include "engine/order.hpp"
#include "engine/pages.hpp"
#include "engine/regions.hpp"
#include <array>
#include <bitset>
#include <boost/container/small_vector.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace engine {
using Number = std::int64_t;
template <class T, std::size_t N> using SmallVector = boost::container::small_vector<T, N>;
constexpr int side = 13, cells = 169, floors_count = 51;
constexpr Number infinite_damage = INT64_MAX / 4;
struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct Unsupported : Error {
  using Error::Error;
};
struct Pos {
  int x = 0, y = 0;
  int index() const { return y * side + x; }
  bool valid() const { return x >= 0 && y >= 0 && x < side && y < side; }
  auto operator<=>(const Pos &) const = default;
};
// An observer sees combat during an input, including speculative inputs.
// Consumers may retain its evidence only after that whole input succeeds.
// It supplies no gameplay state and is deliberately excluded from identity.
class ScopedBattleObserver {
  std::function<void(int,Pos,int)> callback_;
  const std::function<void(int,Pos,int)> *previous_;
public:
  explicit ScopedBattleObserver(std::function<void(int,Pos,int)>);
  ~ScopedBattleObserver();
  ScopedBattleObserver(const ScopedBattleObserver &)=delete;
  ScopedBattleObserver &operator=(const ScopedBattleObserver &)=delete;
};
void record_native_battle(int,Pos,int);
enum Stat {
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
inline constexpr const char *stat_names[] = {"hp",
                                             "atk",
                                             "def",
                                             "gold",
                                             "yk",
                                             "bk",
                                             "rk",
                                             "dmg",
                                             "yd",
                                             "bd",
                                             "rd",
                                             "times1",
                                             "centerFly",
                                             "upFly",
                                             "downFly",
                                             "earthquake",
                                             "bomb",
                                             "bigKey",
                                             "pickaxe",
                                             "superPotion",
                                             "snow",
                                             "knife",
                                             "earthquakeBought",
                                             "coin",
                                             "cross",
                                             "magicImmune",
                                             "won49",
                                             "won50",
                                             "mdef"};
struct Hero {
  std::array<Number, stat_count> values{};
  int floor = 1;
  Pos pos;
  Number &operator[](Stat s) { return values[s]; }
  Number operator[](Stat s) const { return values[s]; }
  auto operator<=>(const Hero &) const = default;
};
enum class Kind : std::uint8_t { empty, wall, enemy, door, item, terrain, npc };
enum EntityFeature : std::uint16_t {
  flower = 1,
  lava = 2,
  fake_wall = 4,
  potion = 8,
  stair = 16,
  cross_enemy = 32,
  dragon = 64,
  combat_item = 128
};
enum class Direction : std::uint8_t {
  up,
  down,
  left,
  right,
  leftup,
  leftdown,
  rightup,
  rightdown,
  forward,
  backward
};
inline constexpr std::array<Pos, 4> directions = {Pos{0, -1}, Pos{0, 1}, Pos{-1, 0}, Pos{1, 0}};
inline constexpr std::array<Pos, 4> auto_dirs = {Pos{0, -1}, Pos{-1, 0}, Pos{0, 1}, Pos{1, 0}};
inline constexpr auto auto_neighbors = [] {
  std::array<std::array<std::uint8_t, 4>, cells> result{};
  for (int i = 0; i < cells; ++i)
    for (int d = 0; d < 4; ++d) {
      int x = i % side + auto_dirs[d].x, y = i / side + auto_dirs[d].y;
      result[i][d] = std::uint8_t(x >= 0 && x < side && y >= 0 && y < side ? y * side + x : 255);
    }
  return result;
}();
inline constexpr auto cell_positions = [] {
  std::array<Pos, cells> result{};
  for (int i = 0; i < cells; ++i)
    result[i] = {i % side, i / side};
  return result;
}();
inline Pos add(Pos a, Pos b) {
  return {a.x + b.x, a.y + b.y};
}
inline int distance(Pos a, Pos b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
Direction direction(std::string_view);
std::string_view direction_name(Direction);
Pos delta(Direction);
int floor_number(std::string_view);
std::string floor_name(int);

// Object-valued expression results have concrete native meanings. Opaque values
// preserve immutable, non-executed import metadata at the I/O boundary only.
struct EnemyRef {
  int entity;
  auto operator<=>(const EnemyRef &) const = default;
};
struct BlockRef {
  int floor, index;
  auto operator<=>(const BlockRef &) const = default;
};
struct SearchRef {
  int floor, entity;
  auto operator<=>(const SearchRef &) const = default;
};
struct Opaque {
  int id;
  bool truth = true;
  auto operator<=>(const Opaque &) const = default;
};
struct NumberArray {
  std::vector<Number> values;
  auto operator<=>(const NumberArray &) const = default;
};
using Value = std::variant<std::monostate, bool, Number, double, std::string, EnemyRef, BlockRef, SearchRef,
                           Opaque, NumberArray>;
bool truth(const Value &);
bool rule_truth(const Value &);
double numeric(const Value &);
Number integer(const Value &);
std::string text(const Value &);
Value number_value(double);
bool equal_value(const Value &, const Value &, bool strict);
std::uint64_t specials(const Value &);

struct Block {
  std::uint16_t entity = 0;
  std::int16_t tile = 0;
  auto operator<=>(const Block &) const = default;
};
enum class Trigger : std::uint8_t { none, action, battle, item, door, change_floor, other };
struct DoorKey {
  int item;
  Number count;
  bool preserve = false;
};
struct Props {
  std::uint32_t specified = 0;
  Trigger trigger = Trigger::none;
  bool no_pass = false, script = false, event_truth = false, can_break = false, has_data = false,
       door = false, door_class = false;
  std::uint8_t in = 0, out = 0;
  int action = 0, touch = 0, out_event = 0, change = 0, entity_id = 0;
  std::string cls = "terrains", id = "none";
  std::vector<DoorKey> keys;
};
struct Actor {
  Kind kind = Kind::empty;
  std::uint16_t features = 0;
  Number hp = 0, atk = 0, def = 0, money = 0, n = 4, def_value = 0, atk_value = 0, value = 0;
  int range = 1;
  std::uint64_t special = 0;
  bool square = false, exp_present = false;
  Number exp = 0;
  int after_battle = 0;
};
struct ItemEffect {
  std::array<Number, 4> amount{};
  std::array<std::uint8_t, 4> stat{};
  std::uint8_t count = 0;
  bool ratio = false, supported = false, immune = false;
  int flag = -1;
};
struct Entity : Actor {
  ItemEffect effect;
  std::string id;
  int inventory_stat = -1, item_class = 0, item_index = -1;
  Block block;
  Props props;
};
enum class Ref : std::uint8_t {
  flag,
  stat,
  item,
  switch_flag,
  status_x,
  status_y,
  status_direction,
  extra,
  auto_battle,
  auto_wall,
  auto_item,
  floor,
  zero
};
struct Target {
  Ref kind = Ref::flag;
  int index = 0;
};
enum class ExprOp : std::uint8_t {
  literal,
  reference,
  call,
  member,
  neg,
  positive,
  logical_not,
  add,
  sub,
  mul,
  div,
  mod,
  lt,
  le,
  gt,
  ge,
  eq,
  ne,
  seq,
  sne,
  logical_and,
  logical_or,
  conditional,
  error
};
enum class Function : std::uint8_t {
  round,
  floor,
  ceil,
  abs,
  min,
  max,
  get_flag,
  has_flag,
  item_count,
  has_item,
  get_status,
  visited,
  replaying,
  hero_loc,
  block_id,
  block_cls,
  block,
  search,
  can_battle,
  enemy_info,
  cal_value
};
struct Expr {
  ExprOp op = ExprOp::literal;
  Value literal;
  Target target;
  Function function{};
  std::vector<int> args;
  std::string name;
  bool bound = false;
  int bound_index = -1, bound_floor = -1;
  Pos bound_pos;
};
enum class Op : std::uint8_t {
  noop,
  assign,
  if_else,
  switch_case,
  choices,
  while_loop,
  for_loop,
  break_loop,
  exit,
  insert,
  function,
  hide,
  show,
  set_block,
  set_layer,
  open_door,
  close_door,
  move_hero,
  move_block,
  generate_move,
  change_floor,
  set_enemy,
  battle,
  win,
  bgm,
  unsupported
};
enum class Assign : std::uint8_t { set, add, sub, mul, div, mod };
struct Location {
  int x = 0, y = 0;
};
struct Motion {
  Direction direction;
  int count;
};
struct Choice {
  int condition = 0, need = 0, program = 0, source = 0;
};
struct Case {
  int condition = 0, program = 0;
  bool is_default = false, fallthrough = false;
};
struct Instruction {
  Op op = Op::noop;
  Target target;
  Assign assign = Assign::set;
  int value = 0, condition = 0, yes = 0, no = 0, program = 0, floor = -1, entity = 0, number = 0, from = 0,
      to = 0, step = 0, source = 0;
  bool remove = false, destruct = false, keep = false, need_key = false, animate = false,
       has_location = false, has_id = false, foreground = false;
  bool ignore_present = false;
  std::uint64_t function = 0;
  std::string name, reason;
  std::vector<Location> locations;
  std::vector<Motion> motions;
  std::vector<Value> arguments;
  std::vector<Choice> choices;
  std::vector<Case> cases;
};
enum class FastOp : std::uint8_t {
  generic,
  noop,
  stat_constant,
  literal_assign,
  fixed_hide,
  fixed_show,
  function
};
struct MicroOp {
  Number immediate = 0;
  int index = 0;
  std::int16_t floor = -1;
  std::uint8_t x = 0, y = 0;
  FastOp op = FastOp::generic;
  Assign assign = Assign::set;
  bool remove = false, destruct = false;
};
class Game;
using NativeStep = void (*)(Game &, int, Pos);
using NativeBlock = void (*)(Game &, int, Pos, std::size_t &);
struct Program {
  std::vector<int> code;
  std::vector<int> next_effect;
  std::vector<NativeBlock> native_blocks;
  int source = 0;
};
enum class GuardOp : std::uint8_t { flag, is_null, block_id, block_class };
struct Guard {
  GuardOp op;
  int index = -1, floor = -1;
  Pos pos;
  bool negate = false;
  std::string value;
};
struct AutoEvent {
  int floor, index, condition, program, source;
  bool multiple = false, current_floor = true;
  std::string symbol;
  bool guards_native = false;
  std::vector<Guard> guards;
};
struct Floor {
  int ratio = 1;
  Pos up{-1, -1}, down{-1, -1};
  bool hidden = false, fly_from = true, fly_to = true, no_direct = false;
  std::array<std::uint8_t, cells> in{}, out{};
  std::array<int, cells> source_payload{}, after_battle{}, after_item{}, after_door{}, change{},
      switch_slot{};
  CellMask source_event;
  CellMask auto_battle_blocked;
  int first_arrive = 0, each_arrive = 0;
  std::vector<int> automatic;
};
struct FlagIds {
  enum : int {
    feature,
    energy,
    addhp,
    curse,
    hatred,
    magic_immune,
    no_zone,
    no_repulse,
    no_laser,
    no_between,
    no_ambush,
    no_auto_check,
    ignore_floor,
    cannot_direct,
    fly,
    extra_functions,
    ratio,
    times,
    arg0,
    arg1,
    arg2,
    arg3,
    arg4,
    talking,
    timeout,
    bgm,
    weapon,
    shield,
    item_hint,
    t,
    count
  };
};
inline constexpr std::array<std::string_view, FlagIds::count> builtin_flag_names = {
    "开启特性",
    "对话能量",
    "addhp",
    "curse",
    "hatred",
    "魔法免疫",
    "no_zone",
    "no_repulse",
    "no_laser",
    "no_betweenAttack",
    "no_ambush",
    "__doNotCheckAutoEvents__",
    "ignoreChangeFloor",
    "cannotMoveDirectly",
    "fly",
    "额外功能开关",
    "ratio",
    "times1",
    "arg0",
    "arg1",
    "arg2",
    "arg3",
    "arg4",
    "talking",
    "timeout",
    "__bgm__",
    "nowWeapon",
    "nowShield",
    "__itemHint__",
    "t"};
struct CorridorGraph;
struct World {
  World();
  std::uint64_t source_tag = 0;
  std::vector<NativeStep> native_steps;
  int first_floor = 0;
  std::vector<Entity> entities;
  std::vector<Actor> actors;
  std::array<Floor, floors_count> floors;
  std::array<std::shared_ptr<const CorridorGraph>, floors_count> corridors;
  std::vector<Props> payloads;
  // Exact source metadata is interned once at the compilation boundary.
  // Runtime identities compare integer IDs, never parse or serialize JSON.
  bool identities_ready = false;
  std::vector<std::array<std::uint32_t, 2>> payload_identity, composed_identity, base_identity;
  std::vector<std::uint32_t> source_identity, opaque_identity, choice_text_identity, menu_part_identity;
  // Compiled combinations of entity rules and immutable floor event overlays.
  std::vector<Props> composed;
  std::vector<std::uint32_t> composition;
  void lower();
  void lower_events();
  std::vector<MicroOp> micro;
  std::vector<std::uint64_t> flag_dependents;
  std::array<std::array<std::uint64_t, cells>, floors_count> cell_dependents{};
  std::uint64_t auto_dynamic = 0, auto_relative = 0;
  bool indexed_automatic = false;
  std::vector<Expr> expressions{{}};
  std::vector<Instruction> instructions{{}};
  std::vector<Program> programs{{}};
  std::vector<AutoEvent> automatic;
  std::vector<std::string> flag_names, extra_names, item_names;
  std::vector<int> temporary_flags;
  std::vector<std::uint8_t> is_temporary;
  std::unordered_map<std::string, int> flag_ids, extra_ids, entity_ids, item_ids, common;
  std::unordered_map<int, Block> tiles;
  FlagIds ids{};
  std::array<int, 6> purchase_atk{}, purchase_def{}, purchase_hp{}, purchase_atk_count{},
      purchase_def_count{}, purchase_hp_count{};
  std::array<std::array<int, 2>, floors_count> dialogue_flags{};
  std::array<int, 256> item_stat{}, item_class{};
  Number hatred = 2, green_potion = 800, yellow_potion = 500, green_gem = 5;
  bool near_stair = true, direct_enabled = true, gentle = false, ignore_stairs = false;
  int help = 0, dialogue = 0, trader = 0;
  std::array<int, 13> march_programs{};
  int march_tail = 0;
};
struct Cell {
  Block retained;
  bool present : 1 = false;
  bool disabled : 1 = false;
  bool event_entry : 1 = false;
  bool source_destroyed : 1 = false;
  bool base_payload : 1 = false;
  bool touch_precompiled : 1 = false;
  int payload = 0; // Immutable import descriptor, zero means a compiled rule.
  std::uint32_t changed = 0;
};
struct LayerChange {
  Pos pos;
  int tile;
};
enum class SweepRule : std::uint8_t { empty, blocked, item, potion, enemy, wall, lava, conditional };
enum class EnterRule : std::uint8_t { generic, empty, enemy, item, door };
struct FloorState {
  mutable IdentityCache identity;
  CellMask dirty_cells;
  std::array<SweepRule, cells> sweep_rule{};
  std::array<EnterRule, cells> enter_rule{};
  CellMask sweep_candidates, sweep_empty;
  CellMask batch_regular, batch_conditional;
  std::array<Cell, cells> cell;
  std::array<std::int16_t, cells> foreground{}, background{};
  CellLog<Pos> order;
  CellOrder<Pos> hazard_order;
  CellMask hazard_cells;
  std::uint64_t hazard_roles_epoch = 1;
  ObservationLog observations;
  std::vector<LayerChange> foreground_changes, background_changes;
  bool disabled_record = false, foreground_record = false, background_record = false;
  std::uint64_t revision = 1, hazard_revision = 1;
  std::array<std::uint8_t, cells> movement_in{}, movement_out{};
  std::array<CellMask, 4> edges;
  CellMask transit, terminal, fire, stairs, restricted, change_cells;
  CellMask empty_cells, item_cells, potion_cells, enemy_cells, fake_wall_cells, lava_cells;
  Regions<false> walk_regions;
  Regions<true> empty_regions;
  int movement_restrictions = 0;
};
struct Flags {
  mutable IdentityCache identity;
  ValuePages<Value> scalar;
  std::vector<std::optional<Value>> extra;
  std::array<Number, 256> inventory{};
};
struct EnemyOverride {
  std::bitset<12> present;
  std::array<Value, 12> value;
};
// Override spelling belongs to observation/export. Combat reads the flat,
// resolved Actor table and never visits this sparse, copy-on-write storage.
struct OverrideTable {
  mutable IdentityCache identity;
  std::vector<std::shared_ptr<EnemyOverride>> entries;
  explicit OverrideTable(std::size_t size) : entries(size) {}
  std::size_t size() const { return entries.size(); }
  const EnemyOverride &at(std::size_t i) const {
    static const EnemyOverride empty;
    const auto &entry = entries.at(i);
    return entry ? *entry : empty;
  }
  EnemyOverride &edit(std::size_t i) {
    identity.reset();
    auto &entry = entries.at(i);
    if (!entry)
      entry = std::make_shared<EnemyOverride>();
    else if (entry.use_count() != 1)
      entry = std::make_shared<EnemyOverride>(*entry);
    return *entry;
  }
};
inline constexpr const char *enemy_fields[] = {"hp",    "atk",        "def",      "money",
                                               "n",     "defValue",   "atkValue", "value",
                                               "range", "zoneSquare", "special",  "exp"};
struct Segment {
  int program = 0, cursor = 0;
};
struct Frame {
  SmallVector<Segment, 4> segments;
  int floor = 0;
  Pos pos;
  int loop = 0, automatic = -1;
};
struct PendingChoice {
  int instruction = 0, floor = 0;
  Pos pos;
  SmallVector<int, 8> options;
};
struct Leave {
  Pos pos;
  Direction direction;
};
struct Hazard {
  std::uint64_t revision = 0, context = 0;
  std::array<Number, cells> before{}, after{};
  CellMask pincer, reactions, positive;
  bool geometry_sensitive = false, plain = true;
};
struct AutoSettings {
  bool battle = false, wall = false;
  int item = 0;
  auto operator<=>(const AutoSettings &) const = default;
};
struct ReachCache {
  std::uint64_t revision = 0, context = 0;
  Number hp = 0;
  int from = 0;
  bool component = false;
  CellMask reached;
};
struct SettledCache {
  std::uint64_t revision = 0, context = 0;
  std::array<Number, 7> stats{};
  AutoSettings options;
  bool ignore = false;
  CellMask settled, blocked_items;
};
enum class TokenOp : std::uint8_t { step, move, fly, choice, turn, item, key, help, get_next };
struct Token {
  TokenOp op;
  int argument = 0;
  Pos pos;
};

// Scalar state and shared roots are the transaction checkpoint. Floor roots
// are journaled lazily, and derived query caches have separate ownership.
struct GameState {
public:
  std::shared_ptr<const World> world;
  Hero hero;
  // Numeric and Boolean builtin flags are authoritative unboxed state.
  // Other values retain exact native type and import metadata in cold storage.
  std::array<std::optional<Value>, FlagIds::count> builtin_flags;
  std::array<std::uint8_t, FlagIds::count> builtin_types = [] {
    std::array<std::uint8_t, FlagIds::count> v;
    v.fill(255);
    return v;
  }();
  std::array<Number, FlagIds::count> builtin_numbers{};
  std::uint32_t builtin_truth = 0;
  Direction facing = Direction::down;
  AutoSettings automatic;
  bool allow_direct = true, gentle_click = false;
  std::array<CellMask, floors_count> blocked_items;
  mutable std::uint64_t geometry_pending = 0, geometry_flushes = 0, geometry_cells = 0;
  std::shared_ptr<Flags> flags;
  std::shared_ptr<OverrideTable> overrides;
  std::shared_ptr<std::vector<Actor>> resolved_enemies;
  const Actor *actor_data = nullptr;
  std::bitset<floors_count> visited, visited_flags;
  std::array<std::optional<Leave>, floors_count> leave;
  SmallVector<int, 8> auto_executed, auto_executing;
  SmallVector<Frame, 4> frames;
  std::optional<PendingChoice> choice;
  std::uint64_t compiled_steps = 0, generic_steps = 0;
  std::uint64_t auto_dirty = UINT64_MAX, auto_tests = 0, auto_skips = 0;
  bool initialized = false, has_auto = false, has_visited_record = true, has_leave = true, has_aed = false,
       has_aei = true, has_enemy_info = false, auto_active = false, executing = false;
  bool temporaries_possible = true;
  Number event_serial = 0;
  bool has_event_serial = false;
  std::string win_reason;
  bool has_win_reason = false, has_exp = false, exp_null = false, mdef_present = false;
  Number exp = 0;
  std::uint64_t hazard_context = 1;
  std::uint64_t hazard_roles_epoch = 1;
  std::uint64_t bfs_count = 0, hazard_builds = 0, auto_bfs_count = 0, auto_no_work = 0, component_queries = 0,
                auto_single = 0;

  bool use_corridors = false, batch_exhausted = false, use_batches = true, use_closure_first = true;
  std::uint64_t batch_count = 0, batch_cells = 0, batch_probes = 0;
  std::uint64_t graph_sweeps = 0, graph_visits = 0;
  bool use_native = true;
  std::uint64_t native_step_count = 0, native_block_count = 0, slow_step_count = 0;

  explicit GameState(std::shared_ptr<const World> w)
      : world(std::move(w)), allow_direct(world->direct_enabled), gentle_click(world->gentle),
        flags(std::make_shared<Flags>()), overrides(std::make_shared<OverrideTable>(world->entities.size())) {
    actor_data = world->actors.data();
    flags->scalar.resize(world->flag_names.size());
    flags->extra.resize(world->extra_names.size());
  }
};
struct QueryCaches {
  mutable CacheTable<Hazard, floors_count> hazards;
  CacheTable<ReachCache, floors_count * 2> reach;
  CacheTable<SettledCache, floors_count> settled;
};
class Transaction;
class Game : public GameState, public QueryCaches {
  friend class Transaction;
  Transaction *transaction_ = nullptr;
  void record_floor(int);
  static Game &movable(Game &);

public:
  mutable std::array<std::shared_ptr<FloorState>, floors_count> floors;
  explicit Game(std::shared_ptr<const World> w) : GameState(std::move(w)) {}
  Game(const Game &);
  Game(Game &&);
  Game &operator=(const Game &);
  Game &operator=(Game &&);
  Transaction transaction();
  // Atomic input: commit on success, restore the complete prior state on an exception.
  void apply(const Token &, bool strict = false);
  std::uint64_t hash() const;
  bool operator==(const Game &) const;
  // Eagerly copy the initial mutable storage for independent full rollouts.
  // This performs no actions and leaves all derived query caches cold.
  void detach();
  void token(const Token &, bool strict = false);
  Value eval(int, int, Pos) const;
  Value read(Target, int, Pos) const;
  void assign(Target, Value, Assign, int, Pos);
  Value flag(int, Value fallback = Number(0)) const;
  std::optional<Value> flag_value(int) const;
  void store_flag(int, std::optional<Value>);
  Number flag_number(int id) const {
    if (unsigned(id) < FlagIds::count)
      return builtin_numbers[id];
    if (id < 0 || std::size_t(id) >= flags->scalar.size() || !flags->scalar[id])
      return 0;
    const auto &v = *flags->scalar[id];
    if (auto *n = std::get_if<Number>(&v))
      return *n;
    if (auto *n = std::get_if<double>(&v))
      return Number(*n);
    if (auto *b = std::get_if<bool>(&v))
      return *b;
    return 0;
  }
  bool flag_truth(int id) const {
    if (unsigned(id) < FlagIds::count)
      return (builtin_truth >> id) & 1;
    if (id < 0 || std::size_t(id) >= flags->scalar.size() || !flags->scalar[id])
      return false;
    const auto &v = *flags->scalar[id];
    if (auto *b = std::get_if<bool>(&v))
      return *b;
    if (auto *n = std::get_if<Number>(&v))
      return *n != 0;
    if (auto *n = std::get_if<double>(&v))
      return Number(*n) != 0;
    if (auto *s = std::get_if<std::string>(&v))
      return !s->empty();
    if (auto *p = std::get_if<Opaque>(&v))
      return p->truth;
    if (auto *a = std::get_if<NumberArray>(&v))
      return !a->values.empty();
    return v.index() != 0;
  }
  void set_flag(int, Value);
  void set_number_flag(int id, Number value) {
    if (unsigned(id) >= FlagIds::count) {
      set_flag(id, Value(value));
      return;
    }
    if (builtin_types[id] == 2 && builtin_numbers[id] == value)
      return;
    builtin_flags[id].reset();
    builtin_types[id] = 2;
    builtin_numbers[id] = value;
    auto bit = std::uint32_t(1) << id;
    builtin_truth = (builtin_truth & ~bit) | (value ? bit : 0);
    if (world->is_temporary[id])
      temporaries_possible = true;
    if (std::size_t(id) < world->flag_dependents.size())
      auto_dirty |= world->flag_dependents[id];
    if (id >= FlagIds::magic_immune && id <= FlagIds::no_ambush)
      ++hazard_context;
  }
  void clear_temporaries();
  Number item_count(int) const;
  void set_item(int, Number, bool increase = false);
  const Cell &cell(int, Pos) const;
  Block block(int, Pos) const;
  const Props &props(int, Pos) const;
  const Props &retained_props(const Cell &) const;
  bool present(int, Pos, bool disabled = false) const;
  bool event_enabled(int, Pos) const;
  FloorState &edit_floor(int);
  void sync_cell(int, Pos);
  void refresh_cell(int, Pos);
  void mark_geometry(int, Pos);
  void ensure_geometry(int) const;
  void invalidate_hazard(int, Block, Block);
  void disabled_flag(int, Pos, bool);
  void remove(int, Pos, bool destruct = false);
  void replace(int, Pos, Block, bool destruct = false, bool activate = false);
  void hide(int, Pos);
  void show(int, Pos);
  void erase_event(int, Pos);
  void layer(int, Pos, int, bool);
  const Actor &enemy(Block) const;
  void resolve_enemy(int);
  Number damage(Block) const;
  Number damage_for(Block, const Hero &) const;
  const Hazard &hazard(int f) const {
    const auto &cache = hazards[f];
    if (cache && cache->revision == floors[f]->hazard_revision && cache->context == hazard_context)
      return *cache;
    return build_hazard(f);
  }
  const Hazard &build_hazard(int) const;
  Number map_damage(int, Pos) const;
  bool can_move(int, Pos, Pos) const;
  bool direct_enabled() const;
  bool can_connect(int, Pos, Pos, bool direct = false);
  std::vector<Pos> path_to(int, Pos, bool click = false) const;
  bool can_delete(int, Pos) const;
  void step(Direction, bool);
  bool step_transaction(Direction, bool);
  void successful_move(int, Pos, Pos);
  void tile_event(int, Pos);
  void auto_sweep(int = -1);
  CellMask sweep_ordered(int);
  bool collect_batch(int, CellMask &);
  void remove_batch(int, CellMask, CellMask, int);
  bool sweep_interact(int, int, CellMask);
  void apply_item(int, int);
  void apply_block(int, Pos, Block);
  void fight(int, Pos, Block, Number);
  bool trigger(int, Pos);
  void after(int, int, Pos);
  void arrive(int, bool);
  void transition(int, Pos, bool flying = false);
  bool has_visited(int) const;
  bool can_fly(int);
  void fly(int);
  void tool(int, bool decrement = true);
  void start(int, int, Pos);
  void start_sequence(std::span<const int>, int, Pos);
  void push(int, int, Pos, int loop = 0);
  void prepend(int);
  void drain();
  void execute(int, int, Pos);
  void execute_slow(int, int, Pos);
  void choose(int);
  void check_auto();
  bool auto_condition(const AutoEvent &) const;
  void function(std::uint64_t, int, Pos);
  SmallVector<Pos, 4> locations(const Instruction &, int, Pos) const;
  int event_floor(const Instruction &) const;
  void change_enemy(int, int, Value);
};

// Nested scopes commit or undo in LIFO order. A scope rolls back unless it
// explicitly commits. Retained Game copies remain independent snapshots.
class Transaction {
  friend class Game;
  struct FloorUndo {
    int floor;
    std::shared_ptr<FloorState> before;
  };
  Game *game_;
  Transaction *parent_;
  std::optional<GameState> before_;
  QueryCaches caches_;
  SmallVector<FloorUndo, 2> floors_;
  std::uint64_t captured_ = 0;
  void capture(int);
  void finish();

public:
  explicit Transaction(Game &);
  Transaction(const Transaction &) = delete;
  Transaction &operator=(const Transaction &) = delete;
  ~Transaction();
  void commit();
  void rollback();
};

// Compilation and parsing live outside the simulation library's timed path.
int compile_expression(World &, std::string_view);
Target bind_target(World &, std::string_view);
int intern_flag(World &, std::string_view);
Token decode_token(const World &, std::string_view);
} // namespace engine
