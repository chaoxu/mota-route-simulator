#include "mota/simulator.hpp"
#include "mota/native_events.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <mutex>
#include <queue>
#include <tuple>

namespace mota {
struct NavigationCache {
  std::shared_ptr<const World> world;
  std::uint64_t context;
  bool hp_dependent;
  std::mutex mutex;
  using Key=std::tuple<int,int,bool,Number,Number,bool>;
  std::map<Key,bool> queries;
  // Cell predicates are independent of endpoints and HP. Preserve directed
  // edge traversal and terminal-stair rules while decoding retained JSON once.
  std::array<std::array<std::atomic<std::uint8_t>,cells>,2> nodes{};
  NavigationCache(std::shared_ptr<const World> source,std::uint64_t key,bool depends_on_hp)
    :world(std::move(source)),context(key),hp_dependent(depends_on_hp) {}
};
struct HazardReactions {
  struct Entry {bool repulse;Pos cell,source,direction;std::uint16_t entity;};
  std::bitset<cells> affected;
  std::vector<Entry> entries;
};
struct AutomaticCache {
  std::shared_ptr<const World> world;
  std::uint64_t context;
  int floor;
  std::array<Number,7> stats;
  Auto options;
  bool ignore_stairs,hp_dependent;
  std::mutex mutex;
  std::bitset<cells> settled;
  static std::array<Number,7> eligibility(const Game &g,bool health) {
    // Gold and key changes do not alter zero-damage combat or automatic tile
    // eligibility. HP matters only for a native pincer on this floor.
    return {g.hero[atk],g.hero[defense],g.hero[mdef],g.hero[cross],g.hero[knife],g.hero[snow],health?g.hero[hp]:0};
  }
  AutomaticCache(const Game &g,int f,bool ignore,bool health):world(g.world),context(g.navigation_context),floor(f),stats(eligibility(g,health)),options(g.automatic),ignore_stairs(ignore),hp_dependent(health) {}
  bool matches(const Game &g,int f,bool ignore) const {
    return world==g.world&&context==g.navigation_context&&floor==f&&stats==eligibility(g,hp_dependent)&&options==g.automatic&&ignore_stairs==ignore;
  }
};
struct HazardCache {
  struct Source {Pos pos;std::uint16_t entity;std::uint64_t special;Number value;int range;bool square;};
  std::shared_ptr<const World> world;
  std::uint64_t context;
  bool immune,zone,repulse,laser,between,ambush;
  std::vector<Source> sources;
  std::array<Number,cells> before{},after{};
  std::bitset<cells> pincer;
  SharedCacheHandle<const HazardReactions> reactions;
};
const std::array<std::string, stat_count> stat_names = {"hp",
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
namespace {
Number cached_map_damage(const HazardCache &cache,Number health,Pos p) {
  const int i=p.index();Number total=cache.before[i];
  if(cache.pincer[i]&&health-total>1)total+=(health-total)/2;
  return total+cache.after[i];
}
std::atomic<std::uint64_t> next_navigation_context{1};
std::uint64_t new_navigation_context(){return next_navigation_context.fetch_add(1,std::memory_order_relaxed);}
constexpr std::array<Pos, 4> route_dirs = {Pos{1, 0}, Pos{-1, 0}, Pos{0, 1}, Pos{0, -1}};
constexpr std::array<Pos, 4> auto_dirs = {Pos{0, -1}, Pos{-1, 0}, Pos{0, 1}, Pos{1, 0}};
const std::set<std::string> wall_ids = {
    "yellowWall", "whiteWall", "whiteWall2", "blueWall", "unbreakableWall", "blockWall",
    "autotile",   "star",      "lava",       "water",    "blueWater"};
std::uint64_t mix(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}
void combine(std::uint64_t &h, std::uint64_t x) {
  h = mix(h ^ x);
}
std::uint64_t cell_hash(int p, Block b) {
  return mix((std::uint64_t(p) << 32) | (std::uint64_t(b.entity) << 16) | std::uint16_t(b.tile));
}
std::uint64_t specials(const Json &j) {
  std::uint64_t m = 0;
  if (j.is_array()) {
    for (auto &v : j) {
      int n = v.get<int>();
      if (n >= 0 && n < 64)
        m |= 1ULL << n;
    }
  } else if (j.is_number()) {
    int n = j.get<int>();
    if (n >= 0 && n < 64)
      m |= 1ULL << n;
  }
  return m;
}
Number number(const Json &j) {
  if (j.is_boolean())
    return j.get<bool>() ? 1 : 0;
  if (j.is_number())
    return j.get<Number>();
  return 0;
}
bool truth(const Json &j) {
  if (j.is_null())
    return false;
  if (j.is_boolean() || j.is_number())
    return number(j) != 0;
  if (j.is_string())
    return !j.get_ref<const std::string &>().empty();
  return !j.empty();
}
Number val(const Json &j, const std::string &key, Number fallback = 0) {
  auto it = j.find(key);
  return it == j.end() || it->is_null() ? fallback : number(*it);
}
const Json &object_member(const Json &object,const std::string &key) {
  static const Json empty=Json::object();
  const auto it=object.find(key);return it==object.end()?empty:*it;
}
bool same_json_value(const Json &a,const Json &b) {
  if(a.type()!=b.type())return false;
  if(a.is_object()||a.is_array()) {
    if(a.size()!=b.size())return false;
    auto x=a.begin(),y=b.begin();
    for(;x!=a.end();++x,++y)if((a.is_object()&&x.key()!=y.key())||!same_json_value(*x,*y))return false;
    return true;
  }
  if(a.is_number_float())return std::bit_cast<std::uint64_t>(a.get<double>())==std::bit_cast<std::uint64_t>(b.get<double>());
  return a==b;
}
struct CombatStats {
  Kind kind;const std::string &id;
  Number hp,atk,def,money,n,def_value,atk_value;
  std::uint64_t special;
};
CombatStats combat_stats(const Game &g,Block b) {
  const auto &e=g.entity(b);CombatStats v{e.kind,e.id,e.hp,e.atk,e.def,e.money,e.n,e.def_value,e.atk_value,e.special};
  if(!g.reference_model) {
    const auto &o=object_member(object_member(g.extra_state->value,"enemy_overrides"),e.id);
    v.hp=val(o,"hp",v.hp);v.atk=val(o,"atk",v.atk);v.def=val(o,"def",v.def);v.money=val(o,"money",v.money);
    v.n=val(o,"n",v.n);v.def_value=val(o,"defValue",v.def_value);v.atk_value=val(o,"atkValue",v.atk_value);
    if(o.contains("special"))v.special=specials(o["special"]);
  }
  return v;
}
int marker(int f, Pos p) {
  if (f < 0 || f > 50 || !p.valid())
    throw RouteError("Invalid floor/position marker");
  return f * cells + p.index();
}
bool at(Pos a, int x, int y) {
  return a.x == x && a.y == y;
}
bool in(Pos p, std::initializer_list<Pos> ps) {
  return std::find(ps.begin(), ps.end(), p) != ps.end();
}
bool active_event(const Json &events,Pos p) {
  auto it=events.find(location(p));
  return it!=events.end()&&!it->is_null()&&(!it->is_object()||it->value("enable",true));
}
Pos add(Pos p, Pos d) {
  return {p.x + d.x, p.y + d.y};
}
int distance(Pos a, Pos b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
Json auto_json(Auto a) {
  return {{"battle", a.battle},
          {"wall", a.wall},
          {"item", a.item == 2 ? Json("nopotion") : Json(a.item == 1)}};
}
void require(bool test, const std::string &message) {
  if (!test)
    throw RouteError(message);
}
void require(bool test, const char *message) {
  if(!test)throw RouteError(message);
}
Kind classify(const std::string &id, const std::string &cls, const Json &enemies) {
  if (wall_ids.contains(id))
    return Kind::wall;
  if (enemies.contains(id) || cls == "enemy48")
    return Kind::enemy;
  if (id == "yellowDoor" || id == "blueDoor" || id == "redDoor" || id == "specialDoor" ||
      id == "steelDoor")
    return Kind::door;
  if (cls == "items")
    return Kind::item;
  if (id == "upFloor" || id == "downFloor" || id == "fakeWall" || id == "fakeWall2" ||
      id == "flower")
    return Kind::terrain;
  if (cls == "npcs" || cls == "npc48" || id == "blueShop" || id == "oldman" || id == "trader" ||
      id == "thief")
    return Kind::npc;
  if (!cls.empty())
    return Kind::terrain;
  return Kind::empty;
}
std::string kind_name(Kind k) {
  switch (k) {
  case Kind::empty:
    return "empty";
  case Kind::wall:
    return "wall";
  case Kind::enemy:
    return "enemy";
  case Kind::door:
    return "door";
  case Kind::item:
    return "item";
  case Kind::terrain:
    return "terrain";
  case Kind::npc:
    return "npc";
  }
  return "";
}
} // namespace
Json read_json(const std::filesystem::path &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("Cannot read " + path.string());
  return Json::parse(file);
}
std::uint64_t
hash_json(const Json &j) { // Canonical ordered JSON, performed only when JSON metadata changes.
  std::uint64_t h = 1469598103934665603ULL;
  for (unsigned char c : j.dump()) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  return h;
}
const std::string &RetainedEventPayload::serialization() const {
  std::call_once(serialized_once,[this]{serialized=value.dump();});return serialized;
}
const std::shared_ptr<const RetainedEventPayload> &RetainedEvents::lookup(Pos p) const {
  if(p.valid())return entries[p.index()];
  static const std::shared_ptr<const RetainedEventPayload> empty;
  const auto found=overflow.find(location(p));return found==overflow.end()?empty:found->second;
}
const Json &RetainedEvents::view(Pos p) const {
  static const Json empty=Json::object();const auto &entry=lookup(p);
  return entry?entry->value:empty;
}
Json RetainedEvents::value(Pos p,Json fallback) const {
  const auto &entry=lookup(p);return entry?entry->value:std::move(fallback);
}
void RetainedEvents::set(Pos p,Json value) {
  auto &entry=p.valid()?entries[p.index()]:overflow[location(p)];
  entry=std::make_shared<const RetainedEventPayload>(std::move(value));
}
void RetainedEvents::erase(Pos p) {if(p.valid())entries[p.index()].reset();else overflow.erase(location(p));}
std::uint64_t RetainedEvents::hash() const {
  if(!overflow.empty()) {
    Json all=Json::object();for(int i=0;i<cells;++i)if(entries[i])all[location({i%side,i/side})]=entries[i]->value;
    for(const auto &[key,entry]:overflow)all[key]=entry->value;return hash_json(all);
  }
  // Match hash_json's JSON object byte order exactly, including numeric types
  // and signed zero. Coordinates contain only decimal digits and a comma.
  static const auto keys=[] {
    std::array<std::pair<std::string,int>,cells> out;
    for(int i=0;i<cells;++i)out[i]={location({i%side,i/side}),i};
    std::sort(out.begin(),out.end());return out;
  }();
  std::uint64_t result=1469598103934665603ULL;
  auto feed=[&](std::string_view bytes){for(unsigned char byte:bytes){result^=byte;result*=1099511628211ULL;}};
  feed("{");bool first=true;
  for(const auto &[key,index]:keys)if(const auto &entry=entries[index]) {
    if(!first)feed(",");first=false;feed("\"");feed(key);feed("\":");feed(entry->serialization());
  }
  feed("}");return result;
}
bool RetainedEvents::operator==(const RetainedEvents &other) const {
  auto equal=[](const auto &left,const auto &right) {
    // JSON values containing NaN (or discarded values) are not reflexive.
    // Sharing their payload must not bypass the original JSON comparison.
    if(left==right)return !left||left->self_equal;
    return left&&right&&left->value==right->value;
  };
  if(overflow.size()!=other.overflow.size())return false;
  auto left=overflow.begin(),right=other.overflow.begin();
  for(;left!=overflow.end();++left,++right)if(left->first!=right->first||!equal(left->second,right->second))return false;
  for(int i=0;i<cells;++i)if(!equal(entries[i],other.entries[i]))return false;
  return true;
}
std::shared_ptr<const World> World::load(const std::filesystem::path &dir) {
  auto w = std::make_shared<World>();
  if (std::filesystem::exists(dir / "native_events.json"))
    w->native_data = read_json(dir / "native_events.json");
  auto enemy = read_json(dir / "enemy_stats.json").at("enemys");
  if(!w->native_data.empty())w->native_data["enemies"]=enemy;
  auto mapping = read_json(dir / "tile_mapping.json").at("tiles");
  auto register_entity = [&](const std::string &id, Kind kind) {
    if (w->by_id.contains(id))
      return w->by_id.at(id);
    Entity e;
    e.id = id;
    e.kind = kind;
    if (enemy.contains(id)) {
      auto &j = enemy.at(id);
      e.hp = val(j, "hp");
      e.atk = val(j, "atk");
      e.def = val(j, "def");
      e.money = val(j, "money");
      e.n = val(j, "n", 4);
      e.def_value = val(j, "defValue");
      e.atk_value = val(j, "atkValue");
      e.value = val(j, "value");
      e.range = int(val(j, "range", 1));
      e.square = truth(j.value("zoneSquare", Json(false)));
      e.special = specials(j.value("special", Json(0)));
    }
    auto idnum = std::uint16_t(w->entities.size());
    w->entities.push_back(std::move(e));
    w->by_id[id] = idnum;
    return idnum;
  };
  for (auto it = mapping.begin(); it != mapping.end(); ++it) {
    int tile = std::stoi(it.key());
    const auto &j = it.value();
    std::string id = j.at("id");
    auto kind = classify(id, j.value("cls", std::string{}), enemy);
    auto entity = register_entity(id, kind);
    auto native = j;
    if (!native.contains("noPass") || native["noPass"].is_null())
      native["noPass"] = native.contains("canPass") ? !truth(native["canPass"]) :
                         j.value("cls", std::string{}) != "items";
    if (!native.contains("trigger") || native["trigger"].is_null()) {
      if (kind == Kind::enemy) native["trigger"] = "battle";
      else if (kind == Kind::item) native["trigger"] = "getItem";
    }
    w->entities[entity].native = std::move(native);
    w->tiles[tile] = {kind == Kind::empty ? std::uint16_t(0) : entity, std::int16_t(tile)};
  }
  for (auto it = enemy.begin(); it != enemy.end(); ++it)
    register_entity(it.key(), Kind::enemy);
  if(!w->tiles.contains(17)) {
    auto n=register_entity("airwall",Kind::terrain);
    w->entities[n].native={{"id","airwall"},{"cls","terrains"},{"noPass",true},{"cannotIn",{"up","down","left","right"}}};
    w->tiles[17]={n,17};
  }
  auto fly = read_json(dir / "fly_points.json");
  w->first_floor=std::filesystem::exists(dir/"mt0_map.json")?0:1;
  for (int f = w->first_floor; f <= 50; ++f) {
    auto raw = read_json(dir / ("mt" + std::to_string(f) + "_map.json"));
    auto &fd = w->floors[f];
    fd.metadata = raw;
    fd.ratio = int(val(raw, "ratio",
                       (f >= 11 && f <= 20)   ? 2
                       : (f >= 31 && f <= 40) ? 4
                       : f >= 41              ? 5
                                              : 1));
    const auto &grid = raw.contains("map") ? raw.at("map") : raw.at("m");
    require(grid.size() == side, "Only 13x13 tower floors are supported");
    for (int y = 0; y < side; ++y) {
      require(grid[y].size() == side, "Only 13x13 tower floors are supported");
      for (int x = 0; x < side; ++x) {
        int t = grid[y][x];
        if (t == 0)
          continue;
        if (!w->tiles.contains(t))
          w->tiles[t] = {register_entity("unknownTile" + std::to_string(t), Kind::terrain),
                         std::int16_t(t)};
        fd.initial[y * side + x] = w->tiles.at(t);
      }
    }
    fd.events = raw.value("events", Json::object());
    fd.change_floor = raw.value("changeFloor", Json::object());
    auto ab = raw.value("afterBattle", Json::object());
    for (auto it = ab.begin(); it != ab.end(); ++it) {
      auto comma = it.key().find(',');
      Pos p{std::stoi(it.key().substr(0, comma)), std::stoi(it.key().substr(comma + 1))};
      fd.after_battle.set(p.index());
      bool open = false;
      if (it.value().is_array())
        for (auto &a : it.value())
          if (a.is_object() && a.value("type", std::string{}) == "openDoor")
            open = true;
      auto s = it.value().dump();
      if (open && s.find("\"waitAsync\"") == std::string::npos &&
          s.find("\"async\":true") == std::string::npos &&
          s.find("\"setBlock\"") == std::string::npos && s.find("\"hide\"") == std::string::npos)
        fd.after_battle_auto.set(p.index());
    }
    auto &fp = fly.at(floor_id(f));
    if (fp.contains("upFloor") && fp["upFloor"].is_array())
      fd.up = json_pos(fp["upFloor"][0], fp["upFloor"][1]);
    if (fp.contains("downFloor") && fp["downFloor"].is_array())
      fd.down = json_pos(fp["downFloor"][0], fp["downFloor"][1]);
  }
  return w;
}
Block World::make_block(const std::string &id, int tile) const {
  auto it = by_id.find(id);
  if (it == by_id.end())
    throw Unsupported("Unknown entity: " + id);
  if(tile==0&&!native_data.empty())for(auto &[n,b]:tiles)if(b.entity==it->second){tile=n;break;}
  return {it->second, std::int16_t(tile)};
}
Game::Game(std::shared_ptr<const World> w, const Json &snapshot) : world(std::move(w)) {
  prepared_native=prepare_native_world(world);
  navigation_context=new_navigation_context();
  auto &h = snapshot.at("hero");
  hero.floor = floor_number(snapshot.at("floorId"));
  require(hero.floor>=world->first_floor,"Snapshot floor is not available in this world");
  hero.pos = json_pos(h.at("x"), h.at("y"));
  require(hero.pos.valid(), "Hero position out of bounds");
  for (Stat s : {hp, atk, defense, yk, bk, rk})
    hero[s] = val(h, stat_names[s]);
  hero[gold] = val(h, "money");
  flags->value = h.value("flags", Json::object());
  flags->hash = hash_json(flags->value);
  // Materialize native defaults so exporting and resuming does not introduce
  // new state/hash fields merely because the export contains their values.
  extra_state->value["values"] = world->native_data.value("values", Json::object());
  extra_state->value["game_flags"] = world->native_data.value("flags", Json::object());
  extra_state->hash = hash_json(extra_state->value);
  items->value = h.value("items", Json::object());
  if (snapshot.contains("items")) items->value = snapshot["items"];
  for (const auto &name : {"tools", "constants", "equips"}) {
    if (!items->value.contains(name)) items->value[name] = h.value(name, Json::object());
  }
  auto tools = items->value["tools"], constants = items->value["constants"];
  if (h.contains("items")) {
    hero[yk] = val(tools,"yellowKey");hero[bk] = val(tools,"blueKey");hero[rk] = val(tools,"redKey");
  }
  items->hash = hash_json(items->value);
  set_extra("direction", h.value("direction", h.value("loc",Json::object()).value("direction",std::string("up"))));
  for(const auto &key:{"exp","mana","lv","equipment"})if(h.contains(key))set_extra(key,h.at(key));
  for (Stat s : {centerFly, upFly, downFly, earthquake, bomb, bigKey, pickaxe, superPotion})
    hero[s] = val(tools, stat_names[s]);
  hero[snow] = val(constants, "snow") ? val(constants, "snow") : val(tools, "snow");
  for (Stat s : {knife, coin, cross})
    hero[s] = val(constants, stat_names[s]) != 0;
  for(auto &section:items->value)if(section.is_object())
    for(const auto &id:{"yellowKey","blueKey","redKey","centerFly","upFly","downFly","earthquake","bomb","bigKey","pickaxe","superPotion","snow","knife","coin","cross"})section.erase(id);
  items->hash=hash_json(items->value);
  hero[times1] = flag(NativeFlag::times1);
  hero[earthquakeBought] = flag(NativeFlag::earthquakeBought);
  hero[magicImmune] = flag_truth(NativeFlag::magic_immune);
  auto vis = flags->value.value("__visited__", Json::object());
  for (auto it = vis.begin(); it != vis.end(); ++it)
    if (truth(it.value()))
      visited.set(floor_number(it.key()));
  visited.set(hero.floor);
  auto au = flags->value.value("__auto__", Json::object());
  if (au.is_object()) {
    automatic.battle = au.value("battle", Json(false)) == Json(true);
    automatic.wall = au.value("wall", Json(false)) == Json(true);
    auto item = au.value("item", Json(false));
    automatic.item = item == Json("nopotion") ? 2 : truth(item) ? 1 : 0;
  }
  for (int f = 0; f <= 50; ++f) {
    if(f>=world->first_floor)present.set(f);
    floors[f] = std::make_shared<FloorState>();
    auto &fs = *floors[f];
    fs.blocks = world->floors[f].initial;
    fs.events = std::make_shared<Json>(world->floors[f].events);
    auto fg = world->floors[f].metadata.value("fgmap",Json::array());
    auto bg = world->floors[f].metadata.value("bgmap",Json::array());
    if (snapshot.contains("maps") && snapshot["maps"].contains(floor_id(f)) &&
        snapshot["maps"][floor_id(f)].contains("fgmap")) fg = snapshot["maps"][floor_id(f)]["fgmap"];
    for (int y=0;y<side && y<int(fg.size());++y)
      for(int x=0;fg[y].is_array() && x<side && x<int(fg[y].size());++x)
        fs.foreground[y*side+x]=std::int16_t(number(fg[y][x]));
    for(int y=0;y<side&&y<int(bg.size());++y)for(int x=0;bg[y].is_array()&&x<side&&x<int(bg[y].size());++x)
      fs.background[y*side+x]=std::int16_t(number(bg[y][x]));
    auto modified_fg = flags->value.value("__fgv__",Json::object()).value(floor_id(f),Json::array());
    for (const auto &v:modified_fg) {
      Pos p=json_pos(v.at(0),v.at(1));
      if(p.valid())fs.foreground[p.index()]=std::int16_t(number(v.at(2)));
    }
    for(const auto &v:flags->value.value("__bgv__",Json::object()).value(floor_id(f),Json::array())) {
      Pos p=json_pos(v.at(0),v.at(1));if(p.valid())fs.background[p.index()]=std::int16_t(number(v.at(2)));
    }
    if (snapshot.contains("maps") && snapshot["maps"].contains(floor_id(f))) {
      auto saved = snapshot["maps"][floor_id(f)].value("map", Json::array());
      for (int y = 0; y < side && y < int(saved.size()); ++y)
        if (saved[y].is_array()) {
          require(saved[y].size() == side, "Invalid snapshot row width");
          for (int x = 0; x < side; ++x) {
            int t = saved[y][x];
            if (t == -1)
              continue;
            Pos p{x, y};
            auto old = fs.blocks[p.index()];
            if (t == 0) {
              fs.blocks[p.index()] = {};
              if (old.tile != 0)
                fs.events->erase(location(p));
            } else {
              auto it = world->tiles.find(t);
              if (it == world->tiles.end())
                throw Unsupported("Unknown snapshot tile " + std::to_string(t));
              fs.blocks[p.index()] = it->second;
            }
          }
        }
    }
    for (int i = 0; i < cells; ++i) {
      Pos p{i%side,i/side};auto b=fs.blocks[i];
      auto event=initialized_block_event(f,p,b);
      bool exists=b.entity||truth(event.value("trigger",Json()));
      if(exists) {
        fs.retained[i]=b;fs.retained_present.set(i);fs.block_order.push_back(p);
        fs.retained_events->set(p,event);
        bool disabled=false;
        auto source=fs.events->value(location(p),Json());
        if(source.is_object())disabled=!source.value("enable",true);
        auto lists=flags->value.value("__disabled__",Json::object()).value(floor_id(f),Json::array());
        if(lists.is_array()&&lists.size()==2) {
          if(std::find(lists[0].begin(),lists[0].end(),Json(i))!=lists[0].end())disabled=true;
          if(std::find(lists[1].begin(),lists[1].end(),Json(i))!=lists[1].end())disabled=false;
        }
        fs.disabled.set(i,disabled);
        if(disabled){fs.blocks[i]={};fs.events->erase(location(p));}
      }
      fs.grid[i] = fs.blocks[i].tile;
    }
    if(f>=world->first_floor)for(int i=0;i<cells;++i) {
      Pos p{i%side,i/side};const auto key=location(p);
      if(fs.events->contains(key)||fs.retained_events->view(p).contains("data"))
        sync_retained_cell(f,p);
    }
    rehash_floor(f);
  }
  pending_records = {{"_pending_auto_before", nullptr},
                     {"_pending_auto_trigger", ""},
                     {"_next_record_before", nullptr},
                     {"_next_record_note", ""}};
  retreat_wizards.insert(marker(47, {1, 9}));
  for (auto [f, p] : std::initializer_list<std::pair<int, Pos>>{
           {35, {6, 7}}, {25, {6, 6}}, {49, {6, 3}}, {50, {6, 5}}})
    no_auto_farm.insert(marker(f, p));
}
FloorState &Game::mutable_floor(int f) {
  require(f >= world->first_floor && f <= 50, "Invalid floor");
  if (floors[f].use_count() != 1)
    floors[f] = std::make_shared<FloorState>(*floors[f]);
  floors[f]->navigation.store(nullptr);
  floors[f]->hazards.store(nullptr);
  floors[f]->automatic_cache.store(nullptr);
  return *floors[f];
}
void Game::refresh_movement_cell(FloorState &fs,int f,Pos p) {
  // This representation is eagerly maintained with the COW floor. Readers
  // inspect two bytes per edge; no atomic or JSON work is needed on that path.
  const auto *prepared=prepared_native.get();
  if(!prepared||fs.movement_source.get()!=prepared||!prepared->matches(world)) {fs.movement_source.reset();return;}
  const auto i=p.index();const auto entity=fs.blocks[i].entity;
  if(entity>=prepared->entities.size()){fs.movement_source.reset();return;}
  auto mask=prepared->entities[entity];
  mask.in|=prepared->floor_in[f][i];mask.out|=prepared->floor_out[f][i];
  for(const auto tile:{fs.background[i],fs.foreground[i]}) {
    PreparedNativeWorld::MovementMask layer;
    if(tile>=0&&std::size_t(tile)<prepared->tiles.size())layer=prepared->tiles[tile];
    else {
      const auto found=world->tiles.find(tile);if(found==world->tiles.end())continue;
      if(found->second.entity>=prepared->entities.size()){fs.movement_source.reset();return;}
      layer=prepared->entities[found->second.entity];
    }
    mask.in|=layer.in;mask.out|=layer.out;
  }
  fs.movement_in[i]=mask.in;fs.movement_out[i]=mask.out;
}
void Game::rebuild_movement_floor(int f) {
  auto &fs=*floors[f];fs.movement_source=prepared_native;
  for(int i=0;i<cells;++i)refresh_movement_cell(fs,f,{i%side,i/side});
}
void Game::rehash_floor(int f) {
  auto &fs = *floors[f];
  fs.navigation.store(nullptr);
  fs.hazards.store(nullptr);
  fs.automatic_cache.store(nullptr);
  fs.hash.invalidate();
  fs.spell_cells.reset();
  fs.event_entries.reset();
  if (!reference_model)
    fs.spell_enemies.clear();
  for (int p = 0; p < cells; ++p) {
    fs.event_entries[p]=fs.events->contains(location({p%side,p/side}));
    auto &e = entity(fs.blocks[p]);
    if (!reference_model && e.kind == Kind::enemy && (e.special & (1ULL << 15)))
      fs.spell_enemies.push_back({p % side, p / side});
  }
  for (auto p : fs.spell_enemies) {
    fs.spell_cells.set(p.index());
  }
  rebuild_movement_floor(f);
}
std::uint64_t Game::floor_hash(int f) const {
  const auto &fs=*floors[f];
  return fs.hash.get([&] {
    auto hash=hash_json(*fs.events);if(!reference_model)hash^=fs.retained_events->hash();
    for(int p=0;p<cells;++p) {
      hash^=cell_hash(p,fs.blocks[p])^mix((std::uint64_t(p)<<32)|std::uint16_t(fs.grid[p]));
      hash^=mix((std::uint64_t(p)<<32)|0x10000ULL|std::uint16_t(fs.foreground[p]));
      hash^=mix((std::uint64_t(p)<<32)|0x20000ULL|std::uint16_t(fs.background[p]));
      if(!reference_model&&fs.retained_present[p])hash^=cell_hash(p,fs.retained[p])^mix((std::uint64_t(p)<<32)|0x40000ULL|(fs.disabled[p]?1:0));
    }
    return hash;
  });
}
Block Game::block(int f, Pos p) const {
  if (f < world->first_floor || f > 50 || !p.valid())
    return {};
  return floors[f]->blocks[p.index()];
}
Block Game::retained_block(int f,Pos p) const {
  return f>=world->first_floor&&f<=50&&p.valid()?floors[f]->retained[p.index()]:Block{};
}
bool Game::block_present(int f,Pos p,bool include_disabled) const {
  if(f<world->first_floor||f>50||!p.valid())return false;
  const auto &fs=*floors[f];return fs.retained_present[p.index()]&&(include_disabled||!fs.disabled[p.index()]);
}
bool Game::block_disabled(int f,Pos p) const {
  return block_present(f,p,true)&&floors[f]->disabled[p.index()];
}
Json Game::retained_block_event(int f,Pos p) const {
  return block_present(f,p,true)?floors[f]->retained_events->view(p):Json();
}
const Json &Game::native_block_event_view(int f,Pos p) const {
  static const Json empty={{"cls","terrains"},{"id","none"},{"noPass",false}};
  return block_present(f,p)?floors[f]->retained_events->view(p):empty;
}
Json Game::initialized_block_event(int f,Pos p,Block b) const {
  Json out=b.entity?entity(b).native:Json{{"cls","terrains"},{"id","none"},{"noPass",false}};
  out.erase("canPass");
  if(entity(b).kind==Kind::enemy) {
    const auto &source=object_member(object_member(world->native_data,"enemies"),entity(b).id);
    if(source.contains("faceIds")&&!source["faceIds"].is_null())out["faceIds"]=source["faceIds"];
  }
  auto merge=[&](Json event) {
    if(event.is_null())return;
    if(event.is_string())event=Json{{"data",Json::array({event})}};
    else if(event.is_array())event=Json{{"data",event}};
    if(!event.is_object())return;
    if(!event.contains("data")||event["data"].is_null())event["data"]=Json::array();
    for(auto it=event.begin();it!=event.end();++it)
      if(it.key()!="enable"&&it.key()!="animate"&&it.key()!="opacity"&&it.key()!="filter"&&!it.value().is_null())out[it.key()]=it.value();
    if(!out.contains("trigger")||!truth(out["trigger"]))out["trigger"]="action";
  };
  if(!flag_truth("R"+floor_id(f)+","+location(p))) {
    merge(world->floors[f].events.value(location(p),Json()));
    if(world->floors[f].change_floor.contains(location(p)))
      merge(Json{{"trigger","changeFloor"},{"data",world->floors[f].change_floor.at(location(p))}});
  }
  return out;
}
void Game::set_block_disabled_flag(int f,Pos p,bool disabled) {
  // The ordered lists are native gameplay state. Mutate the selected floor
  // after one COW split instead of copying every floor's lists per pickup.
  mutable_floor(f); // Also isolate/invalidate navigation when this setter is used directly.
  if(flags.use_count()!=1)flags=std::make_shared<JsonState>(*flags);
  if(!flags->value.contains("__disabled__"))flags->value["__disabled__"]=Json::object();
  auto &all=flags->value["__disabled__"];const auto id=floor_id(f);
  if(!all.is_object())static_cast<void>(all.value(id,Json::array()));
  if(!all.contains(id))all[id]=Json::array({Json::array(),Json::array()});
  auto &lists=all[id];
  require(lists.is_array()&&lists.size()==2,"Invalid native disabled-block flags");
  for(auto &list:lists) {
    if(!list.is_array()) {
      Json filtered=Json::array();for(const auto &v:list)if(number(v)!=p.index())filtered.push_back(v);list=std::move(filtered);continue;
    }
    auto &values=list.get_ref<Json::array_t&>();
    values.erase(std::remove_if(values.begin(),values.end(),[&](const Json &v){return number(v)==p.index();}),values.end());
  }
  lists[disabled?0:1].push_back(p.index());flags->hash.invalidate();
}
void Game::sync_retained_cell(int f,Pos p) {
  auto &fs=mutable_floor(f);int i=p.index();
  if(fs.events.use_count()!=1)fs.events=std::make_shared<Json>(*fs.events);
  bool enabled=fs.retained_present[i]&&!fs.disabled[i];
  fs.blocks[i]=enabled?fs.retained[i]:Block{};fs.grid[i]=fs.blocks[i].tile;
  refresh_movement_cell(fs,f,p);
  fs.events->erase(location(p));
  if(enabled) {
    const auto &event=fs.retained_events->view(p);
    if(event.contains("data")&&event.value("trigger",Json())!="changeFloor") {
      auto source=world->floors[f].events.value(location(p),Json());
      if(source.is_object()){source["data"]=event["data"];source["enable"]=true;(*fs.events)[location(p)]=source;}
      else (*fs.events)[location(p)]=event["data"];
    }
  }
  fs.event_entries[i]=fs.events->contains(location(p));
  fs.hash.invalidate();
  if(!reference_model) {
    const auto &e=entity(fs.blocks[i]);const bool spell=e.kind==Kind::enemy&&(e.special&(1ULL<<15));
    fs.spell_cells.set(i,spell);
    const auto it=std::lower_bound(fs.spell_enemies.begin(),fs.spell_enemies.end(),p,[](Pos a,Pos b){return a.index()<b.index();});
    if(spell&&(it==fs.spell_enemies.end()||*it!=p))fs.spell_enemies.insert(it,p);
    else if(!spell&&it!=fs.spell_enemies.end()&&*it==p)fs.spell_enemies.erase(it);
  }
}
void Game::hide_block(int f,Pos p) {
  if(!block_present(f,p,true))return;
  mutable_floor(f).disabled.set(p.index());set_block_disabled_flag(f,p,true);sync_retained_cell(f,p);
}
void Game::show_block(int f,Pos p) {
  if(!block_disabled(f,p))return;
  mutable_floor(f).disabled.reset(p.index());set_block_disabled_flag(f,p,false);sync_retained_cell(f,p);
}
void Game::remove_block(int f,Pos p,bool destruct) {
  if(!block_present(f,p,true))return;
  auto old=retained_block(f,p);
  if(destruct&&world->floors[f].events.contains(location(p)))set_flag("R"+floor_id(f)+","+location(p),true);
  if(!destruct&&(old.tile==121||old.tile==122))set_flag("对话能量",2);
  auto &fs=mutable_floor(f);int i=p.index();
  fs.retained[i]={};fs.retained_present.reset(i);fs.disabled.reset(i);
  if(fs.retained_events.use_count()!=1)fs.retained_events=std::make_shared<RetainedEvents>(*fs.retained_events);
  fs.retained_events->erase(p);
  fs.block_order.erase(std::remove(fs.block_order.begin(),fs.block_order.end(),p),fs.block_order.end());
  set_block_disabled_flag(f,p,true);sync_retained_cell(f,p);
}
void Game::replace_block(int f,Pos p,Block b,bool destruct,bool activate) {
  require(f>=world->first_floor&&f<=50&&p.valid(),"Block position out of bounds");
  if(destruct)set_flag("R"+floor_id(f)+","+location(p),true);
  auto event=initialized_block_event(f,p,b);
  if(!b.entity&&!truth(event.value("trigger",Json()))){remove_block(f,p,destruct);return;}
  auto &fs=mutable_floor(f);int i=p.index();bool existed=fs.retained_present[i];
  fs.retained[i]=b;fs.retained_present.set(i);
  if(fs.retained_events.use_count()!=1)fs.retained_events=std::make_shared<RetainedEvents>(*fs.retained_events);
  fs.retained_events->set(p,std::move(event));
  if(!existed){fs.block_order.push_back(p);fs.disabled.reset(i);set_block_disabled_flag(f,p,false);}
  else if(destruct||activate){fs.disabled.reset(i);set_block_disabled_flag(f,p,false);}
  sync_retained_cell(f,p);
}
void Game::set_block(int f, Pos p, Block b) {
  require(p.valid(), "Block position out of bounds");
  if (capturing) {
    captured.push_back({f, p, b});
    return;
  }
  if(!reference_model) {
    if(b.entity)replace_block(f,p,b);else remove_block(f,p);
    return;
  }
  auto &fs = mutable_floor(f);
  int i = p.index();
  if(b.entity&&!fs.blocks[i].entity)fs.block_order.push_back(p);
  else if(!b.entity&&fs.blocks[i].entity)fs.block_order.erase(std::remove(fs.block_order.begin(),fs.block_order.end(),p),fs.block_order.end());
  if(!reference_model && ((!b.entity&&(fs.blocks[i].entity||active_event(*fs.events,p))) ||
                         (b.entity&&!fs.blocks[i].entity&&!active_event(*fs.events,p)))) {
    auto disabled=flags->value.value("__disabled__",Json::object());
    auto lists=disabled.value(floor_id(f),Json::array({Json::array(),Json::array()}));
    if(!lists.is_array()||lists.size()!=2)throw RouteError("Invalid native disabled-block flags");
    for(auto &list:lists) {
      Json filtered=Json::array();for(const auto &value:list)if(number(value)!=i)filtered.push_back(value);list=std::move(filtered);
    }
    lists[b.entity?1:0].push_back(i);disabled[floor_id(f)]=lists;set_flag("__disabled__",disabled);
  }
  if (!reference_model && !b.entity && (fs.blocks[i].tile==121 || fs.blocks[i].tile==122))
    set_flag("对话能量",2);
  fs.hash.invalidate();
  fs.blocks[i] = b;
  fs.grid[i] = b.tile;
  refresh_movement_cell(fs,f,p);
  auto &e = entity(b);
  if (!reference_model) {
    bool spell = e.kind == Kind::enemy && (e.special & (1ULL << 15));
    fs.spell_cells.set(i, spell);
    auto it = std::find(fs.spell_enemies.begin(), fs.spell_enemies.end(), p);
    if (spell && it == fs.spell_enemies.end())
      fs.spell_enemies.push_back(p);
    if (!spell && it != fs.spell_enemies.end())
      fs.spell_enemies.erase(it);
    std::sort(fs.spell_enemies.begin(),fs.spell_enemies.end(),[](Pos a,Pos b){return a.index()<b.index();});
  }
}
void Game::set_block(int f, Pos p, const std::string &id, int t) {
  set_block(f, p, world->make_block(id, t));
}
void Game::erase_event(int f, Pos p) {
  auto &fs = mutable_floor(f);
  if (fs.events.use_count() != 1)
    fs.events = std::make_shared<Json>(*fs.events);
  fs.hash.invalidate();
  fs.events->erase(location(p));
  fs.event_entries.reset(p.index());
  if(!reference_model&&fs.retained_present[p.index()]) {
    auto b=fs.retained[p.index()];
    if(fs.retained_events.use_count()!=1)fs.retained_events=std::make_shared<RetainedEvents>(*fs.retained_events);
    fs.retained_events->set(p,b.entity?entity(b).native:Json{{"cls","terrains"},{"id","none"},{"noPass",false}});
    if(!b.entity)remove_block(f,p);
  }
}
void Game::set_event(int f, Pos p, const Json &j) {
  auto &fs = mutable_floor(f);
  if (fs.events.use_count() != 1)
    fs.events = std::make_shared<Json>(*fs.events);
  fs.hash.invalidate();
  (*fs.events)[location(p)] = j;
  fs.event_entries.set(p.index());
  if(!reference_model) {
    if(!fs.retained_present[p.index()]) {
      fs.retained_present.set(p.index());fs.retained[p.index()]=fs.blocks[p.index()];fs.block_order.push_back(p);
    }
    auto event=fs.retained_events->value(p,initialized_block_event(f,p,fs.retained[p.index()]));
    Json source=j;if(source.is_array())source=Json{{"data",source}};else if(source.is_string())source=Json{{"data",Json::array({source})}};
    if(source.is_object()) {
      if(!source.contains("data")||source["data"].is_null())source["data"]=Json::array();
      for(auto it=source.begin();it!=source.end();++it)
        if(it.key()!="enable"&&it.key()!="animate"&&it.key()!="opacity"&&it.key()!="filter"&&!it.value().is_null())event[it.key()]=it.value();
      if(source.contains("enable"))fs.disabled.set(p.index(),!truth(source["enable"]));
    }
    if(!truth(event.value("trigger",Json())))event["trigger"]="action";
    if(fs.retained_events.use_count()!=1)fs.retained_events=std::make_shared<RetainedEvents>(*fs.retained_events);
    fs.retained_events->set(p,std::move(event));sync_retained_cell(f,p);
  }
}
const Json *Game::native_flag_value(NativeFlag flag) const {
  const auto id=std::size_t(flag);const auto &name=native_flag_names.at(id);
  if(!flags->value.is_object())return nullptr;
  const auto &values=flags->value.get_ref<const Json::object_t&>();
  auto &slot=flags->native_slots.slots[id];const auto hint=slot.load(std::memory_order_relaxed);
  if(hint) {
    const auto index=hint-1;
    if(index<=values.size()) {
      if(index<values.size()) {
        const auto &entry=*(values.begin()+index);
        if(entry.first==name)return &entry.second;
        if(entry.first>name&&(index==0||(values.begin()+index-1)->first<name))return nullptr;
      } else if(index==0||(values.begin()+index-1)->first<name)return nullptr;
    }
  }
  const auto found=values.lower_bound(name);slot.store(std::size_t(found-values.begin())+1,std::memory_order_relaxed);
  return found==values.end()||found->first!=name?nullptr:&found->second;
}
Number Game::flag(NativeFlag name) const {const auto *value=native_flag_value(name);return value?number(*value):0;}
bool Game::flag_truth(NativeFlag name) const {const auto *value=native_flag_value(name);return value&&truth(*value);}
Number Game::flag(const std::string &s) const {
  return val(flags->value, s);
}
bool Game::flag_truth(const std::string &s) const {
  auto it = flags->value.find(s);
  return it != flags->value.end() && truth(*it);
}
namespace {
bool native_temporary_flag(const std::string &name) {
  return name.starts_with("@temp@")||(name.size()>3&&name.starts_with("arg")&&
    std::all_of(name.begin()+3,name.end(),[](unsigned char c){return std::isdigit(c);}));
}
}
void Game::set_flag(const std::string &s, Json j) {
  const auto existing=flags->value.find(s);
  if(!reference_model&&j.is_null()) {if(existing==flags->value.end())return;}
  else if(existing!=flags->value.end()&&same_json_value(*existing,j))return;
  if(native_temporary_flag(s))native_temporaries_possible=true;
  if(s=="魔法免疫"||s=="no_zone"||s=="no_repulse"||s=="no_laser"||s=="no_betweenAttack"||s=="no_ambush"||s=="__disabled__"||s.starts_with("RMT"))
    navigation_context=new_navigation_context();
  if (flags.use_count() != 1)
    flags = std::make_shared<JsonState>(*flags);
  if(!reference_model&&j.is_null())flags->value.erase(s);
  else flags->value[s] = std::move(j);
  flags->hash.invalidate();
}
void Game::set_flag_member(const std::string &name,const std::string &key,Json value) {
  const auto existing=flags->value.find(name);
  if(existing!=flags->value.end()) {
    if(!existing->is_object()&&!existing->is_null()) {
      auto copy=*existing;copy[key]=std::move(value);set_flag(name,std::move(copy));return;
    }
    const auto member=existing->find(key);
    if(member!=existing->end()&&same_json_value(*member,value))return;
  }
  if(name=="魔法免疫"||name=="no_zone"||name=="no_repulse"||name=="no_laser"||name=="no_betweenAttack"||name=="no_ambush"||name=="__disabled__"||name.starts_with("RMT"))
    navigation_context=new_navigation_context();
  if(native_temporary_flag(name))native_temporaries_possible=true;
  if(flags.use_count()!=1)flags=std::make_shared<JsonState>(*flags);
  flags->value[name][key]=std::move(value);flags->hash.invalidate();
}
void Game::set_extra(const std::string &s, Json j) {
  const auto existing=extra_state->value.find(s);
  const bool deferred_native=deferred_native_serialization_&&native_continuation&&s=="native_event_state";
  if(deferred_native?same_json_value(native_event_state_json(*this),j):(existing!=extra_state->value.end()&&same_json_value(*existing,j)))return;
  if(s=="native_event_state")native_continuation.reset();
  if(s=="enemy_overrides"||s=="game_flags")navigation_context=new_navigation_context();
  if (extra_state.use_count() != 1)
    extra_state = std::make_shared<JsonState>(*extra_state);
  extra_state->value[s] = std::move(j);
  extra_state->hash.invalidate();
}
namespace {
Stat inventory_stat(const std::string &id) {
  if(id=="yellowKey")return yk;
  if(id=="blueKey")return bk;
  if(id=="redKey")return rk;
  for(Stat s:{centerFly,upFly,downFly,earthquake,bomb,bigKey,pickaxe,superPotion,snow,knife,coin,cross})
    if(id==stat_names[s])return s;
  return stat_count;
}
std::string inventory_id(Stat s) {
  return s==yk?"yellowKey":s==bk?"blueKey":s==rk?"redKey":stat_names[s];
}
std::string item_class(const World &w,const std::string &id) {
  const auto &all=object_member(w.native_data,"items");
  if(all.contains(id))return all[id].value("cls",std::string("tools"));
  if(id=="snow"||id=="knife"||id=="coin"||id=="cross"||id=="book"||id=="fly")return "constants";
  return "tools";
}
Pos direction_delta(const std::string &direction) {
  if(direction=="up")return {0,-1};
  if(direction=="left")return {-1,0};
  if(direction=="down")return {0,1};
  if(direction=="right")return {1,0};
  throw RouteError("Invalid direction: "+direction);
}
std::string direction_name(Pos d) {
  return d.x==1?"right":d.x==-1?"left":d.y==1?"down":"up";
}
bool has_string(const Json &j,const std::string &s) {
  return j.is_array()&&std::find(j.begin(),j.end(),Json(s))!=j.end();
}
}
Json Game::inventory_json() const {
  auto out=items->value;
  for(const auto &c:{"tools","constants","equips"})
    if(!out.contains(c))out[c]=Json::object();
  for(Stat s:{yk,bk,rk,centerFly,upFly,downFly,earthquake,bomb,bigKey,pickaxe,superPotion,snow,knife,coin,cross}) {
    auto id=inventory_id(s), cls=item_class(*world,id);
    if(hero[s])out[cls][id]=hero[s];else out[cls].erase(id);
  }
  return out;
}
Number Game::item_count(const std::string &id) const {
  auto s=inventory_stat(id);
  if(s!=stat_count)return hero[s];
  return val(object_member(items->value,item_class(*world,id)),id);
}
void Game::add_inventory(const std::string &id,Number amount) {
  auto count=std::max(Number(0),item_count(id)+amount);
  if(!reference_model&&item_class(*world,id)=="constants")count=std::min(Number(1),count);
  set_inventory(id,count);
}
void Game::set_inventory(const std::string &id,Number count) {
  if(!reference_model&&item_class(*world,id)=="items")return;
  auto s=inventory_stat(id);
  if(s!=stat_count){hero[s]=std::max(Number(0),count);return;}
  if(items.use_count()!=1)items=std::make_shared<JsonState>(*items);
  auto cls=item_class(*world,id);auto &section=items->value[cls];
  if(!section.is_object())section=Json::object();
  Number n=std::max(Number(0),count);
  if(n)section[id]=n;else section.erase(id);
  items->hash.invalidate();
}
int Game::foreground(int f,Pos p) const {
  return f>=world->first_floor&&f<=50&&p.valid()?floors[f]->foreground[p.index()]:0;
}
void Game::set_foreground(int f,Pos p,int tile) {
  if(!p.valid())return; // Native setBgFgBlock silently ignores out-of-bounds cells.
  auto &fs=mutable_floor(f);auto i=p.index();
  fs.hash.invalidate();
  fs.foreground[i]=std::int16_t(tile);
  refresh_movement_cell(fs,f,p);
  if(!reference_model) {
    auto values=flags->value.value("__fgv__",Json::object());
    auto rows=values.value(floor_id(f),Json::array());Json next=Json::array();
    for(const auto &row:rows)if(json_pos(row.at(0),row.at(1))!=p)next.push_back(row);
    next.push_back({p.x,p.y,tile});values[floor_id(f)]=next;set_flag("__fgv__",values);
  }
}
void Game::set_background(int f,Pos p,int tile) {
  if(!p.valid())return;
  auto &fs=mutable_floor(f);auto i=p.index();
  fs.hash.invalidate();
  fs.background[i]=std::int16_t(tile);
  refresh_movement_cell(fs,f,p);
  if(!reference_model) {
    auto values=flags->value.value("__bgv__",Json::object());Json next=Json::array();
    for(const auto &row:values.value(floor_id(f),Json::array()))if(json_pos(row.at(0),row.at(1))!=p)next.push_back(row);
    next.push_back({p.x,p.y,tile});values[floor_id(f)]=next;set_flag("__bgv__",values);
  }
}
void Game::set_enemy(const std::string &id,const std::string &name,Json value) {
  auto overrides=extra_state->value.value("enemy_overrides",Json::object());
  overrides[id][name]=std::move(value);set_extra("enemy_overrides",std::move(overrides));
  auto info=flags->value.value("enemyInfo",Json::object());
  info[id][name]=extra_state->value["enemy_overrides"][id][name];set_flag("enemyInfo",info);
  // Native hazard queries apply these overrides directly. Floor hashes and
  // spell lists describe their unchanged blocks and immutable base entities.
}
void Game::set_deferred_native_serialization(bool enabled) {
  if(!enabled&&deferred_native_serialization_&&native_continuation) {
    auto value=native_event_state_json(*this);const auto owner=native_continuation;
    deferred_native_serialization_=false;set_extra("native_event_state",std::move(value));native_continuation=owner;
  }
  deferred_native_serialization_=enabled;
}
Json Game::extra_json() const {
  auto result=extra_state->value;
  if(deferred_native_serialization_&&native_continuation)result["native_event_state"]=native_event_state_json(*this);
  return result;
}
Json Game::state_json() const {
  Json j = extra_json();
  for (std::size_t i = 0; i < stat_count; ++i) {
    if (i == mdef && hero.values[i] == 0 && !mdef_present)
      continue;
    bool b = i == knife || i == coin || i == cross || i == magicImmune || i == won49 || i == won50;
    j[stat_names[i]] = b ? Json(bool(hero.values[i])) : Json(hero.values[i]);
  }
  j["floor"] = floor_id(hero.floor);
  j["x"] = hero.pos.x;
  j["y"] = hero.pos.y;
  return j;
}
Json Game::export_json(bool maps) const {
  Json j = {{"model_mode", reference_model ? "python-reference" : "native-corrected"},
            {"state", state_json()},
            {"flags", flags->value},
            {"auto_flags", auto_json(automatic)},
            {"strict_live_model", strict},
            {"visited", Json::array()}};
  if(!reference_model) {
    j["items"]=inventory_json();
    j["values"]=extra_state->value.value("values",world->native_data.value("values",Json::object()));
    j["game_flags"]=extra_state->value.value("game_flags",world->native_data.value("flags",Json::object()));
    j["enemies"]=world->native_data.value("enemies",Json::object());
    auto overrides=extra_state->value.value("enemy_overrides",Json::object());
    for(auto i=overrides.begin();i!=overrides.end();++i)for(auto v=i.value().begin();v!=i.value().end();++v)j["enemies"][i.key()][v.key()]=v.value();
    j["hazard_damage"]=Json::object();
    for(int i=0;i<cells;++i) {
      Pos p{i%side,i/side};
      auto damage=map_damage(hero.floor,p);
      if(damage)j["hazard_damage"][location(p)]=damage;
    }
    auto reactions=hazard_reactions(hero.floor);
    j["hazard_repulse"]=reactions["repulse"];
    j["hazard_ambush"]=reactions["ambush"];
  }
  for (int f = world->first_floor; f <= 50; ++f)
    if (visited[f])
      j["visited"].push_back(floor_id(f));
  for (auto [name, set] : std::initializer_list<std::pair<std::string, const std::set<int> *>>{
           {"blocked_items", &blocked_items},
           {"retreat_wizards", &retreat_wizards},
           {"no_auto_farm", &no_auto_farm}}) {
    j[name] = Json::array();
    for (int m : *set)
      j[name].push_back({floor_id(m / cells), (m % cells) % side, (m % cells) / side});
  }
  j["pending"] = pending_records;
  j["pending"]["_defer_after_action"] = defer_after;
  j["pending"]["_capturing"] = capturing;
  j["pending"]["_captured"] = Json::array();
  j["pending"]["_deferred_after_effects"] = Json::array();
  for (auto [name, v] : std::initializer_list<std::pair<std::string, const std::vector<Effect> *>>{
           {"_captured", &captured}, {"_deferred_after_effects", &deferred}})
    for (auto &e : *v)
      if (e.block.entity)
        j["pending"][name].push_back({"set_block",
                                      floor_id(e.floor),
                                      {e.pos.x, e.pos.y},
                                      kind_name(entity(e.block).kind),
                                      entity(e.block).id,
                                      e.block.tile});
      else
        j["pending"][name].push_back({"set_ground", floor_id(e.floor), {e.pos.x, e.pos.y}});
  if (maps) {
    j["floors"] = Json::object();
    for (int f = world->first_floor; f <= 50; ++f) {
      if (!present[f])
        continue;
      auto &out = j["floors"][floor_id(f)];
      out["events"] = *floors[f]->events;
      out["blocks"] = Json::array();
      out["grid"] = Json::array();
      if(!reference_model) {
        out["fgmap"]=Json::array();
        out["bgmap"]=Json::array();
        out["block_order"]=Json::array();
        out["retained_block_order"]=Json::array();out["retained_blocks"]=Json::object();
        for(auto p:floors[f]->block_order) {
          if(!block_present(f,p,true))continue;
          out["retained_block_order"].push_back({p.x,p.y});
          out["retained_blocks"][location(p)]={{"id",retained_block(f,p).tile},
            {"disable",block_disabled(f,p)},{"event",retained_block_event(f,p)}};
        }
        out["active_block_properties"]=Json::object();
        for(int i=0;i<cells;++i) {
          Pos p{i%side,i/side};
          if(!block_present(f,p))continue;
          auto data=effective_block_event(f,p);Json properties=Json::object();
          for(const auto &key:{"id","cls","noPass","trigger","cannotIn","cannotOut","canBreak","doorInfo","script","event","outEvent","faceIds"})
            if(data.contains(key))properties[key]=data[key];
          out["active_block_properties"][location(p)]=std::move(properties);
        }
        for(auto p:floors[f]->block_order)if(block(f,p).entity)out["block_order"].push_back({p.x,p.y});
        for(const auto &key:{"canFlyFrom","canFlyTo","canUseQuickShop","isHide","cannotMoveDirectly","upFloor","downFloor","cannotMove","cannotMoveIn","firstArrive","eachArrive","afterBattle","afterGetItem","afterOpenDoor","beforeBattle","autoEvent"})
          if(world->floors[f].metadata.contains(key))out[key]=world->floors[f].metadata.at(key);
      }
      out["spell_enemies"] = Json::array();
      out["ratio"] = world->floors[f].ratio;
      out["change_floor"] = world->floors[f].change_floor;
      if(!reference_model) {
        out["source_change_floor"]=world->floors[f].change_floor;
        out["change_floor"]=Json::object();
        for(auto p:floors[f]->block_order)if(block_present(f,p)) {
          auto event=retained_block_event(f,p);
          if(event.value("trigger",Json())=="changeFloor")out["change_floor"][location(p)]=event.value("data",Json::object());
        }
      }
      out["after_battle"] = Json::array();
      out["after_battle_auto"] = Json::array();
      for (int z = 0; z < cells; ++z) {
        Pos p{z % side, z / side};
        if (world->floors[f].after_battle[z])
          out["after_battle"].push_back(location(p));
        if (world->floors[f].after_battle_auto[z])
          out["after_battle_auto"].push_back(location(p));
      }
      std::sort(out["after_battle"].begin(), out["after_battle"].end());
      std::sort(out["after_battle_auto"].begin(), out["after_battle_auto"].end());
      for (auto p : floors[f]->spell_enemies)
        out["spell_enemies"].push_back({p.x, p.y});
      for (int y = 0; y < side; ++y) {
        Json row = Json::array();
        for (int x = 0; x < side; ++x) {
          auto b = block(f, {x, y});
          auto &e = entity(b);
          row.push_back(floors[f]->grid[y * side + x]);
          if (b.entity)
            out["blocks"].push_back({x, y, kind_name(e.kind), e.id, b.tile});
        }
        out["grid"].push_back(row);
        if(!reference_model) {
          Json fg=Json::array();
          for(int x=0;x<side;++x)fg.push_back(foreground(f,{x,y}));
          out["fgmap"].push_back(fg);
          Json bg=Json::array();for(int x=0;x<side;++x)bg.push_back(floors[f]->background[y*side+x]);out["bgmap"].push_back(bg);
        }
      }
    }
  }
  return j;
}
std::uint64_t Game::hash() const {
  std::uint64_t h = flags->hash.get([&]{return hash_json(flags->value);});
  combine(h, deferred_native_serialization_&&native_continuation?hash_json(extra_json()):extra_state->hash.get([&]{return hash_json(extra_state->value);}));
  combine(h, items->hash.get([&]{return hash_json(items->value);}));
  for (auto x : hero.values)
    combine(h, std::uint64_t(x));
  combine(h, hero.floor);
  combine(h, hero.pos.index());
  combine(h, visited.to_ullong());
  combine(h, present.to_ullong());
  combine(h, reference_model);
  combine(h, mdef_present);
  combine(h, hash_json(pending_records));
  combine(h, std::uint64_t(automatic.battle | (automatic.wall << 1) | (automatic.item << 2) |
                           (strict << 4) | (defer_after << 5) | (capturing << 6) |
                           (pending_auto << 7)));
  for (int f = world->first_floor; f <= 50; ++f) {
    combine(h, floor_hash(f));
    for (auto p : floors[f]->spell_enemies)
      combine(h, p.index());
    if(!reference_model)for(auto p:floors[f]->block_order)combine(h,p.index());
  }
  for (auto set : {&blocked_items, &retreat_wizards, &no_auto_farm}) {
    combine(h, set->size());
    for (int x : *set)
      combine(h, x);
  }
  for (auto v : {&captured, &deferred}) {
    combine(h, v->size());
    for (auto &e : *v) {
      combine(h, e.floor);
      combine(h, cell_hash(e.pos.index(), e.block));
    }
  }
  return h;
}
bool Game::operator==(const Game &g) const {
  if (world != g.world || present != g.present || reference_model != g.reference_model ||
      mdef_present != g.mdef_present || pending_records != g.pending_records || hero != g.hero ||
      strict != g.strict || automatic != g.automatic || visited != g.visited ||
      flags->value != g.flags->value || ((deferred_native_serialization_||g.deferred_native_serialization_)?extra_json()!=g.extra_json():extra_state->value!=g.extra_state->value) || items->value != g.items->value ||
      blocked_items != g.blocked_items || retreat_wizards != g.retreat_wizards ||
      no_auto_farm != g.no_auto_farm || defer_after != g.defer_after || capturing != g.capturing ||
      pending_auto != g.pending_auto || captured != g.captured || deferred != g.deferred)
    return false;
  for (int f = world->first_floor; f <= 50; ++f)
    if ((floors[f] != g.floors[f] && *floors[f] != *g.floors[f]) ||
        (!reference_model&&floors[f]->block_order!=g.floors[f]->block_order))
      return false;
  return true;
}
Number Game::enemy_damage(const std::string &id, std::int64_t override) const {
  return enemy_damage(world->make_block(id), override);
}
Entity Game::effective_enemy(Block b) const {
  auto e = entity(b);
  if(!reference_model) {
    const auto &overrides=object_member(extra_state->value,"enemy_overrides");
    if(overrides.contains(e.id)) {
      const auto &v=overrides[e.id];e.hp=val(v,"hp",e.hp);e.atk=val(v,"atk",e.atk);e.def=val(v,"def",e.def);
      e.money=val(v,"money",e.money);e.n=val(v,"n",e.n);
      e.def_value=val(v,"defValue",e.def_value);e.atk_value=val(v,"atkValue",e.atk_value);
      e.value=val(v,"value",e.value);e.range=int(val(v,"range",e.range));
      e.square=truth(v.value("zoneSquare",Json(e.square)));
      if(v.contains("special"))e.special=specials(v["special"]);
    }
  }
  return e;
}
Number Game::enemy_damage(Block b, std::int64_t override) const {
  auto e = combat_stats(*this,b);
  if(e.kind!=Kind::enemy)throw RouteError("Not an enemy: "+e.id);
  if (reference_model && e.id == "redKing" && flag_truth(NativeFlag::flag_6)) {
    e.hp = 5000;
    e.atk = 1580;
    e.def = 190;
  } else if (reference_model && e.id == "redKing" && flag_truth(NativeFlag::flag_5)) {
    if (flag_truth(NativeFlag::flag_7)) {
      e.hp = 500;
      e.atk = 158;
      e.def = 19;
    } else {
      e.hp = 800;
      e.atk = 500;
      e.def = 100;
    }
  }
  Number attack = hero[atk];
  if (hero[cross] && (e.id == "zombie" || e.id == "zombieKnight" || e.id == "vampire"))
    attack *= 2;
  if (hero[knife] && e.id == "magicDragon")
    attack *= 2;
  Number hit = attack - e.def;
  if (hit <= 0)
    return infinite_damage;
  Number turns = (e.hp + hit - 1) / hit, per = std::max(Number(0), e.atk - hero[defense]);
  auto s = override < 0 ? e.special : std::uint64_t(override);
  auto has = [&](int n) { return (s & (1ULL << n)) != 0; };
  if (has(2))
    per = e.atk;
  if (has(4))
    per *= 2;
  if (has(5))
    per *= 3;
  if (has(6))
    per *= e.n;
  Number initial = 0;
  if (has(1))
    initial += per;
  if (has(7))
    initial += e.def_value * hero[defense];
  if (has(9))
    initial += e.n * hero[mdef];
  Number counter = has(8) ? e.atk_value * attack : 0;
  return initial + (turns - 1) * per + turns * counter;
}
std::shared_ptr<const HazardCache> Game::native_hazards(int f) const {
  auto &handle=floors[f]->hazards;
  auto cache=handle.load();
  if(cache&&cache->world==world&&cache->context==navigation_context)return cache;
  auto next=std::make_shared<HazardCache>();next->world=world;next->context=navigation_context;
  next->immune=flag_truth(NativeFlag::magic_immune);next->zone=!flag_truth(NativeFlag::no_zone);next->repulse=!flag_truth(NativeFlag::no_repulse);
  next->laser=!flag_truth(NativeFlag::no_laser);next->between=!flag_truth(NativeFlag::no_betweenAttack);next->ambush=!flag_truth(NativeFlag::no_ambush)&&!next->immune;
  const auto &all_overrides=object_member(extra_state->value,"enemy_overrides");
  for(auto p:floors[f]->block_order) {
    const auto b=block(f,p);const auto &e=entity(b);if(e.kind!=Kind::enemy)continue;
    const auto &o=object_member(all_overrides,e.id);
    const auto special=o.contains("special")?specials(o["special"]):e.special;
    if(!(special&((1ULL<<15)|(1ULL<<16)|(1ULL<<18)|(1ULL<<24)|(1ULL<<27))))continue;
    int range=int(val(o,"range",e.range));if(!range)range=1;
    next->sources.push_back({p,b.entity,special,val(o,"value",e.value),range,truth(o.value("zoneSquare",Json(e.square)))});
  }
  // Only the first matching pincer applies, after the hazards that precede it
  // in retained block order. Split its HP-dependent term from the constants.
  if(!next->immune)for(const auto &s:next->sources)for(int i=0;i<cells;++i) {
    const Pos p{i%side,i/side},q=s.pos;
    auto &total=next->pincer[i]?next->after[i]:next->before[i];
    const int d=s.square?std::max(std::abs(q.x-p.x),std::abs(q.y-p.y)):distance(q,p);
    if((s.special&(1ULL<<15))&&next->zone&&q!=p&&d<=s.range)total+=s.value;
    if((s.special&(1ULL<<18))&&next->repulse&&q!=p&&d==1)total+=s.value;
    if((s.special&(1ULL<<24))&&next->laser&&q!=p&&(q.x==p.x||q.y==p.y))total+=s.value;
    if(!next->pincer[i]&&next->between&&(s.special&(1ULL<<16))&&distance(q,p)==1&&q.x<=p.x&&q.y<=p.y&&
       block(f,{2*p.x-q.x,2*p.y-q.y}).entity==s.entity)next->pincer.set(i);
  }
  handle.store(next);return next;
}
Number Game::map_damage(int f, Pos p) const {
  require(f >= world->first_floor && f <= 50 && p.valid(), "Invalid map-damage query");
  if(!reference_model) {
    return cached_map_damage(*native_hazards(f),hero[hp],p);
  }
  if(hero[magicImmune]!=0)return 0;
  Number total=0;const auto &fs=*floors[f];
  for (Pos q : fs.spell_enemies) {
    if(!reference_model&&flag_truth(NativeFlag::no_zone))break;
    auto &e = entity(fs.blocks[q.index()]);
    if (e.kind != Kind::enemy || q == p)
      continue;
    int d = e.square ? std::max(std::abs(q.x - p.x), std::abs(q.y - p.y)) : distance(q, p);
    if (d <= e.range)
      total += e.value;
  }
  for (auto d : std::array<Pos, 2>{Pos{1, 0}, Pos{0, 1}}) {
    if(!reference_model&&flag_truth(NativeFlag::no_betweenAttack))break;
    auto a = block(f, {p.x - d.x, p.y - d.y}), b = block(f, add(p, d));
    if (a.entity && a.entity == b.entity && entity(a).kind == Kind::enemy &&
        (entity(a).special & (1ULL << 16))) {
      if(reference_model)total+=hero[hp]/2;
      else {if(hero[hp]-total>1)total+=(hero[hp]-total)/2;break;}
    }
  }
  return total;
}
void Game::apply_map_damage(int f, Pos p) {
  auto d = map_damage(f, p);
  if (d <= 0)
    return;
  hero[hp] -= d;
  hero[dmg] += d;
  require(hero[hp] > 0, "Map damage death");
}
bool Game::native_can_move(int f,Pos from,Pos to) const {
  if(!from.valid()||!to.valid()||distance(from,to)!=1)return false;
  const int direction=to.y<from.y?0:to.y>from.y?1:to.x<from.x?2:3;
  const auto bit=std::uint8_t(1U<<direction),back_bit=std::uint8_t(1U<<(direction^1));
  const auto &fs=*floors[f];
  if(fs.movement_source&&fs.movement_source->matches(world))
    return !(fs.movement_out[from.index()]&bit)&&!(fs.movement_in[to.index()]&back_bit);
  std::shared_ptr<const PreparedNativeWorld> fallback;
  const auto *prepared=prepared_native.get();
  if(!prepared||!prepared->matches(world)){fallback=prepare_native_world(world);prepared=fallback.get();}
  if((prepared->floor_out[f][from.index()]&bit)||(prepared->floor_in[f][to.index()]&back_bit))return false;
  if((prepared->entities[block(f,from).entity].out&bit)||(prepared->entities[block(f,to).entity].in&back_bit))return false;
  for(const auto *layer:{&floors[f]->background,&floors[f]->foreground}) {
    for(bool target:{false,true}) {
      const auto tile=(*layer)[(target?to:from).index()];
      if(tile>=0&&std::size_t(tile)<prepared->tiles.size()) {
        if(target?(prepared->tiles[tile].in&back_bit):(prepared->tiles[tile].out&bit))return false;
      } else {
        const auto source=world->tiles.find(tile);if(source==world->tiles.end())continue;
        const auto &native=entity(source->second).native;
        if(has_string(object_member(native,target?"cannotIn":"cannotOut"),direction_name(target?Pos{from.x-to.x,from.y-to.y}:Pos{to.x-from.x,to.y-from.y})))return false;
      }
    }
  }
  return true;
}
std::shared_ptr<const HazardReactions> Game::native_hazard_reactions(int f) const {
  require(f>=world->first_floor&&f<=50,"Invalid hazard-reaction floor");
  const auto cache=native_hazards(f);auto cached=cache->reactions.load();if(cached)return cached;
  auto out=std::make_shared<HazardReactions>();
  const std::array<Pos,8> directions={Pos{0,-1},Pos{-1,0},Pos{0,1},Pos{1,0},Pos{-1,-1},Pos{-1,1},Pos{1,-1},Pos{1,1}};
  for(const auto &source:cache->sources) {
    const auto p=source.pos;const auto special=source.special;
    if(!(special&((1ULL<<18)|(1ULL<<27))))continue;
    for(std::size_t i=0;i<(source.square?8u:4u);++i) {
      auto d=directions[i],to=add(p,d);if(!to.valid())continue;
      if((special&(1ULL<<18))&&cache->repulse) {
        auto target=Pos{p.x-d.x,p.y-d.y};
        if(target.valid()&&!block(f,target).entity&&!event_enabled(f,target)&&(i>=4||native_can_move(f,p,target))) {
          out->affected.set(to.index());out->entries.push_back({true,to,p,{-d.x,-d.y},source.entity});
        }
      }
      if((special&(1ULL<<27))&&cache->ambush&&(i>=4||native_can_move(f,p,to))) {
        out->affected.set(to.index());out->entries.push_back({false,to,p,d,source.entity});
      }
    }
  }
  cache->reactions.store(out);return out;
}
Json Game::hazard_reactions(int f) const {
  Json out={{"repulse",Json::object()},{"ambush",Json::object()}};
  const auto reactions=native_hazard_reactions(f);
  for(const auto &entry:reactions->entries) {
    const auto d=entry.direction;
    const auto name=!d.x||!d.y?direction_name(d):std::string(d.x<0?"left":"right")+(d.y<0?"up":"down");
    auto &list=out[entry.repulse?"repulse":"ambush"][location(entry.cell)];if(list.is_null())list=Json::array();
    list.push_back({entry.source.x,entry.source.y,world->entities[entry.entity].id,name});
  }
  return out;
}
bool Game::event_enabled(int f,Pos p) const {
  if(!p.valid()||!floors[f]->event_entries[p.index()])return false;
  const auto &events=*floors[f]->events;
  if(reference_model)return active_event(events,p);
  const auto &flags_here=object_member(object_member(flags->value,"__disabled__"),floor_id(f));
  if(flags_here.is_array()&&flags_here.size()==2) {
    if(std::find(flags_here[0].begin(),flags_here[0].end(),Json(p.index()))!=flags_here[0].end())return false;
    if(std::find(flags_here[1].begin(),flags_here[1].end(),Json(p.index()))!=flags_here[1].end())return true;
  }
  return active_event(events,p);
}
Json Game::effective_block_event(int f,Pos p) const {
  if(!reference_model) {
    if(block_present(f,p))return retained_block_event(f,p);
    return Json{{"cls","terrains"},{"id","none"},{"noPass",false}};
  }
  auto b=block(f,p);
  Json out=b.entity?entity(b).native:Json{{"cls","terrains"},{"id","none"},{"noPass",false}};
  out.erase("canPass");
  if(entity(b).kind==Kind::enemy) {
    const auto &source=object_member(object_member(world->native_data,"enemies"),entity(b).id);
    if(source.contains("faceIds")&&!source["faceIds"].is_null())out["faceIds"]=source["faceIds"];
  }
  auto merge=[&](Json event) {
    if(event.is_null())return;
    if(event.is_string())event=Json{{"data",Json::array({event})}};
    else if(event.is_array())event=Json{{"data",event}};
    if(!event.is_object())return;
    if(!event.contains("data")||event["data"].is_null())event["data"]=Json::array();
    for(auto it=event.begin();it!=event.end();++it)
      if(it.key()!="enable"&&it.key()!="animate"&&it.key()!="opacity"&&it.key()!="filter"&&!it.value().is_null())out[it.key()]=it.value();
    if(!out.contains("trigger")||!truth(out["trigger"]))out["trigger"]="action";
  };
  if(f>=world->first_floor&&f<=50&&p.valid()) {
    if(!flag_truth("R"+floor_id(f)+","+location(p)))
      merge(floors[f]->events->value(location(p),Json()));
    if(world->floors[f].change_floor.contains(location(p)))
      merge(Json{{"trigger","changeFloor"},{"data",world->floors[f].change_floor.at(location(p))}});
  }
  return out;
}
bool Game::direct_movement_enabled() const {
  const auto &globals=extra_state->value.contains("game_flags")?extra_state->value["game_flags"]:object_member(world->native_data,"flags");
  return hero[hp]>0 && truth(globals.value("enableMoveDirectly",Json(true))) &&
    !flag_truth(NativeFlag::cannotMoveDirectly) && !(flag_truth(NativeFlag::feature)&&flag(NativeFlag::energy)>0) &&
    !truth(world->floors[hero.floor].metadata.value("cannotMoveDirectly",Json(false)));
}
bool Game::dialogue_can_delete(int f,Pos p) const {
  if(reference_model||!flag_truth(NativeFlag::feature)||flag(NativeFlag::energy)<=0)return false;
  auto b=block(f,p);if(!b.entity)return false;
  const auto &e=entity(b);
  const auto &native=native_block_event_view(f,p);
  if(e.kind==Kind::enemy)return enemy_damage(b)>=infinite_damage || enemy_damage(b)+map_damage(f,p)>=hero[hp];
  bool event=event_enabled(f,p);
  bool trigger=native.contains("trigger")&&!native["trigger"].is_null()&&truth(native["trigger"]);
  if(!event&&!trigger)return true;
  auto keys=native.value("doorInfo",Json::object()).value("keys",Json::object());
  for(auto it=keys.begin();it!=keys.end();++it) {
    std::string key=it.key();if(key.ends_with(":o"))key.resize(key.size()-2);
    const auto &defs=object_member(world->native_data,"items");
    if(!defs.contains(key)||item_count(key)<number(it.value()))return true;
  }
  return false;
}
void Game::consume_tool_energy() { if(!reference_model)set_flag("对话能量",flag(NativeFlag::energy)-1); }
void Game::successful_move(int f,Pos from,Pos to) {
  hero.pos=to;
  set_extra("direction",direction_name({to.x-from.x,to.y-from.y}));
  const auto &previous=native_block_event_view(f,from);
  if(previous.contains("outEvent")&&!previous["outEvent"].is_null()) {
    const auto event=previous["outEvent"];native_event_start(event,f,from);
  }
  set_flag("对话能量",flag(NativeFlag::energy)-1);
  apply_map_damage(f,to);
}
bool Game::trigger_native_event(int f,Pos p) {
  if(!block_present(f,p))return false;
  const auto &event=native_block_event_view(f,p);
  if(event.value("trigger",Json())=="changeFloor") {
    auto a=event.value("data",Json::object());auto id=a.value("floorId",std::string{});
    if(id==":next"||id==":before") {
      int delta=id==":next"?1:-1,target=f+delta;
      while(target>=world->first_floor&&target<=50&&truth(world->floors[target].metadata.value("isHide",Json(false))))target+=delta;
      if(target<world->first_floor||target>50)target=f;
      a["floorId"]=floor_id(target);a["type"]="changeFloor";
      native_event_start(Json::array({a}),f,p);return true;
    }
  }
  return native_event_trigger(f,p);
}
void Game::step(const std::string &direction, bool allow_inert_bump) {
  require(!reference_model,"Directional steps require native-corrected mode");
  auto delta=direction_delta(direction);set_extra("direction",direction);
  int f=hero.floor;auto from=hero.pos;auto to=add(from,delta);
  require(hero[hp]>0,"Cannot move at zero HP");
  require(to.x>=1&&to.y>=1&&to.x<=11&&to.y<=11&&native_can_move(f,from,to),"Movement blocked");
  auto b=block(f,to);const auto &e=entity(b);
  const auto &native=native_block_event_view(f,to);
  auto event_serial=extra_state->value.value("native_event_serial",Number(0));
  bool no_pass=(b.entity||event_enabled(f,to))&&truth(native.value("noPass",Json(true)));
  if(e.kind==Kind::enemy)no_pass=enemy_damage(b)>=infinite_damage || enemy_damage(b)+map_damage(f,to)>=hero[hp];
  require(map_damage(f,to)<std::max(Number(1),hero[hp]),"Cannot enter lethal hazard");
  if(!no_pass||dialogue_can_delete(f,to)) {
    if(native_hazard_reactions(f)->affected[to.index()])
      throw Unsupported("Directional repulse/ambush actions are outside the pinned tower mechanics");
  }
  if(no_pass) {
    if(dialogue_can_delete(f,to)) {
      set_flag("对话能量",0);ground(f,to);erase_event(f,to);
      successful_move(f,from,to);trigger_tile_event(f,to);auto_sweep(f);return;
    }
    if(e.kind==Kind::enemy)throw RouteError("Cannot win target battle");
    if(trigger_native_event(f,to))return;
    if(e.kind==Kind::door || e.id=="fakeWall" || e.id=="fakeWall2") {
      auto keys=native.value("doorInfo",Json::object()).value("keys",Json::object());
      require(native.contains("doorInfo"),"Block has no open-door interaction");
      for(auto it=keys.begin();it!=keys.end();++it) {
        auto key=it.key();if(key.ends_with(":o"))key.resize(key.size()-2);
        // Native openDoor resumes replay after an unsuccessful key check.
        // The recorded direction changes facing but consumes no inventory.
        if(allow_inert_bump && item_count(key)<number(it.value()))return;
        require(item_count(key)>=number(it.value()),"Missing door key");
      }
      for(auto it=keys.begin();it!=keys.end();++it)if(!it.key().ends_with(":o"))add_inventory(it.key(),-number(it.value()));
      if(e.id=="yellowDoor")++hero[yd];if(e.id=="blueDoor")++hero[bd];if(e.id=="redDoor")++hero[rd];
      ground(f,to);native_event_after("afterOpenDoor",f,to);return;
    }
    // control.js _moveAction_noPass records ordinary wall bumps and changes
    // facing without moving. Strict planner steps still require progress.
    if(allow_inert_bump && e.kind==Kind::wall &&
       !truth(native.value("trigger",Json())))return;
    throw RouteError("Obstacle cannot be entered");
  }
  successful_move(f,from,to);
  bool started=trigger_native_event(f,to);
  if(!started&&b.entity) {
    if(e.kind==Kind::item||e.kind==Kind::enemy)apply_block(f,to,b);
    else if(e.id!="none"&&e.id!="airwall"&&e.id!="fire"&&e.id!="flower") {
      // Harmless native terrain may carry no trigger.
      require(!truth(e.native.value("trigger",Json())),"Unimplemented terrain trigger: "+e.id);
    }
  }
  if(hero.floor==f&&hero.pos==to)trigger_tile_event(f,to);
  if(event_serial==extra_state->value.value("native_event_serial",Number(0)))auto_sweep(hero.floor);
}
void Game::talk_npc(int f,Pos p,int choice) {
  require(f==hero.floor&&distance(hero.pos,p)==1,"NPC interaction requires an adjacent hero");
  require(entity(block(f,p)).kind==Kind::npc,"No NPC at interaction target");
  require(trigger_native_event(f,p),"NPC has no supported source interaction");
  if(choice>=0&&native_events_pending(*this))native_choose(choice);
}
bool Game::is_wall(int f, Pos p) const {
  if (!p.valid() || f < world->first_floor || f > 50)
    return true;
  auto &e = entity(block(f, p));
  return wall_ids.contains(e.id) || e.kind == Kind::npc ||
         (e.kind == Kind::door && (e.id == "specialDoor" || e.id == "steelDoor"));
}
bool Game::can_enter(int f, Pos p, Pos target, bool click) const {
  if(!reference_model) {
    if(!p.valid()||p.x<1||p.x>11||p.y<1||p.y>11)return false;
    if(p==target)return true;
    auto b=block(f,p);auto &e=entity(b);
    if(foreground(f,p)==103)return false;
    if((b.entity||event_enabled(f,p))&&truth(effective_block_event(f,p).value("noPass",Json(false)))) {
      if(e.kind!=Kind::enemy&&e.kind!=Kind::door)return false;
    }
    if(!b.entity)return true;
    if(e.kind==Kind::enemy)return !click&&enemy_damage(b)<hero[hp];
    if(e.kind==Kind::door)return !click&&((e.id=="yellowDoor"&&hero[yk])||(e.id=="blueDoor"&&hero[bk])||(e.id=="redDoor"&&hero[rk]));
    if(truth(effective_block_event(f,p).value("noPass",Json(true))))return false;
    if(effective_block_event(f,p).value("trigger",Json())=="changeFloor")return false;
    return true;
  }
  if (is_wall(f, p))
    return false;
  auto b = block(f, p);
  if (!b.entity)
    return true;
  auto &e = entity(b);
  if (click && p != target)
    return e.kind == Kind::item && !blocked_items.contains(marker(f, p));
  if (e.kind == Kind::terrain) {
    if (e.id == "fakeWall" || e.id == "fakeWall2" || e.id == "upFloor" || e.id == "downFloor")
      return p == target;
    return e.id == "flower";
  }
  if (e.kind == Kind::item)
    return !blocked_items.contains(marker(f, p));
  if (e.kind == Kind::enemy) {
    if (retreat_wizards.contains(marker(f, p)) && p != target)
      return false;
    auto d = enemy_damage(b);
    return d < infinite_damage && hero[hp] > d;
  }
  if (e.kind == Kind::door)
    return (e.id == "yellowDoor" && hero[yk] > 0) || (e.id == "blueDoor" && hero[bk] > 0) ||
           (e.id == "redDoor" && hero[rk] > 0);
  return false;
}
Number Game::path_cost(int f, Pos p) const {
  auto b = block(f, p);
  auto &e = entity(b);
  Number base = 1;
  if (e.kind == Kind::door)
    base = e.id == "yellowDoor" ? 90 : e.id == "blueDoor" ? 140 : e.id == "redDoor" ? 180 : 9999;
  else if (e.kind == Kind::enemy) {
    auto d = enemy_damage(b);
    base = d == 0 ? 3 : 1000 + (d == infinite_damage ? 99999 : d);
  }
  return base + map_damage(f, p) * 100;
}
std::vector<Pos> Game::path_to(int f, Pos target, bool click) const {
  require(f == hero.floor, "Path requested on another floor");
  if (!target.valid())
    return {};
  if (hero.pos == target)
    return {hero.pos};
  std::array<Number, cells> best;
  best.fill(infinite_damage);
  std::array<int, cells> parent;
  parent.fill(-1);
  using Entry = std::tuple<Number, int, int>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> q;
  int start = hero.pos.index(), seq = 0;
  best[start] = 0;
  q.emplace(0, 0, start);
  while (!q.empty()) {
    auto [cost, order, index] = q.top();
    q.pop();
    (void)order;
    if (cost != best[index])
      continue;
    Pos cur{index % side, index / side};
    if (cur == target)
      break;
    for (auto d : route_dirs) {
      Pos next = add(cur, d);
      if(!reference_model&&!native_can_move(f,cur,next))continue;
      if (!can_enter(f, next, target, click))
        continue;
      auto nc = cost + path_cost(f, next);
      int ni = next.index();
      if (nc >= best[ni])
        continue;
      best[ni] = nc;
      parent[ni] = index;
      q.emplace(nc, ++seq, ni);
    }
  }
  if (parent[target.index()] < 0)
    return {};
  std::vector<Pos> out;
  for (int i = target.index(); i >= 0; i = parent[i])
    out.push_back({i % side, i / side});
  std::reverse(out.begin(), out.end());
  return out;
}
bool Game::enter_flower(int f, Pos prev, Pos p) const {
  if (f == 33 && at(p, 8, 10))
    return at(prev, 7, 10);
  if (f == 38 && at(p, 2, 5))
    return at(prev, 2, 4);
  return true;
}
bool Game::trapped_wizard(int f, Pos p) const {
  if (f != 47 || !retreat_wizards.contains(marker(f, p)))
    return false;
  if (at(p, 8, 3))
    return block(f, {8, 4}).entity != 0;
  if (at(p, 8, 1))
    return is_wall(f, {8, 0});
  if (at(p, 1, 11))
    return is_wall(f, {1, 12});
  return false;
}
bool Game::auto_can_battle(Block b) const {
  constexpr std::uint64_t excluded = (1ULL << 12) | (1ULL << 13) | (1ULL << 14) | (1ULL << 19) |
                                     (1ULL << 21) | (1ULL << 26) | (1ULL << 27) | (1ULL << 28) |
                                     (1ULL << 29);
  return !(combat_stats(*this,b).special & excluded) && enemy_damage(b) == 0;
}
void Game::auto_sweep(int f, const std::set<int> &exclude) {
  if (f < 0)
    f = hero.floor;
  if (f != hero.floor || (!automatic.battle && !automatic.wall && !automatic.item))
    return;
  auto prepared=prepared_native;
  if(!reference_model) {
    if(!direct_movement_enabled())return;
    if(native_events_pending(*this))return;
    auto here=block(f,hero.pos);
    if(here.entity && entity(here).kind!=Kind::item)return;
    if(!prepared||!prepared->matches(world))prepared=prepare_native_world(world);
  }
  if ((reference_model?floors[f]->events->contains(location(hero.pos)):
       prepared->source_events[f][hero.pos.index()]) || map_damage(f, hero.pos) > 0)
    return;
  auto reactions=reference_model?nullptr:native_hazard_reactions(f);
  auto hazards=reference_model?nullptr:native_hazards(f);
  auto damage_at=[&](Pos p){return reference_model?map_damage(f,p):cached_map_damage(*hazards,hero[hp],p);};
  auto reaction_at=[&](Pos p) {
    return !reference_model&&reactions->affected[p.index()];
  };
  if(reaction_at(hero.pos))return;
  std::shared_ptr<AutomaticCache> settled;
  if(!reference_model&&!capturing&&!defer_after&&exclude.empty()&&blocked_items.empty()&&!block(f,hero.pos).entity) {
    auto &handle=floors[f]->automatic_cache;settled=handle.load();const bool ignore=flag_truth(NativeFlag::ignoreChangeFloor);
    if(!settled||!settled->matches(*this,f,ignore)){settled=std::make_shared<AutomaticCache>(*this,f,ignore,hazards->pincer.any());handle.store(settled);}
    bool known=false;{std::lock_guard lock(settled->mutex);known=settled->settled[hero.pos.index()];}
    if(known){if(!native_auto_active)native_event_drain();return;}
  }
  // Native startEvents schedules its body after the synchronous automatic BFS.
  // Conditions and execution flags still update during BFS; only VM draining
  // waits. Restore the scope on failure without executing queued event bodies.
  {
    struct RestoreAutoScope {
      bool &active;
      bool previous;
      ~RestoreAutoScope() { active = previous; }
    } restore{native_auto_active, native_auto_active};
    if(!reference_model)native_auto_active=true;
    std::bitset<cells> seen;
    seen.set(hero.pos.index());
    std::array<Pos, cells> queue{};
    int head = 0, tail = 1;
    queue[0] = hero.pos;
    while (head < tail) {
      Pos cur = queue[head++];
      for (auto dir : auto_dirs) {
        Pos next = add(cur, dir);
        if (!next.valid() || seen[next.index()])
          continue;
        auto b = block(f, next);
        auto &e = entity(b);
        if (e.id == "flower" && !enter_flower(f, cur, next))
          continue;
        seen.set(next.index());
        const auto &fd = world->floors[f];
        if ((reference_model?floors[f]->grid[next.index()]:foreground(f,next)) == 103)
          continue;
        if ((reference_model||block_present(f,next))&&
            (reference_model?fd.change_floor.contains(location(next)):prepared->change_floors[f][next.index()])) {
          const auto &cf = fd.change_floor.at(location(next));
          bool ignore = truth(
              cf.value("ignoreChangeFloor", flags->value.value("ignoreChangeFloor", Json(false))));
          if (!(ignore && floors[f]->grid[next.index()] == 0))
            continue;
        }
        if (reference_model?floors[f]->events->contains(location(next)):event_enabled(f,next))
          continue;
        if (!b.entity) {
        } else if (e.kind == Kind::terrain || e.id == "lava") {
          if (!automatic.wall || !(e.id == "fakeWall" || (reference_model&&(e.id == "fakeWall2" || e.id == "flower")) ||
                                   (e.id == "lava" && hero[snow])))
            continue;
          ground(f, next);
          after_action(f, next, b);
        } else if (e.kind == Kind::item) {
          if (blocked_items.contains(marker(f, next)) || !automatic.item ||
              (automatic.item == 2 && (e.id == "redPotion" || e.id == "bluePotion" || (!reference_model&&(e.id=="greenPotion"||e.id=="yellowPotion"||e.id=="superPotion")))) ||
              damage_at(next) > 0 || reaction_at(next))
            continue;
          apply_item(f, e.id);
          ground(f, next);
          after_action(f, next, b);
        } else if (e.kind == Kind::enemy && automatic.battle) {
          if (exclude.contains(next.index()) || fd.after_battle[next.index()] ||
              (reference_model&&retreat_wizards.contains(marker(f, next)) && !trapped_wizard(f, next)) ||
              (reference_model&&no_auto_farm.contains(marker(f, next))) || !auto_can_battle(b))
            continue;
          if(!reference_model)apply_block(f,next,b);
          else {
            hero[gold] += e.money * (hero[coin] ? 2 : 1);
            ground(f, next);after_action(f,next,b,true);
          }
        } else
          continue;
        if(!reference_model&&b.entity){reactions=native_hazard_reactions(f);hazards=native_hazards(f);}
        if (damage_at(next) > 0 || reaction_at(next))
          continue;
        queue[tail++] = next;
      }
    }
    // With no mutation, every queued empty cell reaches a subset of this same
    // settled region, including under the directional flower rule. Never use
    // `seen`: it also contains blocked cells. A pickup/mutation clears the
    // floor handle, so a sweep that changed eligibility cannot publish here.
    if(settled&&floors[f]->automatic_cache.load()==settled&&settled->matches(*this,f,flag_truth(NativeFlag::ignoreChangeFloor))) {
      std::lock_guard lock(settled->mutex);for(int i=0;i<tail;++i)settled->settled.set(queue[i].index());
    }
  }
  if(!reference_model&&!native_auto_active)native_event_drain();
}
void Game::apply_item(int f, const std::string &id) {
  auto ratio = world->floors[f].ratio;
  if(!reference_model) {
    if(!flags->value.contains("__itemHint__"))set_flag("__itemHint__",Json::array());
    const auto &defs=object_member(world->native_data,"items");
    if(defs.contains(id) && defs[id].value("cls",std::string{})!="items") {
      add_inventory(id);return;
    }
  }
  if (id == "yellowKey")
    ++hero[yk];
  else if (id == "blueKey")
    ++hero[bk];
  else if (id == "redKey")
    ++hero[rk];
  else if (id == "redPotion")
    hero[hp] += 50 * ratio;
  else if (id == "bluePotion")
    hero[hp] += 200 * ratio;
  else if (!reference_model && (id=="greenPotion"||id=="yellowPotion"))
    hero[hp]+=val(object_member(world->native_data,"values"),id)*ratio;
  else if (!reference_model && id=="greenGem")
    hero[mdef]+=val(object_member(world->native_data,"values"),id)*ratio;
  else if (!reference_model && id=="yellowGem") {
    hero[hp]+=1000;hero[atk]+=6;hero[defense]+=6;hero[mdef]+=10;
  }
  else if (!reference_model && id=="pack")hero[gold]+=500;
  else if (id == "redGem")
    hero[atk] += ratio;
  else if (id == "blueGem")
    hero[defense] += ratio;
  else if (id.size() == 6 &&
           (id.starts_with("sword") ||
            id.starts_with("shield"))) { /* shields have 7 characters; handled below */
    if (id.starts_with("sword")) {
      int level = id.back() - '1';
      require(level >= 0 && level < 5, "Unknown sword");
      hero[atk] += std::array<int, 5>{10, 20, 40, 50, 100}[level];
      if(!reference_model)set_flag("nowWeapon",id);
    }
  } else if (id.starts_with("shield")) {
    int level = id.back() - '1';
    require(level >= 0 && level < 5, "Unknown shield");
    hero[defense] += std::array<int, 5>{10, 20, 40, 50, 100}[level];
    if (level == 4)
      hero[magicImmune] = 1;
    if(!reference_model) {
      set_flag("nowShield",id);
      if(level==4)set_flag("魔法免疫",true);
    }
  } else if (id == "centerFly3")
    hero[centerFly] += 3;
  else {
    for (Stat s : {upFly, downFly, centerFly, bigKey, bomb, pickaxe, earthquake, superPotion, snow})
      if (id == stat_names[s]) {
        ++hero[s];
        return;
      }
    for (Stat s : {coin, knife, cross})
      if (id == stat_names[s]) {
        hero[s] = 1;
        return;
      }
    // These utility constants have no combat/resource effect in the Python search model.
    if (id != "book" && id != "fly")
      throw Unsupported("Item effect not modeled: " + id);
  }
}
void Game::apply_block(int f, Pos p, Block b) {
  const auto e = combat_stats(*this,b);
  if (e.kind == Kind::door) {
    Stat key = e.id == "yellowDoor" ? yk : e.id == "blueDoor" ? bk : rk;
    Stat used = e.id == "yellowDoor" ? yd : e.id == "blueDoor" ? bd : rd;
    require(e.id == "yellowDoor" || e.id == "blueDoor" || e.id == "redDoor",
            "Mechanism door cannot be opened with a key");
    require(hero[key] > 0, "Missing door key");
    --hero[key];
    ++hero[used];
    ground(f, p);
    after_action(f, p, b);
    if (strict)
      return;
  } else if (e.kind == Kind::enemy) {
    auto damage = enemy_damage(b);
    require(damage < infinite_damage, "Cannot damage enemy");
    require(hero[hp] > damage, "Battle death");
    if(!reference_model&&flag(NativeFlag::addhp)==1)damage=-damage;
    hero[hp] -= damage;
    hero[dmg] += damage;
    if(reference_model||!flag_truth(NativeFlag::curse))hero[gold] += e.money * (hero[coin] ? 2 : 1);
    if(!reference_model) {
      set_flag("对话能量",0);
      set_flag("hatred",flag(NativeFlag::hatred)+val(object_member(world->native_data,"values"),"hatred"));
      const auto &source=object_member(object_member(world->native_data,"enemies"),e.id);
      const auto &overrides=object_member(object_member(extra_state->value,"enemy_overrides"),e.id);
      const auto existing=extra_state->value.find("exp");
      if((!source.contains("exp")&&!overrides.contains("exp"))||(existing!=extra_state->value.end()&&!existing->is_number()))set_extra("exp",nullptr);
      else set_extra("exp",val(extra_state->value,"exp")+(overrides.contains("exp")?val(overrides,"exp"):val(source,"exp")));
    }
  } else if (e.kind == Kind::item)
    apply_item(f, e.id);
  else if (e.kind != Kind::terrain)
    throw Unsupported("Cannot interact with block " + e.id);
  if (e.kind != Kind::terrain || e.id == "fakeWall" || e.id == "fakeWall2")
    ground(f, p);
  if (e.kind != Kind::door)
    after_action(f, p, b);
  if(reference_model) {
    apply_map_damage(f, p);
    trigger_tile_event(f, p);
  }
}
void Game::go_to(int f, Pos target) {
  require(f == hero.floor, "Click requested on another floor");
  if(!reference_model) {
    auto path=path_to(f,target,true);require(!path.empty(),"No native click path");
    for(std::size_t i=1;i<path.size();++i) {
      auto p=path[i];auto delta=Pos{p.x-hero.pos.x,p.y-hero.pos.y};
      step(direction_name(delta));
      if(hero.floor!=f||hero.pos!=p)return;
      if(native_events_pending(*this))return;
    }
    return;
  }
  if (target == hero.pos && !block(f, target).entity)
    return;
  auto target_block = block(f, target);
  auto &te = entity(target_block);
  bool stair = te.id == "upFloor" || te.id == "downFloor";
  bool auto_enemy =
      strict && te.kind == Kind::enemy && automatic.battle && enemy_damage(target_block) == 0;
  std::vector<Pos> path;
  Pos path_target = target;
  if (auto_enemy) {
    for (auto d : route_dirs) {
      auto p = add(target, d);
      auto &e = entity(block(f, p));
      if (e.kind != Kind::empty && e.kind != Kind::item && e.id != "flower")
        continue;
      auto candidate = path_to(f, p, true);
      if (!candidate.empty() && (path.empty() || candidate.size() < path.size()))
        path = std::move(candidate);
    }
    if (!path.empty())
      path_target = path.back();
  } else
    path = path_to(f, target, strict);
  require(!path.empty(), "No path to " + floor_id(f) + " " + location(target));
  Pos start = hero.pos;
  std::set<int> reserved;
  for (auto p : path)
    if (p != target && entity(block(f, p)).kind == Kind::enemy)
      reserved.insert(p.index());
  for (std::size_t i = 1; i < path.size(); ++i) {
    auto p = path[i];
    auto b = block(f, p);
    auto e = entity(b);
    if (e.id == "flower")
      require(enter_flower(f, hero.pos, p), "Wrong-way flower");
    if (strict && e.kind == Kind::door) {
      if (hero.pos != start)
        auto_sweep(f, reserved);
      apply_block(f, p, b);
      if (p != target)
        go_to(f, target);
      return;
    }
    if (strict && (e.id == "fakeWall" || e.id == "fakeWall2")) {
      apply_block(f, p, b);
      if (p != target)
        go_to(f, target);
      return;
    }
    hero.pos = p;
    if (b.entity) {
      bool kill = strict && e.kind == Kind::enemy;
      defer_after = kill;
      try {
        apply_block(f, p, b);
      } catch (...) {
        defer_after = false;
        throw;
      }
      defer_after = false;
      if (kill) {
        if (!world->floors[f].after_battle[p.index()])
          auto_sweep(f, reserved);
        flush_deferred();
        if (world->floors[f].after_battle_auto[p.index()])
          auto_sweep(f, reserved);
        if (p != target)
          go_to(f, target);
        return;
      }
      if (strict && e.kind == Kind::item) {
        auto_sweep(f, reserved);
        if (p != target)
          go_to(f, target);
        return;
      }
    } else {
      bool damage = !(auto_enemy && p == path_target) && map_damage(f, p) > 0;
      if (!(auto_enemy && p == path_target))
        apply_map_damage(f, p);
      trigger_tile_event(f, p);
      if (damage && strict)
        auto_sweep(f);
      if (strict)
        auto_sweep(f, reserved);
    }
  }
  if (auto_enemy) {
    auto b = block(f, target);
    if (entity(b).kind == Kind::enemy && distance(hero.pos, target) == 1) {
      hero.pos = target;
      defer_after = true;
      try {
        apply_block(f, target, b);
      } catch (...) {
        defer_after = false;
        throw;
      }
      defer_after = false;
    }
  }
  if (strict) {
    if (!stair)
      auto_sweep(f, reserved);
    flush_deferred();
    if (world->floors[f].after_battle_auto[target.index()])
      auto_sweep(f, reserved);
  }
}
void Game::after_action(int f, Pos p, Block b, bool aut) {
  if(!reference_model) {
    const auto &e=entity(b);
    if(e.kind==Kind::enemy)native_event_after("afterBattle",f,p);
    else if(e.kind==Kind::item)native_event_after("afterGetItem",f,p);
    else if(e.kind==Kind::door)native_event_after("afterOpenDoor",f,p);
    return;
  }
  if (!defer_after) {
    after_action_impl(f, p, b, aut);
    return;
  }
  capturing = true;
  captured.clear();
  try {
    after_action_impl(f, p, b, aut);
  } catch (...) {
    deferred.insert(deferred.end(), captured.begin(), captured.end());
    capturing = false;
    captured.clear();
    throw;
  }
  deferred.insert(deferred.end(), captured.begin(), captured.end());
  capturing = false;
  captured.clear();
}
void Game::after_action_impl(int f, Pos p, Block b, bool aut) {
  const auto &e = entity(b);
  bool enemy = e.kind == Kind::enemy;
  auto count = [&](const std::string &name, int n, Pos door) {
    set_flag(name, flag(name) + 1);
    if (flag(name) >= n)
      ground(f, door);
  };
  auto clear = [&](std::initializer_list<Pos> ps) {
    for (auto q : ps)
      ground(f, q);
  };
  auto put = [&](std::initializer_list<Pos> ps, const std::string &id, int t) {
    for (auto q : ps)
      set_block(f, q, id, t);
  };
  auto none = [&](std::initializer_list<Pos> ps) {
    return std::none_of(ps.begin(), ps.end(), [&](Pos q) { return block(f, q).entity != 0; });
  };
  if (f == 8 && enemy && in(p, {{9, 5}, {11, 5}}))
    count("8redKeyGuards", 2, {10, 4});
  if (f == 11 && enemy && e.id == "redPriest" && in(p, {{1, 5}, {3, 5}}))
    count("11shieldPriests", 2, {2, 4});
  if (f == 14 && enemy && e.id == "zombieKnight" && in(p, {{2, 2}, {1, 1}, {3, 1}})) {
    set_flag("14zombieKnightGuards", flag(NativeFlag::flag_0) + 1);
    if (flag(NativeFlag::flag_0) >= 3)
      set_block(f, {1, 3}, "redKey", 23);
  }
  if (f == 17 && enemy && e.id == "yellowGuard") {
    if (in(p, {{1, 8}, {3, 8}}))
      count("17lowerYellowGuards", 2, {2, 7});
    if (in(p, {{1, 5}, {3, 5}}))
      count("17upperYellowGuards", 2, {2, 4});
  }
  if (f == 17 && enemy) {
    if (e.id == "zombie" && in(p, {{9, 8}, {11, 8}}))
      count("17rightLowerGuards", 2, {10, 7});
    if (e.id == "zombieKnight" && in(p, {{9, 5}, {11, 5}}))
      count("17rightUpperGuards", 2, {10, 4});
  }
  if (f == 32 && enemy && e.id == "blueGuard" && in(p, {{1, 10}, {3, 10}}))
    count("32blueGuards", 2, {2, 9});
  if (f == 33 && enemy && in(p, {{9, 5}, {11, 5}, {9, 7}, {11, 7}}) &&
      none({{9, 5}, {11, 5}, {9, 7}, {11, 7}}))
    clear({{10, 4}, {10, 8}});
  if (f == 2 && enemy && e.id == "blueGuard" && in(p, {{6, 2}, {8, 2}})) {
    set_flag("2blueGuards", flag(NativeFlag::flag_1) + 1);
    if (flag(NativeFlag::flag_1) >= 2)
      clear({{5, 5}, {5, 8}, {5, 11}, {9, 5}, {9, 8}, {9, 11}});
  }
  if (f == 34 && enemy && !aut &&
      in(p, {{5, 4}, {7, 4}, {9, 4}, {11, 4}, {5, 8}, {7, 8}, {9, 8}, {11, 8}}) &&
      none({{5, 4}, {7, 4}, {9, 4}, {11, 4}, {5, 8}, {7, 8}, {9, 8}, {11, 8}})) {
    put({{1, 5}, {3, 5}, {1, 7}, {3, 7}}, "yellowKey", 21);
    set_block(f, {2, 6}, "redKey", 23);
  }
  if (f == 38 && enemy && in(p, {{1, 10}, {3, 10}}) && none({{1, 10}, {3, 10}}))
    ground(f, {2, 9});
  if (f == 19 && at(p, 6, 3) && (e.id == "fakeWall2" || e.id == "cross") && !hero[cross])
    set_block(f, {6, 3}, "cross", 55);
  if (f == 20 && enemy && e.id == "vampire") {
    clear({{6, 3}, {6, 9}});
    put({{5, 4}, {6, 4}, {7, 4}}, "yellowKey", 21);
    put({{4, 5}, {4, 6}, {4, 7}}, "redGem", 27);
    put({{8, 5}, {8, 6}, {8, 7}}, "blueGem", 28);
    put({{5, 8}, {6, 8}, {7, 8}}, "bluePotion", 32);
  }
  if (f == 30 && enemy && in(p, {{3, 5}, {4, 5}, {5, 5}, {7, 5}, {8, 5}, {9, 5}}) &&
      none({{3, 5}, {4, 5}, {5, 5}, {7, 5}, {8, 5}, {9, 5}})) {
    ground(f, {6, 4});
    set_flag("30", 5);
  }
  if (f == 41 && enemy && at(p, 2, 2))
    set_flag("41", 1);
  if (f == 41 && enemy && at(p, 10, 2)) {
    clear({{5, 7}, {7, 7}});
    put({{5, 6}, {6, 6}, {7, 6}, {7, 1}}, "yellowWall", 1);
    set_block(f, {6, 5}, "downFly", 52);
  }
  if (f == 43 && enemy && at(p, 9, 1))
    put({{8, 2}, {10, 2}}, "yellowWall", 1);
  if (f == 15 && enemy && at(p, 6, 6))
    clear({{5, 4}, {5, 5}, {5, 6}, {7, 4}, {7, 5}, {7, 6}, {6, 4}, {6, 3}});
  if (f == 25 && enemy && at(p, 6, 6)) {
    put({{4, 8}, {5, 8}, {7, 8}, {8, 8}}, "redKey", 23);
    ground(f, {6, 9});
  }
  if (f == 35 && enemy && at(p, 6, 7)) {
    clear({{6, 3}, {5, 7}, {7, 7}});
    if (!flag_truth(NativeFlag::feature))
      put({{5, 5}, {6, 5}, {7, 5}}, "bluePotion", 32);
    if (reference_model || !flag_truth(NativeFlag::feature))
      set_block(f, {6, 6}, "snow", 54); /* Original MT35: feature mode does not respawn snow. */
  }
  if (f == 44 && enemy && in(p, {{5, 9}, {7, 9}}))
    count("441", 2, {6, 8});
  if (f == 45 && enemy) {
    if (in(p, {{8, 9}, {8, 11}}))
      count("451", 2, {7, 10});
    if (in(p, {{5, 9}, {5, 11}}))
      count("452", 2, {4, 10});
  }
  if (f == 49 && enemy) {
    if (in(p, {{6, 2}, {5, 3}, {7, 3}, {6, 4}})) {
      set_flag("49cardinal", flag(NativeFlag::flag_4) + 1);
      if (flag(NativeFlag::flag_4) >= 4)
        set_flag("49sealed", true);
    }
    if (in(p, {{5, 10}, {7, 10}}))
      count("491", 2, {6, 9});
    if (in(p, {{5, 8}, {7, 8}}))
      count("492", 2, {6, 7});
    if (at(p, 6, 3)) {
      hero[won49] = 1;
      clear({{5, 2}, {6, 2}, {7, 2}, {5, 3}, {6, 3}, {7, 3}, {5, 4}, {6, 4}, {7, 4}});
      set_block(f, {5, 2}, "redKey", 23);
      set_block(f, {7, 2}, "knife", 62);
      put({{2, 4}, {3, 4}, {4, 4}}, "redGem", 27);
      put({{8, 4}, {9, 4}, {10, 4}}, "blueGem", 28);
      put({{5, 5}, {6, 5}, {7, 5}}, "bluePotion", 32);
    }
  }
  if (f == 50 && enemy && at(p, 6, 5))
    hero[won50] = 1;
}
void Game::flush_deferred() {
  auto effects = std::move(deferred);
  deferred.clear();
  for (auto &e : effects)
    set_block(e.floor, e.pos, e.block);
}
void Game::trigger_tile_event(int f, Pos p) {
  if(!reference_model) {
    if(foreground(f,p)!=103)return;
    set_foreground(f,p,0);
    for(auto d:auto_dirs) {
      auto q=add(p,d),r=add(q,d);
      if(q.x<1||q.y<1||q.x>11||q.y>11||r.x<1||r.y<1||r.x>11||r.y>11)continue;
      auto b=block(f,q);if(entity(b).id!="redWizard")continue;
      if(!block_present(f,r,true)) {
        ground(f,q);set_block(f,r,b);
        if(foreground(f,r)==103) {
          auto beyond=add(r,d);
          auto effective=retained_block_event(f,beyond);
          bool data=effective.is_object()&&effective.contains("data")&&!effective["data"].is_null();
          if(!data)set_foreground(f,beyond,103);
          set_foreground(f,q,103);
        }
      }
      set_foreground(f,r,0);
    }
    return;
  }
  if (f == 47 && at(p, 8, 1) && !flag_truth(NativeFlag::flag_3)) {
    auto b = block(f, {8, 2});
    if (entity(b).id == "redWizard") {
      ground(f, {8, 2});
      set_block(f, {8, 3}, b);
      set_flag("47wizardMoved", true);
      retreat_wizards.insert(marker(47, {8, 3}));
    }
  }
  if (f == 42 && at(p, 5, 10) && block(f, {6, 10}).entity)
    ground(f, {6, 10});
  if (f == 41 && at(p, 9, 2) && flag(NativeFlag::flag_2) == 1 && entity(block(f, {10, 2})).id != "redWizard") {
    set_block(f, {10, 2}, "redWizard", 220);
    set_flag("41", 2);
  }
}
void Game::transition(int f, Pos landing, bool flying) {
  require(f >= world->first_floor && f <= 50, "Unknown destination floor");
  if (!landing.valid())
    landing = f > hero.floor ? world->floors[f].down : world->floors[f].up;
  if (!landing.valid() && f == 44)
    landing = {1, 1};
  require(landing.valid(), "No stair landing");
  bool first=!has_visited(f);
  if(!reference_model&&!(flying&&f==hero.floor)) {
    set_flag_member("__leaveLoc__",floor_id(hero.floor),{{"x",hero.pos.x},{"y",hero.pos.y},{"direction",extra_state->value.value("direction",std::string("down"))}});
  }
  hero.floor = f;
  hero.pos = landing;
  if ((reference_model && f != 49 && f != 50) || (!reference_model&&first))
    visited.set(f);
  if(reference_model)apply_map_damage(f,landing);
  else {
    set_flag("对话能量",0);
    if(first)set_flag_member("__visited__",floor_id(f),true);
    native_event_arrive(f,first);
  }
}
bool Game::has_visited(int f) const {
  if(f<world->first_floor||f>50)return false;
  if(reference_model || truth(world->floors[f].metadata.value("isHide",Json(false))))return visited[f];
  for(int n=f;n<=50;++n)if(visited[n])return true;
  return false;
}
bool Game::can_connect(int f,Pos from,Pos to,bool direct) const {
  if(f<world->first_floor||f>50||!from.valid()||!to.valid())return false;
  if(from==to)return true;
  auto &handle=floors[f]->navigation;auto cache=handle.load();
  if(!cache||cache->world!=world||cache->context!=navigation_context) {
    bool hp_dependent=false;
    const auto &overrides=object_member(extra_state->value,"enemy_overrides");
    for(auto b:floors[f]->blocks) {
      const auto &e=entity(b);if(e.kind!=Kind::enemy)continue;
      const auto &v=object_member(overrides,e.id);
      const auto special=e.special|(v.contains("special")?specials(v["special"]):0);
      if(special&(1ULL<<16)){hp_dependent=true;break;}
    }
    cache=std::make_shared<NavigationCache>(world,navigation_context,hp_dependent);handle.store(cache);
  }
  const NavigationCache::Key key{from.index(),to.index(),direct,cache->hp_dependent?hero[hp]:0,hero[magicImmune],reference_model};
  {std::lock_guard lock(cache->mutex);const auto found=cache->queries.find(key);if(found!=cache->queries.end())return found->second;}
  const bool result=[&] {
  const auto hazards=reference_model?nullptr:native_hazards(f);
  auto damage_at=[&](Pos p){return reference_model?map_damage(f,p):cached_map_damage(*hazards,hero[hp],p);};
  if(damage_at(from)>0)return false;
  const auto reactions=native_hazard_reactions(f);
  auto reaction_at=[&](Pos p){return reactions->affected[p.index()];};
  if(reaction_at(from))return false;
  auto initial=block(f,from);
  if((initial.entity||event_enabled(f,from))&&effective_block_event(f,from).value("trigger",Json())!="changeFloor")return false;
  auto pass=[&](Pos p,bool end) {
    auto &node=cache->nodes[reference_model?1:0][p.index()];const std::uint8_t ready=end?4:1,allowed=end?8:2;
    auto bits=node.load(std::memory_order_relaxed);
    if(!(bits&ready)) {
    const bool permits=[&] {
    auto b=block(f,p);
    if(event_enabled(f,p))return false;
    // An absent native block has the fixed empty-terrain event. Avoid building
    // that JSON object for every empty cell visited by this read-only BFS.
    if(reference_model||block_present(f,p)) {
      Json reference_event;
      if(reference_model)reference_event=effective_block_event(f,p);
      const auto &native=reference_model?reference_event:native_block_event_view(f,p);
      if(b.entity&&(truth(native.value("noPass",Json(true)))||truth(native.value("script",Json()))||truth(native.value("event",Json()))))return false;
      bool change=native.value("trigger",Json())=="changeFloor";
      bool trigger=truth(native.value("trigger",Json()));
      if(trigger) {
        if(!change || end)return false;
        const auto &data=object_member(native,"data");
        if(!truth(data.value("ignoreChangeFloor",object_member(world->native_data,"flags").value("ignoreChangeFloor",Json(false)))))return false;
      }
    }
    return true;
    }();
    bits=std::uint8_t(ready|(permits?allowed:0));node.fetch_or(bits,std::memory_order_relaxed);
    }
    if(!(bits&allowed))return false;
    if(damage_at(p)>0 || reaction_at(p) || (direct&&foreground(f,p)==103))return false;
    return true;
  };
  std::bitset<cells> seen;seen.set(from.index());std::array<Pos,cells> q{};q[0]=from;int head=0,tail=1;
  // One finite breadth-first traversal, independent of route HP preferences.
  while(head<tail) {
    auto p=q[head++];
    for(auto d:auto_dirs) {
      auto n=add(p,d);
      if(!n.valid()||seen[n.index()]||!native_can_move(f,p,n)||!pass(n,n==to))continue;
      if(n==to)return true;
      seen.set(n.index());q[tail++]=n;
    }
  }
  return false;
  }();
  {std::lock_guard lock(cache->mutex);if(cache->queries.size()>=256)cache->queries.clear();cache->queries.emplace(key,result);}
  return result;
}
void Game::move_directly(Pos p) {
  require(!reference_model,"Direct movement requires native mode");
  require(direct_movement_enabled(),"Direct movement disabled");
  require(can_connect(hero.floor,hero.pos,p,true),"No legal direct-movement path");
  hero.pos=p;
  auto_sweep(hero.floor); // Native control.moveDirectly wrapper calls update()->core.auto().
}
bool Game::can_fly(int f) const {
  if(f<world->first_floor||f>50||!has_visited(f))return false;
  if(reference_model)return true;
  int from=hero.floor;
  if(!truth(world->floors[from].metadata.value("canFlyFrom",Json(true)))||
     !truth(world->floors[f].metadata.value("canFlyTo",Json(true)))||item_count("fly")<=0)return false;
  auto stair=[](const std::string &s){return s=="upFloor"||s=="downFloor"||s=="portal";};
  bool near=false;
  for(int i=0;i<cells;++i)if(stair(entity(floors[from]->blocks[i]).id)&&distance(hero.pos,{i%side,i/side})<=1)near=true;
  const auto &global=object_member(world->native_data,"flags");
  if(!near&&truth(global.value("flyNearStair",Json(true)))) {
    if(!flag_truth(NativeFlag::flag_29)||!direct_movement_enabled())return false;
    bool accessible=false;
    for(int i=0;i<cells&&!accessible;++i)if(stair(entity(floors[from]->blocks[i]).id))
      for(auto d:auto_dirs)if(can_connect(from,hero.pos,add({i%side,i/side},d),true)){accessible=true;break;}
    if(!accessible)return false;
  }
  if(flag_truth(NativeFlag::fly))return true;
  if(truth(world->floors[f].metadata.value("isHide",Json(false))))return true;
  if(f==from)return direct_movement_enabled()&&can_connect(from,hero.pos,world->floors[f].down,true);
  int delta=f>from?1:-1;
  for(int n=f-delta;n!=from;n-=delta) {
    if(truth(world->floors[n].metadata.value("isHide",Json(false))))return true; // Exact recursive hidden-target short circuit.
    if(!can_connect(n,world->floors[n].up,world->floors[n].down))return false;
  }
  if(from==1||from==45)return true;
  return can_connect(from,hero.pos,delta>0?world->floors[from].up:world->floors[from].down);
}
void Game::fly(int f) {
  if (reference_model && f == hero.floor)
    return;
  require(can_fly(f), "Flight is not legal from this state");
  Pos landing = f >= hero.floor ? world->floors[f].down : world->floors[f].up;
  require(landing.valid(), "No fly landing");
  if(!reference_model){transition(f,landing,true);return;}
  hero.floor = f;
  hero.pos = landing;
  if(reference_model)apply_map_damage(f, landing);
  else {set_flag("对话能量",0);native_event_arrive(f,false);}
}
void Game::center_fly() {
  require(hero[centerFly] > 0, "No centerFly");
  Pos p{side - 1 - hero.pos.x, side - 1 - hero.pos.y};
  auto b=block(hero.floor,p);
  require(!b.entity || (!reference_model&&(entity(b).id=="none"||entity(b).id=="airwall")), "CenterFly landing blocked");
  --hero[centerFly];
  hero.pos = p;
  if(reference_model)apply_map_damage(hero.floor,p);
  else {set_flag("talking",0);consume_tool_energy();}
}
void Game::up_fly() {
  require(hero[upFly] > 0, "No upFly");
  int f = hero.floor + 1;
  // The pinned floorIds starts at MT0; its item predicate rejects index >=49.
  if(!reference_model)require(hero.floor<49,"UpFly unavailable at this floor");
  require(f <= 50 && !is_wall(f, hero.pos) && !block(f, hero.pos).entity, "UpFly landing blocked");
  // Native getBlockId also sees an active action event on an otherwise zero tile.
  if(!reference_model)require(!block_present(f,hero.pos),"UpFly landing blocked by event");
  --hero[upFly];
  // useItem's route decrement occurs before the asynchronous floor arrival,
  // whose afterChangeFloor hook resets dialogue energy to zero.
  if(!reference_model){transition(f,hero.pos);return;}
  hero.floor = f;
  apply_map_damage(f, hero.pos);
}
void Game::down_fly() {
  require(hero[downFly] > 0, "No downFly");
  int f = hero.floor - 1;
  require(f >= (reference_model?1:world->first_floor) && !is_wall(f, hero.pos) && !block(f, hero.pos).entity, "DownFly landing blocked");
  if(!reference_model)require(!block_present(f,hero.pos),"DownFly landing blocked by event");
  --hero[downFly];
  if(!reference_model){transition(f,hero.pos);return;}
  hero.floor = f;
  apply_map_damage(f, hero.pos);
}
void Game::use_big_key() {
  require(hero[bigKey] > 0, "No bigKey");
  --hero[bigKey];
  int f = hero.floor;
  for (int i = 0; i < cells; ++i)
    if (entity(floors[f]->blocks[i]).id == "yellowDoor")
      ground(f, {i % side, i / side});
  consume_tool_energy();
}
void Game::use_earthquake() {
  require(hero[earthquake] > 0, "No earthquake");
  --hero[earthquake];
  int f = hero.floor;
  for (int i = 0; i < cells; ++i) {
    auto &id = entity(floors[f]->blocks[i]).id;
    if (reference_model?(wall_ids.contains(id) && id != "unbreakableWall" && id != "blockWall"):
         truth(entity(floors[f]->blocks[i]).native.value("canBreak",Json(false))))
      ground(f, {i % side, i / side});
  }
  consume_tool_energy();
}
void Game::use_bomb() {
  require(hero[bomb] > 0, "No bomb");
  if(!reference_model) {
    std::vector<Pos> targets;Json todo=Json::array();Number reward=0;int f=hero.floor;
    for(auto d:auto_dirs) {
      auto p=add(hero.pos,d);auto b=block(f,p);auto e=effective_enemy(b);
      if(e.kind!=Kind::enemy||e.hp>=500||effective_block_event(f,p).value("trigger",Json())!="battle")continue;
      targets.push_back(p);reward+=e.money;
      for(const auto &data:{world->floors[f].metadata.value("afterBattle",Json::object()).value(location(p),Json()),
                           world->native_data.value("enemies",Json::object()).value(e.id,Json::object()).value("afterBattle",Json())}) {
        if(data.is_array())for(const auto &a:data)todo.push_back(a);
        else if(!data.is_null())todo.push_back(data);
      }
    }
    auto order=floors[f]->block_order;
    for(auto it=order.rbegin();it!=order.rend();++it)
      if(std::find(targets.begin(),targets.end(),*it)!=targets.end()){ground(f,*it);erase_event(f,*it);}
    hero[gold]+=reward;
    if(!todo.empty())native_event_start(todo,f,hero.pos);
    --hero[bomb];consume_tool_energy();return;
  }
  --hero[bomb];
  Number money = 0;
  for (auto d : route_dirs) {
    auto p = add(hero.pos, d);
    auto b = block(hero.floor, p);
    auto &e = entity(b);
    if (e.kind == Kind::enemy && e.hp < 500) {
      money += e.money;
      ground(hero.floor, p);
      after_action(hero.floor, p, b);
    }
  }
  hero[gold] += money;
  consume_tool_energy();
}
void Game::use_pickaxe(int f, Pos p) {
  require(hero[pickaxe] > 0, "No pickaxe");
  if(!reference_model) {
    require(f==hero.floor,"Pickaxe used on another floor");--hero[pickaxe];
    for(auto d:auto_dirs) {
      auto q=add(hero.pos,d);
      if(truth(entity(block(f,q)).native.value("canBreak",Json(false)))) {
        ground(f,q);native_event_after("afterOpenDoor",f,q);
      }
    }
    consume_tool_energy();return;
  }
  require(f == hero.floor && distance(hero.pos, p) == 1, "Pickaxe target not adjacent");
  require(wall_ids.contains(entity(block(f, p)).id), "Pickaxe target is not a wall");
  --hero[pickaxe];
  ground(f, p);
}
void Game::use_snow() {
  require(hero[snow] > 0, "No snow");
  int n = 0;
  for (auto d : route_dirs) {
    auto p = add(hero.pos, d);
    if (entity(block(hero.floor, p)).id == "lava") {
      ground(hero.floor, p);
      ++n;
    }
  }
  if(reference_model)require(n > 0, "No adjacent lava");
  else if(!n)add_inventory("snow");
  consume_tool_energy();
}
void Game::use_super_potion() {
  require(hero[superPotion] > 0, "No superPotion");
  --hero[superPotion];
  hero[hp] += Number(std::floor(double(hero[atk] + hero[defense]) * 0.74 + 0.5)) * 10;
  consume_tool_energy();
}
void Game::set_auto(const Json &j) {
  for (auto it = j.begin(); it != j.end(); ++it) {
    require(it.key() == "battle" || it.key() == "wall" || it.key() == "item",
            "Unknown auto setting");
    require(it.value() == Json(true) || it.value() == Json(false) || it.value() == Json("nopotion"),
            "Invalid auto setting");
    if (it.key() == "battle")
      automatic.battle = truth(it.value());
    if (it.key() == "wall")
      automatic.wall = truth(it.value());
    if (it.key() == "item")
      automatic.item = it.value() == Json("nopotion") ? 2 : truth(it.value()) ? 1 : 0;
  }
  set_flag("__auto__", auto_json(automatic));
}
void Game::go_to_decomposed(int f, Pos target, int limit) {
  auto initial = block(f, target);
  for (int i = 0; i < limit; ++i) {
    if ((!initial.entity && hero.pos == target) || (initial.entity && !block(f, target).entity))
      return;
    auto before = *this;
    try {
      go_to(f, target);
      return;
    } catch (const RouteError &) {
      *this = before;
    }
    auto path = path_to(f, target, false);
    require(!path.empty(), "No decomposed path");
    bool advanced = false;
    for (auto p : path) {
      if (p == hero.pos)
        continue;
      auto &e = entity(block(f, p));
      if (e.kind == Kind::enemy || e.kind == Kind::door || e.id == "fakeWall" ||
          e.id == "fakeWall2") {
        go_to(f, p);
        advanced = true;
        break;
      }
    }
    require(advanced, "No blocker to resolve");
  }
  throw RouteError("Decomposed path limit");
}
void Game::move_adjacent(int f, Pos target) {
  std::vector<std::pair<int, Pos>> candidates;
  for (auto d : route_dirs) {
    Pos p = add(target, d);
    auto path = path_to(f, p, true);
    if (!path.empty())
      candidates.emplace_back(int(path.size()), p);
  }
  bool loose = false;
  if (candidates.empty()) {
    loose = true;
    for (auto d : route_dirs) {
      Pos p = add(target, d);
      auto path = path_to(f, p, false);
      if (!path.empty())
        candidates.emplace_back(int(path.size()), p);
    }
  }
  require(!candidates.empty(), "No adjacent path");
  auto p = std::min_element(candidates.begin(), candidates.end())->second;
  if (loose)
    go_to_decomposed(f, p);
  else
    go_to(f, p);
}
void Game::talk_trader(int f, Pos p) {
  if(!reference_model){move_adjacent(f,p);talk_npc(f,p);return;}
  move_adjacent(f, p);
  std::string key = floor_id(f) + "@" + std::to_string(p.x) + "@" + std::to_string(p.y) + "@A";
  Number cost = f == 47 ? 4000 : (f == 45 || f == 31) ? 1000 : (f == 6 || f == 7) ? 50 : -1;
  require(cost >= 0, "Unmodeled trader");
  require(f == 47 ? hero[earthquakeBought] < 1 : !flag_truth(key), "Trader already used");
  require(hero[gold] >= cost, "Insufficient merchant gold");
  hero[gold] -= cost;
  if (f == 47) {
    ++hero[earthquake];
    ++hero[earthquakeBought];
  } else {
    set_flag(key, 1);
    if (f == 45)
      hero[hp] += 2000;
    if (f == 31) {
      hero[yk] += 4;
      ++hero[bk];
    }
    if (f == 6)
      ++hero[bk];
    if (f == 7)
      hero[yk] += 5;
  }
}
void Game::buy_shop(const std::string &kind, int count) {
  require(reference_model,"Native shop purchases require the source shop menu");
  require(count >= 0, "Negative shop count");
  for (int i = 0; i < count; ++i) {
    Number times = hero[times1], cost = 20 + 10 * (times + 1) * times;
    require(hero[gold] >= cost, "Insufficient shop gold");
    hero[gold] -= cost;
    require(kind == "atk" || kind == "def" || kind == "hp", "Unknown shop purchase");
    if (kind == "atk")
      hero[atk] += 2 * world->floors[hero.floor].ratio;
    if (kind == "def")
      hero[defense] += 4 * world->floors[hero.floor].ratio;
    if (kind == "hp")
      hero[hp] += 100 * (times + 1);
    ++hero[times1];
  }
}
void Game::talk_mt2_bless() {
  move_adjacent(2, {11, 7});
  if(!reference_model){talk_npc(2,{11,7});return;}
  hero[atk] += Number(std::floor(hero[atk] * 0.03 + 0.5));
  hero[defense] += Number(std::floor(hero[defense] * 0.03 + 0.5));
  ground(2, {11, 7});
}
void Game::talk_mt16_oldman() {
  if(!reference_model){move_adjacent(16,{11,11});talk_npc(16,{11,11});return;}
  go_to(16, {11, 11});
  ++hero[superPotion];
  ground(16, {11, 11});
}
void Game::talk_mt26_princess() {
  move_adjacent(26, {6, 6});
  if(!reference_model){talk_npc(26,{6,6});return;}
  set_flag("营救公主", true);
  ground(26, {6, 6});
  set_block(24, {6, 1}, "whiteWall2", 1);
  for (auto p : {Pos{5, 1}, Pos{7, 1}, Pos{6, 2}, Pos{6, 3}, Pos{6, 4}})
    ground(24, p);
}
void Game::trigger_mt24_to_mt50() {
  go_to(24, {6, 2});
  if(!reference_model)return;
  hero.floor = 50;
  hero.pos = {6, 7};
  visited.set(50);
}
void Game::trigger_mt50_final() {
  move_adjacent(50, {6, 5});
  if(!reference_model){talk_npc(50,{6,5});return;}
  set_flag("50final", true);
  set_flag("与50层小偷对话", true);
  set_block(50, {6, 5}, "redKing", 245);
  go_to(50, {6, 5});
}
void Game::event_mt32_knight() { // Original MT32.js event, including feature-mode zero-HP survival.
  require(hero.floor == 32 && at(hero.pos, 6, 10), "MT32 knight event requires its trigger tile");
  require(floors[32]->events->contains("6,10"), "MT32 knight event already removed");
  if(!reference_model){require(trigger_native_event(32,{6,10}),"MT32 source event unavailable");return;}
  auto b = world->make_block("yellowKnight");
  auto damage = enemy_damage(b, 1ULL << 1);
  if (damage >= hero[hp]) {
    require(flag_truth(NativeFlag::feature) && !flag_truth(NativeFlag::addhp) && hero[atk] > entity(b).def,
            "MT32 event battle death");
    hero[hp] = 0;
  } else {
    hero[hp] -= damage;
    hero[dmg] += damage;
  }
  hero[gold] += entity(b).money * (hero[coin] ? 2 : 1);
  ground(32, {6, 10});
  ground(32, {6, 2});
  ground(32, {6, 9});
  erase_event(32, {6, 10});
}
void Game::clear_mt10_boss_dyn() {
  require(reference_model,"Native MT10 battles require source event execution");
  for (Pos p : std::initializer_list<Pos>{
           {5, 4}, {7, 4}, {6, 4}, {5, 5}, {7, 5}, {5, 6}, {6, 6}, {7, 6}, {6, 1}}) {
    auto b = block(10, p);
    auto &e = entity(b);
    if (e.kind != Kind::enemy)
      continue;
    auto damage = enemy_damage(b);
    require(damage < hero[hp], "MT10 battle death");
    hero[hp] -= damage;
    hero[dmg] += damage;
    hero[gold] += e.money * (hero[coin] ? 2 : 1);
    ground(10, p);
    after_action(10, p, b);
  }
  hero.pos = {6, 1};
}
void Game::action(const Json &a) {
  auto op = a.at("op").get<std::string>();
  int f = a.contains("floor") ? floor_number(a.at("floor")) : hero.floor;
  Pos p = json_pos(a.value("x", Json(hero.pos.x)), a.value("y", Json(hero.pos.y)));
  if(!reference_model && (op=="event_mt33_sword_trap"||op=="oldman_mt2_1000g"||
      op=="thief_mt2_open_35"||op=="thief_mt35_depart"||op=="thief_mt29_open_passage"||
      op=="buy_mt38_yellow_keys"||op=="event_mt40_boss"||op=="change_by_stair"))
    throw Unsupported("Legacy macro requires python-reference mode; use native movement/menu actions: "+op);
  if(op=="native_token")native_token(a.at("token"));
  else if(op=="step")step(a.at("direction"));
  else if(op=="move_directly")move_directly(p);
  else if(op=="talk_npc")talk_npc(f,p,a.value("choice",0));
  else if(op=="set_foreground")set_foreground(f,p,checked_int(a.at("tile")));
  else if (op == "go_to")
    go_to(f, p);
  else if (op == "go_to_decomposed")
    go_to_decomposed(f, p);
  else if (op == "set_auto")
    set_auto(a.at("values"));
  else if (op == "auto_sweep")
    auto_sweep(f);
  else if (op == "buy_shop")
    buy_shop(a.at("kind"), checked_int(a.value("count", Json(1))));
  else if (op == "talk_trader")
    talk_trader(f, p);
  else if (op == "use_pickaxe")
    use_pickaxe(f, p);
  else if (op == "fly")
    fly(f);
  else if (op == "transition")
    transition(f, a.contains("x") ? p : Pos{-1, -1});
  else if (op == "change_by_stair") {
    go_to(hero.floor, p);
    transition(f);
  } else if (op == "center_fly")
    center_fly();
  else if (op == "up_fly")
    up_fly();
  else if (op == "down_fly")
    down_fly();
  else if (op == "use_big_key")
    use_big_key();
  else if (op == "use_earthquake")
    use_earthquake();
  else if (op == "use_bomb")
    use_bomb();
  else if (op == "use_snow")
    use_snow();
  else if (op == "use_super_potion")
    use_super_potion();
  else if (op == "talk_mt2_bless")
    talk_mt2_bless();
  else if (op == "talk_mt16_oldman")
    talk_mt16_oldman();
  else if (op == "talk_mt26_princess")
    talk_mt26_princess();
  else if (op == "trigger_mt24_to_mt50")
    trigger_mt24_to_mt50();
  else if (op == "trigger_mt50_final")
    trigger_mt50_final();
  else if (op == "event_mt32_knight")
    event_mt32_knight();
  else if (op == "clear_mt10_boss_dyn")
    clear_mt10_boss_dyn();
  else if (op == "move_adjacent")
    move_adjacent(f, p);
  else if (op == "event_mt33_sword_trap") {
    require(hero.floor == 33, "MT33 event on wrong floor");
    hero.pos = {10, 5};
    erase_event(33, {10, 5});
    set_block(33, {11, 4}, "yellowWall", 1);
    set_block(33, {10, 4}, "specialDoor", 85);
    set_block(33, {10, 8}, "specialDoor", 85);
  } else if (op == "oldman_mt2_1000g") {
    require(hero.floor == 2, "MT2 event on wrong floor");
    hero[gold] += 1000;
    ground(2, {11, 4});
  } else if (op == "thief_mt2_open_35") {
    require(hero.floor == 2, "MT2 event on wrong floor");
    ground(35, {4, 9});
    ground(2, {10, 11});
  } else if (op == "thief_mt35_depart") {
    require(hero.floor == 35, "MT35 event on wrong floor");
    ground(35, {5, 10});
    for (int i = 0; i < cells; ++i) {
      auto &id = entity(floors[35]->blocks[i]).id;
      if (id == "fakeWall" || id == "fakeWall2")
        ground(35, {i % side, i / side});
    }
  } else if (op == "thief_mt29_open_passage") {
    move_adjacent(29, {6, 2});
    ground(29, {6, 2});
    ground(29, {6, 3});
  } else if (op == "buy_mt38_yellow_keys") {
    require(hero.floor == 38, "MT38 merchant on wrong floor");
    require(hero[gold] >= 200, "Insufficient key merchant gold");
    hero[gold] -= 200;
    hero[yk] += 3;
    ground(38, {5, 2});
  } else if (op == "event_mt40_boss") {
    require(hero.floor == 40, "MT40 event on wrong floor");
    for (Pos q : std::initializer_list<Pos>{{5, 4},
                                            {4, 4},
                                            {3, 4},
                                            {7, 4},
                                            {8, 4},
                                            {9, 4},
                                            {4, 2},
                                            {3, 2},
                                            {2, 2},
                                            {8, 2},
                                            {9, 2},
                                            {10, 2},
                                            {6, 1}}) {
      auto b = block(40, q);
      if (entity(b).kind != Kind::enemy)
        continue;
      auto damage = enemy_damage(b, 1ULL << 1);
      require(damage < hero[hp], "MT40 event battle death");
      hero[hp] -= damage;
      hero[dmg] += damage;
      hero[gold] += entity(b).money;
      ground(40, q);
    }
    for (Pos q : {Pos{2, 2}, Pos{3, 2}, Pos{4, 2}})
      set_block(40, q, "yellowKey", 21);
    for (Pos q : {Pos{8, 2}, Pos{9, 2}, Pos{10, 2}})
      set_block(40, q, "redGem", 27);
    for (Pos q : {Pos{7, 4}, Pos{8, 4}, Pos{9, 4}})
      set_block(40, q, "blueGem", 28);
    set_block(40, {6, 1}, "upFloor", 87);
    ground(40, {6, 7});
    set_flag("402", true);
  } else if (op == "apply_item")
    apply_item(f, a.at("eid"));
  else
    throw Unsupported("Unsupported action: " + op);
}

Game Game::from_reference(std::shared_ptr<const World> source, const Json &ref) {
  auto w = std::make_shared<World>(*source);
  auto state = ref.at("state");
  Json snapshot = {{"floorId", state.at("floor")},
                   {"hero",
                    {{"x", state.at("x")},
                     {"y", state.at("y")},
                     {"hp", state.at("hp")},
                     {"atk", state.at("atk")},
                     {"def", state.at("def")},
                     {"money", state.at("gold")},
                     {"yk", state.at("yk")},
                     {"bk", state.at("bk")},
                     {"rk", state.at("rk")}}}};
  Game g(w, snapshot);
  auto mode = ref.value("model_mode", std::string("python-reference"));
  require(mode == "python-reference" || mode == "native-corrected", "Unknown model mode");
  if(mode=="native-corrected") {
    const auto legacy=state.value("native_event_state",Json::object());
    const bool cached=!legacy.value("hidden",Json::object()).empty()||!legacy.value("removed",Json::object()).empty();
    if(cached)for(const auto &floor:ref.at("floors"))
      require(floor.contains("retained_blocks")&&floor.contains("retained_block_order"),
              "Legacy hidden/removed checkpoint lacks exact retained state; replay from the initial state");
  }
  // Preserve imported hazard caches while restoring; restore the mode after import.
  g.reference_model = true;
  g.mdef_present = state.contains("mdef");
  g.present.reset();
  for (std::size_t i = 0; i < stat_count; ++i) {
    g.hero.values[i] = val(state, stat_names[i]);
    state.erase(stat_names[i]);
  }
  state.erase("floor");
  state.erase("x");
  state.erase("y");
  g.extra_state->value = state;
  for(const auto &key:{"values","game_flags"}) {
    if(ref.contains(key))g.extra_state->value[key]=ref.at(key);
    else if(mode=="native-corrected"&&!g.extra_state->value.contains(key))
      g.extra_state->value[key]=g.world->native_data.value(std::string(key)=="game_flags"?"flags":"values",Json::object());
  }
  g.extra_state->hash = hash_json(g.extra_state->value);
  if(ref.contains("items"))g.items->value=ref.at("items");
  else if(state.contains("items"))g.items->value=state.at("items");
  for(auto &section:g.items->value)if(section.is_object())
    for(const auto &id:{"yellowKey","blueKey","redKey","centerFly","upFly","downFly","earthquake","bomb","bigKey","pickaxe","superPotion","snow","knife","coin","cross"})section.erase(id);
  g.items->hash=hash_json(g.items->value);
  g.flags->value = ref.at("flags");
  g.native_temporaries_possible=true;
  g.flags->hash = hash_json(g.flags->value);
  g.strict = ref.at("strict_live_model");
  auto a = ref.at("auto_flags");
  g.automatic = {truth(a.at("battle")), truth(a.at("wall")),
                 a.at("item") == Json("nopotion") ? 2
                 : truth(a.at("item"))            ? 1
                                                  : 0};
  g.visited.reset();
  for (auto &f : ref.at("visited"))
    g.visited.set(floor_number(f));
  for (auto [key, target] : std::initializer_list<std::pair<std::string, std::set<int> *>>{
           {"blocked_items", &g.blocked_items},
           {"retreat_wizards", &g.retreat_wizards},
           {"no_auto_farm", &g.no_auto_farm}}) {
    target->clear();
    for (auto &p : ref.at(key)) {
      Pos position = json_pos(p[1], p[2]);
      require(position.valid(), "Reference exclusion position out of bounds");
      target->insert(marker(floor_number(p[0]), position));
    }
  }
  auto parse_kind = [](const std::string &k) {
    if (k == "wall")
      return Kind::wall;
    if (k == "terrain")
      return Kind::terrain;
    if (k == "item")
      return Kind::item;
    if (k == "enemy")
      return Kind::enemy;
    if (k == "npc")
      return Kind::npc;
    if (k == "door")
      return Kind::door;
    throw Unsupported("Unknown block kind: " + k);
  };
  std::map<std::pair<std::string, Kind>, std::uint16_t> variants;
  for (auto it = ref.at("floors").begin(); it != ref.at("floors").end(); ++it) {
    int f = floor_number(it.key());
    auto &j = it.value();
    g.present.set(f);
    auto &fs = *g.floors[f];
    auto &fd = w->floors[f];
    fs.blocks.fill({});
    fs.retained.fill({});fs.retained_present.reset();fs.disabled.reset();
    fs.retained_events=std::make_shared<RetainedEvents>();
    fs.block_order.clear();
    fs.grid.fill(0);
    if(j.contains("fgmap")) {
      fs.foreground.fill(0);const auto &fg=j["fgmap"];
      for(int y=0;y<side&&y<int(fg.size());++y)
        for(int x=0;fg[y].is_array()&&x<side&&x<int(fg[y].size());++x)
          fs.foreground[y*side+x]=std::int16_t(number(fg[y][x]));
    }
    if(j.contains("bgmap")) {
      fs.background.fill(0);const auto &bg=j["bgmap"];
      for(int y=0;y<side&&y<int(bg.size());++y)for(int x=0;bg[y].is_array()&&x<side&&x<int(bg[y].size());++x)
        fs.background[y*side+x]=std::int16_t(number(bg[y][x]));
    }
    fs.events = std::make_shared<Json>(j.at("events"));
    fd.ratio = j.at("ratio");
    fd.change_floor = j.value("source_change_floor",j.at("change_floor"));
    for(const auto &key:{"canFlyFrom","canFlyTo","canUseQuickShop","isHide","cannotMoveDirectly","upFloor","downFloor","cannotMove","cannotMoveIn","firstArrive","eachArrive","afterBattle","afterGetItem","afterOpenDoor","beforeBattle","autoEvent"})
      if(j.contains(key))fd.metadata[key]=j.at(key);
    if(j.contains("upFloor")&&j["upFloor"].is_array()&&j["upFloor"].size()==2)fd.up=json_pos(j["upFloor"][0],j["upFloor"][1]);
    if(j.contains("downFloor")&&j["downFloor"].is_array()&&j["downFloor"].size()==2)fd.down=json_pos(j["downFloor"][0],j["downFloor"][1]);
    fd.after_battle.reset();
    fd.after_battle_auto.reset();
    auto &grid = j.at("grid");
    require(grid.size() == side, "Reference importer requires 13x13 floors");
    for (int y = 0; y < side; ++y) {
      require(grid[y].size() == side, "Reference importer requires 13x13 floors");
      for (int x = 0; x < side; ++x)
        fs.grid[y * side + x] = grid[y][x];
    }
    for (auto &row : j.at("blocks")) {
      Pos p = json_pos(row[0], row[1]);
      require(p.valid(), "Reference block position out of bounds");
      Kind kind = parse_kind(row[2]);
      std::string id = row[3];
      auto key = std::make_pair(id, kind);
      std::uint16_t index;
      if (variants.contains(key))
        index = variants.at(key);
      else if (w->by_id.contains(id) && w->entities[w->by_id.at(id)].kind == kind)
        index = w->by_id.at(id);
      else {
        Entity entity;
        if (w->by_id.contains(id))
          entity = w->entities[w->by_id.at(id)];
        entity.id = id;
        entity.kind = kind;
        index = std::uint16_t(w->entities.size());
        w->entities.push_back(entity);
        variants[key] = index;
      }
      fs.blocks[p.index()] = {index, std::int16_t(row[4].get<int>())};
      fs.block_order.push_back(p);
    }
    if(j.contains("block_order")) {
      fs.block_order.clear();std::bitset<cells> used;
      for(const auto &row:j["block_order"]) {
        Pos p=json_pos(row.at(0),row.at(1));require(p.valid()&&!used[p.index()]&&fs.blocks[p.index()].entity,"Invalid native block order");
        used.set(p.index());fs.block_order.push_back(p);
      }
      for(int i=0;i<cells;++i)require(bool(fs.blocks[i].entity)==used[i],"Incomplete native block order");
    }
    for (auto [key, target] : std::initializer_list<std::pair<std::string, std::bitset<cells> *>>{
             {"after_battle", &fd.after_battle}, {"after_battle_auto", &fd.after_battle_auto}})
      for (auto &item : j.at(key)) {
        auto str = item.get<std::string>();
        auto comma = str.find(',');
        Pos p{std::stoi(str.substr(0, comma)), std::stoi(str.substr(comma + 1))};
        require(p.valid(), "Reference event position out of bounds");
        target->set(p.index());
      }
    fs.spell_enemies.clear();
    for (auto &p : j.at("spell_enemies")) {
      Pos q = json_pos(p[0], p[1]);
      require(q.valid(), "Reference spell position out of bounds");
      fs.spell_enemies.push_back(q);
    }
    g.rehash_floor(f);
  }
  auto pending = ref.at("pending");
  g.defer_after = pending.at("_defer_after_action");
  g.capturing = pending.at("_capturing");
  g.pending_auto = !pending.at("_pending_auto_before").is_null();
  for (auto [key, target] : std::initializer_list<std::pair<std::string, std::vector<Effect> *>>{
           {"_captured", &g.captured}, {"_deferred_after_effects", &g.deferred}}) {
    target->clear();
    for (auto &e : pending.at(key)) {
      int f = floor_number(e[1]);
      Pos p = json_pos(e[2][0], e[2][1]);
      require(p.valid(), "Reference effect position out of bounds");
      require(e[0] == "set_ground" || e[0] == "set_block", "Unknown deferred effect");
      Block b{};
      if (e[0] == "set_block")
        b = w->make_block(e[4], e[5]);
      target->push_back({f, p, b});
    }
    pending.erase(key);
  }
  pending.erase("_defer_after_action");
  pending.erase("_capturing");
  g.pending_records = pending;
  if(mode=="native-corrected")for(auto it=ref.at("floors").begin();it!=ref.at("floors").end();++it) {
    int f=floor_number(it.key());const auto &j=it.value();auto &fs=*g.floors[f];
    auto previous_order=fs.block_order;fs.block_order.clear();
    if(j.contains("retained_blocks")) {
      require(j["retained_blocks"].is_object(),"Invalid retained blocks");
      fs.blocks.fill({});fs.grid.fill(0);fs.events=std::make_shared<Json>(Json::object());
      for(auto r=j["retained_blocks"].begin();r!=j["retained_blocks"].end();++r) {
        auto comma=r.key().find(',');require(comma!=std::string::npos,"Invalid retained coordinate");
        Pos p{std::stoi(r.key().substr(0,comma)),std::stoi(r.key().substr(comma+1))};
        require(p.valid(),"Retained block outside floor");int tile=checked_int(r.value().at("id"));
        Block b{};if(tile){require(w->tiles.contains(tile),"Unknown retained tile");b=w->tiles.at(tile);}
        require(r.value().at("event").is_object(),"Invalid retained block event");
        fs.retained[p.index()]=b;fs.retained_present.set(p.index());fs.disabled.set(p.index(),r.value().value("disable",false));
        fs.retained_events->set(p,r.value().at("event"));
      }
      if(j.contains("retained_block_order")) {
        std::bitset<cells> used;
        for(const auto &row:j["retained_block_order"]) {
          Pos p=json_pos(row.at(0),row.at(1));
          require(p.valid()&&fs.retained_present[p.index()]&&!used[p.index()],"Invalid retained block order");
          used.set(p.index());fs.block_order.push_back(p);
        }
        require(used==fs.retained_present,"Incomplete retained block order");
      } else for(int i=0;i<cells;++i)if(fs.retained_present[i])fs.block_order.push_back({i%side,i/side});
    } else {
      for(int i=0;i<cells;++i) {
        Pos p{i%side,i/side};auto b=fs.blocks[i];auto event=g.effective_block_event(f,p);
        if(!b.entity&&!truth(event.value("trigger",Json())))continue;
        fs.retained[i]=b;fs.retained_present.set(i);fs.retained_events->set(p,std::move(event));
      }
      for(auto p:previous_order)if(fs.retained_present[p.index()])fs.block_order.push_back(p);
      for(int i=0;i<cells;++i)if(fs.retained_present[i]&&std::find(fs.block_order.begin(),fs.block_order.end(),Pos{i%side,i/side})==fs.block_order.end())
        fs.block_order.push_back({i%side,i/side});
    }
    for(auto p:fs.block_order)g.sync_retained_cell(f,p);
  }
  g.reference_model = mode == "python-reference";
  if(!g.reference_model) {
    if(w->first_floor==0&&!ref.at("floors").contains("MT0"))g.present.set(0);
    for(int f=w->first_floor;f<=50;++f)g.rehash_floor(f);
  }
  // Import changes this private World after Game construction. Rebuild the
  // immutable descriptors only after all imported metadata/entities are final.
  w->prepared.store(nullptr);g.prepared_native=prepare_native_world(w);
  for(int f=w->first_floor;f<floor_count;++f)g.rebuild_movement_floor(f);
  return g;
}
} // namespace mota
