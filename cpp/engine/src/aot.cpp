// Generated from immutable tower rules. No route or checkpoint outcomes are inputs.
#include "engine/aot.hpp"
#include <cmath>
namespace engine {
namespace {
inline double as_number(const Value &v) {
  if(auto n=std::get_if<Number>(&v))return double(*n);
  if(auto n=std::get_if<double>(&v))return *n;
  if(auto b=std::get_if<bool>(&v))return *b;
  if(v.index()==0)return 0;
  return numeric(v);
}
inline Number as_int(const Value &v) {
  if(auto n=std::get_if<Number>(&v))return *n;
  if(auto n=std::get_if<double>(&v))return Number(*n);
  if(auto b=std::get_if<bool>(&v))return *b;
  return 0;
}
inline bool as_truth(const Value &v) {
  if(auto b=std::get_if<bool>(&v))return *b;
  if(auto n=std::get_if<Number>(&v))return *n!=0;
  if(auto n=std::get_if<double>(&v))return *n!=0;
  if(auto s=std::get_if<std::string>(&v))return !s->empty();
  return v.index()!=0;
}
template<ExprOp op> Value binary(Value a,Value b) {
  if constexpr(op==ExprOp::eq||op==ExprOp::ne||op==ExprOp::seq||op==ExprOp::sne) {
    bool r=equal_value(a,b,op==ExprOp::seq||op==ExprOp::sne);
    return (op==ExprOp::ne||op==ExprOp::sne)?!r:r;
  } else if constexpr(op==ExprOp::add) {
    if(a.index()==4||b.index()==4)return text(a)+text(b);
    return number_value(as_number(a)+as_number(b));
  } else if constexpr(op==ExprOp::sub)return number_value(as_number(a)-as_number(b));
  else if constexpr(op==ExprOp::mul)return number_value(as_number(a)*as_number(b));
  else if constexpr(op==ExprOp::div||op==ExprOp::mod) {
    if(as_number(b)==0)throw Error(op==ExprOp::div?"Division by zero":"Modulo zero");
    return number_value(op==ExprOp::div?as_number(a)/as_number(b):std::fmod(as_number(a),as_number(b)));
  } else {
    int order=0;
    if(a.index()==4 && b.index()==4)order=std::get<std::string>(a).compare(std::get<std::string>(b));
    else {auto x=as_number(a),y=as_number(b);order=x<y?-1:x>y?1:0;}
    return op==ExprOp::lt?order<0:op==ExprOp::le?order<=0:op==ExprOp::gt?order>0:order>=0;
  }
}
static void op_0(Game &g,int f,Pos p){ }
static void op_1(Game &g,int f,Pos p){{auto b=g.world->entities.at(38).block;b.tile=std::int16_t(166);{auto q=p;int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_3(Game &g,int f,Pos p){ }
static void op_4(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(10LL))))};g.remove(g.hero.floor,q,true);}
}
static void op_6(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.read(Target{Ref(6),0},f,p);auto b=g.world->expressions[5].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?4:5);}
static void op_8(Game &g,int f,Pos p){ }
static void op_9(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(5LL))))};g.remove(g.hero.floor,q,true);}
}
static void op_11(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.read(Target{Ref(6),0},f,p);auto b=g.world->expressions[14].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?7:8);}
static void op_12(Game &g,int f,Pos p){{auto key=g.read(Target{Ref(11),0},f,p);SmallVector<int,4> parts;bool done=false;if(!done && (equal_value(g.world->expressions[2].literal,key,true))){parts.push_back(3);done=true;}if(!done && (equal_value(g.world->expressions[11].literal,key,true))){parts.push_back(6);done=true;}for(auto it=parts.rbegin();it!=parts.rend();++it)g.prepend(*it);}}
static void op_13(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,true);}
}
static void op_15(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[15]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_16(Game &g,int f,Pos p){g.assign(Target{Ref(2),1},Value(Number(1LL)),Assign(0),f,p);}
static void op_17(Game &g,int f,Pos p){g.assign(Target{Ref(0),14},Value(false),Assign(0),f,p);}
static void op_18(Game &g,int f,Pos p){ }
static void op_19(Game &g,int f,Pos p){g.function(4380188494662846530ULL,f,p);}
static void op_20(Game &g,int f,Pos p){g.assign(Target{Ref(0),0},Value(Number(1LL)),Assign(0),f,p);}
static void op_21(Game &g,int f,Pos p){g.assign(Target{Ref(0),0},Value(Number(0LL)),Assign(0),f,p);}
static void op_22(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(5LL))))};g.remove(35,q,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(5LL))))};g.remove(35,q,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(5LL))))};g.remove(35,q,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(6LL))))};g.remove(35,q,false);}
}
static void op_23(Game &g,int f,Pos p){{PendingChoice next;next.instruction=23;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);if(!next.options.empty())g.choice=std::move(next);}}
static void op_24(Game &g,int f,Pos p){g.assign(Target{Ref(0),2},Value(Number(1LL)),Assign(0),f,p);}
static void op_25(Game &g,int f,Pos p){g.assign(Target{Ref(0),2},Value(Number(0LL)),Assign(0),f,p);}
static void op_26(Game &g,int f,Pos p){{PendingChoice next;next.instruction=26;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);if(!next.options.empty())g.choice=std::move(next);}}
static void op_27(Game &g,int f,Pos p){ }
static void op_28(Game &g,int f,Pos p){g.assign(Target{Ref(0),15},Value(true),Assign(0),f,p);}
static void op_29(Game &g,int f,Pos p){g.assign(Target{Ref(0),8787},Value(true),Assign(0),f,p);}
static void op_30(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[30]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_31(Game &g,int f,Pos p){ }
static void op_32(Game &g,int f,Pos p){ }
static void op_33(Game &g,int f,Pos p){ }
static void op_34(Game &g,int f,Pos p){ }
static void op_35(Game &g,int f,Pos p){ }
static void op_36(Game &g,int f,Pos p){ }
static void op_37(Game &g,int f,Pos p){ }
static void op_38(Game &g,int f,Pos p){ }
static void op_39(Game &g,int f,Pos p){ }
static void op_40(Game &g,int f,Pos p){ }
static void op_41(Game &g,int f,Pos p){ }
static void op_42(Game &g,int f,Pos p){ }
static void op_43(Game &g,int f,Pos p){ }
static void op_44(Game &g,int f,Pos p){ }
static void op_45(Game &g,int f,Pos p){ }
static void op_46(Game &g,int f,Pos p){ }
static void op_47(Game &g,int f,Pos p){ }
static void op_48(Game &g,int f,Pos p){ }
static void op_49(Game &g,int f,Pos p){ }
static void op_50(Game &g,int f,Pos p){ }
static void op_51(Game &g,int f,Pos p){ }
static void op_52(Game &g,int f,Pos p){ }
static void op_53(Game &g,int f,Pos p){ }
static void op_54(Game &g,int f,Pos p){ }
static void op_55(Game &g,int f,Pos p){ }
static void op_56(Game &g,int f,Pos p){ }
static void op_57(Game &g,int f,Pos p){ }
static void op_58(Game &g,int f,Pos p){ }
static void op_59(Game &g,int f,Pos p){ }
static void op_60(Game &g,int f,Pos p){ }
static void op_61(Game &g,int f,Pos p){ }
static void op_62(Game &g,int f,Pos p){ }
static void op_63(Game &g,int f,Pos p){ }
static void op_64(Game &g,int f,Pos p){ }
static void op_65(Game &g,int f,Pos p){ }
static void op_66(Game &g,int f,Pos p){ }
static void op_67(Game &g,int f,Pos p){ }
static void op_68(Game &g,int f,Pos p){ }
static void op_69(Game &g,int f,Pos p){ }
static void op_70(Game &g,int f,Pos p){ }
static void op_71(Game &g,int f,Pos p){ }
static void op_72(Game &g,int f,Pos p){ }
static void op_73(Game &g,int f,Pos p){ }
static void op_74(Game &g,int f,Pos p){ }
static void op_75(Game &g,int f,Pos p){ }
static void op_76(Game &g,int f,Pos p){{PendingChoice next;next.instruction=76;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);next.options.push_back(2);next.options.push_back(3);next.options.push_back(4);next.options.push_back(5);next.options.push_back(6);next.options.push_back(7);next.options.push_back(8);next.options.push_back(9);if(!next.options.empty())g.choice=std::move(next);}}
static void op_77(Game &g,int f,Pos p){g.assign(Target{Ref(0),14},Value(true),Assign(0),f,p);}
static void op_78(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[78]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_85(Game &g,int f,Pos p){ }
static void op_86(Game &g,int f,Pos p){g.assign(Target{Ref(0),8788},Value(Number(1LL)),Assign(1),f,p);}
static void op_87(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8788,Value(Number(0LL))))?29:30);}
static void op_88(Game &g,int f,Pos p){ }
static void op_90(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(8LL))))};g.show(g.hero.floor,q);}
}
static void op_92(Game &g,int f,Pos p){{auto q=p;g.hide(g.hero.floor,q);}
}
static void op_93(Game &g,int f,Pos p){ }
static void op_94(Game &g,int f,Pos p){{auto v=as_int(number_value(std::floor(as_number(([&](){auto a=Value(g.hero.values[1]);auto b=Value(double(0.029999999999999999));return binary<ExprOp(9)>(std::move(a),std::move(b));}()))+0.5)));g.hero.values.at(1)+=v;}}
static void op_95(Game &g,int f,Pos p){{auto v=as_int(number_value(std::floor(as_number(([&](){auto a=Value(g.hero.values[2]);auto b=Value(double(0.029999999999999999));return binary<ExprOp(9)>(std::move(a),std::move(b));}()))+0.5)));g.hero.values.at(2)+=v;}}
static void op_96(Game &g,int f,Pos p){ }
static void op_97(Game &g,int f,Pos p){{auto q=p;g.hide(g.hero.floor,q);}
}
static void op_98(Game &g,int f,Pos p){{PendingChoice next;next.instruction=98;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);if(!next.options.empty())g.choice=std::move(next);}}
static void op_99(Game &g,int f,Pos p){ }
static void op_101(Game &g,int f,Pos p){{auto q=p;g.hide(g.hero.floor,q);}
}
static void op_102(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[102]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_103(Game &g,int f,Pos p){ }
static void op_104(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(9LL))))};int floor=35;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_105(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(10LL))))};g.show(35,q);}
}
static void op_106(Game &g,int f,Pos p){{auto q=p;g.hide(g.hero.floor,q);}
}
static void op_107(Game &g,int f,Pos p){g.assign(Target{Ref(0),8789},Value(Number(1LL)),Assign(0),f,p);}
static void op_108(Game &g,int f,Pos p){ }
static void op_109(Game &g,int f,Pos p){{auto b=g.world->entities.at(109).block;b.tile=std::int16_t(245);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
}}
static void op_110(Game &g,int f,Pos p){ }
static void op_111(Game &g,int f,Pos p){{auto b=g.world->entities.at(110).block;b.tile=std::int16_t(246);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(9LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(10LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
}}
static void op_112(Game &g,int f,Pos p){ }
static void op_113(Game &g,int f,Pos p){g.function(11483209625496021176ULL,f,p);}
static void op_114(Game &g,int f,Pos p){ }
static void op_115(Game &g,int f,Pos p){ }
static void op_116(Game &g,int f,Pos p){ }
static void op_117(Game &g,int f,Pos p){ }
static void op_118(Game &g,int f,Pos p){ }
static void op_119(Game &g,int f,Pos p){ }
static void op_120(Game &g,int f,Pos p){ }
static void op_121(Game &g,int f,Pos p){ }
static void op_122(Game &g,int f,Pos p){ }
static void op_123(Game &g,int f,Pos p){ }
static void op_124(Game &g,int f,Pos p){ }
static void op_125(Game &g,int f,Pos p){ }
static void op_126(Game &g,int f,Pos p){{auto v=as_int(Value(Number(400LL)));g.hero.values.at(0)=v;}}
static void op_127(Game &g,int f,Pos p){{auto v=as_int(Value(Number(10LL)));g.hero.values.at(1)=v;}}
static void op_128(Game &g,int f,Pos p){{auto v=as_int(Value(Number(10LL)));g.hero.values.at(2)=v;}}
static void op_129(Game &g,int f,Pos p){g.assign(Target{Ref(0),8789},Value(Number(1LL)),Assign(0),f,p);}
static void op_130(Game &g,int f,Pos p){g.assign(Target{Ref(0),26},Value{},Assign(0),f,p);}
static void op_131(Game &g,int f,Pos p){g.assign(Target{Ref(0),27},Value{},Assign(0),f,p);}
static void op_132(Game &g,int f,Pos p){g.assign(Target{Ref(0),5},Value(false),Assign(0),f,p);}
static void op_133(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(7LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(8LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(9LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(10LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(9LL))))};g.remove(g.hero.floor,q,false);}
}
static void op_134(Game &g,int f,Pos p){{int floor=2;Pos landing=Pos{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(8LL))))};g.transition(floor,landing);g.facing=Direction(1);}}
static void op_135(Game &g,int f,Pos p){ }
static void op_136(Game &g,int f,Pos p){ }
static void op_137(Game &g,int f,Pos p){ }
static void op_138(Game &g,int f,Pos p){ }
static void op_139(Game &g,int f,Pos p){ }
static void op_140(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[140]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_141(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[141]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_142(Game &g,int f,Pos p){g.assign(Target{Ref(0),16},Value(Number(1LL)),Assign(0),f,p);}
static void op_143(Game &g,int f,Pos p){g.assign(Target{Ref(0),8790},([&](){auto a=Value(Number(20LL));auto b=([&](){auto a=([&](){auto a=Value(Number(10LL));auto b=([&](){auto a=g.flag(17,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(7)>(std::move(a),std::move(b));}());return binary<ExprOp(9)>(std::move(a),std::move(b));}());auto b=g.flag(17,Value(Number(0LL)));return binary<ExprOp(9)>(std::move(a),std::move(b));}());return binary<ExprOp(7)>(std::move(a),std::move(b));}()),Assign(0),f,p);}
static void op_144(Game &g,int f,Pos p){ }
static void op_145(Game &g,int f,Pos p){g.function(1748709932395650272ULL,f,p);}
static void op_146(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(Number(100LL));auto b=([&](){auto a=g.flag(17,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(7)>(std::move(a),std::move(b));}());return binary<ExprOp(9)>(std::move(a),std::move(b));}()));g.hero.values.at(0)+=v;}}
static void op_147(Game &g,int f,Pos p){{auto v=as_int(g.flag(8790,Value(Number(0LL))));g.hero.values.at(3)-=v;}}
static void op_148(Game &g,int f,Pos p){g.assign(Target{Ref(0),17},Value(Number(1LL)),Assign(1),f,p);}
static void op_149(Game &g,int f,Pos p){ }
static void op_150(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.hero.values[3]);auto b=g.flag(8790,Value(Number(0LL)));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?41:42);}
static void op_151(Game &g,int f,Pos p){ }
static void op_152(Game &g,int f,Pos p){g.function(1055034504406656087ULL,f,p);}
static void op_153(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(Number(2LL));auto b=g.flag(16,Value(Number(0LL)));return binary<ExprOp(9)>(std::move(a),std::move(b));}()));g.hero.values.at(1)+=v;}}
static void op_154(Game &g,int f,Pos p){{auto v=as_int(g.flag(8790,Value(Number(0LL))));g.hero.values.at(3)-=v;}}
static void op_155(Game &g,int f,Pos p){g.assign(Target{Ref(0),17},Value(Number(1LL)),Assign(1),f,p);}
static void op_156(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.hero.values[3]);auto b=g.flag(8790,Value(Number(0LL)));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?44:42);}
static void op_157(Game &g,int f,Pos p){ }
static void op_158(Game &g,int f,Pos p){g.function(5132412011428455169ULL,f,p);}
static void op_159(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(Number(4LL));auto b=g.flag(16,Value(Number(0LL)));return binary<ExprOp(9)>(std::move(a),std::move(b));}()));g.hero.values.at(2)+=v;}}
static void op_160(Game &g,int f,Pos p){{auto v=as_int(g.flag(8790,Value(Number(0LL))));g.hero.values.at(3)-=v;}}
static void op_161(Game &g,int f,Pos p){g.assign(Target{Ref(0),17},Value(Number(1LL)),Assign(1),f,p);}
static void op_162(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.hero.values[3]);auto b=g.flag(8790,Value(Number(0LL)));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?46:42);}
static void op_163(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_164(Game &g,int f,Pos p){{PendingChoice next;next.instruction=164;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);next.options.push_back(2);next.options.push_back(3);if(!next.options.empty())g.choice=std::move(next);}}
static void op_165(Game &g,int f,Pos p){if(as_truth(Value(true)))g.push(39,f,p,165);}
static void op_166(Game &g,int f,Pos p){g.set_flag(g.world->ids.arg0,g.world->instructions[166].name);g.push(38,f,p);}
static void op_167(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[167]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_168(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[168]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_169(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[169]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_170(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[170]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_171(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[171]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_172(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[172]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_173(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[173]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_174(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[174]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_175(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[175]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_176(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[176]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_178(Game &g,int f,Pos p){g.assign(Target{Ref(0),8791},Value(Number(1LL)),Assign(1),f,p);}
static void op_179(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8791,Value(Number(0LL))))?49:50);}
static void op_180(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[180]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_181(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[181]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_182(Game &g,int f,Pos p){ }
static void op_183(Game &g,int f,Pos p){ }
static void op_184(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_185(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_186(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_187(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(9LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_188(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_189(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(11LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_190(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_191(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_192(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_193(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(9LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_194(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_195(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(11LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_196(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_197(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};g.show(g.hero.floor,q);}
}
static void op_198(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(11LL))))};g.show(g.hero.floor,q);}
}
static void op_202(Game &g,int f,Pos p){g.assign(Target{Ref(0),8792},Value(true),Assign(0),f,p);}
static void op_203(Game &g,int f,Pos p){ }
static void op_204(Game &g,int f,Pos p){ }
static void op_205(Game &g,int f,Pos p){ }
static void op_206(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_207(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_208(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_209(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(9LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_210(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_211(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(11LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_212(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_213(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_214(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_215(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(9LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_216(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_217(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(11LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_218(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_219(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};g.show(g.hero.floor,q);}
}
static void op_220(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(11LL))))};g.show(g.hero.floor,q);}
}
static void op_224(Game &g,int f,Pos p){ }
static void op_225(Game &g,int f,Pos p){g.assign(Target{Ref(0),8792},Value(true),Assign(0),f,p);}
static void op_226(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,true);}
}
static void op_227(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8793,Value(Number(0LL))))?10:53);}
static void op_228(Game &g,int f,Pos p){ }
static void op_229(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_231(Game &g,int f,Pos p){g.set_flag(g.world->ids.bgm,Value{});}
static void op_236(Game &g,int f,Pos p){ }
static void op_237(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_238(Game &g,int f,Pos p){ }
static void op_241(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(270,f,p);auto b=g.world->expressions[271].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?56:57);}
static void op_243(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(4LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_245(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(279,f,p);auto b=g.world->expressions[280].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?58:59);}
static void op_247(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(4LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_249(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(290,f,p);auto b=g.world->expressions[291].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?60:61);}
static void op_250(Game &g,int f,Pos p){ }
static void op_253(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(3LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_255(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(303,f,p);auto b=g.world->expressions[304].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?62:63);}
static void op_257(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(9LL)))),int(as_int(Value(Number(3LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_259(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(314,f,p);auto b=g.world->expressions[315].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?64:65);}
static void op_260(Game &g,int f,Pos p){ }
static void op_262(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(3LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_264(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(325,f,p);auto b=g.world->expressions[326].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?66:67);}
static void op_266(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(3LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_268(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(336,f,p);auto b=g.world->expressions[337].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?68:69);}
static void op_269(Game &g,int f,Pos p){ }
static void op_271(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(3LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_273(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(347,f,p);auto b=g.world->expressions[348].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?70:71);}
static void op_275(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(11LL)))),int(as_int(Value(Number(3LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_277(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(358,f,p);auto b=g.world->expressions[359].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?72:73);}
static void op_278(Game &g,int f,Pos p){ }
static void op_281(Game &g,int f,Pos p){ }
static void op_282(Game &g,int f,Pos p){g.assign(Target{Ref(3),0},Value(true),Assign(0),f,p);}
static void op_283(Game &g,int f,Pos p){g.assign(Target{Ref(0),8793},Value(true),Assign(0),f,p);}
static void op_284(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_286(Game &g,int f,Pos p){ }
static void op_288(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_289(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8792,Value(Number(0LL))))?75:10);}
static void op_290(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[290]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_291(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[291]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_294(Game &g,int f,Pos p){g.assign(Target{Ref(0),8794},Value(Number(1LL)),Assign(1),f,p);}
static void op_295(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8794,Value(Number(0LL))))?78:79);}
static void op_296(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[296]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_297(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[297]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_298(Game &g,int f,Pos p){{auto v=as_int(Value(Number(1000LL)));g.hero.values.at(3)-=v;}}
static void op_299(Game &g,int f,Pos p){g.assign(Target{Ref(2),52},Value(Number(1LL)),Assign(1),f,p);}
static void op_300(Game &g,int f,Pos p){ }
static void op_301(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.hero.values[3]);auto b=Value(Number(1000LL));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?84:85);}
static void op_302(Game &g,int f,Pos p){{PendingChoice next;next.instruction=302;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);if(!next.options.empty())g.choice=std::move(next);}}
static void op_303(Game &g,int f,Pos p){if(as_truth(Value(true)))g.push(82,f,p,303);}
static void op_304(Game &g,int f,Pos p){ }
static void op_306(Game &g,int f,Pos p){{auto b=g.world->entities.at(12).block;b.tile=std::int16_t(124);{auto q=p;int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_307(Game &g,int f,Pos p){g.assign(Target{Ref(3),0},Value(true),Assign(0),f,p);}
static void op_308(Game &g,int f,Pos p){g.prepend(as_truth(g.read(Target{Ref(3),0},f,p))?81:86);}
static void op_309(Game &g,int f,Pos p){g.assign(Target{Ref(0),16},Value(Number(2LL)),Assign(0),f,p);}
static void op_310(Game &g,int f,Pos p){g.set_flag(g.world->ids.arg0,g.world->instructions[310].name);g.push(38,f,p);}
static void op_311(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[311]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_312(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[312]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_313(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[313]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_314(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[314]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_315(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[315]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_316(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[316]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_318(Game &g,int f,Pos p){{auto b=g.world->entities.at(92).block;b.tile=std::int16_t(23);{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_319(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[319]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_320(Game &g,int f,Pos p){ }
static void op_323(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(4LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(5LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(6LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(4LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(5LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(6LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(4LL))))};g.remove(g.hero.floor,q,false);}
}
static void op_325(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[325]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_326(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[326]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_327(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[327]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_328(Game &g,int f,Pos p){g.assign(Target{Ref(2),40},Value(Number(1LL)),Assign(1),f,p);}
static void op_329(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_330(Game &g,int f,Pos p){{PendingChoice next;next.instruction=330;next.floor=f;next.pos=p;next.options.push_back(0);if(!next.options.empty())g.choice=std::move(next);}}
static void op_331(Game &g,int f,Pos p){ }
static void op_333(Game &g,int f,Pos p){{auto b=g.world->entities.at(9).block;b.tile=std::int16_t(121);{auto q=p;int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_334(Game &g,int f,Pos p){g.assign(Target{Ref(3),0},Value(true),Assign(0),f,p);}
static void op_335(Game &g,int f,Pos p){g.prepend(as_truth(g.read(Target{Ref(3),0},f,p))?92:94);}
static void op_336(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[336]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_337(Game &g,int f,Pos p){g.assign(Target{Ref(0),8795},Value(Number(1LL)),Assign(1),f,p);}
static void op_338(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8795,Value(Number(0LL))))?78:96);}
static void op_339(Game &g,int f,Pos p){g.assign(Target{Ref(0),8796},Value(Number(1LL)),Assign(1),f,p);}
static void op_340(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8796,Value(Number(0LL))))?49:98);}
static void op_342(Game &g,int f,Pos p){g.assign(Target{Ref(0),8797},Value(Number(1LL)),Assign(1),f,p);}
static void op_343(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8797,Value(Number(0LL))))?100:101);}
static void op_345(Game &g,int f,Pos p){g.assign(Target{Ref(0),8798},Value(Number(1LL)),Assign(1),f,p);}
static void op_346(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8798,Value(Number(0LL))))?103:104);}
static void op_347(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[347]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_348(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[348]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_349(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[349]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_350(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[350]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_351(Game &g,int f,Pos p){{auto b=g.world->entities.at(196).block;b.tile=std::int16_t(55);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_352(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[352]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_353(Game &g,int f,Pos p){g.assign(Target{Ref(0),8799},Value(Number(0LL)),Assign(0),f,p);}
static void op_354(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[354]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_355(Game &g,int f,Pos p){ }
static void op_356(Game &g,int f,Pos p){ }
static void op_357(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_358(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_359(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_360(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_361(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_362(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_363(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_364(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_365(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_366(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_367(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_368(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_371(Game &g,int f,Pos p){ }
static void op_372(Game &g,int f,Pos p){{auto b=g.world->entities.at(220).block;b.tile=std::int16_t(85);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_373(Game &g,int f,Pos p){ }
static void op_374(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(5LL))))};g.layer(g.hero.floor,q,0,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(5LL))))};g.layer(g.hero.floor,q,0,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(5LL))))};g.layer(g.hero.floor,q,0,false);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(6LL))))};g.layer(g.hero.floor,q,0,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(6LL))))};g.layer(g.hero.floor,q,0,false);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(7LL))))};g.layer(g.hero.floor,q,0,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(7LL))))};g.layer(g.hero.floor,q,0,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(7LL))))};g.layer(g.hero.floor,q,0,false);}
}
static void op_375(Game &g,int f,Pos p){{auto b=g.world->entities.at(66).block;b.tile=std::int16_t(206);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_384(Game &g,int f,Pos p){ }
static void op_385(Game &g,int f,Pos p){{auto b=g.world->entities.at(68).block;b.tile=std::int16_t(208);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
}}
static void op_386(Game &g,int f,Pos p){ }
static void op_387(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(8LL))))};g.remove(g.hero.floor,q,true);}
}
static void op_388(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[388]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_389(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[389]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_390(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[390]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_391(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[391]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_392(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[392]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_393(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[393]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_394(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,true);}
}
static void op_396(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[396]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_397(Game &g,int f,Pos p){ }
static void op_398(Game &g,int f,Pos p){g.function(9649070759834986272ULL,f,p);}
static void op_399(Game &g,int f,Pos p){ }
static void op_400(Game &g,int f,Pos p){{auto begin=Value(Number(24LL));auto end=Value(Number(50LL));auto step=Value(Number(1LL));auto first=as_int(begin),last=as_int(end),increment=as_int(step);if((increment>0&&first>last)||(increment<0&&first<last))return;g.assign(Target{Ref(0),8801},begin,Assign::set,f,p);g.set_flag(8802,g.world->instructions[400].arguments[0]);g.set_flag(8803,g.world->instructions[400].arguments[1]);g.push(112,f,p,400);}}
static void op_401(Game &g,int f,Pos p){{int floor=50;Pos landing=Pos{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(7LL))))};g.transition(floor,landing);g.facing=Direction(1);}}
static void op_402(Game &g,int f,Pos p){ }
static void op_403(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8800,Value(Number(0LL))))?111:10);}
static void op_404(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[404]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_405(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[405]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_406(Game &g,int f,Pos p){ }
static void op_407(Game &g,int f,Pos p){{auto b=g.world->entities.at(92).block;b.tile=std::int16_t(23);{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_408(Game &g,int f,Pos p){{auto b=g.world->entities.at(92).block;b.tile=std::int16_t(23);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_409(Game &g,int f,Pos p){{auto b=g.world->entities.at(92).block;b.tile=std::int16_t(23);{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_410(Game &g,int f,Pos p){{auto b=g.world->entities.at(92).block;b.tile=std::int16_t(23);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_411(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};g.hide(g.hero.floor,q);}
}
static void op_412(Game &g,int f,Pos p){ }
static void op_413(Game &g,int f,Pos p){{auto q=p;g.hide(g.hero.floor,q);}
}
static void op_414(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[414]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_415(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[415]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_416(Game &g,int f,Pos p){ }
static void op_417(Game &g,int f,Pos p){ }
static void op_418(Game &g,int f,Pos p){ }
static void op_419(Game &g,int f,Pos p){ }
static void op_420(Game &g,int f,Pos p){{auto b=g.world->entities.at(159).block;b.tile=std::int16_t(321);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(1LL))))};int floor=24;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_421(Game &g,int f,Pos p){{auto b=g.world->entities.at(1).block;b.tile=std::int16_t(1);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(1LL))))};int floor=24;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_422(Game &g,int f,Pos p){{auto b=g.world->entities.at(1).block;b.tile=std::int16_t(1);{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(1LL))))};int floor=24;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_423(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(2LL))))};int floor=24;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_424(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(3LL))))};int floor=24;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_425(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(4LL))))};int floor=24;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_426(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(2LL))))};g.show(24,q);}
}
static void op_427(Game &g,int f,Pos p){g.assign(Target{Ref(0),8800},Value(true),Assign(0),f,p);}
static void op_428(Game &g,int f,Pos p){g.set_flag(g.world->ids.bgm,g.world->instructions[428].name);}
static void op_429(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8800,Value(Number(0LL))))?116:117);}
static void op_430(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[430]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_431(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[431]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_432(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[432]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_433(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[433]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_434(Game &g,int f,Pos p){ }
static void op_435(Game &g,int f,Pos p){g.assign(Target{Ref(2),52},Value(Number(1LL)),Assign(2),f,p);}
static void op_436(Game &g,int f,Pos p){{auto v=as_int(Value(Number(100LL)));g.hero.values.at(3)+=v;}}
static void op_437(Game &g,int f,Pos p){g.assign(Target{Ref(0),8804},Value(Number(1LL)),Assign(1),f,p);}
static void op_438(Game &g,int f,Pos p){ }
static void op_439(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.item_count(52));auto b=Value(Number(1LL));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?121:122);}
static void op_440(Game &g,int f,Pos p){ }
static void op_441(Game &g,int f,Pos p){g.assign(Target{Ref(2),52},Value(Number(5LL)),Assign(2),f,p);}
static void op_442(Game &g,int f,Pos p){{auto v=as_int(Value(Number(500LL)));g.hero.values.at(3)+=v;}}
static void op_443(Game &g,int f,Pos p){g.assign(Target{Ref(0),8804},Value(Number(5LL)),Assign(1),f,p);}
static void op_444(Game &g,int f,Pos p){ }
static void op_445(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.item_count(52));auto b=Value(Number(5LL));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?124:125);}
static void op_446(Game &g,int f,Pos p){ }
static void op_447(Game &g,int f,Pos p){g.assign(Target{Ref(0),8805},Value(g.item_count(52)),Assign(0),f,p);}
static void op_448(Game &g,int f,Pos p){g.assign(Target{Ref(2),52},Value(Number(0LL)),Assign(0),f,p);}
static void op_449(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(Number(100LL));auto b=g.flag(8805,Value(Number(0LL)));return binary<ExprOp(9)>(std::move(a),std::move(b));}()));g.hero.values.at(3)+=v;}}
static void op_450(Game &g,int f,Pos p){g.assign(Target{Ref(0),8804},g.flag(8805,Value(Number(0LL))),Assign(1),f,p);}
static void op_451(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.item_count(52));auto b=Value(Number(1LL));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?127:122);}
static void op_452(Game &g,int f,Pos p){{PendingChoice next;next.instruction=452;next.floor=f;next.pos=p;next.options.push_back(0);if(as_truth(g.flag(15,Value(Number(0LL)))))next.options.push_back(1);if(as_truth(g.flag(15,Value(Number(0LL)))))next.options.push_back(2);next.options.push_back(3);if(!next.options.empty())g.choice=std::move(next);}}
static void op_453(Game &g,int f,Pos p){if(as_truth(Value(true)))g.push(119,f,p,453);}
static void op_454(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[454]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_455(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[455]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_456(Game &g,int f,Pos p){ }
static void op_457(Game &g,int f,Pos p){ }
static void op_458(Game &g,int f,Pos p){ }
static void op_459(Game &g,int f,Pos p){ }
static void op_460(Game &g,int f,Pos p){ }
static void op_463(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(11LL))))};g.show(2,q);}
}
static void op_464(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_465(Game &g,int f,Pos p){ }
static void op_466(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(15,Value(Number(0LL))))?130:131);}
static void op_467(Game &g,int f,Pos p){ }
static void op_470(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(11LL))))};g.show(2,q);}
}
static void op_471(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_472(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(669,f,p);auto b=Value(Number(0LL));return binary<ExprOp(14)>(std::move(a),std::move(b));}()))?129:132);}
static void op_473(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[473]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_474(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[474]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_475(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[475]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_477(Game &g,int f,Pos p){g.assign(Target{Ref(0),8806},Value(Number(1LL)),Assign(1),f,p);}
static void op_478(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.flag(8806,Value(Number(0LL)));auto b=Value(Number(5LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}()))?134:135);}
static void op_479(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[479]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_480(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[480]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_481(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[481]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_482(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[482]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_484(Game &g,int f,Pos p){g.assign(Target{Ref(0),8807},Value(Number(1LL)),Assign(1),f,p);}
static void op_485(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8807,Value(Number(0LL))))?137:138);}
static void op_487(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(2LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};g.remove(g.hero.floor,q,false);}
}
static void op_488(Game &g,int f,Pos p){{auto b=g.world->entities.at(88).block;b.tile=std::int16_t(226);{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(1LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_490(Game &g,int f,Pos p){ }
static void op_493(Game &g,int f,Pos p){g.function(9613085027927065058ULL,f,p);}
static void op_494(Game &g,int f,Pos p){ }
static void op_495(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.hero.values[1]);auto b=g.eval(726,f,p);return binary<ExprOp(14)>(std::move(a),std::move(b));}()))?143:140);}
static void op_496(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(!as_truth(g.flag(2,Value(Number(0LL)))));return as_truth(a)?g.flag(0,Value(Number(0LL))):a;}()))?142:140);}
static void op_497(Game &g,int f,Pos p){g.prepend(as_truth(Value(g.damage(g.world->entities[88].block)<g.hero[hp]))?140:141);}
static void op_499(Game &g,int f,Pos p){ }
static void op_502(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_503(Game &g,int f,Pos p){g.assign(Target{Ref(0),16},Value(Number(4LL)),Assign(0),f,p);}
static void op_504(Game &g,int f,Pos p){g.set_flag(g.world->ids.arg0,g.world->instructions[504].name);g.push(38,f,p);}
static void op_505(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[505]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_506(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[506]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_507(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[507]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_508(Game &g,int f,Pos p){{auto b=g.world->entities.at(1).block;b.tile=std::int16_t(1);{Pos q{int(as_int(Value(Number(11LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_509(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,true);}
}
static void op_510(Game &g,int f,Pos p){{auto b=g.world->entities.at(220).block;b.tile=std::int16_t(85);{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_511(Game &g,int f,Pos p){g.assign(Target{Ref(0),8808},Value(true),Assign(0),f,p);}
static void op_512(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,true);}
}
static void op_513(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=([&](){auto a=g.eval(738,f,p);auto b=g.world->expressions[739].literal;return binary<ExprOp(19)>(std::move(a),std::move(b));}());return as_truth(a)?([&](){auto a=([&](){auto a=g.eval(743,f,p);auto b=g.world->expressions[744].literal;return binary<ExprOp(19)>(std::move(a),std::move(b));}());return as_truth(a)?([&](){auto a=([&](){auto a=g.eval(748,f,p);auto b=g.world->expressions[749].literal;return binary<ExprOp(19)>(std::move(a),std::move(b));}());return as_truth(a)?([&](){auto a=g.eval(753,f,p);auto b=g.world->expressions[754].literal;return binary<ExprOp(19)>(std::move(a),std::move(b));}()):a;}()):a;}()):a;}()))?146:147);}
static void op_516(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[516]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_517(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[517]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_518(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(6LL))))};g.remove(g.hero.floor,q,false);}
}
static void op_519(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(1LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_520(Game &g,int f,Pos p){{auto b=g.world->entities.at(92).block;b.tile=std::int16_t(23);{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_521(Game &g,int f,Pos p){ }
static void op_522(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[522]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_524(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(7LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(7LL))))};g.remove(g.hero.floor,q,false);}
}
static void op_525(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_526(Game &g,int f,Pos p){{auto b=g.world->entities.at(195).block;b.tile=std::int16_t(54);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_527(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(0,Value(Number(0LL))))?10:151);}
static void op_528(Game &g,int f,Pos p){ }
static void op_529(Game &g,int f,Pos p){ }
static void op_530(Game &g,int f,Pos p){ }
static void op_532(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_533(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[533]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_534(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[534]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_535(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[535]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_536(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[536]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_537(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[537]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_538(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[538]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_539(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[539]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_540(Game &g,int f,Pos p){g.assign(Target{Ref(0),8809},Value(Number(1LL)),Assign(1),f,p);}
static void op_541(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8809,Value(Number(0LL))))?137:154);}
static void op_542(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[542]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_543(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[543]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_545(Game &g,int f,Pos p){{auto b=g.world->entities.at(170).block;b.tile=std::int16_t(331);{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_546(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[546]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_547(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8810,Value(Number(0LL))))?157:10);}
static void op_548(Game &g,int f,Pos p){g.set_flag(g.world->ids.bgm,Value{});}
static void op_554(Game &g,int f,Pos p){ }
static void op_555(Game &g,int f,Pos p){g.function(16456221250688271729ULL,f,p);}
static void op_561(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_562(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_563(Game &g,int f,Pos p){{auto b=g.world->entities.at(70).block;b.tile=std::int16_t(21);{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_564(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_565(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(9LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_566(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_567(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_568(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_569(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_570(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_571(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_572(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(9LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_573(Game &g,int f,Pos p){{auto b=g.world->entities.at(222).block;b.tile=std::int16_t(87);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(1LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_574(Game &g,int f,Pos p){g.assign(Target{Ref(0),8810},Value(true),Assign(0),f,p);}
static void op_575(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(1LL))))};g.show(g.hero.floor,q);}
}
static void op_576(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(7LL))))};g.remove(g.hero.floor,q,false);}
}
static void op_577(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[577]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_578(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(10LL))))};g.show(42,q);}
}
static void op_579(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[579]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_580(Game &g,int f,Pos p){g.assign(Target{Ref(0),8811},Value(Number(1LL)),Assign(0),f,p);}
static void op_581(Game &g,int f,Pos p){ }
static void op_582(Game &g,int f,Pos p){ }
static void op_583(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_589(Game &g,int f,Pos p){ }
static void op_590(Game &g,int f,Pos p){{auto b=g.world->entities.at(193).block;b.tile=std::int16_t(52);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_591(Game &g,int f,Pos p){{auto b=g.world->entities.at(1).block;b.tile=std::int16_t(1);{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(1LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_592(Game &g,int f,Pos p){ }
static void op_593(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_594(Game &g,int f,Pos p){ }
static void op_595(Game &g,int f,Pos p){ }
static void op_596(Game &g,int f,Pos p){{auto b=g.world->entities.at(82).block;b.tile=std::int16_t(220);{auto q=p;int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!true&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,true,false&&floor==g.hero.floor&&animate);}
}}
static void op_597(Game &g,int f,Pos p){g.assign(Target{Ref(0),8811},Value(Number(2LL)),Assign(0),f,p);}
static void op_598(Game &g,int f,Pos p){ }
static void op_599(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=([&](){auto a=g.flag(8811,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?([&](){auto a=([&](){auto a=g.read(Target{Ref(4),0},f,p);auto b=Value(Number(9LL));return binary<ExprOp(18)>(std::move(a),std::move(b));}());return as_truth(a)?([&](){auto a=([&](){auto a=g.read(Target{Ref(5),0},f,p);auto b=Value(Number(2LL));return binary<ExprOp(18)>(std::move(a),std::move(b));}());return as_truth(a)?Value(g.has_visited(42)):a;}()):a;}()):a;}()))?163:10);}
static void op_600(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[600]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_601(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[601]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_602(Game &g,int f,Pos p){ }
static void op_604(Game &g,int f,Pos p){{auto b=g.world->entities.at(109).block;b.tile=std::int16_t(245);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(6LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
}}
static void op_605(Game &g,int f,Pos p){ }
static void op_606(Game &g,int f,Pos p){ }
static void op_607(Game &g,int f,Pos p){ }
static void op_608(Game &g,int f,Pos p){{auto b=g.world->entities.at(110).block;b.tile=std::int16_t(246);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(8LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
}}
static void op_609(Game &g,int f,Pos p){ }
static void op_610(Game &g,int f,Pos p){ }
static void op_611(Game &g,int f,Pos p){g.function(12352334077086398388ULL,f,p);}
static void op_612(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(8LL))))};g.remove(g.hero.floor,q,false);}
}
static void op_613(Game &g,int f,Pos p){ }
static void op_614(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(6LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(7LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(8LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(8LL))))};g.remove(g.hero.floor,q,false);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(9LL))))};g.remove(g.hero.floor,q,false);}
}
static void op_615(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_616(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[616]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_617(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[617]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_619(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(1LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_620(Game &g,int f,Pos p){{auto q=p;g.show(g.hero.floor,q);}
}
static void op_621(Game &g,int f,Pos p){ }
static void op_622(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.item_count(52));auto b=Value(Number(1LL));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?167:168);}
static void op_623(Game &g,int f,Pos p){{auto b=g.world->entities.at(1).block;b.tile=std::int16_t(1);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_624(Game &g,int f,Pos p){ }
static void op_627(Game &g,int f,Pos p){ }
static void op_628(Game &g,int f,Pos p){ }
static void op_630(Game &g,int f,Pos p){{auto q=p;g.remove(g.hero.floor,q,false);}
}
static void op_631(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(1053,f,p);auto b=g.world->expressions[1054].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?170:10);}
static void op_632(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.eval(1040,f,p);auto b=g.world->expressions[1041].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}()))?166:169);}
static void op_633(Game &g,int f,Pos p){{auto b=g.world->entities.at(1).block;b.tile=std::int16_t(1);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_634(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[634]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_635(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[635]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_637(Game &g,int f,Pos p){g.assign(Target{Ref(0),8812},Value(Number(1LL)),Assign(1),f,p);}
static void op_638(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8812,Value(Number(0LL))))?173:174);}
static void op_639(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[639]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_640(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[640]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_642(Game &g,int f,Pos p){g.assign(Target{Ref(0),8813},Value(Number(1LL)),Assign(1),f,p);}
static void op_643(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8813,Value(Number(0LL))))?176:177);}
static void op_645(Game &g,int f,Pos p){g.assign(Target{Ref(0),8814},Value(Number(1LL)),Assign(1),f,p);}
static void op_646(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8814,Value(Number(0LL))))?179:180);}
static void op_647(Game &g,int f,Pos p){g.assign(Target{Ref(0),16},Value(Number(5LL)),Assign(0),f,p);}
static void op_648(Game &g,int f,Pos p){g.set_flag(g.world->ids.arg0,g.world->instructions[648].name);g.push(38,f,p);}
static void op_649(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[649]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_650(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[650]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_651(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[651]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_652(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[652]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_653(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[653]);Pos landing=g.world->floors[floor].down;g.transition(floor,landing);}}
static void op_654(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[654]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_655(Game &g,int f,Pos p){ }
static void op_656(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(2LL))))};g.hide(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(2LL))))};g.hide(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(2LL))))};g.hide(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(3LL))))};g.hide(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(3LL))))};g.hide(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(3LL))))};g.hide(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(4LL))))};g.hide(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(4LL))))};g.hide(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(4LL))))};g.hide(g.hero.floor,q);}
}
static void op_657(Game &g,int f,Pos p){{auto b=g.world->entities.at(92).block;b.tile=std::int16_t(23);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_658(Game &g,int f,Pos p){{auto b=g.world->entities.at(204).block;b.tile=std::int16_t(62);{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_659(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(2LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_660(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(3LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_661(Game &g,int f,Pos p){{auto b=g.world->entities.at(131).block;b.tile=std::int16_t(27);{Pos q{int(as_int(Value(Number(4LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_662(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(8LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_663(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(9LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_664(Game &g,int f,Pos p){{auto b=g.world->entities.at(132).block;b.tile=std::int16_t(28);{Pos q{int(as_int(Value(Number(10LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_665(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_666(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_667(Game &g,int f,Pos p){{auto b=g.world->entities.at(157).block;b.tile=std::int16_t(32);{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(5LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_668(Game &g,int f,Pos p){{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(2LL))))};g.show(g.hero.floor,q);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(2LL))))};g.show(g.hero.floor,q);}
}
static void op_671(Game &g,int f,Pos p){{auto b=g.world->entities.at(220).block;b.tile=std::int16_t(85);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_672(Game &g,int f,Pos p){{auto b=g.world->entities.at(109).block;b.tile=std::int16_t(245);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
}}
static void op_673(Game &g,int f,Pos p){ }
static void op_674(Game &g,int f,Pos p){g.set_flag(g.world->ids.bgm,Value{});}
static void op_675(Game &g,int f,Pos p){{auto b=g.world->entities.at(0).block;{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_676(Game &g,int f,Pos p){{auto b=g.world->entities.at(110).block;b.tile=std::int16_t(246);{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(5LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(4LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(3LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(7LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(2LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,true&&floor==g.hero.floor&&animate);}
}}
static void op_677(Game &g,int f,Pos p){{auto q=p;g.hide(g.hero.floor,q);}
}
static void op_679(Game &g,int f,Pos p){g.assign(Target{Ref(0),8815},Value(Number(1LL)),Assign(1),f,p);}
static void op_680(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8815,Value(Number(0LL))))?185:186);}
static void op_682(Game &g,int f,Pos p){g.assign(Target{Ref(0),8816},Value(Number(1LL)),Assign(1),f,p);}
static void op_683(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8816,Value(Number(0LL))))?188:189);}
static void op_684(Game &g,int f,Pos p){{int floor=g.event_floor(g.world->instructions[684]);Pos landing=g.world->floors[floor].up;g.transition(floor,landing);}}
static void op_685(Game &g,int f,Pos p){g.assign(Target{Ref(0),8818},Value(true),Assign(0),f,p);}
static void op_686(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8817,Value(Number(0LL))))?191:10);}
static void op_690(Game &g,int f,Pos p){ }
static void op_691(Game &g,int f,Pos p){ }
static void op_692(Game &g,int f,Pos p){ }
static void op_695(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8818,Value(Number(0LL))))?195:196);}
static void op_698(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8818,Value(Number(0LL))))?198:199);}
static void op_699(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(0,Value(Number(0LL))))?194:197);}
static void op_700(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(2,Value(Number(0LL))))?193:10);}
static void op_701(Game &g,int f,Pos p){g.assign(Target{Ref(0),8819},g.world->expressions[1246].literal,Assign(0),f,p);}
static void op_702(Game &g,int f,Pos p){g.assign(Target{Ref(0),8819},g.world->expressions[1247].literal,Assign(0),f,p);}
static void op_703(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(0,Value(Number(0LL))))?200:201);}
static void op_704(Game &g,int f,Pos p){g.assign(Target{Ref(0),8820},g.world->expressions[1251].literal,Assign(0),f,p);}
static void op_705(Game &g,int f,Pos p){g.assign(Target{Ref(0),8820},g.world->expressions[1252].literal,Assign(0),f,p);}
static void op_706(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8818,Value(Number(0LL))))?202:203);}
static void op_707(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1254].literal,Assign(0),f,p);}
static void op_708(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_709(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1255].literal,Assign(0),f,p);}
static void op_710(Game &g,int f,Pos p){{auto v=as_int(Value(g.hero.values[1]));g.hero.values.at(0)=v;}}
static void op_711(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_712(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1258].literal,Assign(0),f,p);}
static void op_713(Game &g,int f,Pos p){{auto v=as_int(Value(g.hero.values[2]));g.hero.values.at(0)=v;}}
static void op_714(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_715(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1261].literal,Assign(0),f,p);}
static void op_716(Game &g,int f,Pos p){{auto v=as_int(Value(g.hero.values[3]));g.hero.values.at(0)=v;}}
static void op_717(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_718(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1264].literal,Assign(0),f,p);}
static void op_719(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(g.hero.values[1]);auto b=Value(g.hero.values[2]);return binary<ExprOp(7)>(std::move(a),std::move(b));}()));g.hero.values.at(0)=v;}}
static void op_720(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_721(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1270].literal,Assign(0),f,p);}
static void op_722(Game &g,int f,Pos p){{auto v=as_int(g.flag(17,Value(Number(0LL))));g.hero.values.at(0)=v;}}
static void op_723(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_724(Game &g,int f,Pos p){g.function(11852332805925902340ULL,f,p);}
static void op_725(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1274].literal,Assign(0),f,p);}
static void op_726(Game &g,int f,Pos p){{auto v=as_int(g.flag(29,Value(Number(0LL))));g.hero.values.at(0)=v;}}
static void op_727(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_728(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1278].literal,Assign(0),f,p);}
static void op_729(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(g.hero.values[1]);auto b=([&](){auto a=Value(Number(1000000000LL));auto b=([&](){auto a=Value(Number(100000LL));auto b=Value(g.hero.values[1]);return binary<ExprOp(9)>(std::move(a),std::move(b));}());return binary<ExprOp(8)>(std::move(a),std::move(b));}());return binary<ExprOp(7)>(std::move(a),std::move(b));}()));g.hero.values.at(0)=v;}}
static void op_730(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_731(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1288].literal,Assign(0),f,p);}
static void op_732(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(g.hero.values[2]);auto b=([&](){auto a=Value(Number(1000000000LL));auto b=([&](){auto a=Value(Number(100000LL));auto b=Value(g.hero.values[2]);return binary<ExprOp(9)>(std::move(a),std::move(b));}());return binary<ExprOp(8)>(std::move(a),std::move(b));}());return binary<ExprOp(7)>(std::move(a),std::move(b));}()));g.hero.values.at(0)=v;}}
static void op_733(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_734(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1298].literal,Assign(0),f,p);}
static void op_735(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=([&](){auto a=Value(g.hero.values[1]);auto b=Value(g.hero.values[2]);return binary<ExprOp(7)>(std::move(a),std::move(b));}());auto b=([&](){auto a=Value(Number(1000000000LL));auto b=([&](){auto a=Value(Number(100000LL));auto b=([&](){auto a=Value(g.hero.values[1]);auto b=Value(g.hero.values[2]);return binary<ExprOp(7)>(std::move(a),std::move(b));}());return binary<ExprOp(9)>(std::move(a),std::move(b));}());return binary<ExprOp(8)>(std::move(a),std::move(b));}());return binary<ExprOp(7)>(std::move(a),std::move(b));}()));g.hero.values.at(0)=v;}}
static void op_736(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_737(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1314].literal,Assign(0),f,p);}
static void op_738(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=([&](){auto a=Value(Number(100000LL));auto b=([&](){auto a=Value(Number(1000LL));auto b=g.flag(17,Value(Number(0LL)));return binary<ExprOp(9)>(std::move(a),std::move(b));}());return binary<ExprOp(8)>(std::move(a),std::move(b));}());auto b=g.flag(17,Value(Number(0LL)));return binary<ExprOp(7)>(std::move(a),std::move(b));}()));g.hero.values.at(0)=v;}}
static void op_739(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_740(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1326].literal,Assign(0),f,p);}
static void op_741(Game &g,int f,Pos p){{auto v=as_int(Value(g.item_count(52)));g.hero.values.at(0)=v;}}
static void op_742(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_743(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1329].literal,Assign(0),f,p);}
static void op_744(Game &g,int f,Pos p){{auto v=as_int(Value(g.item_count(5)));g.hero.values.at(0)=v;}}
static void op_745(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_746(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1332].literal,Assign(0),f,p);}
static void op_747(Game &g,int f,Pos p){{auto v=as_int(Value(g.item_count(29)));g.hero.values.at(0)=v;}}
static void op_748(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_749(Game &g,int f,Pos p){g.assign(Target{Ref(0),8821},g.world->expressions[1335].literal,Assign(0),f,p);}
static void op_750(Game &g,int f,Pos p){g.function(16714455262767558548ULL,f,p);}
static void op_751(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_752(Game &g,int f,Pos p){{PendingChoice next;next.instruction=752;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);next.options.push_back(2);next.options.push_back(3);next.options.push_back(4);if(!next.options.empty())g.choice=std::move(next);}}
static void op_753(Game &g,int f,Pos p){{PendingChoice next;next.instruction=753;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);next.options.push_back(2);next.options.push_back(3);next.options.push_back(4);if(!next.options.empty())g.choice=std::move(next);}}
static void op_754(Game &g,int f,Pos p){{PendingChoice next;next.instruction=754;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);next.options.push_back(2);next.options.push_back(3);next.options.push_back(4);next.options.push_back(5);next.options.push_back(6);next.options.push_back(7);if(!next.options.empty())g.choice=std::move(next);}}
static void op_755(Game &g,int f,Pos p){if(as_truth(Value(true)))g.push(204,f,p,755);}
static void op_757(Game &g,int f,Pos p){g.set_flag(g.world->ids.bgm,Value{});}
static void op_758(Game &g,int f,Pos p){ }
static void op_759(Game &g,int f,Pos p){ }
static void op_760(Game &g,int f,Pos p){{auto q=p;g.hide(g.hero.floor,q);}
}
static void op_764(Game &g,int f,Pos p){{auto b=g.world->entities.at(109).block;b.tile=std::int16_t(245);{auto q=p;int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_765(Game &g,int f,Pos p){{auto q=p;g.show(g.hero.floor,q);}
}
static void op_766(Game &g,int f,Pos p){ }
static void op_767(Game &g,int f,Pos p){ }
static void op_768(Game &g,int f,Pos p){ }
static void op_769(Game &g,int f,Pos p){ }
static void op_770(Game &g,int f,Pos p){ }
static void op_771(Game &g,int f,Pos p){ }
static void op_772(Game &g,int f,Pos p){ }
static void op_773(Game &g,int f,Pos p){ }
static void op_774(Game &g,int f,Pos p){ }
static void op_775(Game &g,int f,Pos p){ }
static void op_776(Game &g,int f,Pos p){ }
static void op_777(Game &g,int f,Pos p){g.assign(Target{Ref(0),8817},Value(true),Assign(0),f,p);}
static void op_778(Game &g,int f,Pos p){ }
static void op_779(Game &g,int f,Pos p){ }
static void op_780(Game &g,int f,Pos p){ }
static void op_781(Game &g,int f,Pos p){ }
static void op_782(Game &g,int f,Pos p){ }
static void op_783(Game &g,int f,Pos p){ }
static void op_784(Game &g,int f,Pos p){ }
static void op_785(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(Number(1LL));auto b=g.flag(19,Value(Number(0LL)));return binary<ExprOp(9)>(std::move(a),std::move(b));}()));g.hero.values.at(1)+=v;}}
static void op_786(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(Number(2LL));auto b=g.flag(19,Value(Number(0LL)));return binary<ExprOp(9)>(std::move(a),std::move(b));}()));g.hero.values.at(2)+=v;}}
static void op_787(Game &g,int f,Pos p){{auto v=as_int(([&](){auto a=Value(Number(200LL));auto b=g.flag(19,Value(Number(0LL)));return binary<ExprOp(9)>(std::move(a),std::move(b));}()));g.hero.values.at(0)+=v;}}
static void op_788(Game &g,int f,Pos p){{PendingChoice next;next.instruction=788;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);next.options.push_back(2);if(!next.options.empty())g.choice=std::move(next);}}
static void op_789(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(50LL)),Assign(0),f,p);}
static void op_790(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1363].literal,Assign(0),f,p);}
static void op_791(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(50LL)),Assign(0),f,p);}
static void op_792(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1366].literal,Assign(0),f,p);}
static void op_793(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(800LL)),Assign(0),f,p);}
static void op_794(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1369].literal,Assign(0),f,p);}
static void op_795(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(200LL)),Assign(0),f,p);}
static void op_796(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1372].literal,Assign(0),f,p);}
static void op_797(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(1000LL)),Assign(0),f,p);}
static void op_798(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1375].literal,Assign(0),f,p);}
static void op_799(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(200LL)),Assign(0),f,p);}
static void op_800(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1378].literal,Assign(0),f,p);}
static void op_801(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(2000LL)),Assign(0),f,p);}
static void op_802(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1381].literal,Assign(0),f,p);}
static void op_803(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(1000LL)),Assign(0),f,p);}
static void op_804(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1384].literal,Assign(0),f,p);}
static void op_805(Game &g,int f,Pos p){g.assign(Target{Ref(0),8822},Value(Number(4000LL)),Assign(0),f,p);}
static void op_806(Game &g,int f,Pos p){g.assign(Target{Ref(0),8823},g.world->expressions[1387].literal,Assign(0),f,p);}
static void op_807(Game &g,int f,Pos p){ }
static void op_808(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_809(Game &g,int f,Pos p){{auto key=g.flag(19,Value(Number(0LL)));SmallVector<int,4> parts;bool done=false;if(!done && (equal_value(Value(Number(6LL)),key,true))){parts.push_back(229);done=true;}if(!done && (equal_value(Value(Number(7LL)),key,true))){parts.push_back(230);done=true;}if(!done && (equal_value(Value(Number(12LL)),key,true))){parts.push_back(231);done=true;}if(!done && (equal_value(Value(Number(15LL)),key,true))){parts.push_back(232);done=true;}if(!done && (equal_value(Value(Number(31LL)),key,true))){parts.push_back(233);done=true;}if(!done && (equal_value(Value(Number(38LL)),key,true))){parts.push_back(234);done=true;}if(!done && (equal_value(Value(Number(39LL)),key,true))){parts.push_back(235);done=true;}if(!done && (equal_value(Value(Number(45LL)),key,true))){parts.push_back(236);done=true;}if(!done && (equal_value(Value(Number(47LL)),key,true))){parts.push_back(237);done=true;}if(!done && (true)){parts.push_back(238);done=true;}for(auto it=parts.rbegin();it!=parts.rend();++it)g.prepend(*it);}}
static void op_810(Game &g,int f,Pos p){{auto v=as_int(g.flag(8822,Value(Number(0LL))));g.hero.values.at(3)-=v;}}
static void op_811(Game &g,int f,Pos p){g.assign(Target{Ref(2),5},Value(Number(1LL)),Assign(1),f,p);}
static void op_812(Game &g,int f,Pos p){g.assign(Target{Ref(2),52},Value(Number(5LL)),Assign(1),f,p);}
static void op_813(Game &g,int f,Pos p){g.assign(Target{Ref(2),29},Value(Number(1LL)),Assign(1),f,p);}
static void op_814(Game &g,int f,Pos p){g.assign(Target{Ref(2),52},Value(Number(4LL)),Assign(1),f,p);}
static void op_815(Game &g,int f,Pos p){g.assign(Target{Ref(2),5},Value(Number(1LL)),Assign(1),f,p);}
static void op_816(Game &g,int f,Pos p){g.assign(Target{Ref(2),52},Value(Number(3LL)),Assign(1),f,p);}
static void op_817(Game &g,int f,Pos p){g.assign(Target{Ref(2),5},Value(Number(3LL)),Assign(1),f,p);}
static void op_818(Game &g,int f,Pos p){ }
static void op_819(Game &g,int f,Pos p){{auto v=as_int(Value(Number(2000LL)));g.hero.values.at(0)+=v;}}
static void op_820(Game &g,int f,Pos p){g.assign(Target{Ref(2),15},Value(Number(1LL)),Assign(1),f,p);}
static void op_821(Game &g,int f,Pos p){{auto key=g.flag(19,Value(Number(0LL)));SmallVector<int,4> parts;bool done=false;if(!done && (equal_value(Value(Number(6LL)),key,true))){parts.push_back(241);done=true;}if(!done && (equal_value(Value(Number(7LL)),key,true))){parts.push_back(242);done=true;}if(!done && (equal_value(Value(Number(12LL)),key,true))){parts.push_back(243);done=true;}if(!done && (equal_value(Value(Number(15LL)),key,true))){parts.push_back(241);done=true;}if(!done && (equal_value(Value(Number(31LL)),key,true))){parts.push_back(244);done=true;}if(!done && (equal_value(Value(Number(38LL)),key,true))){parts.push_back(245);done=true;}if(!done && (equal_value(Value(Number(39LL)),key,true))){parts.push_back(246);done=true;}if(!done && (equal_value(Value(Number(45LL)),key,true))){parts.push_back(247);done=true;}if(!done && (equal_value(Value(Number(47LL)),key,true))){parts.push_back(248);done=true;}if(!done && (true)){parts.push_back(10);done=true;}for(auto it=parts.rbegin();it!=parts.rend();++it)g.prepend(*it);}}
static void op_822(Game &g,int f,Pos p){g.function(3857571145612688057ULL,f,p);}
static void op_823(Game &g,int f,Pos p){ }
static void op_824(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.hero.values[3]);auto b=g.flag(8822,Value(Number(0LL)));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?240:249);}
static void op_825(Game &g,int f,Pos p){{PendingChoice next;next.instruction=825;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);if(!next.options.empty())g.choice=std::move(next);}}
static void op_826(Game &g,int f,Pos p){g.assign(Target{Ref(0),8824},Value(Number(0LL)),Assign(0),f,p);}
static void op_827(Game &g,int f,Pos p){ }
static void op_828(Game &g,int f,Pos p){ }
static void op_829(Game &g,int f,Pos p){ }
static void op_830(Game &g,int f,Pos p){g.assign(Target{Ref(2),52},Value(Number(1LL)),Assign(2),f,p);}
static void op_831(Game &g,int f,Pos p){{auto v=as_int(Value(Number(10LL)));g.hero.values.at(3)+=v;}}
static void op_832(Game &g,int f,Pos p){ }
static void op_833(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.item_count(52));auto b=Value(Number(1LL));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?253:254);}
static void op_834(Game &g,int f,Pos p){g.assign(Target{Ref(2),5},Value(Number(1LL)),Assign(2),f,p);}
static void op_835(Game &g,int f,Pos p){{auto v=as_int(Value(Number(50LL)));g.hero.values.at(3)+=v;}}
static void op_836(Game &g,int f,Pos p){ }
static void op_837(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=Value(g.item_count(5));auto b=Value(Number(1LL));return binary<ExprOp(15)>(std::move(a),std::move(b));}()))?256:257);}
static void op_838(Game &g,int f,Pos p){g.frames.clear();g.choice.reset();}
static void op_839(Game &g,int f,Pos p){{PendingChoice next;next.instruction=839;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);next.options.push_back(2);if(!next.options.empty())g.choice=std::move(next);}}
static void op_840(Game &g,int f,Pos p){if(as_truth(Value(Number(1LL))))g.push(251,f,p,840);}
static void op_841(Game &g,int f,Pos p){g.assign(Target{Ref(0),8825},Value(false),Assign(0),f,p);}
static void op_842(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1437].literal,Assign(0),f,p);}
static void op_843(Game &g,int f,Pos p){g.assign(Target{Ref(0),8825},Value(true),Assign(0),f,p);}
static void op_844(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1440].literal,Assign(0),f,p);}
static void op_845(Game &g,int f,Pos p){g.assign(Target{Ref(0),8825},Value(true),Assign(0),f,p);}
static void op_846(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1443].literal,Assign(0),f,p);}
static void op_847(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1448].literal,Assign(0),f,p);}
static void op_848(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1449].literal,Assign(0),f,p);}
static void op_849(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(22,Value(Number(0LL))))?264:265);}
static void op_850(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1451].literal,Assign(0),f,p);}
static void op_851(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1453].literal,Assign(0),f,p);}
static void op_852(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1455].literal,Assign(0),f,p);}
static void op_853(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1457].literal,Assign(0),f,p);}
static void op_854(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1459].literal,Assign(0),f,p);}
static void op_855(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1461].literal,Assign(0),f,p);}
static void op_856(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1463].literal,Assign(0),f,p);}
static void op_857(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1465].literal,Assign(0),f,p);}
static void op_858(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1470].literal,Assign(0),f,p);}
static void op_859(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1471].literal,Assign(0),f,p);}
static void op_860(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(22,Value(Number(0LL))))?275:276);}
static void op_861(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1473].literal,Assign(0),f,p);}
static void op_862(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1475].literal,Assign(0),f,p);}
static void op_863(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1477].literal,Assign(0),f,p);}
static void op_864(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1479].literal,Assign(0),f,p);}
static void op_865(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1484].literal,Assign(0),f,p);}
static void op_866(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1485].literal,Assign(0),f,p);}
static void op_867(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(22,Value(Number(0LL))))?282:283);}
static void op_868(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1487].literal,Assign(0),f,p);}
static void op_869(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1492].literal,Assign(0),f,p);}
static void op_870(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1493].literal,Assign(0),f,p);}
static void op_871(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(22,Value(Number(0LL))))?286:287);}
static void op_872(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1495].literal,Assign(0),f,p);}
static void op_873(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1497].literal,Assign(0),f,p);}
static void op_874(Game &g,int f,Pos p){g.assign(Target{Ref(0),8826},g.world->expressions[1499].literal,Assign(0),f,p);}
static void op_875(Game &g,int f,Pos p){ }
static void op_876(Game &g,int f,Pos p){{int n=1;while(n--&&g.frames.size()>1)g.frames.pop_back();}}
static void op_877(Game &g,int f,Pos p){{auto key=g.flag(19,Value(Number(0LL)));SmallVector<int,4> parts;bool done=false;if(!done && (equal_value(Value(Number(2LL)),key,true))){parts.push_back(260);done=true;}if(!done && (equal_value(Value(Number(3LL)),key,true))){parts.push_back(261);done=true;}if(!done && (equal_value(Value(Number(4LL)),key,true))){parts.push_back(262);done=true;}if(!done && (equal_value(Value(Number(6LL)),key,true))){parts.push_back(263);done=true;}if(!done && (equal_value(Value(Number(7LL)),key,true))){parts.push_back(266);done=true;}if(!done && (equal_value(Value(Number(12LL)),key,true))){parts.push_back(267);done=true;}if(!done && (equal_value(Value(Number(15LL)),key,true))){parts.push_back(268);done=true;}if(!done && (equal_value(Value(Number(16LL)),key,true))){parts.push_back(269);done=true;}if(!done && (equal_value(Value(Number(18LL)),key,true))){parts.push_back(270);done=true;}if(!done && (equal_value(Value(Number(21LL)),key,true))){parts.push_back(271);done=true;}if(!done && (equal_value(Value(Number(23LL)),key,true))){parts.push_back(272);done=true;}if(!done && (equal_value(Value(Number(27LL)),key,true))){parts.push_back(273);done=true;}if(!done && (equal_value(Value(Number(31LL)),key,true))){parts.push_back(274);done=true;}if(!done && (equal_value(Value(Number(33LL)),key,true))){parts.push_back(277);done=true;}if(!done && (equal_value(Value(Number(36LL)),key,true))){parts.push_back(278);done=true;}if(!done && (equal_value(Value(Number(37LL)),key,true))){parts.push_back(279);done=true;}if(!done && (equal_value(Value(Number(38LL)),key,true))){parts.push_back(280);done=true;}if(!done && (equal_value(Value(Number(39LL)),key,true))){parts.push_back(281);done=true;}if(!done && (equal_value(Value(Number(42LL)),key,true))){parts.push_back(284);done=true;}if(!done && (equal_value(Value(Number(45LL)),key,true))){parts.push_back(285);done=true;}if(!done && (equal_value(Value(Number(46LL)),key,true))){parts.push_back(288);done=true;}if(!done && (equal_value(Value(Number(47LL)),key,true))){parts.push_back(289);done=true;}if(!done && (equal_value(Value(Number(48LL)),key,true))){parts.push_back(290);done=true;}if(!done && (true)){parts.push_back(291);done=true;}for(auto it=parts.rbegin();it!=parts.rend();++it)g.prepend(*it);}}
static void op_878(Game &g,int f,Pos p){ }
static void op_879(Game &g,int f,Pos p){{auto v=as_int(Value(Number(1000LL)));g.hero.values.at(3)+=v;}}
static void op_880(Game &g,int f,Pos p){ }
static void op_881(Game &g,int f,Pos p){g.assign(Target{Ref(2),8},Value(Number(1LL)),Assign(1),f,p);}
static void op_882(Game &g,int f,Pos p){{auto key=g.flag(19,Value(Number(0LL)));SmallVector<int,4> parts;bool done=false;if(!done && (equal_value(Value(Number(2LL)),key,true))){parts.push_back(294);done=true;}if(!done && (equal_value(Value(Number(3LL)),key,true))){parts.push_back(295);done=true;}if(!done && (true)){parts.push_back(10);done=true;}for(auto it=parts.rbegin();it!=parts.rend();++it)g.prepend(*it);}}
static void op_883(Game &g,int f,Pos p){{PendingChoice next;next.instruction=883;next.floor=f;next.pos=p;next.options.push_back(0);if(!next.options.empty())g.choice=std::move(next);}}
static void op_884(Game &g,int f,Pos p){ }
static void op_885(Game &g,int f,Pos p){ }
static void op_886(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(22,Value(Number(0LL))))?297:298);}
static void op_887(Game &g,int f,Pos p){g.prepend(as_truth(g.flag(8825,Value(Number(0LL))))?292:296);}
static void op_888(Game &g,int f,Pos p){g.function(4155912196564257628ULL,f,p);}
static void op_889(Game &g,int f,Pos p){{Pos q{int(as_int(g.flag(20,Value(Number(0LL))))),int(as_int(g.flag(21,Value(Number(0LL)))))};g.remove(g.hero.floor,q,false);}
}
static void op_890(Game &g,int f,Pos p){g.function(9800606157649413712ULL,f,p);}
static void op_891(Game &g,int f,Pos p){ }
static void op_892(Game &g,int f,Pos p){ }
static void op_893(Game &g,int f,Pos p){ }
static void op_894(Game &g,int f,Pos p){ }
static void op_895(Game &g,int f,Pos p){ }
static void op_896(Game &g,int f,Pos p){ }
static void op_897(Game &g,int f,Pos p){ }
static void op_898(Game &g,int f,Pos p){ }
static void op_899(Game &g,int f,Pos p){ }
static void op_900(Game &g,int f,Pos p){ }
static void op_901(Game &g,int f,Pos p){ }
static void op_902(Game &g,int f,Pos p){g.assign(Target{Ref(2),1},Value(Number(1LL)),Assign(0),f,p);}
static void op_903(Game &g,int f,Pos p){ }
static void op_904(Game &g,int f,Pos p){g.assign(Target{Ref(0),15},Value(true),Assign(0),f,p);}
static void op_905(Game &g,int f,Pos p){{PendingChoice next;next.instruction=905;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);if(!next.options.empty())g.choice=std::move(next);}}
static void op_906(Game &g,int f,Pos p){ }
static void op_907(Game &g,int f,Pos p){g.assign(Target{Ref(2),1},Value(Number(0LL)),Assign(0),f,p);}
static void op_908(Game &g,int f,Pos p){ }
static void op_909(Game &g,int f,Pos p){g.assign(Target{Ref(0),15},Value(false),Assign(0),f,p);}
static void op_910(Game &g,int f,Pos p){{PendingChoice next;next.instruction=910;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);if(!next.options.empty())g.choice=std::move(next);}}
static void op_911(Game &g,int f,Pos p){g.function(14362199384434025912ULL,f,p);}
static void op_912(Game &g,int f,Pos p){g.function(8696697210236011015ULL,f,p);}
static void op_913(Game &g,int f,Pos p){ }
static void op_914(Game &g,int f,Pos p){g.function(15702400654609893004ULL,f,p);}
static void op_915(Game &g,int f,Pos p){ }
static void op_916(Game &g,int f,Pos p){g.function(952108694152041818ULL,f,p);}
static void op_917(Game &g,int f,Pos p){ }
static void op_918(Game &g,int f,Pos p){g.function(14337444706862829927ULL,f,p);}
static void op_919(Game &g,int f,Pos p){ }
static void op_920(Game &g,int f,Pos p){g.function(3779378094499321005ULL,f,p);}
static void op_921(Game &g,int f,Pos p){ }
static void op_922(Game &g,int f,Pos p){g.function(17259411391776624394ULL,f,p);}
static void op_923(Game &g,int f,Pos p){ }
static void op_924(Game &g,int f,Pos p){g.function(14585032360387175852ULL,f,p);}
static void op_925(Game &g,int f,Pos p){ }
static void op_926(Game &g,int f,Pos p){g.function(952108694152041818ULL,f,p);}
static void op_927(Game &g,int f,Pos p){ }
static void op_928(Game &g,int f,Pos p){{PendingChoice next;next.instruction=928;next.floor=f;next.pos=p;if(as_truth(Value(!as_truth(g.read(Target{Ref(8),0},f,p)))))next.options.push_back(0);if(as_truth(g.read(Target{Ref(8),0},f,p)))next.options.push_back(1);if(as_truth(Value(!as_truth(g.read(Target{Ref(10),0},f,p)))))next.options.push_back(2);if(as_truth(g.read(Target{Ref(10),0},f,p)))next.options.push_back(3);if(as_truth(Value(!as_truth(g.read(Target{Ref(9),0},f,p)))))next.options.push_back(4);if(as_truth(g.read(Target{Ref(9),0},f,p)))next.options.push_back(5);next.options.push_back(6);if(as_truth(([&](){auto a=g.read(Target{Ref(10),0},f,p);auto b=Value(true);return binary<ExprOp(18)>(std::move(a),std::move(b));}())))next.options.push_back(7);if(as_truth(([&](){auto a=g.read(Target{Ref(10),0},f,p);auto b=g.world->expressions[1548].literal;return binary<ExprOp(18)>(std::move(a),std::move(b));}())))next.options.push_back(8);if(!next.options.empty())g.choice=std::move(next);}}
static void op_929(Game &g,int f,Pos p){if(as_truth(Value(Number(1LL))))g.push(309,f,p,929);}
static void op_930(Game &g,int f,Pos p){g.function(1589162448891063218ULL,f,p);}
static void op_931(Game &g,int f,Pos p){g.assign(Target{Ref(0),8787},Value(false),Assign(0),f,p);}
static void op_932(Game &g,int f,Pos p){g.assign(Target{Ref(0),8787},Value(true),Assign(0),f,p);}
static void op_933(Game &g,int f,Pos p){{PendingChoice next;next.instruction=933;next.floor=f;next.pos=p;next.options.push_back(0);next.options.push_back(1);next.options.push_back(2);if(as_truth(Value(!as_truth(g.flag(15,Value(Number(0LL)))))))next.options.push_back(3);if(as_truth(g.flag(15,Value(Number(0LL)))))next.options.push_back(4);if(as_truth(g.flag(15,Value(Number(0LL)))))next.options.push_back(5);next.options.push_back(6);next.options.push_back(7);if(as_truth(g.flag(8787,Value(Number(0LL)))))next.options.push_back(8);if(as_truth(Value(!as_truth(g.flag(8787,Value(Number(0LL)))))))next.options.push_back(9);if(!next.options.empty())g.choice=std::move(next);}}
static void op_934(Game &g,int f,Pos p){if(as_truth(Value(true)))g.push(300,f,p,934);}
static void op_935(Game &g,int f,Pos p){g.assign(Target{Ref(0),8827},Value(Number(1LL)),Assign(0),f,p);}
static void op_936(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.flag(8827,Value(Number(0LL)));auto b=Value(Number(0LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}()))?322:10);}
static void op_937(Game &g,int f,Pos p){ }
static void op_938(Game &g,int f,Pos p){ }
static void op_939(Game &g,int f,Pos p){ }
static void op_940(Game &g,int f,Pos p){ }
static void op_941(Game &g,int f,Pos p){ }
static void op_942(Game &g,int f,Pos p){ }
static void op_943(Game &g,int f,Pos p){ }
static void op_944(Game &g,int f,Pos p){ }
static void op_945(Game &g,int f,Pos p){ }
static void op_946(Game &g,int f,Pos p){g.assign(Target{Ref(0),8827},Value(Number(3LL)),Assign(0),f,p);}
static void op_947(Game &g,int f,Pos p){g.set_flag(g.world->ids.arg0,g.world->instructions[947].name);g.push(321,f,p);}
static void op_948(Game &g,int f,Pos p){g.assign(Target{Ref(0),8827},Value(Number(2LL)),Assign(0),f,p);}
static void op_949(Game &g,int f,Pos p){g.set_flag(g.world->ids.arg0,g.world->instructions[949].name);g.push(321,f,p);}
static void op_950(Game &g,int f,Pos p){g.assign(Target{Ref(0),8827},Value(Number(1LL)),Assign(0),f,p);}
static void op_951(Game &g,int f,Pos p){g.frames.clear();g.choice.reset();}
static void op_952(Game &g,int f,Pos p){{PendingChoice next;next.instruction=952;next.floor=f;next.pos=p;if(as_truth(([&](){auto a=g.flag(372,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(0);if(as_truth(([&](){auto a=g.flag(543,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(1);if(as_truth(([&](){auto a=g.flag(714,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(2);if(as_truth(([&](){auto a=([&](){auto a=g.flag(1056,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?Value(!as_truth(g.flag(22,Value(Number(0LL))))):a;}())))next.options.push_back(3);if(as_truth(([&](){auto a=([&](){auto a=g.flag(1057,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?g.flag(22,Value(Number(0LL))):a;}())))next.options.push_back(4);if(as_truth(([&](){auto a=g.flag(1227,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(5);if(as_truth(([&](){auto a=g.flag(2082,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(6);if(as_truth(([&](){auto a=g.flag(2595,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(7);if(as_truth(([&](){auto a=g.flag(2766,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(8);next.options.push_back(9);next.options.push_back(10);next.options.push_back(11);if(!next.options.empty())g.choice=std::move(next);}}
static void op_953(Game &g,int f,Pos p){if(as_truth(Value(true)))g.push(324,f,p,953);}
static void op_954(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.flag(8827,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}()))?323:10);}
static void op_955(Game &g,int f,Pos p){ }
static void op_956(Game &g,int f,Pos p){ }
static void op_957(Game &g,int f,Pos p){ }
static void op_958(Game &g,int f,Pos p){ }
static void op_959(Game &g,int f,Pos p){ }
static void op_960(Game &g,int f,Pos p){ }
static void op_961(Game &g,int f,Pos p){ }
static void op_962(Game &g,int f,Pos p){ }
static void op_963(Game &g,int f,Pos p){ }
static void op_964(Game &g,int f,Pos p){g.assign(Target{Ref(0),8827},Value(Number(1LL)),Assign(0),f,p);}
static void op_965(Game &g,int f,Pos p){g.set_flag(g.world->ids.arg0,g.world->instructions[965].name);g.push(321,f,p);}
static void op_966(Game &g,int f,Pos p){{PendingChoice next;next.instruction=966;next.floor=f;next.pos=p;if(as_truth(([&](){auto a=g.flag(3108,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(0);if(as_truth(([&](){auto a=g.flag(3621,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(1);if(as_truth(([&](){auto a=g.flag(3963,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(2);if(as_truth(([&](){auto a=g.flag(4647,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(3);if(as_truth(([&](){auto a=([&](){auto a=g.flag(5332,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?g.flag(22,Value(Number(0LL))):a;}())))next.options.push_back(4);if(as_truth(([&](){auto a=([&](){auto a=g.flag(5331,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?Value(!as_truth(g.flag(22,Value(Number(0LL))))):a;}())))next.options.push_back(5);if(as_truth(([&](){auto a=g.flag(5673,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(6);if(as_truth(([&](){auto a=g.flag(6186,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(7);if(as_truth(([&](){auto a=g.flag(6357,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(8);next.options.push_back(9);next.options.push_back(10);next.options.push_back(11);if(!next.options.empty())g.choice=std::move(next);}}
static void op_967(Game &g,int f,Pos p){if(as_truth(Value(true)))g.push(338,f,p,967);}
static void op_968(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.flag(8827,Value(Number(0LL)));auto b=Value(Number(2LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}()))?337:10);}
static void op_969(Game &g,int f,Pos p){ }
static void op_970(Game &g,int f,Pos p){ }
static void op_971(Game &g,int f,Pos p){ }
static void op_972(Game &g,int f,Pos p){ }
static void op_973(Game &g,int f,Pos p){ }
static void op_974(Game &g,int f,Pos p){ }
static void op_975(Game &g,int f,Pos p){ }
static void op_976(Game &g,int f,Pos p){ }
static void op_977(Game &g,int f,Pos p){ }
static void op_978(Game &g,int f,Pos p){{PendingChoice next;next.instruction=978;next.floor=f;next.pos=p;if(as_truth(([&](){auto a=g.flag(6528,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(0);if(as_truth(([&](){auto a=([&](){auto a=g.flag(6700,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?g.flag(22,Value(Number(0LL))):a;}())))next.options.push_back(1);if(as_truth(([&](){auto a=([&](){auto a=g.flag(6699,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?Value(!as_truth(g.flag(22,Value(Number(0LL))))):a;}())))next.options.push_back(2);if(as_truth(([&](){auto a=g.flag(7212,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(3);if(as_truth(([&](){auto a=([&](){auto a=g.flag(7726,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?g.flag(22,Value(Number(0LL))):a;}())))next.options.push_back(4);if(as_truth(([&](){auto a=([&](){auto a=g.flag(7725,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}());return as_truth(a)?Value(!as_truth(g.flag(22,Value(Number(0LL))))):a;}())))next.options.push_back(5);if(as_truth(([&](){auto a=g.flag(7896,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(6);if(as_truth(([&](){auto a=g.flag(8067,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(7);if(as_truth(([&](){auto a=g.flag(8238,Value(Number(0LL)));auto b=Value(Number(1LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}())))next.options.push_back(8);next.options.push_back(9);next.options.push_back(10);next.options.push_back(11);if(!next.options.empty())g.choice=std::move(next);}}
static void op_979(Game &g,int f,Pos p){if(as_truth(Value(true)))g.push(350,f,p,979);}
static void op_980(Game &g,int f,Pos p){g.prepend(as_truth(([&](){auto a=g.flag(8827,Value(Number(0LL)));auto b=Value(Number(3LL));return binary<ExprOp(16)>(std::move(a),std::move(b));}()))?349:10);}
static void op_981(Game &g,int f,Pos p){g.set_flag(g.world->ids.arg0,g.world->instructions[981].name);g.push(299,f,p);}
static void op_1008(Game &g,int f,Pos p){{auto b=g.world->entities.at(222).block;b.tile=std::int16_t(87);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(1LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void op_1009(Game &g,int f,Pos p){{auto b=g.world->entities.at(88).block;b.tile=std::int16_t(226);{Pos q{int(as_int(Value(Number(6LL)))),int(as_int(Value(Number(7LL))))};int floor=g.hero.floor;const auto &c=g.cell(floor,q);bool zero=!b.entity&&!false&&!c.source_destroyed&&g.world->floors[floor].source_event[q.index()];bool animate=c.retained.tile!=0?!c.disabled&&(b.entity||zero):bool(b.entity);g.replace(floor,q,b,false,false&&floor==g.hero.floor&&animate);}
}}
static void block_1_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_1(g,f,p);
}
static void block_5_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_4(g,f,p);
}
static void block_8_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_9(g,f,p);
}
static void block_9_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_13(g,f,p);
}
static void block_11_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_16(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_17(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_19(g,f,p);
}
static void block_11_6(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=7;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_28(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_29(g,f,p);
}
static void block_12_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_20(g,f,p);
}
static void block_13_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_21(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_22(g,f,p);
}
static void block_14_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_24(g,f,p);
}
static void block_15_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_25(g,f,p);
}
static void block_17_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=7;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=13;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=14;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=15;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=16;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=17;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=18;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=19;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=20;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=21;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=22;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=23;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=24;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=25;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=26;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=27;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=28;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=29;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=30;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=31;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=32;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=33;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=34;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=35;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_18_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_19_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_20_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_21_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_22_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_23_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_24_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_25_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_26_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_27_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_77(g,f,p);
}
static void block_29_6(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_30_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_86(g,f,p);
}
static void block_31_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_31_2(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_90(g,f,p);
}
static void block_31_4(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_92(g,f,p);
}
static void block_33_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_94(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_95(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_97(g,f,p);
}
static void block_34_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_34_2(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_101(g,f,p);
}
static void block_35_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_104(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_105(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_106(g,f,p);
}
static void block_36_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_107(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_109(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
++g.native_step_count;op_111(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=7;
++g.native_step_count;op_113(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=13;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=14;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=15;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=16;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=17;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=18;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=19;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=20;
++g.native_step_count;op_126(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=21;
++g.native_step_count;op_127(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=22;
++g.native_step_count;op_128(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=23;
++g.native_step_count;op_129(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=24;
++g.native_step_count;op_130(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=25;
++g.native_step_count;op_131(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=26;
++g.native_step_count;op_132(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=27;
++g.native_step_count;op_133(g,f,p);
}
static void block_36_28(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=29;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=30;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=31;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=32;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_37_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_142(g,f,p);
}
static void block_39_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_143(g,f,p);
}
static void block_41_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_145(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_146(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_147(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_148(g,f,p);
}
static void block_42_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_44_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_152(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_153(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_154(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_155(g,f,p);
}
static void block_46_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_158(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_159(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_160(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_161(g,f,p);
}
static void block_50_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_178(g,f,p);
}
static void block_51_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_184(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_185(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
++g.native_step_count;op_186(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
++g.native_step_count;op_187(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=7;
++g.native_step_count;op_188(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_189(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
++g.native_step_count;op_190(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
++g.native_step_count;op_191(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
++g.native_step_count;op_192(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
++g.native_step_count;op_193(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=13;
++g.native_step_count;op_194(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=14;
++g.native_step_count;op_195(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=15;
++g.native_step_count;op_196(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=16;
++g.native_step_count;op_197(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=17;
++g.native_step_count;op_198(g,f,p);
}
static void block_51_20(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=21;
++g.native_step_count;op_202(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_53_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_206(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_207(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
++g.native_step_count;op_208(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
++g.native_step_count;op_209(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=7;
++g.native_step_count;op_210(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_211(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
++g.native_step_count;op_212(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
++g.native_step_count;op_213(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
++g.native_step_count;op_214(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
++g.native_step_count;op_215(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=13;
++g.native_step_count;op_216(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=14;
++g.native_step_count;op_217(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=15;
++g.native_step_count;op_218(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=16;
++g.native_step_count;op_219(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=17;
++g.native_step_count;op_220(g,f,p);
}
static void block_53_20(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=21;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=22;
++g.native_step_count;op_225(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_226(g,f,p);
}
static void block_54_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_229(g,f,p);
}
static void block_55_1(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_231(g,f,p);
}
static void block_55_6(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=7;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_237(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
}
static void block_55_12(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=13;
}
static void block_55_16(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=17;
}
static void block_55_19(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=20;
}
static void block_55_22(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=23;
}
static void block_55_25(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=26;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=27;
++g.native_step_count;op_282(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=28;
++g.native_step_count;op_283(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_284(g,f,p);
}
static void block_59_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_243(g,f,p);
}
static void block_61_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_247(g,f,p);
}
static void block_63_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_253(g,f,p);
}
static void block_65_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_257(g,f,p);
}
static void block_67_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_262(g,f,p);
}
static void block_69_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_266(g,f,p);
}
static void block_71_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_271(g,f,p);
}
static void block_73_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_275(g,f,p);
}
static void block_75_1(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=2;
}
static void block_75_3(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_288(g,f,p);
}
static void block_79_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_294(g,f,p);
}
static void block_84_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_298(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_299(g,f,p);
}
static void block_85_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_86_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_86_2(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_306(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_307(g,f,p);
}
static void block_87_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_309(g,f,p);
}
static void block_88_1(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_318(g,f,p);
}
static void block_89_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_90_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_323(g,f,p);
}
static void block_93_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_328(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_329(g,f,p);
}
static void block_94_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_94_2(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_333(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_334(g,f,p);
}
static void block_96_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_337(g,f,p);
}
static void block_98_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_339(g,f,p);
}
static void block_101_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_342(g,f,p);
}
static void block_104_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_345(g,f,p);
}
static void block_105_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_351(g,f,p);
}
static void block_106_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_353(g,f,p);
}
static void block_107_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_357(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_358(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
++g.native_step_count;op_359(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
++g.native_step_count;op_360(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=7;
++g.native_step_count;op_361(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_362(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
++g.native_step_count;op_363(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
++g.native_step_count;op_364(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
++g.native_step_count;op_365(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
++g.native_step_count;op_366(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=13;
++g.native_step_count;op_367(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=14;
++g.native_step_count;op_368(g,f,p);
}
static void block_107_16(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_108_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_372(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_374(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_375(g,f,p);
}
static void block_108_12(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=13;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=14;
++g.native_step_count;op_385(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=15;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_387(g,f,p);
}
static void block_109_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_394(g,f,p);
}
static void block_111_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_111_3(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_112_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_398(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_113_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_407(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_408(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_409(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
++g.native_step_count;op_410(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_411(g,f,p);
}
static void block_114_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_413(g,f,p);
}
static void block_116_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_117_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_420(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_421(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
++g.native_step_count;op_422(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
++g.native_step_count;op_423(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=7;
++g.native_step_count;op_424(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_425(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
++g.native_step_count;op_426(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
++g.native_step_count;op_427(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_428(g,f,p);
}
static void block_121_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_435(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_436(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_437(g,f,p);
}
static void block_122_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_124_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_441(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_442(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_443(g,f,p);
}
static void block_125_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_127_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_447(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_448(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_449(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_450(g,f,p);
}
static void block_130_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
}
static void block_130_7(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_463(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_464(g,f,p);
}
static void block_131_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_132_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_132_3(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_470(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_471(g,f,p);
}
static void block_135_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_477(g,f,p);
}
static void block_138_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_484(g,f,p);
}
static void block_139_1(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_487(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_488(g,f,p);
}
static void block_139_4(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=5;
}
static void block_139_8(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=9;
}
static void block_139_11(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_502(g,f,p);
}
static void block_143_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_493(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_144_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_503(g,f,p);
}
static void block_145_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_508(g,f,p);
}
static void block_146_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_509(g,f,p);
}
static void block_147_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_510(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_511(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_512(g,f,p);
}
static void block_149_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_518(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_519(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_520(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_150_1(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_524(g,f,p);
}
static void block_151_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_525(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_526(g,f,p);
}
static void block_152_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
}
static void block_152_4(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_532(g,f,p);
}
static void block_154_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_540(g,f,p);
}
static void block_155_1(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_545(g,f,p);
}
static void block_158_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_548(g,f,p);
}
static void block_158_6(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=7;
}
static void block_158_13(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=14;
++g.native_step_count;op_561(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=15;
++g.native_step_count;op_562(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=16;
++g.native_step_count;op_563(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=17;
++g.native_step_count;op_564(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=18;
++g.native_step_count;op_565(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=19;
++g.native_step_count;op_566(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=20;
++g.native_step_count;op_567(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=21;
++g.native_step_count;op_568(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=22;
++g.native_step_count;op_569(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=23;
++g.native_step_count;op_570(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=24;
++g.native_step_count;op_571(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=25;
++g.native_step_count;op_572(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=26;
++g.native_step_count;op_573(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=27;
++g.native_step_count;op_574(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=28;
++g.native_step_count;op_575(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_576(g,f,p);
}
static void block_159_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_578(g,f,p);
}
static void block_160_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_580(g,f,p);
}
static void block_161_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_583(g,f,p);
}
static void block_161_8(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=9;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
++g.native_step_count;op_590(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
++g.native_step_count;op_591(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_593(g,f,p);
}
static void block_163_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_596(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_597(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_164_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_164_2(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_604(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=7;
++g.native_step_count;op_608(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
++g.native_step_count;op_611(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
++g.native_step_count;op_612(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=13;
++g.native_step_count;op_614(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_615(g,f,p);
}
static void block_167_1(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_619(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_620(g,f,p);
}
static void block_168_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_170_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_623(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
}
static void block_170_4(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=5;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
}
static void block_170_7(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_630(g,f,p);
}
static void block_171_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_633(g,f,p);
}
static void block_174_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_637(g,f,p);
}
static void block_177_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_642(g,f,p);
}
static void block_180_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_645(g,f,p);
}
static void block_181_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_647(g,f,p);
}
static void block_182_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_656(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_657(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_658(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
++g.native_step_count;op_659(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
++g.native_step_count;op_660(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=7;
++g.native_step_count;op_661(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_662(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
++g.native_step_count;op_663(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
++g.native_step_count;op_664(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
++g.native_step_count;op_665(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
++g.native_step_count;op_666(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=13;
++g.native_step_count;op_667(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=14;
++g.native_step_count;op_668(g,f,p);
}
static void block_183_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_671(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_672(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_674(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
++g.native_step_count;op_675(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
++g.native_step_count;op_676(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_677(g,f,p);
}
static void block_186_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_679(g,f,p);
}
static void block_189_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_682(g,f,p);
}
static void block_190_4(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=5;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_191_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_685(g,f,p);
}
static void block_192_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_200_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_701(g,f,p);
}
static void block_201_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_702(g,f,p);
}
static void block_202_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_704(g,f,p);
}
static void block_203_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_705(g,f,p);
}
static void block_205_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_707(g,f,p);
}
static void block_206_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_709(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_710(g,f,p);
}
static void block_207_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_712(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_713(g,f,p);
}
static void block_208_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_715(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_716(g,f,p);
}
static void block_209_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_718(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_719(g,f,p);
}
static void block_210_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_721(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_722(g,f,p);
}
static void block_211_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_724(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_725(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
++g.native_step_count;op_726(g,f,p);
}
static void block_213_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_728(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_729(g,f,p);
}
static void block_214_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_731(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_732(g,f,p);
}
static void block_215_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_734(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_735(g,f,p);
}
static void block_216_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_737(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_738(g,f,p);
}
static void block_218_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_740(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_741(g,f,p);
}
static void block_219_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_743(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_744(g,f,p);
}
static void block_220_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_746(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_747(g,f,p);
}
static void block_221_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_749(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_750(g,f,p);
}
static void block_222_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_757(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_760(g,f,p);
}
static void block_222_7(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=8;
++g.native_step_count;op_764(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=9;
++g.native_step_count;op_765(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=10;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=11;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=12;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=13;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=14;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=15;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=16;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=17;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=18;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=19;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=20;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_777(g,f,p);
}
static void block_223_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_224_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_225_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_785(g,f,p);
}
static void block_226_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_786(g,f,p);
}
static void block_227_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_787(g,f,p);
}
static void block_228_2(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_826(g,f,p);
}
static void block_229_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_789(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_790(g,f,p);
}
static void block_230_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_791(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_792(g,f,p);
}
static void block_231_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_793(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_794(g,f,p);
}
static void block_232_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_795(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_796(g,f,p);
}
static void block_233_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_797(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_798(g,f,p);
}
static void block_234_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_799(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_800(g,f,p);
}
static void block_235_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_801(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_802(g,f,p);
}
static void block_236_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_803(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_804(g,f,p);
}
static void block_237_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_805(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_806(g,f,p);
}
static void block_238_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_240_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_810(g,f,p);
}
static void block_240_2(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_822(g,f,p);
}
static void block_241_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_811(g,f,p);
}
static void block_242_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_812(g,f,p);
}
static void block_243_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_813(g,f,p);
}
static void block_244_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_814(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_815(g,f,p);
}
static void block_245_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_816(g,f,p);
}
static void block_246_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_817(g,f,p);
}
static void block_247_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_819(g,f,p);
}
static void block_248_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_820(g,f,p);
}
static void block_249_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_250_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
}
static void block_253_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_830(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_831(g,f,p);
}
static void block_254_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_256_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_834(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_835(g,f,p);
}
static void block_257_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_259_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_841(g,f,p);
}
static void block_259_3(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=4;
++g.native_step_count;op_888(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_889(g,f,p);
}
static void block_260_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_842(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_843(g,f,p);
}
static void block_261_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_844(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_845(g,f,p);
}
static void block_262_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_846(g,f,p);
}
static void block_264_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_847(g,f,p);
}
static void block_265_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_848(g,f,p);
}
static void block_266_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_850(g,f,p);
}
static void block_267_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_851(g,f,p);
}
static void block_268_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_852(g,f,p);
}
static void block_269_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_853(g,f,p);
}
static void block_270_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_854(g,f,p);
}
static void block_271_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_855(g,f,p);
}
static void block_272_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_856(g,f,p);
}
static void block_273_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_857(g,f,p);
}
static void block_275_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_858(g,f,p);
}
static void block_276_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_859(g,f,p);
}
static void block_277_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_861(g,f,p);
}
static void block_278_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_862(g,f,p);
}
static void block_279_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_863(g,f,p);
}
static void block_280_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_864(g,f,p);
}
static void block_282_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_865(g,f,p);
}
static void block_283_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_866(g,f,p);
}
static void block_284_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_868(g,f,p);
}
static void block_286_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_869(g,f,p);
}
static void block_287_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_870(g,f,p);
}
static void block_288_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_872(g,f,p);
}
static void block_289_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_873(g,f,p);
}
static void block_290_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_874(g,f,p);
}
static void block_291_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_294_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_879(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_295_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_881(g,f,p);
}
static void block_297_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_298_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_299_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_890(g,f,p);
}
static void block_301_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=3;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=4;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=5;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=6;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_302_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_303_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_304_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_305_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_902(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_904(g,f,p);
}
static void block_306_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
}
static void block_307_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_907(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
++g.native_step_count;op_909(g,f,p);
}
static void block_309_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_911(g,f,p);
}
static void block_310_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_912(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_311_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_914(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_312_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_916(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_313_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_918(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_314_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_920(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_315_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_922(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_316_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_924(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_317_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_926(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.pop_back();
}
static void block_318_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_930(g,f,p);
}
static void block_319_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_931(g,f,p);
}
static void block_320_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_932(g,f,p);
}
static void block_322_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
++g.native_step_count;op_935(g,f,p);
}
static void block_325_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_326_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_327_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_328_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_329_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_330_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_331_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_332_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_333_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_334_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_946(g,f,p);
}
static void block_335_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_948(g,f,p);
}
static void block_336_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_950(g,f,p);
}
static void block_339_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_340_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_341_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_342_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_343_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_344_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_345_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_346_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_347_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_348_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_964(g,f,p);
}
static void block_351_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_352_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_353_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_354_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_355_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_356_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_357_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_358_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_359_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.pop_back();
}
static void block_374_0(Game &g,int f,Pos p,std::size_t &operations){
g.frames.back().segments.back().cursor=1;
++g.native_step_count;op_1008(g,f,p);
if(++operations>200000)throw Error("Event operation limit");
g.frames.back().segments.back().cursor=2;
++g.native_step_count;op_1009(g,f,p);
}
}
void install_aot(World &w){if(w.source_tag!=12548849013351059438ULL)return;
if(w.instructions.size()!=1011||w.programs.size()!=375)throw Error("Compiled tower layout mismatch");
w.native_steps.resize(1011);
w.native_steps[0]=op_0;
w.native_steps[1]=op_1;
w.native_steps[3]=op_3;
w.native_steps[4]=op_4;
w.native_steps[6]=op_6;
w.native_steps[8]=op_8;
w.native_steps[9]=op_9;
w.native_steps[11]=op_11;
w.native_steps[12]=op_12;
w.native_steps[13]=op_13;
w.native_steps[15]=op_15;
w.native_steps[16]=op_16;
w.native_steps[17]=op_17;
w.native_steps[18]=op_18;
w.native_steps[19]=op_19;
w.native_steps[20]=op_20;
w.native_steps[21]=op_21;
w.native_steps[22]=op_22;
w.native_steps[23]=op_23;
w.native_steps[24]=op_24;
w.native_steps[25]=op_25;
w.native_steps[26]=op_26;
w.native_steps[27]=op_27;
w.native_steps[28]=op_28;
w.native_steps[29]=op_29;
w.native_steps[30]=op_30;
w.native_steps[31]=op_31;
w.native_steps[32]=op_32;
w.native_steps[33]=op_33;
w.native_steps[34]=op_34;
w.native_steps[35]=op_35;
w.native_steps[36]=op_36;
w.native_steps[37]=op_37;
w.native_steps[38]=op_38;
w.native_steps[39]=op_39;
w.native_steps[40]=op_40;
w.native_steps[41]=op_41;
w.native_steps[42]=op_42;
w.native_steps[43]=op_43;
w.native_steps[44]=op_44;
w.native_steps[45]=op_45;
w.native_steps[46]=op_46;
w.native_steps[47]=op_47;
w.native_steps[48]=op_48;
w.native_steps[49]=op_49;
w.native_steps[50]=op_50;
w.native_steps[51]=op_51;
w.native_steps[52]=op_52;
w.native_steps[53]=op_53;
w.native_steps[54]=op_54;
w.native_steps[55]=op_55;
w.native_steps[56]=op_56;
w.native_steps[57]=op_57;
w.native_steps[58]=op_58;
w.native_steps[59]=op_59;
w.native_steps[60]=op_60;
w.native_steps[61]=op_61;
w.native_steps[62]=op_62;
w.native_steps[63]=op_63;
w.native_steps[64]=op_64;
w.native_steps[65]=op_65;
w.native_steps[66]=op_66;
w.native_steps[67]=op_67;
w.native_steps[68]=op_68;
w.native_steps[69]=op_69;
w.native_steps[70]=op_70;
w.native_steps[71]=op_71;
w.native_steps[72]=op_72;
w.native_steps[73]=op_73;
w.native_steps[74]=op_74;
w.native_steps[75]=op_75;
w.native_steps[76]=op_76;
w.native_steps[77]=op_77;
w.native_steps[78]=op_78;
w.native_steps[85]=op_85;
w.native_steps[86]=op_86;
w.native_steps[87]=op_87;
w.native_steps[88]=op_88;
w.native_steps[90]=op_90;
w.native_steps[92]=op_92;
w.native_steps[93]=op_93;
w.native_steps[94]=op_94;
w.native_steps[95]=op_95;
w.native_steps[96]=op_96;
w.native_steps[97]=op_97;
w.native_steps[98]=op_98;
w.native_steps[99]=op_99;
w.native_steps[101]=op_101;
w.native_steps[102]=op_102;
w.native_steps[103]=op_103;
w.native_steps[104]=op_104;
w.native_steps[105]=op_105;
w.native_steps[106]=op_106;
w.native_steps[107]=op_107;
w.native_steps[108]=op_108;
w.native_steps[109]=op_109;
w.native_steps[110]=op_110;
w.native_steps[111]=op_111;
w.native_steps[112]=op_112;
w.native_steps[113]=op_113;
w.native_steps[114]=op_114;
w.native_steps[115]=op_115;
w.native_steps[116]=op_116;
w.native_steps[117]=op_117;
w.native_steps[118]=op_118;
w.native_steps[119]=op_119;
w.native_steps[120]=op_120;
w.native_steps[121]=op_121;
w.native_steps[122]=op_122;
w.native_steps[123]=op_123;
w.native_steps[124]=op_124;
w.native_steps[125]=op_125;
w.native_steps[126]=op_126;
w.native_steps[127]=op_127;
w.native_steps[128]=op_128;
w.native_steps[129]=op_129;
w.native_steps[130]=op_130;
w.native_steps[131]=op_131;
w.native_steps[132]=op_132;
w.native_steps[133]=op_133;
w.native_steps[134]=op_134;
w.native_steps[135]=op_135;
w.native_steps[136]=op_136;
w.native_steps[137]=op_137;
w.native_steps[138]=op_138;
w.native_steps[139]=op_139;
w.native_steps[140]=op_140;
w.native_steps[141]=op_141;
w.native_steps[142]=op_142;
w.native_steps[143]=op_143;
w.native_steps[144]=op_144;
w.native_steps[145]=op_145;
w.native_steps[146]=op_146;
w.native_steps[147]=op_147;
w.native_steps[148]=op_148;
w.native_steps[149]=op_149;
w.native_steps[150]=op_150;
w.native_steps[151]=op_151;
w.native_steps[152]=op_152;
w.native_steps[153]=op_153;
w.native_steps[154]=op_154;
w.native_steps[155]=op_155;
w.native_steps[156]=op_156;
w.native_steps[157]=op_157;
w.native_steps[158]=op_158;
w.native_steps[159]=op_159;
w.native_steps[160]=op_160;
w.native_steps[161]=op_161;
w.native_steps[162]=op_162;
w.native_steps[163]=op_163;
w.native_steps[164]=op_164;
w.native_steps[165]=op_165;
w.native_steps[166]=op_166;
w.native_steps[167]=op_167;
w.native_steps[168]=op_168;
w.native_steps[169]=op_169;
w.native_steps[170]=op_170;
w.native_steps[171]=op_171;
w.native_steps[172]=op_172;
w.native_steps[173]=op_173;
w.native_steps[174]=op_174;
w.native_steps[175]=op_175;
w.native_steps[176]=op_176;
w.native_steps[178]=op_178;
w.native_steps[179]=op_179;
w.native_steps[180]=op_180;
w.native_steps[181]=op_181;
w.native_steps[182]=op_182;
w.native_steps[183]=op_183;
w.native_steps[184]=op_184;
w.native_steps[185]=op_185;
w.native_steps[186]=op_186;
w.native_steps[187]=op_187;
w.native_steps[188]=op_188;
w.native_steps[189]=op_189;
w.native_steps[190]=op_190;
w.native_steps[191]=op_191;
w.native_steps[192]=op_192;
w.native_steps[193]=op_193;
w.native_steps[194]=op_194;
w.native_steps[195]=op_195;
w.native_steps[196]=op_196;
w.native_steps[197]=op_197;
w.native_steps[198]=op_198;
w.native_steps[202]=op_202;
w.native_steps[203]=op_203;
w.native_steps[204]=op_204;
w.native_steps[205]=op_205;
w.native_steps[206]=op_206;
w.native_steps[207]=op_207;
w.native_steps[208]=op_208;
w.native_steps[209]=op_209;
w.native_steps[210]=op_210;
w.native_steps[211]=op_211;
w.native_steps[212]=op_212;
w.native_steps[213]=op_213;
w.native_steps[214]=op_214;
w.native_steps[215]=op_215;
w.native_steps[216]=op_216;
w.native_steps[217]=op_217;
w.native_steps[218]=op_218;
w.native_steps[219]=op_219;
w.native_steps[220]=op_220;
w.native_steps[224]=op_224;
w.native_steps[225]=op_225;
w.native_steps[226]=op_226;
w.native_steps[227]=op_227;
w.native_steps[228]=op_228;
w.native_steps[229]=op_229;
w.native_steps[231]=op_231;
w.native_steps[236]=op_236;
w.native_steps[237]=op_237;
w.native_steps[238]=op_238;
w.native_steps[241]=op_241;
w.native_steps[243]=op_243;
w.native_steps[245]=op_245;
w.native_steps[247]=op_247;
w.native_steps[249]=op_249;
w.native_steps[250]=op_250;
w.native_steps[253]=op_253;
w.native_steps[255]=op_255;
w.native_steps[257]=op_257;
w.native_steps[259]=op_259;
w.native_steps[260]=op_260;
w.native_steps[262]=op_262;
w.native_steps[264]=op_264;
w.native_steps[266]=op_266;
w.native_steps[268]=op_268;
w.native_steps[269]=op_269;
w.native_steps[271]=op_271;
w.native_steps[273]=op_273;
w.native_steps[275]=op_275;
w.native_steps[277]=op_277;
w.native_steps[278]=op_278;
w.native_steps[281]=op_281;
w.native_steps[282]=op_282;
w.native_steps[283]=op_283;
w.native_steps[284]=op_284;
w.native_steps[286]=op_286;
w.native_steps[288]=op_288;
w.native_steps[289]=op_289;
w.native_steps[290]=op_290;
w.native_steps[291]=op_291;
w.native_steps[294]=op_294;
w.native_steps[295]=op_295;
w.native_steps[296]=op_296;
w.native_steps[297]=op_297;
w.native_steps[298]=op_298;
w.native_steps[299]=op_299;
w.native_steps[300]=op_300;
w.native_steps[301]=op_301;
w.native_steps[302]=op_302;
w.native_steps[303]=op_303;
w.native_steps[304]=op_304;
w.native_steps[306]=op_306;
w.native_steps[307]=op_307;
w.native_steps[308]=op_308;
w.native_steps[309]=op_309;
w.native_steps[310]=op_310;
w.native_steps[311]=op_311;
w.native_steps[312]=op_312;
w.native_steps[313]=op_313;
w.native_steps[314]=op_314;
w.native_steps[315]=op_315;
w.native_steps[316]=op_316;
w.native_steps[318]=op_318;
w.native_steps[319]=op_319;
w.native_steps[320]=op_320;
w.native_steps[323]=op_323;
w.native_steps[325]=op_325;
w.native_steps[326]=op_326;
w.native_steps[327]=op_327;
w.native_steps[328]=op_328;
w.native_steps[329]=op_329;
w.native_steps[330]=op_330;
w.native_steps[331]=op_331;
w.native_steps[333]=op_333;
w.native_steps[334]=op_334;
w.native_steps[335]=op_335;
w.native_steps[336]=op_336;
w.native_steps[337]=op_337;
w.native_steps[338]=op_338;
w.native_steps[339]=op_339;
w.native_steps[340]=op_340;
w.native_steps[342]=op_342;
w.native_steps[343]=op_343;
w.native_steps[345]=op_345;
w.native_steps[346]=op_346;
w.native_steps[347]=op_347;
w.native_steps[348]=op_348;
w.native_steps[349]=op_349;
w.native_steps[350]=op_350;
w.native_steps[351]=op_351;
w.native_steps[352]=op_352;
w.native_steps[353]=op_353;
w.native_steps[354]=op_354;
w.native_steps[355]=op_355;
w.native_steps[356]=op_356;
w.native_steps[357]=op_357;
w.native_steps[358]=op_358;
w.native_steps[359]=op_359;
w.native_steps[360]=op_360;
w.native_steps[361]=op_361;
w.native_steps[362]=op_362;
w.native_steps[363]=op_363;
w.native_steps[364]=op_364;
w.native_steps[365]=op_365;
w.native_steps[366]=op_366;
w.native_steps[367]=op_367;
w.native_steps[368]=op_368;
w.native_steps[371]=op_371;
w.native_steps[372]=op_372;
w.native_steps[373]=op_373;
w.native_steps[374]=op_374;
w.native_steps[375]=op_375;
w.native_steps[384]=op_384;
w.native_steps[385]=op_385;
w.native_steps[386]=op_386;
w.native_steps[387]=op_387;
w.native_steps[388]=op_388;
w.native_steps[389]=op_389;
w.native_steps[390]=op_390;
w.native_steps[391]=op_391;
w.native_steps[392]=op_392;
w.native_steps[393]=op_393;
w.native_steps[394]=op_394;
w.native_steps[396]=op_396;
w.native_steps[397]=op_397;
w.native_steps[398]=op_398;
w.native_steps[399]=op_399;
w.native_steps[400]=op_400;
w.native_steps[401]=op_401;
w.native_steps[402]=op_402;
w.native_steps[403]=op_403;
w.native_steps[404]=op_404;
w.native_steps[405]=op_405;
w.native_steps[406]=op_406;
w.native_steps[407]=op_407;
w.native_steps[408]=op_408;
w.native_steps[409]=op_409;
w.native_steps[410]=op_410;
w.native_steps[411]=op_411;
w.native_steps[412]=op_412;
w.native_steps[413]=op_413;
w.native_steps[414]=op_414;
w.native_steps[415]=op_415;
w.native_steps[416]=op_416;
w.native_steps[417]=op_417;
w.native_steps[418]=op_418;
w.native_steps[419]=op_419;
w.native_steps[420]=op_420;
w.native_steps[421]=op_421;
w.native_steps[422]=op_422;
w.native_steps[423]=op_423;
w.native_steps[424]=op_424;
w.native_steps[425]=op_425;
w.native_steps[426]=op_426;
w.native_steps[427]=op_427;
w.native_steps[428]=op_428;
w.native_steps[429]=op_429;
w.native_steps[430]=op_430;
w.native_steps[431]=op_431;
w.native_steps[432]=op_432;
w.native_steps[433]=op_433;
w.native_steps[434]=op_434;
w.native_steps[435]=op_435;
w.native_steps[436]=op_436;
w.native_steps[437]=op_437;
w.native_steps[438]=op_438;
w.native_steps[439]=op_439;
w.native_steps[440]=op_440;
w.native_steps[441]=op_441;
w.native_steps[442]=op_442;
w.native_steps[443]=op_443;
w.native_steps[444]=op_444;
w.native_steps[445]=op_445;
w.native_steps[446]=op_446;
w.native_steps[447]=op_447;
w.native_steps[448]=op_448;
w.native_steps[449]=op_449;
w.native_steps[450]=op_450;
w.native_steps[451]=op_451;
w.native_steps[452]=op_452;
w.native_steps[453]=op_453;
w.native_steps[454]=op_454;
w.native_steps[455]=op_455;
w.native_steps[456]=op_456;
w.native_steps[457]=op_457;
w.native_steps[458]=op_458;
w.native_steps[459]=op_459;
w.native_steps[460]=op_460;
w.native_steps[463]=op_463;
w.native_steps[464]=op_464;
w.native_steps[465]=op_465;
w.native_steps[466]=op_466;
w.native_steps[467]=op_467;
w.native_steps[470]=op_470;
w.native_steps[471]=op_471;
w.native_steps[472]=op_472;
w.native_steps[473]=op_473;
w.native_steps[474]=op_474;
w.native_steps[475]=op_475;
w.native_steps[477]=op_477;
w.native_steps[478]=op_478;
w.native_steps[479]=op_479;
w.native_steps[480]=op_480;
w.native_steps[481]=op_481;
w.native_steps[482]=op_482;
w.native_steps[484]=op_484;
w.native_steps[485]=op_485;
w.native_steps[487]=op_487;
w.native_steps[488]=op_488;
w.native_steps[490]=op_490;
w.native_steps[493]=op_493;
w.native_steps[494]=op_494;
w.native_steps[495]=op_495;
w.native_steps[496]=op_496;
w.native_steps[497]=op_497;
w.native_steps[499]=op_499;
w.native_steps[502]=op_502;
w.native_steps[503]=op_503;
w.native_steps[504]=op_504;
w.native_steps[505]=op_505;
w.native_steps[506]=op_506;
w.native_steps[507]=op_507;
w.native_steps[508]=op_508;
w.native_steps[509]=op_509;
w.native_steps[510]=op_510;
w.native_steps[511]=op_511;
w.native_steps[512]=op_512;
w.native_steps[513]=op_513;
w.native_steps[516]=op_516;
w.native_steps[517]=op_517;
w.native_steps[518]=op_518;
w.native_steps[519]=op_519;
w.native_steps[520]=op_520;
w.native_steps[521]=op_521;
w.native_steps[522]=op_522;
w.native_steps[524]=op_524;
w.native_steps[525]=op_525;
w.native_steps[526]=op_526;
w.native_steps[527]=op_527;
w.native_steps[528]=op_528;
w.native_steps[529]=op_529;
w.native_steps[530]=op_530;
w.native_steps[532]=op_532;
w.native_steps[533]=op_533;
w.native_steps[534]=op_534;
w.native_steps[535]=op_535;
w.native_steps[536]=op_536;
w.native_steps[537]=op_537;
w.native_steps[538]=op_538;
w.native_steps[539]=op_539;
w.native_steps[540]=op_540;
w.native_steps[541]=op_541;
w.native_steps[542]=op_542;
w.native_steps[543]=op_543;
w.native_steps[545]=op_545;
w.native_steps[546]=op_546;
w.native_steps[547]=op_547;
w.native_steps[548]=op_548;
w.native_steps[554]=op_554;
w.native_steps[555]=op_555;
w.native_steps[561]=op_561;
w.native_steps[562]=op_562;
w.native_steps[563]=op_563;
w.native_steps[564]=op_564;
w.native_steps[565]=op_565;
w.native_steps[566]=op_566;
w.native_steps[567]=op_567;
w.native_steps[568]=op_568;
w.native_steps[569]=op_569;
w.native_steps[570]=op_570;
w.native_steps[571]=op_571;
w.native_steps[572]=op_572;
w.native_steps[573]=op_573;
w.native_steps[574]=op_574;
w.native_steps[575]=op_575;
w.native_steps[576]=op_576;
w.native_steps[577]=op_577;
w.native_steps[578]=op_578;
w.native_steps[579]=op_579;
w.native_steps[580]=op_580;
w.native_steps[581]=op_581;
w.native_steps[582]=op_582;
w.native_steps[583]=op_583;
w.native_steps[589]=op_589;
w.native_steps[590]=op_590;
w.native_steps[591]=op_591;
w.native_steps[592]=op_592;
w.native_steps[593]=op_593;
w.native_steps[594]=op_594;
w.native_steps[595]=op_595;
w.native_steps[596]=op_596;
w.native_steps[597]=op_597;
w.native_steps[598]=op_598;
w.native_steps[599]=op_599;
w.native_steps[600]=op_600;
w.native_steps[601]=op_601;
w.native_steps[602]=op_602;
w.native_steps[604]=op_604;
w.native_steps[605]=op_605;
w.native_steps[606]=op_606;
w.native_steps[607]=op_607;
w.native_steps[608]=op_608;
w.native_steps[609]=op_609;
w.native_steps[610]=op_610;
w.native_steps[611]=op_611;
w.native_steps[612]=op_612;
w.native_steps[613]=op_613;
w.native_steps[614]=op_614;
w.native_steps[615]=op_615;
w.native_steps[616]=op_616;
w.native_steps[617]=op_617;
w.native_steps[619]=op_619;
w.native_steps[620]=op_620;
w.native_steps[621]=op_621;
w.native_steps[622]=op_622;
w.native_steps[623]=op_623;
w.native_steps[624]=op_624;
w.native_steps[627]=op_627;
w.native_steps[628]=op_628;
w.native_steps[630]=op_630;
w.native_steps[631]=op_631;
w.native_steps[632]=op_632;
w.native_steps[633]=op_633;
w.native_steps[634]=op_634;
w.native_steps[635]=op_635;
w.native_steps[637]=op_637;
w.native_steps[638]=op_638;
w.native_steps[639]=op_639;
w.native_steps[640]=op_640;
w.native_steps[642]=op_642;
w.native_steps[643]=op_643;
w.native_steps[645]=op_645;
w.native_steps[646]=op_646;
w.native_steps[647]=op_647;
w.native_steps[648]=op_648;
w.native_steps[649]=op_649;
w.native_steps[650]=op_650;
w.native_steps[651]=op_651;
w.native_steps[652]=op_652;
w.native_steps[653]=op_653;
w.native_steps[654]=op_654;
w.native_steps[655]=op_655;
w.native_steps[656]=op_656;
w.native_steps[657]=op_657;
w.native_steps[658]=op_658;
w.native_steps[659]=op_659;
w.native_steps[660]=op_660;
w.native_steps[661]=op_661;
w.native_steps[662]=op_662;
w.native_steps[663]=op_663;
w.native_steps[664]=op_664;
w.native_steps[665]=op_665;
w.native_steps[666]=op_666;
w.native_steps[667]=op_667;
w.native_steps[668]=op_668;
w.native_steps[671]=op_671;
w.native_steps[672]=op_672;
w.native_steps[673]=op_673;
w.native_steps[674]=op_674;
w.native_steps[675]=op_675;
w.native_steps[676]=op_676;
w.native_steps[677]=op_677;
w.native_steps[679]=op_679;
w.native_steps[680]=op_680;
w.native_steps[682]=op_682;
w.native_steps[683]=op_683;
w.native_steps[684]=op_684;
w.native_steps[685]=op_685;
w.native_steps[686]=op_686;
w.native_steps[690]=op_690;
w.native_steps[691]=op_691;
w.native_steps[692]=op_692;
w.native_steps[695]=op_695;
w.native_steps[698]=op_698;
w.native_steps[699]=op_699;
w.native_steps[700]=op_700;
w.native_steps[701]=op_701;
w.native_steps[702]=op_702;
w.native_steps[703]=op_703;
w.native_steps[704]=op_704;
w.native_steps[705]=op_705;
w.native_steps[706]=op_706;
w.native_steps[707]=op_707;
w.native_steps[708]=op_708;
w.native_steps[709]=op_709;
w.native_steps[710]=op_710;
w.native_steps[711]=op_711;
w.native_steps[712]=op_712;
w.native_steps[713]=op_713;
w.native_steps[714]=op_714;
w.native_steps[715]=op_715;
w.native_steps[716]=op_716;
w.native_steps[717]=op_717;
w.native_steps[718]=op_718;
w.native_steps[719]=op_719;
w.native_steps[720]=op_720;
w.native_steps[721]=op_721;
w.native_steps[722]=op_722;
w.native_steps[723]=op_723;
w.native_steps[724]=op_724;
w.native_steps[725]=op_725;
w.native_steps[726]=op_726;
w.native_steps[727]=op_727;
w.native_steps[728]=op_728;
w.native_steps[729]=op_729;
w.native_steps[730]=op_730;
w.native_steps[731]=op_731;
w.native_steps[732]=op_732;
w.native_steps[733]=op_733;
w.native_steps[734]=op_734;
w.native_steps[735]=op_735;
w.native_steps[736]=op_736;
w.native_steps[737]=op_737;
w.native_steps[738]=op_738;
w.native_steps[739]=op_739;
w.native_steps[740]=op_740;
w.native_steps[741]=op_741;
w.native_steps[742]=op_742;
w.native_steps[743]=op_743;
w.native_steps[744]=op_744;
w.native_steps[745]=op_745;
w.native_steps[746]=op_746;
w.native_steps[747]=op_747;
w.native_steps[748]=op_748;
w.native_steps[749]=op_749;
w.native_steps[750]=op_750;
w.native_steps[751]=op_751;
w.native_steps[752]=op_752;
w.native_steps[753]=op_753;
w.native_steps[754]=op_754;
w.native_steps[755]=op_755;
w.native_steps[757]=op_757;
w.native_steps[758]=op_758;
w.native_steps[759]=op_759;
w.native_steps[760]=op_760;
w.native_steps[764]=op_764;
w.native_steps[765]=op_765;
w.native_steps[766]=op_766;
w.native_steps[767]=op_767;
w.native_steps[768]=op_768;
w.native_steps[769]=op_769;
w.native_steps[770]=op_770;
w.native_steps[771]=op_771;
w.native_steps[772]=op_772;
w.native_steps[773]=op_773;
w.native_steps[774]=op_774;
w.native_steps[775]=op_775;
w.native_steps[776]=op_776;
w.native_steps[777]=op_777;
w.native_steps[778]=op_778;
w.native_steps[779]=op_779;
w.native_steps[780]=op_780;
w.native_steps[781]=op_781;
w.native_steps[782]=op_782;
w.native_steps[783]=op_783;
w.native_steps[784]=op_784;
w.native_steps[785]=op_785;
w.native_steps[786]=op_786;
w.native_steps[787]=op_787;
w.native_steps[788]=op_788;
w.native_steps[789]=op_789;
w.native_steps[790]=op_790;
w.native_steps[791]=op_791;
w.native_steps[792]=op_792;
w.native_steps[793]=op_793;
w.native_steps[794]=op_794;
w.native_steps[795]=op_795;
w.native_steps[796]=op_796;
w.native_steps[797]=op_797;
w.native_steps[798]=op_798;
w.native_steps[799]=op_799;
w.native_steps[800]=op_800;
w.native_steps[801]=op_801;
w.native_steps[802]=op_802;
w.native_steps[803]=op_803;
w.native_steps[804]=op_804;
w.native_steps[805]=op_805;
w.native_steps[806]=op_806;
w.native_steps[807]=op_807;
w.native_steps[808]=op_808;
w.native_steps[809]=op_809;
w.native_steps[810]=op_810;
w.native_steps[811]=op_811;
w.native_steps[812]=op_812;
w.native_steps[813]=op_813;
w.native_steps[814]=op_814;
w.native_steps[815]=op_815;
w.native_steps[816]=op_816;
w.native_steps[817]=op_817;
w.native_steps[818]=op_818;
w.native_steps[819]=op_819;
w.native_steps[820]=op_820;
w.native_steps[821]=op_821;
w.native_steps[822]=op_822;
w.native_steps[823]=op_823;
w.native_steps[824]=op_824;
w.native_steps[825]=op_825;
w.native_steps[826]=op_826;
w.native_steps[827]=op_827;
w.native_steps[828]=op_828;
w.native_steps[829]=op_829;
w.native_steps[830]=op_830;
w.native_steps[831]=op_831;
w.native_steps[832]=op_832;
w.native_steps[833]=op_833;
w.native_steps[834]=op_834;
w.native_steps[835]=op_835;
w.native_steps[836]=op_836;
w.native_steps[837]=op_837;
w.native_steps[838]=op_838;
w.native_steps[839]=op_839;
w.native_steps[840]=op_840;
w.native_steps[841]=op_841;
w.native_steps[842]=op_842;
w.native_steps[843]=op_843;
w.native_steps[844]=op_844;
w.native_steps[845]=op_845;
w.native_steps[846]=op_846;
w.native_steps[847]=op_847;
w.native_steps[848]=op_848;
w.native_steps[849]=op_849;
w.native_steps[850]=op_850;
w.native_steps[851]=op_851;
w.native_steps[852]=op_852;
w.native_steps[853]=op_853;
w.native_steps[854]=op_854;
w.native_steps[855]=op_855;
w.native_steps[856]=op_856;
w.native_steps[857]=op_857;
w.native_steps[858]=op_858;
w.native_steps[859]=op_859;
w.native_steps[860]=op_860;
w.native_steps[861]=op_861;
w.native_steps[862]=op_862;
w.native_steps[863]=op_863;
w.native_steps[864]=op_864;
w.native_steps[865]=op_865;
w.native_steps[866]=op_866;
w.native_steps[867]=op_867;
w.native_steps[868]=op_868;
w.native_steps[869]=op_869;
w.native_steps[870]=op_870;
w.native_steps[871]=op_871;
w.native_steps[872]=op_872;
w.native_steps[873]=op_873;
w.native_steps[874]=op_874;
w.native_steps[875]=op_875;
w.native_steps[876]=op_876;
w.native_steps[877]=op_877;
w.native_steps[878]=op_878;
w.native_steps[879]=op_879;
w.native_steps[880]=op_880;
w.native_steps[881]=op_881;
w.native_steps[882]=op_882;
w.native_steps[883]=op_883;
w.native_steps[884]=op_884;
w.native_steps[885]=op_885;
w.native_steps[886]=op_886;
w.native_steps[887]=op_887;
w.native_steps[888]=op_888;
w.native_steps[889]=op_889;
w.native_steps[890]=op_890;
w.native_steps[891]=op_891;
w.native_steps[892]=op_892;
w.native_steps[893]=op_893;
w.native_steps[894]=op_894;
w.native_steps[895]=op_895;
w.native_steps[896]=op_896;
w.native_steps[897]=op_897;
w.native_steps[898]=op_898;
w.native_steps[899]=op_899;
w.native_steps[900]=op_900;
w.native_steps[901]=op_901;
w.native_steps[902]=op_902;
w.native_steps[903]=op_903;
w.native_steps[904]=op_904;
w.native_steps[905]=op_905;
w.native_steps[906]=op_906;
w.native_steps[907]=op_907;
w.native_steps[908]=op_908;
w.native_steps[909]=op_909;
w.native_steps[910]=op_910;
w.native_steps[911]=op_911;
w.native_steps[912]=op_912;
w.native_steps[913]=op_913;
w.native_steps[914]=op_914;
w.native_steps[915]=op_915;
w.native_steps[916]=op_916;
w.native_steps[917]=op_917;
w.native_steps[918]=op_918;
w.native_steps[919]=op_919;
w.native_steps[920]=op_920;
w.native_steps[921]=op_921;
w.native_steps[922]=op_922;
w.native_steps[923]=op_923;
w.native_steps[924]=op_924;
w.native_steps[925]=op_925;
w.native_steps[926]=op_926;
w.native_steps[927]=op_927;
w.native_steps[928]=op_928;
w.native_steps[929]=op_929;
w.native_steps[930]=op_930;
w.native_steps[931]=op_931;
w.native_steps[932]=op_932;
w.native_steps[933]=op_933;
w.native_steps[934]=op_934;
w.native_steps[935]=op_935;
w.native_steps[936]=op_936;
w.native_steps[937]=op_937;
w.native_steps[938]=op_938;
w.native_steps[939]=op_939;
w.native_steps[940]=op_940;
w.native_steps[941]=op_941;
w.native_steps[942]=op_942;
w.native_steps[943]=op_943;
w.native_steps[944]=op_944;
w.native_steps[945]=op_945;
w.native_steps[946]=op_946;
w.native_steps[947]=op_947;
w.native_steps[948]=op_948;
w.native_steps[949]=op_949;
w.native_steps[950]=op_950;
w.native_steps[951]=op_951;
w.native_steps[952]=op_952;
w.native_steps[953]=op_953;
w.native_steps[954]=op_954;
w.native_steps[955]=op_955;
w.native_steps[956]=op_956;
w.native_steps[957]=op_957;
w.native_steps[958]=op_958;
w.native_steps[959]=op_959;
w.native_steps[960]=op_960;
w.native_steps[961]=op_961;
w.native_steps[962]=op_962;
w.native_steps[963]=op_963;
w.native_steps[964]=op_964;
w.native_steps[965]=op_965;
w.native_steps[966]=op_966;
w.native_steps[967]=op_967;
w.native_steps[968]=op_968;
w.native_steps[969]=op_969;
w.native_steps[970]=op_970;
w.native_steps[971]=op_971;
w.native_steps[972]=op_972;
w.native_steps[973]=op_973;
w.native_steps[974]=op_974;
w.native_steps[975]=op_975;
w.native_steps[976]=op_976;
w.native_steps[977]=op_977;
w.native_steps[978]=op_978;
w.native_steps[979]=op_979;
w.native_steps[980]=op_980;
w.native_steps[981]=op_981;
w.native_steps[1008]=op_1008;
w.native_steps[1009]=op_1009;
w.programs[0].native_blocks.resize(0);
w.programs[1].native_blocks.resize(1);
w.programs[2].native_blocks.resize(1);
w.programs[3].native_blocks.resize(1);
w.programs[4].native_blocks.resize(1);
w.programs[5].native_blocks.resize(3);
w.programs[6].native_blocks.resize(1);
w.programs[7].native_blocks.resize(1);
w.programs[8].native_blocks.resize(3);
w.programs[9].native_blocks.resize(2);
w.programs[10].native_blocks.resize(0);
w.programs[11].native_blocks.resize(9);
w.programs[12].native_blocks.resize(1);
w.programs[13].native_blocks.resize(2);
w.programs[14].native_blocks.resize(1);
w.programs[15].native_blocks.resize(1);
w.programs[16].native_blocks.resize(1);
w.programs[17].native_blocks.resize(36);
w.programs[18].native_blocks.resize(1);
w.programs[19].native_blocks.resize(1);
w.programs[20].native_blocks.resize(1);
w.programs[21].native_blocks.resize(1);
w.programs[22].native_blocks.resize(1);
w.programs[23].native_blocks.resize(1);
w.programs[24].native_blocks.resize(1);
w.programs[25].native_blocks.resize(1);
w.programs[26].native_blocks.resize(1);
w.programs[27].native_blocks.resize(1);
w.programs[28].native_blocks.resize(1);
w.programs[29].native_blocks.resize(7);
w.programs[30].native_blocks.resize(1);
w.programs[31].native_blocks.resize(5);
w.programs[32].native_blocks.resize(1);
w.programs[33].native_blocks.resize(5);
w.programs[34].native_blocks.resize(3);
w.programs[35].native_blocks.resize(4);
w.programs[36].native_blocks.resize(33);
w.programs[37].native_blocks.resize(2);
w.programs[38].native_blocks.resize(1);
w.programs[39].native_blocks.resize(2);
w.programs[40].native_blocks.resize(1);
w.programs[41].native_blocks.resize(5);
w.programs[42].native_blocks.resize(1);
w.programs[43].native_blocks.resize(1);
w.programs[44].native_blocks.resize(5);
w.programs[45].native_blocks.resize(1);
w.programs[46].native_blocks.resize(5);
w.programs[47].native_blocks.resize(1);
w.programs[48].native_blocks.resize(1);
w.programs[49].native_blocks.resize(1);
w.programs[50].native_blocks.resize(1);
w.programs[51].native_blocks.resize(22);
w.programs[52].native_blocks.resize(1);
w.programs[53].native_blocks.resize(23);
w.programs[54].native_blocks.resize(2);
w.programs[55].native_blocks.resize(29);
w.programs[56].native_blocks.resize(1);
w.programs[57].native_blocks.resize(1);
w.programs[58].native_blocks.resize(1);
w.programs[59].native_blocks.resize(2);
w.programs[60].native_blocks.resize(1);
w.programs[61].native_blocks.resize(2);
w.programs[62].native_blocks.resize(1);
w.programs[63].native_blocks.resize(2);
w.programs[64].native_blocks.resize(1);
w.programs[65].native_blocks.resize(2);
w.programs[66].native_blocks.resize(1);
w.programs[67].native_blocks.resize(2);
w.programs[68].native_blocks.resize(1);
w.programs[69].native_blocks.resize(2);
w.programs[70].native_blocks.resize(1);
w.programs[71].native_blocks.resize(2);
w.programs[72].native_blocks.resize(1);
w.programs[73].native_blocks.resize(2);
w.programs[74].native_blocks.resize(1);
w.programs[75].native_blocks.resize(4);
w.programs[76].native_blocks.resize(1);
w.programs[77].native_blocks.resize(1);
w.programs[78].native_blocks.resize(1);
w.programs[79].native_blocks.resize(1);
w.programs[80].native_blocks.resize(1);
w.programs[81].native_blocks.resize(1);
w.programs[82].native_blocks.resize(1);
w.programs[83].native_blocks.resize(1);
w.programs[84].native_blocks.resize(2);
w.programs[85].native_blocks.resize(1);
w.programs[86].native_blocks.resize(4);
w.programs[87].native_blocks.resize(2);
w.programs[88].native_blocks.resize(2);
w.programs[89].native_blocks.resize(3);
w.programs[90].native_blocks.resize(2);
w.programs[91].native_blocks.resize(1);
w.programs[92].native_blocks.resize(1);
w.programs[93].native_blocks.resize(2);
w.programs[94].native_blocks.resize(4);
w.programs[95].native_blocks.resize(1);
w.programs[96].native_blocks.resize(1);
w.programs[97].native_blocks.resize(1);
w.programs[98].native_blocks.resize(1);
w.programs[99].native_blocks.resize(1);
w.programs[100].native_blocks.resize(1);
w.programs[101].native_blocks.resize(1);
w.programs[102].native_blocks.resize(1);
w.programs[103].native_blocks.resize(1);
w.programs[104].native_blocks.resize(1);
w.programs[105].native_blocks.resize(1);
w.programs[106].native_blocks.resize(1);
w.programs[107].native_blocks.resize(17);
w.programs[108].native_blocks.resize(16);
w.programs[109].native_blocks.resize(2);
w.programs[110].native_blocks.resize(1);
w.programs[111].native_blocks.resize(4);
w.programs[112].native_blocks.resize(2);
w.programs[113].native_blocks.resize(6);
w.programs[114].native_blocks.resize(2);
w.programs[115].native_blocks.resize(1);
w.programs[116].native_blocks.resize(2);
w.programs[117].native_blocks.resize(11);
w.programs[118].native_blocks.resize(1);
w.programs[119].native_blocks.resize(1);
w.programs[120].native_blocks.resize(1);
w.programs[121].native_blocks.resize(4);
w.programs[122].native_blocks.resize(1);
w.programs[123].native_blocks.resize(1);
w.programs[124].native_blocks.resize(4);
w.programs[125].native_blocks.resize(1);
w.programs[126].native_blocks.resize(1);
w.programs[127].native_blocks.resize(5);
w.programs[128].native_blocks.resize(1);
w.programs[129].native_blocks.resize(1);
w.programs[130].native_blocks.resize(9);
w.programs[131].native_blocks.resize(1);
w.programs[132].native_blocks.resize(5);
w.programs[133].native_blocks.resize(1);
w.programs[134].native_blocks.resize(1);
w.programs[135].native_blocks.resize(1);
w.programs[136].native_blocks.resize(1);
w.programs[137].native_blocks.resize(1);
w.programs[138].native_blocks.resize(1);
w.programs[139].native_blocks.resize(12);
w.programs[140].native_blocks.resize(1);
w.programs[141].native_blocks.resize(1);
w.programs[142].native_blocks.resize(1);
w.programs[143].native_blocks.resize(2);
w.programs[144].native_blocks.resize(2);
w.programs[145].native_blocks.resize(2);
w.programs[146].native_blocks.resize(1);
w.programs[147].native_blocks.resize(3);
w.programs[148].native_blocks.resize(2);
w.programs[149].native_blocks.resize(4);
w.programs[150].native_blocks.resize(3);
w.programs[151].native_blocks.resize(2);
w.programs[152].native_blocks.resize(5);
w.programs[153].native_blocks.resize(1);
w.programs[154].native_blocks.resize(1);
w.programs[155].native_blocks.resize(2);
w.programs[156].native_blocks.resize(1);
w.programs[157].native_blocks.resize(1);
w.programs[158].native_blocks.resize(29);
w.programs[159].native_blocks.resize(1);
w.programs[160].native_blocks.resize(1);
w.programs[161].native_blocks.resize(13);
w.programs[162].native_blocks.resize(1);
w.programs[163].native_blocks.resize(5);
w.programs[164].native_blocks.resize(14);
w.programs[165].native_blocks.resize(1);
w.programs[166].native_blocks.resize(1);
w.programs[167].native_blocks.resize(3);
w.programs[168].native_blocks.resize(1);
w.programs[169].native_blocks.resize(1);
w.programs[170].native_blocks.resize(8);
w.programs[171].native_blocks.resize(1);
w.programs[172].native_blocks.resize(1);
w.programs[173].native_blocks.resize(1);
w.programs[174].native_blocks.resize(1);
w.programs[175].native_blocks.resize(1);
w.programs[176].native_blocks.resize(1);
w.programs[177].native_blocks.resize(1);
w.programs[178].native_blocks.resize(1);
w.programs[179].native_blocks.resize(1);
w.programs[180].native_blocks.resize(1);
w.programs[181].native_blocks.resize(2);
w.programs[182].native_blocks.resize(16);
w.programs[183].native_blocks.resize(7);
w.programs[184].native_blocks.resize(1);
w.programs[185].native_blocks.resize(1);
w.programs[186].native_blocks.resize(1);
w.programs[187].native_blocks.resize(1);
w.programs[188].native_blocks.resize(1);
w.programs[189].native_blocks.resize(1);
w.programs[190].native_blocks.resize(6);
w.programs[191].native_blocks.resize(1);
w.programs[192].native_blocks.resize(6);
w.programs[193].native_blocks.resize(1);
w.programs[194].native_blocks.resize(1);
w.programs[195].native_blocks.resize(1);
w.programs[196].native_blocks.resize(1);
w.programs[197].native_blocks.resize(1);
w.programs[198].native_blocks.resize(1);
w.programs[199].native_blocks.resize(1);
w.programs[200].native_blocks.resize(1);
w.programs[201].native_blocks.resize(1);
w.programs[202].native_blocks.resize(1);
w.programs[203].native_blocks.resize(1);
w.programs[204].native_blocks.resize(1);
w.programs[205].native_blocks.resize(2);
w.programs[206].native_blocks.resize(3);
w.programs[207].native_blocks.resize(3);
w.programs[208].native_blocks.resize(3);
w.programs[209].native_blocks.resize(3);
w.programs[210].native_blocks.resize(3);
w.programs[211].native_blocks.resize(4);
w.programs[212].native_blocks.resize(1);
w.programs[213].native_blocks.resize(3);
w.programs[214].native_blocks.resize(3);
w.programs[215].native_blocks.resize(3);
w.programs[216].native_blocks.resize(3);
w.programs[217].native_blocks.resize(1);
w.programs[218].native_blocks.resize(3);
w.programs[219].native_blocks.resize(3);
w.programs[220].native_blocks.resize(3);
w.programs[221].native_blocks.resize(3);
w.programs[222].native_blocks.resize(21);
w.programs[223].native_blocks.resize(6);
w.programs[224].native_blocks.resize(2);
w.programs[225].native_blocks.resize(1);
w.programs[226].native_blocks.resize(1);
w.programs[227].native_blocks.resize(1);
w.programs[228].native_blocks.resize(3);
w.programs[229].native_blocks.resize(2);
w.programs[230].native_blocks.resize(2);
w.programs[231].native_blocks.resize(2);
w.programs[232].native_blocks.resize(2);
w.programs[233].native_blocks.resize(2);
w.programs[234].native_blocks.resize(2);
w.programs[235].native_blocks.resize(2);
w.programs[236].native_blocks.resize(2);
w.programs[237].native_blocks.resize(2);
w.programs[238].native_blocks.resize(2);
w.programs[239].native_blocks.resize(1);
w.programs[240].native_blocks.resize(3);
w.programs[241].native_blocks.resize(1);
w.programs[242].native_blocks.resize(1);
w.programs[243].native_blocks.resize(1);
w.programs[244].native_blocks.resize(2);
w.programs[245].native_blocks.resize(1);
w.programs[246].native_blocks.resize(1);
w.programs[247].native_blocks.resize(2);
w.programs[248].native_blocks.resize(1);
w.programs[249].native_blocks.resize(1);
w.programs[250].native_blocks.resize(4);
w.programs[251].native_blocks.resize(1);
w.programs[252].native_blocks.resize(1);
w.programs[253].native_blocks.resize(2);
w.programs[254].native_blocks.resize(1);
w.programs[255].native_blocks.resize(1);
w.programs[256].native_blocks.resize(2);
w.programs[257].native_blocks.resize(1);
w.programs[258].native_blocks.resize(1);
w.programs[259].native_blocks.resize(5);
w.programs[260].native_blocks.resize(2);
w.programs[261].native_blocks.resize(2);
w.programs[262].native_blocks.resize(1);
w.programs[263].native_blocks.resize(1);
w.programs[264].native_blocks.resize(1);
w.programs[265].native_blocks.resize(1);
w.programs[266].native_blocks.resize(1);
w.programs[267].native_blocks.resize(1);
w.programs[268].native_blocks.resize(1);
w.programs[269].native_blocks.resize(1);
w.programs[270].native_blocks.resize(1);
w.programs[271].native_blocks.resize(1);
w.programs[272].native_blocks.resize(1);
w.programs[273].native_blocks.resize(1);
w.programs[274].native_blocks.resize(1);
w.programs[275].native_blocks.resize(1);
w.programs[276].native_blocks.resize(1);
w.programs[277].native_blocks.resize(1);
w.programs[278].native_blocks.resize(1);
w.programs[279].native_blocks.resize(1);
w.programs[280].native_blocks.resize(1);
w.programs[281].native_blocks.resize(1);
w.programs[282].native_blocks.resize(1);
w.programs[283].native_blocks.resize(1);
w.programs[284].native_blocks.resize(1);
w.programs[285].native_blocks.resize(1);
w.programs[286].native_blocks.resize(1);
w.programs[287].native_blocks.resize(1);
w.programs[288].native_blocks.resize(1);
w.programs[289].native_blocks.resize(1);
w.programs[290].native_blocks.resize(1);
w.programs[291].native_blocks.resize(2);
w.programs[292].native_blocks.resize(1);
w.programs[293].native_blocks.resize(1);
w.programs[294].native_blocks.resize(3);
w.programs[295].native_blocks.resize(1);
w.programs[296].native_blocks.resize(1);
w.programs[297].native_blocks.resize(1);
w.programs[298].native_blocks.resize(1);
w.programs[299].native_blocks.resize(2);
w.programs[300].native_blocks.resize(1);
w.programs[301].native_blocks.resize(7);
w.programs[302].native_blocks.resize(2);
w.programs[303].native_blocks.resize(1);
w.programs[304].native_blocks.resize(2);
w.programs[305].native_blocks.resize(3);
w.programs[306].native_blocks.resize(2);
w.programs[307].native_blocks.resize(3);
w.programs[308].native_blocks.resize(1);
w.programs[309].native_blocks.resize(2);
w.programs[310].native_blocks.resize(2);
w.programs[311].native_blocks.resize(2);
w.programs[312].native_blocks.resize(2);
w.programs[313].native_blocks.resize(2);
w.programs[314].native_blocks.resize(2);
w.programs[315].native_blocks.resize(2);
w.programs[316].native_blocks.resize(2);
w.programs[317].native_blocks.resize(2);
w.programs[318].native_blocks.resize(1);
w.programs[319].native_blocks.resize(1);
w.programs[320].native_blocks.resize(1);
w.programs[321].native_blocks.resize(4);
w.programs[322].native_blocks.resize(1);
w.programs[323].native_blocks.resize(1);
w.programs[324].native_blocks.resize(1);
w.programs[325].native_blocks.resize(1);
w.programs[326].native_blocks.resize(1);
w.programs[327].native_blocks.resize(1);
w.programs[328].native_blocks.resize(1);
w.programs[329].native_blocks.resize(1);
w.programs[330].native_blocks.resize(1);
w.programs[331].native_blocks.resize(1);
w.programs[332].native_blocks.resize(1);
w.programs[333].native_blocks.resize(1);
w.programs[334].native_blocks.resize(2);
w.programs[335].native_blocks.resize(2);
w.programs[336].native_blocks.resize(2);
w.programs[337].native_blocks.resize(1);
w.programs[338].native_blocks.resize(1);
w.programs[339].native_blocks.resize(1);
w.programs[340].native_blocks.resize(1);
w.programs[341].native_blocks.resize(1);
w.programs[342].native_blocks.resize(1);
w.programs[343].native_blocks.resize(1);
w.programs[344].native_blocks.resize(1);
w.programs[345].native_blocks.resize(1);
w.programs[346].native_blocks.resize(1);
w.programs[347].native_blocks.resize(1);
w.programs[348].native_blocks.resize(2);
w.programs[349].native_blocks.resize(1);
w.programs[350].native_blocks.resize(1);
w.programs[351].native_blocks.resize(1);
w.programs[352].native_blocks.resize(1);
w.programs[353].native_blocks.resize(1);
w.programs[354].native_blocks.resize(1);
w.programs[355].native_blocks.resize(1);
w.programs[356].native_blocks.resize(1);
w.programs[357].native_blocks.resize(1);
w.programs[358].native_blocks.resize(1);
w.programs[359].native_blocks.resize(1);
w.programs[360].native_blocks.resize(1);
w.programs[361].native_blocks.resize(2);
w.programs[362].native_blocks.resize(2);
w.programs[363].native_blocks.resize(2);
w.programs[364].native_blocks.resize(2);
w.programs[365].native_blocks.resize(2);
w.programs[366].native_blocks.resize(2);
w.programs[367].native_blocks.resize(2);
w.programs[368].native_blocks.resize(2);
w.programs[369].native_blocks.resize(2);
w.programs[370].native_blocks.resize(2);
w.programs[371].native_blocks.resize(2);
w.programs[372].native_blocks.resize(2);
w.programs[373].native_blocks.resize(2);
w.programs[374].native_blocks.resize(3);
w.programs[1].native_blocks[0]=block_1_0;
w.programs[5].native_blocks[0]=block_5_0;
w.programs[8].native_blocks[0]=block_8_0;
w.programs[9].native_blocks[0]=block_9_0;
w.programs[11].native_blocks[0]=block_11_0;
w.programs[11].native_blocks[6]=block_11_6;
w.programs[12].native_blocks[0]=block_12_0;
w.programs[13].native_blocks[0]=block_13_0;
w.programs[14].native_blocks[0]=block_14_0;
w.programs[15].native_blocks[0]=block_15_0;
w.programs[17].native_blocks[0]=block_17_0;
w.programs[18].native_blocks[0]=block_18_0;
w.programs[19].native_blocks[0]=block_19_0;
w.programs[20].native_blocks[0]=block_20_0;
w.programs[21].native_blocks[0]=block_21_0;
w.programs[22].native_blocks[0]=block_22_0;
w.programs[23].native_blocks[0]=block_23_0;
w.programs[24].native_blocks[0]=block_24_0;
w.programs[25].native_blocks[0]=block_25_0;
w.programs[26].native_blocks[0]=block_26_0;
w.programs[27].native_blocks[0]=block_27_0;
w.programs[29].native_blocks[6]=block_29_6;
w.programs[30].native_blocks[0]=block_30_0;
w.programs[31].native_blocks[0]=block_31_0;
w.programs[31].native_blocks[2]=block_31_2;
w.programs[31].native_blocks[4]=block_31_4;
w.programs[33].native_blocks[0]=block_33_0;
w.programs[34].native_blocks[0]=block_34_0;
w.programs[34].native_blocks[2]=block_34_2;
w.programs[35].native_blocks[0]=block_35_0;
w.programs[36].native_blocks[0]=block_36_0;
w.programs[36].native_blocks[28]=block_36_28;
w.programs[37].native_blocks[0]=block_37_0;
w.programs[39].native_blocks[0]=block_39_0;
w.programs[41].native_blocks[0]=block_41_0;
w.programs[42].native_blocks[0]=block_42_0;
w.programs[44].native_blocks[0]=block_44_0;
w.programs[46].native_blocks[0]=block_46_0;
w.programs[50].native_blocks[0]=block_50_0;
w.programs[51].native_blocks[0]=block_51_0;
w.programs[51].native_blocks[20]=block_51_20;
w.programs[53].native_blocks[0]=block_53_0;
w.programs[53].native_blocks[20]=block_53_20;
w.programs[54].native_blocks[0]=block_54_0;
w.programs[55].native_blocks[1]=block_55_1;
w.programs[55].native_blocks[6]=block_55_6;
w.programs[55].native_blocks[12]=block_55_12;
w.programs[55].native_blocks[16]=block_55_16;
w.programs[55].native_blocks[19]=block_55_19;
w.programs[55].native_blocks[22]=block_55_22;
w.programs[55].native_blocks[25]=block_55_25;
w.programs[59].native_blocks[0]=block_59_0;
w.programs[61].native_blocks[0]=block_61_0;
w.programs[63].native_blocks[0]=block_63_0;
w.programs[65].native_blocks[0]=block_65_0;
w.programs[67].native_blocks[0]=block_67_0;
w.programs[69].native_blocks[0]=block_69_0;
w.programs[71].native_blocks[0]=block_71_0;
w.programs[73].native_blocks[0]=block_73_0;
w.programs[75].native_blocks[1]=block_75_1;
w.programs[75].native_blocks[3]=block_75_3;
w.programs[79].native_blocks[0]=block_79_0;
w.programs[84].native_blocks[0]=block_84_0;
w.programs[85].native_blocks[0]=block_85_0;
w.programs[86].native_blocks[0]=block_86_0;
w.programs[86].native_blocks[2]=block_86_2;
w.programs[87].native_blocks[0]=block_87_0;
w.programs[88].native_blocks[1]=block_88_1;
w.programs[89].native_blocks[0]=block_89_0;
w.programs[90].native_blocks[0]=block_90_0;
w.programs[93].native_blocks[0]=block_93_0;
w.programs[94].native_blocks[0]=block_94_0;
w.programs[94].native_blocks[2]=block_94_2;
w.programs[96].native_blocks[0]=block_96_0;
w.programs[98].native_blocks[0]=block_98_0;
w.programs[101].native_blocks[0]=block_101_0;
w.programs[104].native_blocks[0]=block_104_0;
w.programs[105].native_blocks[0]=block_105_0;
w.programs[106].native_blocks[0]=block_106_0;
w.programs[107].native_blocks[0]=block_107_0;
w.programs[107].native_blocks[16]=block_107_16;
w.programs[108].native_blocks[0]=block_108_0;
w.programs[108].native_blocks[12]=block_108_12;
w.programs[109].native_blocks[0]=block_109_0;
w.programs[111].native_blocks[0]=block_111_0;
w.programs[111].native_blocks[3]=block_111_3;
w.programs[112].native_blocks[0]=block_112_0;
w.programs[113].native_blocks[0]=block_113_0;
w.programs[114].native_blocks[0]=block_114_0;
w.programs[116].native_blocks[0]=block_116_0;
w.programs[117].native_blocks[0]=block_117_0;
w.programs[121].native_blocks[0]=block_121_0;
w.programs[122].native_blocks[0]=block_122_0;
w.programs[124].native_blocks[0]=block_124_0;
w.programs[125].native_blocks[0]=block_125_0;
w.programs[127].native_blocks[0]=block_127_0;
w.programs[130].native_blocks[0]=block_130_0;
w.programs[130].native_blocks[7]=block_130_7;
w.programs[131].native_blocks[0]=block_131_0;
w.programs[132].native_blocks[0]=block_132_0;
w.programs[132].native_blocks[3]=block_132_3;
w.programs[135].native_blocks[0]=block_135_0;
w.programs[138].native_blocks[0]=block_138_0;
w.programs[139].native_blocks[1]=block_139_1;
w.programs[139].native_blocks[4]=block_139_4;
w.programs[139].native_blocks[8]=block_139_8;
w.programs[139].native_blocks[11]=block_139_11;
w.programs[143].native_blocks[0]=block_143_0;
w.programs[144].native_blocks[0]=block_144_0;
w.programs[145].native_blocks[0]=block_145_0;
w.programs[146].native_blocks[0]=block_146_0;
w.programs[147].native_blocks[0]=block_147_0;
w.programs[149].native_blocks[0]=block_149_0;
w.programs[150].native_blocks[1]=block_150_1;
w.programs[151].native_blocks[0]=block_151_0;
w.programs[152].native_blocks[0]=block_152_0;
w.programs[152].native_blocks[4]=block_152_4;
w.programs[154].native_blocks[0]=block_154_0;
w.programs[155].native_blocks[1]=block_155_1;
w.programs[158].native_blocks[0]=block_158_0;
w.programs[158].native_blocks[6]=block_158_6;
w.programs[158].native_blocks[13]=block_158_13;
w.programs[159].native_blocks[0]=block_159_0;
w.programs[160].native_blocks[0]=block_160_0;
w.programs[161].native_blocks[0]=block_161_0;
w.programs[161].native_blocks[8]=block_161_8;
w.programs[163].native_blocks[0]=block_163_0;
w.programs[164].native_blocks[0]=block_164_0;
w.programs[164].native_blocks[2]=block_164_2;
w.programs[167].native_blocks[1]=block_167_1;
w.programs[168].native_blocks[0]=block_168_0;
w.programs[170].native_blocks[0]=block_170_0;
w.programs[170].native_blocks[4]=block_170_4;
w.programs[170].native_blocks[7]=block_170_7;
w.programs[171].native_blocks[0]=block_171_0;
w.programs[174].native_blocks[0]=block_174_0;
w.programs[177].native_blocks[0]=block_177_0;
w.programs[180].native_blocks[0]=block_180_0;
w.programs[181].native_blocks[0]=block_181_0;
w.programs[182].native_blocks[0]=block_182_0;
w.programs[183].native_blocks[0]=block_183_0;
w.programs[186].native_blocks[0]=block_186_0;
w.programs[189].native_blocks[0]=block_189_0;
w.programs[190].native_blocks[4]=block_190_4;
w.programs[191].native_blocks[0]=block_191_0;
w.programs[192].native_blocks[0]=block_192_0;
w.programs[200].native_blocks[0]=block_200_0;
w.programs[201].native_blocks[0]=block_201_0;
w.programs[202].native_blocks[0]=block_202_0;
w.programs[203].native_blocks[0]=block_203_0;
w.programs[205].native_blocks[0]=block_205_0;
w.programs[206].native_blocks[0]=block_206_0;
w.programs[207].native_blocks[0]=block_207_0;
w.programs[208].native_blocks[0]=block_208_0;
w.programs[209].native_blocks[0]=block_209_0;
w.programs[210].native_blocks[0]=block_210_0;
w.programs[211].native_blocks[0]=block_211_0;
w.programs[213].native_blocks[0]=block_213_0;
w.programs[214].native_blocks[0]=block_214_0;
w.programs[215].native_blocks[0]=block_215_0;
w.programs[216].native_blocks[0]=block_216_0;
w.programs[218].native_blocks[0]=block_218_0;
w.programs[219].native_blocks[0]=block_219_0;
w.programs[220].native_blocks[0]=block_220_0;
w.programs[221].native_blocks[0]=block_221_0;
w.programs[222].native_blocks[0]=block_222_0;
w.programs[222].native_blocks[7]=block_222_7;
w.programs[223].native_blocks[0]=block_223_0;
w.programs[224].native_blocks[0]=block_224_0;
w.programs[225].native_blocks[0]=block_225_0;
w.programs[226].native_blocks[0]=block_226_0;
w.programs[227].native_blocks[0]=block_227_0;
w.programs[228].native_blocks[2]=block_228_2;
w.programs[229].native_blocks[0]=block_229_0;
w.programs[230].native_blocks[0]=block_230_0;
w.programs[231].native_blocks[0]=block_231_0;
w.programs[232].native_blocks[0]=block_232_0;
w.programs[233].native_blocks[0]=block_233_0;
w.programs[234].native_blocks[0]=block_234_0;
w.programs[235].native_blocks[0]=block_235_0;
w.programs[236].native_blocks[0]=block_236_0;
w.programs[237].native_blocks[0]=block_237_0;
w.programs[238].native_blocks[0]=block_238_0;
w.programs[240].native_blocks[0]=block_240_0;
w.programs[240].native_blocks[2]=block_240_2;
w.programs[241].native_blocks[0]=block_241_0;
w.programs[242].native_blocks[0]=block_242_0;
w.programs[243].native_blocks[0]=block_243_0;
w.programs[244].native_blocks[0]=block_244_0;
w.programs[245].native_blocks[0]=block_245_0;
w.programs[246].native_blocks[0]=block_246_0;
w.programs[247].native_blocks[0]=block_247_0;
w.programs[248].native_blocks[0]=block_248_0;
w.programs[249].native_blocks[0]=block_249_0;
w.programs[250].native_blocks[0]=block_250_0;
w.programs[253].native_blocks[0]=block_253_0;
w.programs[254].native_blocks[0]=block_254_0;
w.programs[256].native_blocks[0]=block_256_0;
w.programs[257].native_blocks[0]=block_257_0;
w.programs[259].native_blocks[0]=block_259_0;
w.programs[259].native_blocks[3]=block_259_3;
w.programs[260].native_blocks[0]=block_260_0;
w.programs[261].native_blocks[0]=block_261_0;
w.programs[262].native_blocks[0]=block_262_0;
w.programs[264].native_blocks[0]=block_264_0;
w.programs[265].native_blocks[0]=block_265_0;
w.programs[266].native_blocks[0]=block_266_0;
w.programs[267].native_blocks[0]=block_267_0;
w.programs[268].native_blocks[0]=block_268_0;
w.programs[269].native_blocks[0]=block_269_0;
w.programs[270].native_blocks[0]=block_270_0;
w.programs[271].native_blocks[0]=block_271_0;
w.programs[272].native_blocks[0]=block_272_0;
w.programs[273].native_blocks[0]=block_273_0;
w.programs[275].native_blocks[0]=block_275_0;
w.programs[276].native_blocks[0]=block_276_0;
w.programs[277].native_blocks[0]=block_277_0;
w.programs[278].native_blocks[0]=block_278_0;
w.programs[279].native_blocks[0]=block_279_0;
w.programs[280].native_blocks[0]=block_280_0;
w.programs[282].native_blocks[0]=block_282_0;
w.programs[283].native_blocks[0]=block_283_0;
w.programs[284].native_blocks[0]=block_284_0;
w.programs[286].native_blocks[0]=block_286_0;
w.programs[287].native_blocks[0]=block_287_0;
w.programs[288].native_blocks[0]=block_288_0;
w.programs[289].native_blocks[0]=block_289_0;
w.programs[290].native_blocks[0]=block_290_0;
w.programs[291].native_blocks[0]=block_291_0;
w.programs[294].native_blocks[0]=block_294_0;
w.programs[295].native_blocks[0]=block_295_0;
w.programs[297].native_blocks[0]=block_297_0;
w.programs[298].native_blocks[0]=block_298_0;
w.programs[299].native_blocks[0]=block_299_0;
w.programs[301].native_blocks[0]=block_301_0;
w.programs[302].native_blocks[0]=block_302_0;
w.programs[303].native_blocks[0]=block_303_0;
w.programs[304].native_blocks[0]=block_304_0;
w.programs[305].native_blocks[0]=block_305_0;
w.programs[306].native_blocks[0]=block_306_0;
w.programs[307].native_blocks[0]=block_307_0;
w.programs[309].native_blocks[0]=block_309_0;
w.programs[310].native_blocks[0]=block_310_0;
w.programs[311].native_blocks[0]=block_311_0;
w.programs[312].native_blocks[0]=block_312_0;
w.programs[313].native_blocks[0]=block_313_0;
w.programs[314].native_blocks[0]=block_314_0;
w.programs[315].native_blocks[0]=block_315_0;
w.programs[316].native_blocks[0]=block_316_0;
w.programs[317].native_blocks[0]=block_317_0;
w.programs[318].native_blocks[0]=block_318_0;
w.programs[319].native_blocks[0]=block_319_0;
w.programs[320].native_blocks[0]=block_320_0;
w.programs[322].native_blocks[0]=block_322_0;
w.programs[325].native_blocks[0]=block_325_0;
w.programs[326].native_blocks[0]=block_326_0;
w.programs[327].native_blocks[0]=block_327_0;
w.programs[328].native_blocks[0]=block_328_0;
w.programs[329].native_blocks[0]=block_329_0;
w.programs[330].native_blocks[0]=block_330_0;
w.programs[331].native_blocks[0]=block_331_0;
w.programs[332].native_blocks[0]=block_332_0;
w.programs[333].native_blocks[0]=block_333_0;
w.programs[334].native_blocks[0]=block_334_0;
w.programs[335].native_blocks[0]=block_335_0;
w.programs[336].native_blocks[0]=block_336_0;
w.programs[339].native_blocks[0]=block_339_0;
w.programs[340].native_blocks[0]=block_340_0;
w.programs[341].native_blocks[0]=block_341_0;
w.programs[342].native_blocks[0]=block_342_0;
w.programs[343].native_blocks[0]=block_343_0;
w.programs[344].native_blocks[0]=block_344_0;
w.programs[345].native_blocks[0]=block_345_0;
w.programs[346].native_blocks[0]=block_346_0;
w.programs[347].native_blocks[0]=block_347_0;
w.programs[348].native_blocks[0]=block_348_0;
w.programs[351].native_blocks[0]=block_351_0;
w.programs[352].native_blocks[0]=block_352_0;
w.programs[353].native_blocks[0]=block_353_0;
w.programs[354].native_blocks[0]=block_354_0;
w.programs[355].native_blocks[0]=block_355_0;
w.programs[356].native_blocks[0]=block_356_0;
w.programs[357].native_blocks[0]=block_357_0;
w.programs[358].native_blocks[0]=block_358_0;
w.programs[359].native_blocks[0]=block_359_0;
w.programs[374].native_blocks[0]=block_374_0;
}
} // namespace engine
