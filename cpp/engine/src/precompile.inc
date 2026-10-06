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
