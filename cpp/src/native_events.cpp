// Source-driven event interpreter for the pinned 2026 tower 51 project.
// Rendering/audio commands have no gameplay effect and are explicitly listed.
// Unknown commands and function bodies throw: no snapshot-derived repairs.
#include "mota/native_events.hpp"
#include "mota/native_expression.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <optional>
#include <regex>

namespace mota {
namespace {
constexpr const char *state_key = "native_event_state";
bool truthy(const Json &v) {
  if (v.is_null()) return false;
  if (v.is_boolean()) return v.get<bool>();
  if (v.is_number()) return v.get<double>() != 0;
  if (v.is_string()) return !v.get<std::string>().empty();
  return true;
}
Number number(const Json &v) {
  if (v.is_null()) return 0;
  if (v.is_boolean()) return v.get<bool>() ? 1 : 0;
  if (v.is_number()) return Number(v.get<double>());
  throw Unsupported("Event expected number: " + v.dump());
}
std::string string_value(const Json &v) {
  return v.is_string() ? v.get<std::string>() : v.dump();
}
Json actions(const Json &value) {
  if (value.is_null()) return Json::array();
  if (value.is_array()) return value;
  if (value.is_object() && value.contains("data") && !value.contains("type")) return value.at("data");
  return Json::array({value});
}
bool actions_empty(const Json &value) {
  if(value.is_null())return true;
  if(value.is_array())return value.empty();
  if(value.is_object()&&value.contains("data")&&!value.contains("type"))return value.at("data").empty();
  return false;
}
// events.precompile rewrites expression fields before cloning the execution
// list. The prefixes below are the ones supported by this interpreter; switch
// references remain contextual, as in utils.replaceValue.
std::string replace_flag_reference(std::string text,const std::string &prefix) {
  std::size_t offset=0;
  while((offset=text.find(prefix,offset))!=std::string::npos) {
    auto begin=offset+prefix.size(),end=begin;
    while(end<text.size()) {
      unsigned char byte=text[end];unsigned code=byte;std::size_t width=1;
      if(byte>=0xc0) {
        width=byte<0xe0?2:byte<0xf0?3:4;code=byte&((1u<<(7-width))-1);
        if(end+width>text.size())break;
        for(std::size_t i=1;i<width;++i)code=(code<<6)|(static_cast<unsigned char>(text[end+i])&63);
      }
      bool allowed=(code>='a'&&code<='z')||(code>='A'&&code<='Z')||(code>='0'&&code<='9')||code=='_'||
        (code>=0x4e00&&code<=0x9fcc)||(code>=0x3040&&code<=0x30ff)||
        (code>=0x2160&&code<=0x216b)||(code>=0x391&&code<=0x3c9);
      if(!allowed)break;end+=width;
    }
    if(end==begin){offset=begin;continue;}
    auto replacement="core.getFlag('"+text.substr(begin,end-begin)+"', 0)";
    text.replace(offset,end-offset,replacement);offset+=replacement.size();
  }
  return text;
}
void precompile_value(Json &value) {
  if(!value.is_string())return;
  auto text=value.get<std::string>();
  if(text.find(':')==std::string::npos&&text.find("：")==std::string::npos)return;
  struct Rule {const char *prefix;std::regex pattern;std::string replacement;};
  static const std::vector<Rule> rules={
    {"status:",std::regex("status:([a-zA-Z0-9_]+)"),"core.getStatus('$1')"},
    {"item:",std::regex("item:([a-zA-Z0-9_]+)"),"core.itemCount('$1')"},
    {"blockId:",std::regex("blockId:([0-9]+),([0-9]+)"),"core.getBlockId($1, $2)"},
    {"blockCls:",std::regex("blockCls:([0-9]+),([0-9]+)"),"core.getBlockCls($1, $2)"},
    {"temp:",std::regex("temp:([a-zA-Z0-9_]+)"),"core.getFlag('@temp@$1', 0)"}
  };
  for(const auto &[prefix,pattern,replacement]:rules)if(text.find(prefix)!=std::string::npos)text=std::regex_replace(text,pattern,replacement);
  value=replace_flag_reference(replace_flag_reference(text,"flag:"),"flag：");
}
void precompile_array(Json &value) {
  if(value.is_array())for(auto &entry:value)precompile_array(entry);
  else precompile_value(value);
}
void precompile_text(Json &value) {
  if(!value.is_string())return;auto text=value.get<std::string>();std::size_t offset=0;
  while((offset=text.find("${",offset))!=std::string::npos) {
    auto end=text.find('}',offset+2);if(end==std::string::npos)break;
    auto body=text.substr(offset+2,end-offset-2);
    if(body.find_first_of("\r\n")!=std::string::npos){offset=end+1;continue;}
    Json compiled=body;precompile_value(compiled);auto replacement=compiled.get<std::string>();
    text.replace(offset+2,end-offset-2,replacement);offset+=replacement.size()+3;
  }
  value=std::move(text);
}
void precompile_event(Json &data) {
  if(data.is_string()){precompile_text(data);return;}
  if(data.is_array()){for(auto &entry:data)precompile_event(entry);return;}
  if(!data.is_object()||!data.contains("type"))return;
  const auto type=data.value("type",std::string());
  auto expression=[&](const char *key){if(data.contains(key))precompile_value(data[key]);};
  auto nested=[&](const char *key){if(data.contains(key))precompile_event(data[key]);};
  auto array=[&](const char *key){if(data.contains(key))precompile_array(data[key]);};
  if(type=="if"){expression("condition");nested("true");nested("false");}
  if(type=="switch") {
    expression("condition");for(auto &entry:data.at("caseList")) {
      if(entry.contains("case"))precompile_value(entry["case"]);
      if(entry.contains("action"))precompile_event(entry["action"]);
    }
  }
  if(type=="choices"&&data.contains("choices"))for(auto &entry:data["choices"]) {
    if(entry.contains("condition"))precompile_value(entry["condition"]);
    if(entry.contains("text"))precompile_text(entry["text"]);
    if(entry.contains("action"))precompile_event(entry["action"]);
  }
  if(type=="confirm"){nested("yes");nested("no");}
  if(type=="for"){expression("from");expression("to");expression("step");nested("data");}
  if(type=="while"||type=="dowhile"){expression("condition");nested("data");}
  if(type=="forEach")nested("data");
  if(type=="jump"||type=="moveEnemyOnPoint"){array("from");array("to");array("dxy");}
  if(type=="showImage"){array("sloc");array("loc");}
  if(type=="moveImage")array("to");
  if(type=="rotateImage"||type=="scaleImage")array("center");
  if(type=="fillPolygon"||type=="strokePolygon")array("nodes");
  if(type=="previewUI")nested("action");
  if(type=="wait"&&data.contains("data"))for(auto &entry:data["data"]) {
    for(const auto *key:{"px","py","condition"})if(entry.contains(key))precompile_array(entry[key]);
    if(entry.contains("action"))precompile_event(entry["action"]);
  }
  static const std::set<std::string> texts={"text","autoText","scrollText","tip","textImage","input","input2","choices","confirm","fillText","fillBoldText","drawTextContent"};
  static const std::set<std::string> locs={"show","hide","setBlock","setBlockOpacity","showFloorImg","hideFloorImg","showBgFgMap","hideBgFgMap","setBgFgBlock","animate","setViewport","move","jumoHero","changeFloor","changePos","showTextImage","showGif","openDoor","closeDoor","battle","trigger","insert","setEnemyOnPoint","resetEnemyOnPoint"};
  static const std::set<std::string> values={"setValue","setEnemy","setEnemyOnPoint","setEquip","setFloor","setGlobalValue"};
  if(texts.contains(type)&&data.contains("text"))precompile_text(data["text"]);
  if(locs.contains(type))array("loc");
  if(values.contains(type))expression("value");
}
std::optional<Stat> stat_key(const std::string &s) {
  if (s == "hp") return hp;
  if (s == "atk") return atk;
  if (s == "def") return defense;
  if (s == "money") return gold;
  if (s == "mdef") return mdef;
  return {};
}
Pos direction_delta(const std::string &d) {
  if (d == "up") return {0,-1};
  if (d == "right") return {1,0};
  if (d == "down") return {0,1};
  if (d == "left") return {-1,0};
  if (d == "leftup") return {-1,-1};
  if (d == "leftdown") return {-1,1};
  if (d == "rightup") return {1,-1};
  if (d == "rightdown") return {1,1};
  throw Unsupported("Unsupported event movement direction: " + d);
}
struct VM;
thread_local VM *active = nullptr;
void clear_temporaries(Game &g) {
  if(!g.native_temporaries_possible)return;
  std::vector<std::string> temporary;
  const auto &flags=g.flags->value.get_ref<const Json::object_t&>();
  for(auto it=flags.lower_bound("@temp@");it!=flags.end()&&it->first.starts_with("@temp@");++it)temporary.push_back(it->first);
  for(auto it=flags.lower_bound("arg");it!=flags.end()&&it->first.starts_with("arg");++it) {
    const auto &name=it->first;
    if(name.size()>3&&std::all_of(name.begin()+3,name.end(),[](unsigned char c){return std::isdigit(c);}))temporary.push_back(name);
  }
  for(const auto &name:temporary)g.set_flag(name,nullptr);
  g.native_temporaries_possible=false;
}
bool same_native_value(const Json &a,const Json &b) {
  if(a.type()!=b.type())return false;
  if(a.is_object()||a.is_array()) {
    if(a.size()!=b.size())return false;
    auto x=a.begin(),y=b.begin();
    for(;x!=a.end();++x,++y)if((a.is_object()&&x.key()!=y.key())||!same_native_value(*x,*y))return false;
    return true;
  }
  if(a.is_number_float())return std::bit_cast<std::uint64_t>(a.get<double>())==std::bit_cast<std::uint64_t>(b.get<double>());
  return a==b;
}
// Immutable instruction arrays survive input boundaries. Only cursors and a
// small vector of frames are copied when a branch resumes a pending event.
struct NativeFrame {
  struct Context {
    int floor;Pos position;std::shared_ptr<const Json> loop;
    bool operator==(const Context &) const = default;
  };
  std::optional<Context> context;
  static std::shared_ptr<Json> empty_metadata() {
    static const auto empty=std::make_shared<Json>(Json::object());return empty;
  }
  struct Segment {
    std::shared_ptr<const Json> program;
    std::size_t cursor=0;
    bool operator==(const Segment &) const = default;
  };
  std::shared_ptr<Json> metadata;
  std::vector<Segment> segments;
  std::shared_ptr<const Json> repeat_program;
  bool compact=false;
  explicit NativeFrame(Json value):metadata(std::make_shared<Json>(std::move(value))) {
    if(metadata->is_object()&&metadata->contains("todo")&&metadata->at("todo").is_array()) {
      auto program=std::move((*metadata)["todo"]);metadata->erase("todo");
      set_program(std::make_shared<const Json>(std::move(program)));
    }
  }
  NativeFrame(int floor,Pos position,const Json &loop,const std::shared_ptr<const Json> &owner,std::shared_ptr<const Json> program):metadata(empty_metadata()) {
    std::shared_ptr<const Json> loop_owner;
    if(!loop.is_null())loop_owner=owner?std::shared_ptr<const Json>(owner,&loop):std::make_shared<const Json>(loop);
    context=Context{floor,position,std::move(loop_owner)};set_program(std::move(program));
  }
  int event_floor(){return context?context->floor:field("floor").get<int>();}
  Pos event_position(){return context?context->position:Pos{field("x"),field("y")};}
  const Json &event_loop(){static const Json none;return context?(context->loop?*context->loop:none):field("loop");}
  Json &mutate() {if(metadata.use_count()!=1)metadata=std::make_shared<Json>(*metadata);return *metadata;}
  const Json &field(const char *name) {
    auto found=metadata->find(name);return found==metadata->end()?mutate()[name]:*found;
  }
  void set_program(std::shared_ptr<const Json> program) {
    segments.clear();compact=program->is_array();
    if(compact) {
      if(metadata->contains("todo"))mutate().erase("todo");
      if(!program->empty())segments.push_back({std::move(program),0});
    } else mutate()["todo"]=*program;
  }
  bool empty() {return compact?segments.empty():field("todo").empty();}
  Json remaining() const {
    if(!compact)return metadata->contains("todo")?metadata->at("todo"):Json();
    Json todo=Json::array();auto &out=todo.get_ref<Json::array_t&>();std::size_t size=0;
    for(const auto &segment:segments)size+=segment.program->size()-segment.cursor;
    out.reserve(size);
    for(auto it=segments.rbegin();it!=segments.rend();++it) {
      const auto &program=it->program->get_ref<const Json::array_t&>();
      out.insert(out.end(),program.begin()+it->cursor,program.end());
    }
    return todo;
  }
  Json export_json() const {
    auto out=*metadata;
    if(context){out["floor"]=context->floor;out["x"]=context->position.x;out["y"]=context->position.y;out["loop"]=context->loop?*context->loop:Json();}
    if(compact)out["todo"]=remaining();return out;
  }
  void prepend(std::shared_ptr<const Json> program) {
    if(compact&&program->is_array()) {
      if(!program->empty())segments.push_back({std::move(program),0});
    } else {
      auto out=*program,todo=remaining();out.insert(out.end(),todo.begin(),todo.end());
      set_program(std::make_shared<const Json>(std::move(out)));
    }
  }
  void repeat(const Json &loop) {
    if(!repeat_program) {
      const auto &data=loop.at("data");
      // Repetition normalizes the loop body but does not precompile it again.
      if(data.is_array())repeat_program=context?std::shared_ptr<const Json>(context->loop,&data):std::shared_ptr<const Json>(metadata,&data);
      else repeat_program=std::make_shared<const Json>(actions(data));
    }
    set_program(repeat_program);
  }
  bool equivalent(const NativeFrame &other) const {
    return metadata==other.metadata&&context==other.context&&compact==other.compact&&segments==other.segments;
  }
};
struct NativeChoice {
  Json value;
  std::shared_ptr<const Json> program;
  std::vector<const Json *> options;
};
} // namespace
struct NativeContinuation {
  std::shared_ptr<Json> metadata;
  std::vector<NativeFrame> stack;
  std::shared_ptr<const NativeChoice> choice;
  bool compact_stack=false;
  bool frames_empty() const{return compact_stack?stack.empty():metadata->at("frames").empty();}
  Json export_json() const {
    auto state=*metadata;
    if(compact_stack) {
      auto frames=Json::array();auto &out=frames.get_ref<Json::array_t&>();out.reserve(stack.size());
      for(const auto &frame:stack)out.push_back(frame.export_json());state["frames"]=std::move(frames);
    }
    if(choice)state["choice"]=choice->value;
    return state;
  }
};
namespace {
struct VM : NativeContinuation {
  Game &g;
  std::shared_ptr<const NativeContinuation> original;
  std::shared_ptr<const Json> executing_program;
  int f;
  Pos p;
  std::size_t operations = 0;
  explicit VM(Game &game) : g(game), original(game.native_continuation), f(game.hero.floor), p(game.hero.pos) {
    if(original){static_cast<NativeContinuation &>(*this)=*original;return;}
    auto state=game.extra_state->value.value(state_key, Json::object());
    if (!state.contains("frames")) state["frames"] = Json::array();
    // The floor state now owns disabled cells, payloads, and iteration order.
    state.erase("hidden");state.erase("removed");
    if(state["frames"].is_array()) {
      compact_stack=true;auto &frames=state["frames"].get_ref<Json::array_t&>();stack.reserve(frames.size());
      for(auto &frame:frames)stack.emplace_back(std::move(frame));state.erase("frames");
    }
    if(state.contains("choice")) {
      choice=std::make_shared<const NativeChoice>(NativeChoice{std::move(state["choice"]),{}, {}});state.erase("choice");
    }
    metadata=std::make_shared<Json>(std::move(state));
  }
  Json &mutate_metadata(){if(metadata.use_count()!=1)metadata=std::make_shared<Json>(*metadata);return *metadata;}
  void set_metadata(const std::string &key,Json value) {
    auto it=metadata->find(key);if(it!=metadata->end()&&same_native_value(*it,value))return;
    mutate_metadata()[key]=std::move(value);
  }
  bool unchanged() const {
    if(!original||original!=g.native_continuation||metadata!=original->metadata||choice!=original->choice||compact_stack!=original->compact_stack||stack.size()!=original->stack.size())return false;
    for(std::size_t i=0;i<stack.size();++i)if(!stack[i].equivalent(original->stack[i]))return false;
    return true;
  }
  void save() {
    // Canonical JSON remains observable and unchanged throughout execution.
    // Skip serialization only when the complete continuation is unchanged.
    if(unchanged())return;
    if(!g.defers_native_serialization())g.set_extra(state_key,NativeContinuation::export_json());
    g.native_continuation=std::make_shared<const NativeContinuation>(static_cast<const NativeContinuation &>(*this));
  }
  std::size_t frame_count() const{return compact_stack?stack.size():metadata->at("frames").size();}
  void clear_frames(){compact_stack=true;stack.clear();if(metadata->contains("frames"))mutate_metadata().erase("frames");}
  void pop_frame(){if(compact_stack)stack.pop_back();else mutate_metadata()["frames"].erase(metadata->at("frames").end()-1);}
  NativeFrame &frame(){if(!compact_stack)throw Unsupported("Native VM frames must be an array");return stack.back();}
  void set_choice(const Json &a) {
    std::vector<const Json *> options;
    for(const auto &entry:a.at("choices")) {
      if(entry.value("_disabled",false))continue;
      if(entry.contains("condition")&&!entry["condition"].is_null()&&entry["condition"]!=""&&!truthy(eval(entry["condition"])))continue;
      options.push_back(&entry);
    }
    if(options.empty())return;
    const auto text=a.value("text",std::string());
    auto previous=original?original->choice:nullptr;
    if(previous) {
      const auto &old=previous->value;
      bool equal=old.is_object()&&old.size()==5&&old.contains("options")&&old["options"].is_array()&&old["options"].size()==options.size();
      for(const auto &[key,value]:std::vector<std::pair<std::string,Json>>{{"floor",f},{"x",p.x},{"y",p.y},{"text",text}})
        equal=equal&&old.contains(key)&&same_native_value(old[key],value);
      if(equal)for(std::size_t i=0;i<options.size();++i) {
        if(previous->program==executing_program&&previous->options.size()==options.size()&&previous->options[i]==options[i])continue;
        if(!same_native_value(old["options"][i],*options[i])){equal=false;break;}
      }
      if(equal){choice=std::move(previous);return;}
    }
    Json values=Json::array();for(const auto *entry:options)values.push_back(*entry);
    choice=std::make_shared<const NativeChoice>(NativeChoice{
      {{"options",std::move(values)},{"floor",f},{"x",p.x},{"y",p.y},{"text",text}},executing_program,std::move(options)});
  }
  void push(const Json &data, int floor, Pos position, const Json &loop = nullptr) {
    std::shared_ptr<const Json> program;
    const auto &prepared=g.prepared_native;
    if(prepared&&prepared->matches(g.world)) {
      const auto found=prepared->event_programs.find(&data);
      if(found!=prepared->event_programs.end())program=found->second;
    }
    if(!program) {auto compiled=actions(data);precompile_event(compiled);program=std::make_shared<const Json>(std::move(compiled));}
    if(!compact_stack) {
      Json value={{"floor",floor},{"x",position.x},{"y",position.y},{"loop",loop}};
      value["todo"]=*program;auto &frames=mutate_metadata()["frames"];frames.push_back(std::move(value));
      for(auto &frame:frames)stack.emplace_back(std::move(frame));compact_stack=true;mutate_metadata().erase("frames");return;
    }
    stack.emplace_back(floor,position,loop,executing_program,std::move(program));
  }
  void prepend(const Json &data) {
    frame().prepend(std::make_shared<const Json>(actions(data)));
  }
  void prepend_borrowed(const Json &data) {
    if(executing_program&&data.is_array())frame().prepend(std::shared_ptr<const Json>(executing_program,&data));
    else prepend(data);
  }
  Json resolve(const std::string &name, const std::vector<Json> &args) {
    if (args.empty()) {
      if (name.starts_with("flag:")) return g.flags->value.value(name.substr(5), Json(0));
      if (name.starts_with("item:")) return g.item_count(name.substr(5));
      if (name.starts_with("temp:")) return g.flags->value.value("@temp@"+name.substr(5),Json(0));
      if (name.starts_with("switch:")) return g.flags->value.value(floor_id(f)+"@"+std::to_string(p.x)+"@"+std::to_string(p.y)+"@"+name.substr(7),Json(0));
      if (name.starts_with("status:")) {
        auto key=name.substr(7);
        if (auto s=stat_key(key)) return g.hero[*s];
        if (key=="x") return g.hero.pos.x;
        if (key=="y") return g.hero.pos.y;
        if (key=="direction") return g.extra_state->value.value("direction",Json("down"));
        return g.extra_state->value.value(key,Json(0));
      }
      if (name.starts_with("blockId:")) {
        auto key=name.substr(8); auto comma=key.find(',');
        if(comma==std::string::npos) throw Unsupported("Invalid blockId reference");
        Pos q{std::stoi(key.substr(0,comma)),std::stoi(key.substr(comma+1))};
        return g.block_present(g.hero.floor,q)?g.effective_block_event(g.hero.floor,q).value("id",Json("none")):Json(nullptr);
      }
      if(name=="core.status.hero.flags.__auto__.battle") return g.automatic.battle;
      if(name=="core.status.hero.flags.__auto__.wall") return g.automatic.wall;
      if(name=="core.status.hero.flags.__auto__.item") return g.automatic.item==2?Json("nopotion"):Json(g.automatic.item==1);
      if(name=="core.status.floorId") return floor_id(g.hero.floor);
      if(name=="main.levelChoose.length") return 0;
    }
    auto argstr=[&](std::size_t i){return string_value(args.at(i));};
    auto argnum=[&](std::size_t i){return number(args.at(i));};
    if(name=="core.getFlag") return g.flags->value.value(argstr(0),args.size()>1?args[1]:Json(0));
    if(name=="core.hasFlag") return g.flag_truth(argstr(0));
    if(name=="core.itemCount") return g.item_count(argstr(0));
    if(name=="core.getStatus")return resolve("status:"+argstr(0),{});
    if(name=="core.hasItem") return g.item_count(argstr(0))>0;
    if(name=="core.hasVisitedFloor") return g.has_visited(floor_number(argstr(0)));
    if(name=="core.isReplaying") return true;
    if(name=="core.getHeroLoc") {
      auto key=argstr(0); if(key=="x")return g.hero.pos.x; if(key=="y")return g.hero.pos.y;
      if(key=="direction") return g.extra_state->value.value("direction",Json("down"));
    }
    if(name=="core.getBlockId"||name=="core.getBlockCls"||name=="core.getBlock") {
      int floor=args.size()>2&&!args[2].is_null()?floor_number(argstr(2)):g.hero.floor;
      Pos q{int(argnum(0)),int(argnum(1))};auto b=g.block(floor,q);
      if(!g.block_present(floor,q))return nullptr;
      auto event=g.effective_block_event(floor,q);
      if(name=="core.getBlockId")return event.value("id",Json("none"));
      if(name=="core.getBlock")return Json{{"id",b.tile},{"x",q.x},{"y",q.y},{"event",event}};
      return event.value("cls",Json("terrains"));
    }
    if(name=="core.searchBlock") {
      int floor=args.size()>1?floor_number(argstr(1)):g.hero.floor;
      Json a=Json::array();
      for(int i=0;i<cells;++i)if(g.entity(g.floors[floor]->blocks[i]).id==argstr(0))a.push_back({{"x",i%side},{"y",i/side}});
      return a;
    }
    if(name=="core.canBattle")return g.enemy_damage(argstr(0))<g.hero[hp];
    if(name=="core.getEnemyInfo") {
      auto b=g.world->make_block(argstr(0)); auto &e=g.entity(b);
      Json result={{"hp",e.hp},{"atk",e.atk},{"def",e.def},{"money",e.money}};
      auto all=g.extra_state->value.value("enemy_overrides",Json::object());
      if(all.contains(e.id))for(auto it=all[e.id].begin();it!=all[e.id].end();++it)result[it.key()]=it.value();
      return result;
    }
    if(name=="core.calValue")return eval(args.at(0));
    throw Unsupported("Unsupported native expression reference: "+name);
  }
  Json eval(const Json &v) {
    if(!v.is_string())return v;
    return native_eval(v.get<std::string>(),[&](const std::string &n,const std::vector<Json>&a){return resolve(n,a);});
  }
  void assign(const std::string &name, Json v, const std::string &op="=") {
    if(op!="=") {
      auto old=resolve(name,{});
      if(op=="+=")v=number(old)+number(v);
      else if(op=="-=")v=number(old)-number(v);
      else if(op=="*=")v=number(old)*number(v);
      else if(op=="/=") {if(number(v)==0)throw RouteError("Event division by zero");v=number(old)/number(v);}
      else if(op=="%=") {if(number(v)==0)throw RouteError("Event modulo by zero");v=number(old)%number(v);}
      else throw Unsupported("Unsupported event assignment operator: "+op);
    }
    if(name.starts_with("flag:")) {
      auto key=name.substr(5);g.set_flag(key,v);
      if(key=="times1")g.hero[times1]=number(v);
      if(key=="魔法免疫")g.hero[magicImmune]=truthy(v);
      return;
    }
    if(name.starts_with("item:")) {
      auto key=name.substr(5);auto count=g.item_count(key);auto value=number(v);
      // events._setValue_setItem uses getItem for increases and setItem for
      // reductions, so permanent-item acquisition still follows its cap.
      if(value>count)g.add_inventory(key,value-count);else g.set_inventory(key,value);
      return;
    }
    if(name.starts_with("temp:")){g.set_flag("@temp@"+name.substr(5),v);return;}
    if(name.starts_with("switch:")){g.set_flag(floor_id(f)+"@"+std::to_string(p.x)+"@"+std::to_string(p.y)+"@"+name.substr(7),v);return;}
    if(name.starts_with("status:")) {
      auto key=name.substr(7);if(auto s=stat_key(key)){g.hero[*s]=number(v);return;}
      if(key=="x"){g.hero.pos.x=int(number(v));return;}if(key=="y"){g.hero.pos.y=int(number(v));return;}
      g.set_extra(key,v);return;
    }
    throw Unsupported("Unsupported event assignment target: "+name);
  }
  int event_floor(const Json &a) {
    if(!a.contains("floorId")||a["floorId"].is_null()||a["floorId"]=="")return g.hero.floor;
    auto id=a["floorId"].get<std::string>();
    if(id==":next"||id==":before") {
      const int step=id==":next"?1:-1;
      for(int target=g.hero.floor+step;target>=g.world->first_floor&&target<floor_count;target+=step)
        if(!truthy(g.world->floors[target].metadata.value("isHide",Json(false))))return target;
      throw RouteError("Relative event floor has no visible destination");
    }
    return floor_number(id);
  }
  std::vector<Pos> locations(const Json &a) {
    if(!a.contains("loc"))return {p};
    auto loc=a["loc"];if(!loc.is_array())throw Unsupported("Invalid native event loc");
    if(loc.size()==2&&!loc[0].is_array())return {{int(number(eval(loc[0]))),int(number(eval(loc[1])))}};
    std::vector<Pos> result;for(auto &q:loc)result.push_back({int(number(eval(q.at(0)))),int(number(eval(q.at(1))))});return result;
  }
  Block block_value(const Json &v) {
    if(v.is_number_integer()) {auto tile=checked_int(v);return tile==0?Block{}:g.world->tiles.at(tile);}
    auto s=v.get<std::string>();
    if(s=="0")return {};
    if(!s.empty()&&std::all_of(s.begin(),s.end(),[](unsigned char c){return std::isdigit(c);}))return g.world->tiles.at(std::stoi(s));
    return g.world->make_block(s);
  }
  bool is_disabled(int floor,Pos q) {
    return g.block_disabled(floor,q);
  }
  void hide(int floor,Pos q,bool remove,bool destruct=false) {
    if(remove)g.remove_block(floor,q,destruct);else g.hide_block(floor,q);
  }
  void show(int floor,Pos q) {
    g.show_block(floor,q);
  }
  void execute_function(const std::string &source);
  void execute(const Json &a);
  void drain() {
    if(g.native_auto_active){save();return;}
    if(active==this)return;
    if(active && &active->g==&g)throw RouteError("Recursive native VM mismatch");
    auto previous=active;active=this;
    try {
      while(!frames_empty()&&!choice) {
        if(++operations>200000)throw RouteError("Native event operation limit (probable nonterminating source loop)");
        auto &current=frame();f=current.event_floor();p=current.event_position();
        if(current.empty()) {
          const auto &loop=current.event_loop();
          bool repeat=false;
          if(!loop.is_null()) {
            if(loop.value("type",std::string())=="for") {
              auto name=loop.at("name").get<std::string>();auto letter=name.substr(5);
              auto end=eval(g.flags->value.at("@temp@for-to@"+letter));
              auto step=eval(g.flags->value.at("@temp@for-step@"+letter));
              if(end.is_number()&&step.is_number()) {
                auto increment=number(step);
                if(increment==0)repeat=true;
                else {
                  auto current=number(resolve(name,{}))+increment;assign(name,current);
                  repeat=increment>0?current<=number(end):current>=number(end);
                }
              }
            } else repeat=truthy(eval(loop.at("condition")));
          }
          if(repeat){current.repeat(loop);continue;}
          if(current.metadata->contains("auto_symbol")) {
            auto symbol=current.metadata->at("auto_symbol");auto executing=g.flags->value.value("__aei__",Json::array());
            auto result=Json::array();for(const auto &v:executing)if(v!=symbol)result.push_back(v);g.set_flag("__aei__",result);
          }
          pop_frame();continue;
        }
        if(current.compact) {
          auto &segment=current.segments.back();const auto program=segment.program;const auto index=segment.cursor++;
          if(segment.cursor==program->size())current.segments.pop_back();
          executing_program=program;execute((*program)[index]);executing_program.reset();
        } else {
          auto &value=current.mutate();auto action=std::move(value["todo"][0]);value["todo"].erase(value["todo"].begin());execute(action);
        }
      }
      if(frames_empty()&&!choice) {
        clear_temporaries(g);
      }
      active=previous;save();
    }catch(...){active=previous;save();throw;}
  }
};
std::uint64_t function_hash(const std::string &s) {
  std::uint64_t h=14695981039346656037ULL;
  for(unsigned char c:s){h^=c;h*=1099511628211ULL;}
  return h;
}
void VM::execute_function(const std::string &source) {
  // These hashes identify complete pinned source bodies (including whitespace),
  // not checkpoints. A changed source body is unsupported until re-audited.
  switch(function_hash(source)) {
    case 0x8802bf5fe2330650ULL: // route.push('help'): input is already the original help token.
    case 0x160dd6d3d88aabb2ULL: // showAltarSummary: display only.
    case 0x46ef99a17b25b78fULL: // checkBgm.
    case 0x9f5c8e50510968b8ULL: // MT3 rotating damage sprites.
    case 0x85e862bbac92a720ULL: // MT24 floor title animation.
    case 0xab6c4e65cbaff7b4ULL: // MT20 battle sprite.
    case 0x3cc98e3a89a1c842ULL: // MT1 help-icon highlight (returns while replaying).
      return;
    case 0xc750c83ef68625b8ULL:
      if(!g.flags->value.contains("__auto__"))g.set_auto({{"battle",false},{"item",false},{"wall",false}});
      return;
    case 0x78b0e0091f3f8207ULL:g.set_auto({{"battle",true}});return;
    case 0xd9ea223352e2468cULL:g.set_auto({{"battle",false}});return;
    case 0x0d3691d0167e555aULL:g.set_auto({{"item",true}});return;
    case 0xc6f8d5ff735d1167ULL:g.set_auto({{"item",false}});return;
    case 0x34730c5a5a8654adULL:g.set_auto({{"wall",true}});return;
    case 0xef85bf376f2d0f0aULL:g.set_auto({{"wall",false}});return;
    case 0xca6871a82e7465acULL:g.set_auto({{"item","nopotion"}});return;
    case 0x39acc41fb1ed0b5cULL:g.set_flag("对话"+std::to_string(g.flag(NativeFlag::arg1))+(g.flag_truth(NativeFlag::arg4)?"02":""),true);return;
    case 0x3588d884c60a7ab9ULL:g.set_flag(floor_id(g.hero.floor)+"@"+std::to_string(g.flag(NativeFlag::arg2))+"@"+std::to_string(g.flag(NativeFlag::arg3))+"@A",1);return;
    case 0x1844aa6b1fa3dce0ULL:
    case 0x0ea43c4abcf06457ULL:
    case 0x4739fd881e242301ULL: {
      auto h=function_hash(source);std::string kind=h==0x1844aa6b1fa3dce0ULL?"hp":h==0x0ea43c4abcf06457ULL?"atk":"def";
      auto ratio=g.flags->value.contains("ratio")?g.flag(NativeFlag::ratio):1;
      auto quantity=kind=="hp"?100*(g.flag(NativeFlag::times1)+1):(kind=="atk"?2:4)*ratio;
      auto key=std::to_string(ratio)+"区购买"+kind;
      g.set_flag(key,g.flag(key)+quantity);g.set_flag(key+"次数",g.flag(key+"次数")+1);return;
    }
    case 0xa47bf1e400ec1804ULL: {
      Number n=0;for(int floor=g.world->first_floor;floor<floor_count;++floor)for(auto b:g.floors[floor]->blocks)n+=g.entity(b).kind==Kind::enemy;
      g.set_flag("t",n);return;
    }
    case 0xe7f5ac81276b2f94ULL: {
      Number n=0;auto inventory=g.inventory_json();
      for(auto section:{"constants","tools","equips"})for(auto it=inventory[section].begin();it!=inventory[section].end();++it) {
        if(std::string(section)=="tools"&&(it.key()=="yellowKey"||it.key()=="blueKey"||it.key()=="redKey"))continue;
        n+=number(it.value());
      }
      g.hero[hp]=n-1;return;
    }
    case 0x856889e68d3801e2ULL: { // MT32 intentional zero-HP survival, dr=false.
      g.hero[hp]=0;auto e=g.effective_enemy(g.world->make_block("yellowKnight"));
      if(!g.flag_truth(NativeFlag::curse))g.hero[gold]+=e.money*(g.item_count("coin")?2:1);
      g.set_flag("hatred",g.flag(NativeFlag::hatred)+number(g.world->native_data["values"].value("hatred",Json(0))));
      g.set_flag("对话能量",0);return;
    }
    case 0xe4603e09e8824571ULL: { // Exact MT40 source-generated marching battle sequence.
      Json todo=Json::array();const std::array<Pos,13> positions={{{5,4},{4,4},{3,4},{7,4},{8,4},{9,4},{4,2},{3,2},{2,2},{8,2},{9,2},{10,2},{6,1}}};
      for(auto q:positions)if(g.block(g.hero.floor,q).entity) {
        Json steps=Json::array();int dx=6-q.x,dy=7-q.y;
        if(dx)steps.push_back(std::string(dx>0?"right:":"left:")+std::to_string(std::abs(dx)));
        if(dy)steps.push_back(std::string(dy>0?"down:":"up:")+std::to_string(std::abs(dy)));
        todo.push_back({{"type","move"},{"loc",{q.x,q.y}},{"keep",true},{"steps",steps}});
        todo.push_back({{"type","battle"},{"loc",{6,7}}});
      }
      todo.push_back({{"type","setBlock"},{"number","upFloor"},{"loc",{{6,1}}}});
      todo.push_back({{"type","setBlock"},{"number","yellowKnight"},{"loc",{{6,7}}}});
      todo.push_back({{"type","move"},{"loc",{6,7}},{"keep",false},{"steps",{"up:6"}}});
      prepend(todo);return;
    }
    case 0x6b8c0e900f4f21aaULL: // The pinned firstData has no difficulty choices.
      return;
    case 0x08c5d93e467f79dcULL:throw Unsupported("Replay-file picker cannot execute inside a continuous input replay");
    default:throw Unsupported("Unsupported source event function, FNV64="+std::to_string(function_hash(source)));
  }
}
void VM::execute(const Json &a) {
  if(a.is_null()||a.is_string())return; // Narrative text auto-advances during native replay.
  if(!a.is_object())throw Unsupported("Invalid source event: "+a.dump());
  if(a.value("_disabled",false))return;
  const auto type=a.value("type",std::string("text"));
  static const std::set<std::string> presentation={"comment","text","tip","playSound","stopSound","sleep","waitAsync","setText","setCurtain","setAttribute","update","previewUI","fillRect","fillBoldText","drawSelector","clearMap","strokeRect","animate"};
  if(presentation.contains(type))return;
  if(type=="playBgm"){g.set_flag("__bgm__",a.value("keep",false)?a.at("name"):Json());return;}
  if(type=="setValue"){assign(a.at("name"),eval(a.at("value")),a.value("operator",std::string("=")));return;}
  if(type=="if") {
    const auto branch=a.find(truthy(eval(a.at("condition")))?"true":"false");
    if(branch!=a.end())prepend_borrowed(*branch);else prepend(Json::array());return;
  }
  if(type=="switch") {
    auto key=eval(a.at("condition"));Json todo=Json::array();
    for(auto &c:a.at("caseList"))if(!c.value("_disabled",false)&&(c.at("case")=="default"||eval(c.at("case"))==key)) {
      auto body=actions(c.at("action"));todo.insert(todo.end(),body.begin(),body.end());if(!c.value("nobreak",false))break;
    }
    prepend(todo);return;
  }
  if(type=="choices"){set_choice(a);return;}
  if(type=="while") {if(truthy(eval(a.at("condition"))))push(a.at("data"),f,p,a);return;}
  if(type=="for") {
    const auto name=a.at("name").get<std::string>();
    if(name.size()!=6||!name.starts_with("temp:")||name[5]<'A'||name[5]>'Z')return;
    auto first=eval(a.at("from")),last=eval(a.at("to")),step=eval(a.at("step"));
    if(!first.is_number()||!last.is_number()||!step.is_number())return;
    if((number(step)>0&&number(first)>number(last))||(number(step)<0&&number(first)<number(last)))return;
    assign(name,first);auto letter=name.substr(5);
    g.set_flag("@temp@for-to@"+letter,a.at("to"));g.set_flag("@temp@for-step@"+letter,a.at("step"));
    // Native for is a do-while frame. Bounds/step are re-evaluated after each
    // iteration and break exits this frame, rather than a pre-expanded body.
    push(a.at("data"),f,p,a);return;
  }
  if(type=="break") {int n=a.value("n",1);while(n--&&frame_count()>1)pop_frame();return;}
  if(type=="exit"){clear_frames();choice.reset();return;}
  if(type=="insert") {
    // insertCommonEvent stores argument values verbatim; null entries preserve
    // an existing argument. Expressions are evaluated by the receiving event.
    if(a.contains("args")){int i=1;for(auto &v:a["args"]){if(!v.is_null())g.set_flag("arg"+std::to_string(i),v);++i;}}
    if(!a.contains("name"))throw Unsupported("Coordinate insert events are not implemented");
    auto name=a["name"].get<std::string>();auto &common=g.world->native_data.at("commonEvent");
    if(!common.contains(name))throw Unsupported("Unknown common event: "+name);
    g.set_flag("arg0",name);push(common[name],f,p);return;
  }
  if(type=="function"){execute_function(a.at("function"));return;}
  if(type=="hide"){for(auto q:locations(a))hide(event_floor(a),q,a.value("remove",false)||a.value("destruct",false),a.value("destruct",false));return;}
  if(type=="show"){for(auto q:locations(a))show(event_floor(a),q);return;}
  if(type=="setBlock") {
    auto b=block_value(a.at("number"));
    for(auto q:locations(a)) {
      int floor=event_floor(a);
      const auto &source_events=g.world->floors[floor].events;
      bool was_disabled=is_disabled(floor,q);
      auto origin=g.retained_block(floor,q);
      bool origin_nonzero=origin.tile!=0;
      const auto erased_key="R"+floor_id(floor)+","+location(q);
      bool source_active=!a.value("destruct",false)&&!g.flag_truth(erased_key);
      bool zero_action=!b.entity&&source_active&&source_events.contains(location(q));
      // plugins.animateSetBlocks shows replacements of enabled nonzero blocks,
      // and every new nonzero block whose origin is absent or tile zero. A
      // disabled nonzero origin takes the plain-setBlock branch instead.
      const bool animate_show=origin_nonzero?!was_disabled&&(b.entity||zero_action):bool(b.entity);
      g.replace_block(floor,q,b,a.value("destruct",false),a.value("time",0)>0&&floor==g.hero.floor&&animate_show);
    }
    return;
  }
  if(type=="setBgFgBlock") {
    auto name=a.at("name").get<std::string>();auto b=block_value(a.at("number"));
    for(auto q:locations(a)) {
      if(name=="fg")g.set_foreground(event_floor(a),q,b.tile);
      else if(name=="bg")g.set_background(event_floor(a),q,b.tile);
      else throw Unsupported("Invalid background/foreground selector");
    }
    return;
  }
  if(type=="openDoor") {
    const int floor=event_floor(a);
    for(auto q:locations(a)) {
      // _action_openDoor removes remote blocks without opening-door hooks.
      if(floor!=g.hero.floor){hide(floor,q,true);continue;}
      auto b=g.block(floor,q);auto event=g.effective_block_event(floor,q);
      const auto cls=event.value("cls",std::string());
      if((!b.entity&&!g.event_enabled(floor,q))||
         (cls!="animates"&&cls!="npc48")||!event.contains("doorInfo"))continue;
      const auto keys=event["doorInfo"].value("keys",Json::object());
      if(a.value("needKey",false)) {
        bool enough=true;
        const auto items=g.world->native_data.value("items",Json::object());
        for(auto it=keys.begin();it!=keys.end();++it) {
          auto key=it.key();if(key.ends_with(":o"))key.resize(key.size()-2);
          if(!items.contains(key)||g.item_count(key)<number(it.value())){enough=false;break;}
        }
        if(!enough)continue; // The native action continues after a failed open.
        for(auto it=keys.begin();it!=keys.end();++it)
          if(!it.key().ends_with(":o"))g.add_inventory(it.key(),-number(it.value()));
        const auto id=g.entity(b).id;
        if(id=="yellowDoor")++g.hero[yd];
        if(id=="blueDoor")++g.hero[bd];
        if(id=="redDoor")++g.hero[rd];
      }
      hide(floor,q,true);native_events_after(g,"afterOpenDoor",floor,q);
    }
    return;
  }
  if(type=="closeDoor") {
    // events.closeDoor refuses an occupied active cell, then runs the native
    // setBlock + showBlock sequence. It must not overwrite items or actions.
    const auto id=a.value("id",std::string());
    if(!g.world->by_id.contains(id))return;
    const auto b=g.world->make_block(id);
    if(!g.entity(b).native.contains("doorInfo"))return;
    const int floor=g.hero.floor; // _action_closeDoor has no floor argument.
    for(auto q:locations(a)) {
      if(g.block_present(floor,q))continue;
      g.set_block(floor,q,b);show(floor,q);
    }
    return;
  }
  if(type=="moveHero") {
    // events.eventMoveHero is forced animation movement: it updates location
    // and facing without invoking ordinary movement, combat or tile events.
    for(const auto &v:a.value("steps",Json::array())) {
      auto token=v.get<std::string>();auto colon=token.find(':');auto dir=token.substr(0,colon);
      int count=colon==std::string::npos?1:std::stoi(token.substr(colon+1));
      if(dir=="speed")continue;
      int sign=dir=="backward"?-1:1;
      if(dir=="forward"||dir=="backward")dir=g.extra_state->value.value("direction",std::string("down"));
      auto delta=direction_delta(dir);g.hero.pos.x+=sign*delta.x*std::max(count,0);g.hero.pos.y+=sign*delta.y*std::max(count,0);
      auto facing=dir.starts_with("left")?"left":dir.starts_with("right")?"right":dir;
      g.set_extra("direction",facing);
    }
    return;
  }
  if(type=="move"||type=="generateMove") {
    int floor=event_floor(a);auto start=locations(a).at(0),finish=start;
    // Native _getAndRemoveBlock returns before removal when getBlockInfo
    // cannot find an active drawable block, including tile-zero actions.
    if(type=="move"&&(!g.block_present(floor,start)||!g.block(floor,start).entity))return;
    auto b=type=="generateMove"?block_value(a.at("id")):g.block(floor,start);
    if(type=="move"){g.ground(floor,start);g.erase_event(floor,start);}
    for(auto &s:a.value("steps",Json::array())) {
      auto token=s.get<std::string>();auto colon=token.find(':');auto dir=token.substr(0,colon);int n=colon==std::string::npos?1:std::stoi(token.substr(colon+1));
      if(dir=="speed")continue;auto delta=direction_delta(dir);finish.x+=delta.x*n;finish.y+=delta.y*n;
    }
    if(a.value("keep",false)&&b.entity) {
      if(!finish.valid())throw RouteError("Event move ends outside floor");
      g.set_block(floor,finish,b);
      // Native _moveBlock_doMove calls setBlock then showBlock. The destination
      // can have a disabled source event which the moved actor must activate.
      show(floor,finish);
    }
    return;
  }
  if(type=="changeFloor") {
    int target=event_floor(a);Pos landing;
    if(a.contains("loc"))landing=locations(a).at(0);
    else if(a.value("stair",std::string())=="downFloor")landing=g.world->floors[target].down;
    else if(a.value("stair",std::string())=="upFloor")landing=g.world->floors[target].up;
    else throw Unsupported("Native changeFloor without explicit landing/stair");
    g.transition(target,landing);if(a.contains("direction"))g.set_extra("direction",a["direction"]);return;
  }
  if(type=="setEnemy") {
    auto id=a.at("id").get<std::string>(),name=a.at("name").get<std::string>();auto value=eval(a.at("value"));
    auto overrides=g.extra_state->value.value("enemy_overrides",Json::object());
    auto info=resolve("core.getEnemyInfo",{id});auto old=info.value(name,Json(0));
    auto op=a.value("operator",std::string("="));
    if(op=="/="){if(number(value)==0)throw RouteError("Enemy division by zero");value=number(old)/number(value);}
    else if(op=="+=")value=number(old)+number(value);else if(op!="=")throw Unsupported("Enemy operator "+op);
    overrides[id][name]=value;g.set_enemy(id,name,value);return;
  }
  if(type=="battle") {
    auto floor=event_floor(a);bool located=!truthy(a.value("id",Json()));
    if(located&&floor!=g.hero.floor)return;
    auto q=located?locations(a).at(0):Pos{-1,-1};
    auto b=located?g.block(floor,q):g.world->make_block(a.at("id"));
    if(!b.entity)return;const auto e=g.effective_enemy(b);if(e.kind!=Kind::enemy)throw RouteError("Event battle target is not enemy");
    auto damage=g.enemy_damage(b);
    if(g.flag(NativeFlag::addhp)==1&&damage<infinite_damage)damage=-damage;
    if(damage>=g.hero[hp])throw RouteError("Fatal native event battle: "+e.id);
    g.hero[hp]-=damage;g.hero[dmg]+=damage;
    if(!g.flag_truth(NativeFlag::curse))g.hero[gold]+=e.money*(g.item_count("coin")?2:1);
    g.set_flag("hatred",g.flag(NativeFlag::hatred)+number(g.world->native_data["values"].value("hatred",Json(0))));
    g.set_flag("对话能量",0);
    if(located){g.ground(floor,q);g.erase_event(floor,q);native_events_after(g,"afterBattle",floor,q);}
    return;
  }
  if(type=="win") {
    auto text=a.value("reason",std::string());
    std::string reason;std::size_t cursor=0;
    for(;;) {
      auto begin=text.find("${",cursor);
      if(begin==std::string::npos){reason+=text.substr(cursor);break;}
      reason+=text.substr(cursor,begin-cursor);
      auto end=text.find('}',begin+2);
      if(end==std::string::npos)throw Unsupported("Unterminated native win reason expression");
      reason+=string_value(eval(text.substr(begin+2,end-begin-2)));cursor=end+1;
    }
    g.hero[won50]=1;g.set_extra("native_win_reason",reason);clear_frames();return;
  }
  throw Unsupported("Unsupported source event type: "+type);
}
} // namespace

std::shared_ptr<const PreparedNativeWorld> prepare_native_world(const std::shared_ptr<const World> &world) {
  auto old=world->prepared.load();if(old&&old->matches(world))return old;
  auto prepared=std::make_shared<PreparedNativeWorld>();prepared->source=world;
  auto prepare_program=[&](const Json &source) {
    try {
      auto program=actions(source);precompile_event(program);
      prepared->event_programs.emplace(&source,std::make_shared<const Json>(std::move(program)));
    } catch(const Json::exception &) {
      // Preserve lazy errors for malformed source that may never execute.
    } catch(const RouteError &) {}
  };
  const auto common=world->native_data.find("commonEvent");
  if(common!=world->native_data.end()&&common->is_object())for(const auto &program:*common)prepare_program(program);
  const std::array<const char*,4> directions={"up","down","left","right"};
  auto mask=[&](const Json &value){
    std::uint8_t result=0;
    if(value.is_array())for(int d=0;d<4;++d) {
      if(std::find(value.begin(),value.end(),Json(directions[d]))!=value.end())result|=std::uint8_t(1U<<d);
    }
    return result;
  };
  for(const auto &entity:world->entities)prepared->entities.push_back({mask(entity.native.value("cannotIn",Json())),mask(entity.native.value("cannotOut",Json()))});
  const int max_tile=world->tiles.empty()?0:std::clamp(world->tiles.rbegin()->first,0,65535);
  prepared->tiles.resize(std::size_t(max_tile)+1);
  for(const auto &[tile,block]:world->tiles)if(tile>=0&&tile<=max_tile)prepared->tiles[tile]=prepared->entities[block.entity];
  for(int floor=world->first_floor;floor<floor_count;++floor) {
    const auto &meta=world->floors[floor].metadata;
    for(const auto *kind:{"firstArrive","eachArrive"}) {
      const auto program=meta.find(kind);if(program!=meta.end())prepare_program(*program);
    }
    for(const auto *kind:{"afterBattle","afterGetItem","afterOpenDoor"}) {
      const auto events=meta.find(kind);
      if(events!=meta.end()&&events->is_object())for(const auto &program:*events)prepare_program(program);
    }
    const auto out=meta.value("cannotMove",Json::object()),in=meta.value("cannotMoveIn",Json::object());
    for(int p=0;p<cells;++p){const auto loc=location({p%side,p/side});prepared->floor_out[floor][p]=mask(out.value(loc,Json()));prepared->floor_in[floor][p]=mask(in.value(loc,Json()));
      prepared->source_events[floor][p]=!world->floors[floor].events.value(loc,Json()).is_null();
      prepared->change_floors[floor][p]=world->floors[floor].change_floor.contains(loc);
    }
    const auto events=meta.find("autoEvent");if(events==meta.end())continue;
    for(auto loc=events->begin();loc!=events->end();++loc)for(auto it=loc.value().begin();it!=loc.value().end();++it) {
      const auto &event=it.value();if(!event.is_object())continue;
      const auto data=event.find("data");if(data!=event.end())prepare_program(*data);
      const auto comma=loc.key().find(',');const Pos p{std::stoi(loc.key().substr(0,comma)),std::stoi(loc.key().substr(comma+1))};
      prepared->auto_events.push_back({floor,p,floor_id(floor)+"@"+std::to_string(p.x)+"@"+std::to_string(p.y)+"@"+it.key(),
        event.value("multiExecute",false),event.value("currentFloor",true),&event});
    }
  }
  for(int floor=world->first_floor;floor<floor_count;++floor)for(std::size_t i=0;i<prepared->auto_events.size();++i) {
    const auto &event=prepared->auto_events[i];if(!event.current_floor||event.floor==floor)prepared->auto_for_floor[floor].push_back(i);
  }
  world->prepared.store(prepared);return prepared;
}

Json native_event_state_json(const Game &g) {
  if(g.native_continuation)return g.native_continuation->export_json();
  return g.extra_state->value.value(state_key,Json::object());
}
bool native_events_pending(const Game &g) {
  if(active && &active->g==&g)return active->choice||!active->frames_empty();
  if(g.native_continuation)return g.native_continuation->choice||!g.native_continuation->frames_empty();
  const auto it=g.extra_state->value.find(state_key);if(it==g.extra_state->value.end())return false;
  const auto &state=*it;const auto frames=state.find("frames");
  return state.contains("choice")||(frames!=state.end()&&!frames->empty());
}
void Game::native_event_start(const Json &data,int f,Pos p) {
  if(actions_empty(data))return;
  set_extra("native_event_serial",extra_state->value.value("native_event_serial",Number(0))+1);
  if(active && &active->g==this){active->push(data,f,p);return;}
  VM vm(*this);vm.push(data,f,p);vm.drain();
}
void Game::native_event_drain(){
  if(active && &active->g==this)return;
  if(native_continuation) {
    const bool empty=native_continuation->frames_empty(),choice=bool(native_continuation->choice);
    if(native_auto_active||empty||choice) {
      if(!native_auto_active&&empty&&!choice)clear_temporaries(*this);
      return;
    }
    VM vm(*this);vm.drain();return;
  }
  const auto it=extra_state->value.find(state_key);
  if(it!=extra_state->value.end()&&it->contains("frames")&&!it->contains("hidden")&&!it->contains("removed")) {
    const bool empty=it->at("frames").empty(),choice=it->contains("choice");
    if(native_auto_active||empty||choice) {
      if(!native_auto_active&&empty&&!choice)clear_temporaries(*this);
      return;
    }
  }
  VM vm(*this);vm.drain();
}
void Game::native_choose(int index) {
  VM vm(*this);
  if(!vm.choice)throw RouteError("choices token without pending native choice");
  const auto owner=vm.choice;const auto &choice=owner->value;
  auto apply=[&](auto &value,bool borrow) {
    if(index<0)index+=int(value["options"].size());
    if(index%100>50)index+=int(value["options"].size());
    if(index<0||std::size_t(index%100)>=value["options"].size())throw RouteError("Native choice index out of range: "+std::to_string(index));
    set_flag("timeout",index/100);index%=100;vm.choice.reset();
    const auto &selected=value["options"][index];
    vm.f=value["floor"];vm.p={value["x"],value["y"]};
    if(selected.contains("need")&&!selected["need"].is_null()&&selected["need"]!=""&&!truthy(vm.eval(selected["need"])))throw RouteError("Native choice requirement failed");
    if(vm.frames_empty())throw RouteError("Native choice has no event frame");
    if(!borrow||!selected.is_object())vm.prepend(selected.value("action",Json::array()));
    else {
      const auto action=selected.find("action");
      if(action!=selected.end()) {
        vm.executing_program=std::shared_ptr<const Json>(owner,&owner->value);
        vm.prepend_borrowed(*action);vm.executing_program.reset();
      } else vm.prepend(Json::array());
    }
    vm.drain();
  };
  if(choice.is_object()&&choice.contains("options")&&choice["options"].is_array()&&choice.contains("floor")&&choice.contains("x")&&choice.contains("y"))apply(choice,true);
  else {auto mutable_choice=choice;apply(mutable_choice,false);}
}
bool Game::native_event_trigger(int f,Pos p) {
  if(world->native_data.empty())throw Unsupported("Pinned native event metadata was not loaded");
  if(!block_present(f,p))return false;
  const auto &e=entity(block(f,p));
  const auto &event=native_block_event_view(f,p);
  // Touch events are passed to insertAction by reference. Native precompile
  // mutates this retained payload before the execution list is cloned; ordinary
  // action events below are cloned by _sys_action and leave their payload alone.
  if(event.contains("event")&&!actions_empty(event["event"])) {
    auto updated=event;precompile_event(updated["event"]);
    auto &fs=mutable_floor(f);
    if(fs.retained_events.use_count()!=1)fs.retained_events=std::make_shared<RetainedEvents>(*fs.retained_events);
    fs.retained_events->set(p,updated);sync_retained_cell(f,p);
    native_event_start(updated["event"],f,p);return true;
  }
  // Attached data does not override battle/getItem/openDoor. Dispatch the
  // retained effective trigger so hidden/removed stairs cannot reappear here.
  if(event.value("trigger",Json())=="action") {
    const auto body=event.find("data");
    if(body!=event.end()&&!actions_empty(*body)){
      // Preserve the action wrapper's first normalization without copying its
      // array; native_event_start performs the same second normalization.
      const auto &normalized=body->is_object()&&body->contains("data")&&!body->contains("type")?body->at("data"):*body;
      native_event_start(normalized,f,p);return true;
    }
  }
  if(e.id=="oldman"||e.id=="trader") {
    auto used=flag_truth(floor_id(f)+"@"+std::to_string(p.x)+"@"+std::to_string(p.y)+"@A");
    auto name=e.id=="oldman"||used?"对话":"商人";
    Json args={f,p.x,p.y};if(std::string(name)=="对话")args.push_back(e.id=="trader"&&used?1:0);
    native_event_start(Json::array({{{"type","insert"},{"name",name},{"args",args}}}),f,p);return true;
  }
  if(event.value("trigger",Json())=="changeFloor") {
    auto a=event.at("data");a["type"]="changeFloor";
    if(a.contains("loc"))a.erase("stair");
    native_event_start(Json::array({a}),f,p);return true;
  }
  return false;
}
void native_events_after(Game &g,const std::string &kind,int f,Pos p) {
  const auto &meta=g.world->floors[f].metadata;const auto source=meta.find(kind);
  if(source!=meta.end()&&source->contains(location(p)))g.native_event_start(source->at(location(p)),f,p);
}
void Game::native_event_after(const std::string &kind,int f,Pos p){native_events_after(*this,kind,f,p);native_events_check_auto(*this);}
void native_events_arrive(Game &g,int floor,bool first) {
  const auto &source=g.world->floors[floor].metadata;
  Json todo=Json::array();
  // Native afterChangeFloor inserts eachArrive, then firstArrive at the front.
  if(first){auto a=actions(source.value("firstArrive",Json::array()));todo.insert(todo.end(),a.begin(),a.end());}
  auto each=actions(source.value("eachArrive",Json::array()));todo.insert(todo.end(),each.begin(),each.end());
  g.native_event_start(todo,floor,g.hero.pos);
}
void Game::native_event_arrive(int floor,bool first){native_events_arrive(*this,floor,first);native_events_check_auto(*this);}
void native_events_check_auto(Game &g) {
  if(g.flag_truth(NativeFlag::do_not_check_auto_events))return;
  auto prepared=g.prepared_native;if(!prepared||!prepared->matches(g.world))prepared=prepare_native_world(g.world);
  const auto &candidates=prepared->auto_for_floor.at(g.hero.floor);
  if(candidates.empty()){g.native_event_drain();return;}
  std::optional<VM> local;
  VM *vm=(active && &active->g==&g)?active:nullptr;
  auto executed=g.flags->value.value("__aed__",Json::array());
  auto executing=g.flags->value.value("__aei__",Json::array());
  auto removed=g.flags->value.value("__removed__",Json::array());
  auto contains=[](const Json &a,const Json &v){return std::find(a.begin(),a.end(),v)!=a.end();};
  for(auto index:candidates) {
      const auto &entry=prepared->auto_events[index];const auto &event=*entry.source;const auto &symbol=entry.symbol;
      const int floor=entry.floor;const auto p=entry.pos;
      if(contains(removed,floor_id(floor)))continue;
      if(contains(executed,symbol)&&!entry.multi_execute)continue;
      if(contains(executing,symbol))continue;
      if(!vm){local.emplace(g);vm=&*local;}
      vm->f=floor;vm->p=p;
      if(truthy(vm->eval(event.at("condition")))) {
        auto clean=Json::array();for(const auto &v:executed)if(v!=symbol)clean.push_back(v);executed=clean;executed.push_back(symbol);
        executing.push_back(symbol);g.set_flag("__aed__",executed);g.set_flag("__aei__",executing);
        g.set_extra("native_event_serial",g.extra_state->value.value("native_event_serial",Number(0))+1);
        const auto data=event.find("data");
        if(data!=event.end())vm->push(*data,floor,p);else vm->push(Json::array(),floor,p);
        vm->frame().mutate()["auto_symbol"]=symbol;
      }
  }
  if(local)vm->drain();else if(!vm)g.native_event_drain();
}

void Game::native_token(const std::string &token, bool strict_steps) {
  if(reference_model)throw RouteError("Original tokens require native-corrected mode");
  if(world->native_data.empty())throw Unsupported("Original tokens require pinned native source metadata");
  // events.win marks gameOver before the native win callback stops replay.
  // A candidate suffix cannot perform movement, purchases or tools after it.
  if(hero[won50])throw RouteError("Native route action after terminal win: "+token);
  // A fresh route starts after native firstData/startText has been installed and
  // is waiting on MT1's firstArrive choice. Initializing that queue uses source,
  // not an expected checkpoint. Mid-game imports do not synthesize startup.
  bool initialized;
  if(native_continuation)initialized=native_continuation->metadata->value("initialized",false);
  else {
    const auto state=extra_state->value.find(state_key);
    initialized=state!=extra_state->value.end()&&state->value("initialized",false);
  }
  if(!initialized) {
    VM vm(*this);vm.set_metadata("initialized",true);vm.save();
    if(hero.floor==1&&!flags->value.contains("开启特性")) {
      const auto &meta=world->floors[1].metadata;const auto first=meta.find("firstArrive");
      if(first!=meta.end())native_event_start(*first,1,hero.pos);
    }
  }
  if(token.starts_with("choices:")){native_choose(std::stoi(token.substr(8)));return;}
  if(native_events_pending(*this))throw RouteError("Native route action while event choice is pending: "+token);
  if(token=="help") {native_event_start(Json::array({{{"type","insert"},{"name","游戏说明"}}}),hero.floor,hero.pos);return;}
  if(token=="up"||token=="right"||token=="down"||token=="left")step(token,!strict_steps);
  else if(token.starts_with("move:")) {
    auto s=token.substr(5);auto colon=s.find(':');if(colon==std::string::npos)throw RouteError("Invalid move token");
    move_directly({std::stoi(s.substr(0,colon)),std::stoi(s.substr(colon+1))});
  }
  else if(token.starts_with("fly:"))fly(floor_number(token.substr(4)));
  else if(token.starts_with("turn:"))set_extra("direction",token.substr(5));
  else if(token=="turn") {
    std::array<std::string,4> dirs={"up","right","down","left"};auto d=extra_state->value.value("direction",std::string("down"));
    auto it=std::find(dirs.begin(),dirs.end(),d);set_extra("direction",dirs[(std::size_t(it-dirs.begin())+1)%4]);
  }
  else if(token.starts_with("item:")) {
    auto id=token.substr(5);
    if(id=="centerFly")center_fly();else if(id=="upFly")up_fly();else if(id=="downFly")down_fly();
    else if(id=="earthquake")use_earthquake();else if(id=="bomb")use_bomb();else if(id=="bigKey")use_big_key();
    else if(id=="snow")use_snow();else if(id=="superPotion")use_super_potion();
    else if(id=="pickaxe") {auto d=direction_delta(extra_state->value.value("direction",std::string("down")));use_pickaxe(hero.floor,{hero.pos.x+d.x,hero.pos.y+d.y});}
    else if(id=="book"||id=="I300"||id=="I333") {
      if(item_count(id)<=0)throw RouteError("Missing replay UI item "+id);
      // The pinned I300/I333 have no useItemEffect; book only draws a UI.
      set_flag("对话能量",flag(NativeFlag::energy)-1);
    }
    else throw Unsupported("Unsupported replay item: "+id);
  }
  else if(token.starts_with("key:")) {
    int key=std::stoi(token.substr(4));
    auto use_without_record=[&](const std::string &id) {
      if(id=="pickaxe"){auto d=direction_delta(extra_state->value.value("direction",std::string("down")));use_pickaxe(hero.floor,{hero.pos.x+d.x,hero.pos.y+d.y});}
      else if(id=="bomb")use_bomb();else if(id=="earthquake")use_earthquake();else if(id=="upFly")up_fly();else if(id=="downFly")down_fly();
      else throw Unsupported("Unsupported source shortcut item: "+id);
      // functions.js key handlers call useItem(id,true). Core public methods
      // implement useItem(id,false), whose final route-record decrement is absent.
      // Floor-arrival tools already end after the source's energy reset.
      if(id!="upFly"&&id!="downFly")set_flag("对话能量",flag(NativeFlag::energy)+1);
    };
    if(key==49)use_without_record("pickaxe");
    else if(key==50)use_without_record("bomb");
    else if(key==52) {
      bool used=false;
      for(const std::string id:{"icePickaxe","freezeBadge","earthquake","upFly","downFly","jumpShoes","lifeWand","poisonWine","weakWine","curseWine","superWine"}) {
        if(item_count(id)<=0)continue;
        auto backup=*this;
        try{use_without_record(id);used=true;break;}
        catch(const Unsupported&){*this=std::move(backup);throw;}
        catch(const RouteError&){*this=std::move(backup);}
      }
      if(!used)throw RouteError("No usable item for original key:52");
    } else throw Unsupported("Unsupported original key token: "+token);
  }
  else if(token=="getNext") {
    // events.js tries the facing direction first, then up/down/left/right;
    // collection does not turn or move the hero.
    auto globals=extra_state->value.value("game_flags",world->native_data.value("flags",Json::object()));
    if(!truthy(globals.value("enableGentleClick",Json(false))))return;
    VM vm(*this);
    std::vector<std::string> dirs={extra_state->value.value("direction",std::string("down")),"up","down","left","right"};
    for(const auto &dir:dirs) {
      auto d=direction_delta(dir);Pos q{hero.pos.x+d.x,hero.pos.y+d.y};auto b=block(hero.floor,q);auto &e=entity(b);
      if(!b.entity||vm.is_disabled(hero.floor,q)||!native_can_move(hero.floor,hero.pos,q))continue;
      auto event=effective_block_event(hero.floor,q);
      if(event.value("trigger",Json())!="getItem"||truthy(event.value("script",Json()))||truthy(event.value("event",Json())))continue;
      apply_item(hero.floor,e.id);ground(hero.floor,q);native_event_after("afterGetItem",hero.floor,q);return;
    }
    return; // Native getNextItem returns false without changing gameplay state.
  }
  else throw Unsupported("Unsupported original replay token: "+token);
  native_events_check_auto(*this);
}
} // namespace mota
