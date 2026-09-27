#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <algorithm>
#include <any>
#include <cstdint>
#include <map>

using namespace std;
using bytecode = vector<tuple<int, int, vector<any>>>;

vector<string> keywords = {
  "println", "print", "prompt", "repeat", "set", "increment", "while", "if"
};

string f;
int lc;

pair<int, size_t> kwMatch(string_view text, size_t pos){
  pos = text.find_first_not_of(" \t", pos);
  if (pos == string::npos) return {0, 0};

  for(size_t i = 0; i < keywords.size(); i++){
    if(text.substr(pos).starts_with(keywords[i])){
      return {static_cast<int>(i + 1), pos + keywords[i].size()};
    }
  }
  return {0, 0};
}

void throwErr(string e){
  cerr << f << ":" << (lc + 1) << ": " << e << "\n";
  exit(1);
}

pair<string, int> spMatch(const string& str, size_t pos){
  pos = str.find_first_not_of(" \t", pos);
  if (pos == string::npos || pos >= str.length()) return {"", -1};

  if(str[pos] == '"'){
    size_t end_pos = str.find('"', pos + 1);
    if(end_pos == string::npos){
      throwErr("unclosed quote");
    }
    size_t len = (end_pos + 1) - pos;
    return {str.substr(pos, len), static_cast<int>(end_pos + 1)};
  }

  auto start_it = str.begin() + pos;
  auto end_it = find(start_it, str.end(), ' ');
  return {string(start_it, end_it), int(end_it - str.begin())};
}

pair<uint8_t, any> parseExp(string str){
  if(str.length() < 1){
    throwErr("incomplete statement");
  }else if(str.length() >= 2 && str.starts_with('"') && str.ends_with('"')){
    return {1, str.substr(1, str.length() - 2)};
  }else if(!str.empty() && all_of(str.begin(), str.end(), [](char c){return isdigit(c) || c == '.' || c == '-';})){
    return {2, stof(str)};
  }else{
    return {3, str};
  }
}

tuple<any, uint8_t, any> parseBool(string str){
  auto [vz, off] = spMatch(str, 0);
  auto [va, off2] = spMatch(str, off + 1);
  auto [vq, off3] = spMatch(str, off2 + 1);

  uint8_t operand = 0;

  if(va.length() == 1){
    if (va[0] == '>') operand = 0;
    else if (va[0] == '<') operand = 1;
  }else if(va.length() == 2){
    if (va == "==") operand = 2;
    else if (va == "!=") operand = 3;
    else if (va == ">=") operand = 4;
    else if (va == "<=") operand = 5;
  }

  return {parseExp(vz), operand, parseExp(vq)};
}

bytecode compile(string fin){
  f = fin;
  ifstream ifs(fin);

  if(!ifs.is_open()){
    cerr << "file does not exist?\n";
    exit(1);
  }

  bytecode cd;
  lc = -1;
  string ln;

  while(getline(ifs, ln)){
    lc++;
    if (size_t first = ln.find_first_not_of(" \t\r"); first == string::npos || ln[first] == '#') continue;

    vector<any> args;

    // indentation amount
    auto it = find_if(ln.begin(), ln.end(), [](char c) { return c != ' '; });
    int sp = distance(ln.begin(), it);
    if(sp % 2 != 0){
      throwErr("bad indentation?");
    }

    // keyword & offset
    auto [op, off] = kwMatch(ln, 0);
    if(op == 0){
      throwErr("unknown keyword, did you make a typo?");
    }

    // keywords
    switch(op){
      case 1: {
        auto [vn, off2] = spMatch(ln, off);
        args.push_back(parseExp(vn));
        break;
      }
      case 2: {
        auto [vn, off2] = spMatch(ln, off);
        args.push_back(parseExp(vn));
        break;
      }
      case 3: {
        auto [vn, off2] = spMatch(ln, off);
        if(off2 == -1){
          throwErr("invalid syntax");
        }
        auto [va, off3] = spMatch(ln, off2 + 4);
        args.push_back(parseExp(vn));
        args.push_back(parseExp(va));
        break;
      }
      case 4: {
        auto [vn, off2] = spMatch(ln, off);
        auto parsed = parseExp(vn);
        if(parsed.second.type() != typeid(float) && parsed.second.type() != typeid(string)){
          throwErr("repeat cannot take NaN");
        }
        args.push_back(parsed);
        break;
      }
      case 5: {
        auto [vn, off2] = spMatch(ln, off);
        if(off2 == -1){
          throwErr("invalid syntax");
        }
        auto [va, off3] = spMatch(ln, off2 + 4);
        args.push_back(parseExp(vn));
        args.push_back(parseExp(va));
        break;
      }
      case 6: {
        auto [vn, off2] = spMatch(ln, off);
        args.push_back(parseExp(vn));
        break;
      }
      case 7: {
        auto [vn, va, vq] = parseBool(ln.erase(0, off + (sp / 2)));
        args.push_back(vn);
        args.push_back(va);
        args.push_back(vq);
        break;
      }
      case 8: {
        auto [vn, va, vq] = parseBool(ln.erase(0, off + (sp / 2)));
        args.push_back(vn);
        args.push_back(va);
        args.push_back(vq);
        break;
      }
    }

    cd.push_back({op, (sp / 2), args});
  }

  return cd;
}

map<string, any> vars;

any evalArg(const any& arg){
  auto [t, v] = any_cast<pair<uint8_t, any>>(arg);
  if(t == 3){
    string name = any_cast<string>(v);
    return vars.count(name) ? vars[name] : 0.0f;
  }
  return v;
}

void exec(const bytecode& cd, size_t& pc){
  auto [op, sp, args] = cd[pc];

  auto compareValues = [](const auto& lv, const auto& rv, uint8_t op) -> bool {
    switch(op){
      case 0: return lv > rv;
      case 1: return lv < rv;
      case 2: return lv == rv;
      case 3: return lv != rv;
      case 4: return lv >= rv;
      case 5: return lv <= rv;
    }
    return false;
  };

  auto evaluateCondition = [&](const auto& args) -> bool {
    auto l = evalArg(args[0]);
    uint8_t op = any_cast<uint8_t>(args[1]);
    auto r = evalArg(args[2]);

    if(l.type() == typeid(float) && r.type() == typeid(float)){
      return compareValues(any_cast<float>(l), any_cast<float>(r), op);
    }else if(l.type() == typeid(std::string) && r.type() == typeid(std::string)){
      return compareValues(any_cast<const std::string&>(l), any_cast<const std::string&>(r), op);
    }
    return false;
  };

  auto executeBlock = [&](size_t s_pc, size_t e_pc) {
    size_t ip = s_pc;
    while(ip < e_pc){
      exec(cd, ip);
      ip++;
    }
  };

  switch(op){
    case 1: { // println
      auto val = evalArg(args[0]);
      if(val.type() == typeid(string)) cout << any_cast<string>(val) << "\n";
      else if(val.type() == typeid(float)) cout << any_cast<float>(val) << "\n";
      break;
    }
    case 2: { // print
      auto val = evalArg(args[0]);
      if(val.type() == typeid(string)) cout << any_cast<string>(val);
      else if(val.type() == typeid(float)) cout << any_cast<float>(val);
      break;
    }
    case 3: { // prompt
      string input;
      cout << any_cast<string>(any_cast<pair<uint8_t, any>>(args[0]).second);
      getline(cin, input);
      vars[any_cast<string>(any_cast<pair<uint8_t, any>>(args[1]).second)] = input;
      break;
    }
    case 4: { // repeat
      float cnt = any_cast<float>(evalArg(args[0]));
      size_t s_pc = pc + 1;
      size_t e_pc = s_pc;
      while(e_pc < cd.size() && get<1>(cd[e_pc]) > sp) e_pc++;

      for(int i = 0; i < cnt; i++){
        size_t ip = s_pc;
        while(ip < e_pc){
          exec(cd, ip);
          ip++;
        }
      }
      pc = e_pc - 1;
      break;
    }
    case 5: { // set
      string name = any_cast<string>(any_cast<pair<uint8_t, any>>(args[0]).second);
      vars[name] = evalArg(args[1]);
      break;
    }
    case 6: { // increment
      string name = any_cast<string>(any_cast<pair<uint8_t, any>>(args[0]).second);
      float cur = vars.count(name) ? any_cast<float>(vars[name]) : 0.0f;
      vars[name] = cur + 1.0f;
      break;
    }
    case 7: { // while
      size_t s_pc = pc + 1;
      size_t e_pc = s_pc;
      while(e_pc < cd.size() && get<1>(cd[e_pc]) > sp) e_pc++;

      while(evaluateCondition(args)){
        executeBlock(s_pc, e_pc);
      }
      pc = e_pc - 1;
      break;
    }
    case 8: { // if
      size_t s_pc = pc + 1;
      size_t e_pc = s_pc;
      while(e_pc < cd.size() && get<1>(cd[e_pc]) > sp) e_pc++;

      if(evaluateCondition(args)){
        executeBlock(s_pc, e_pc);
      }
      pc = e_pc - 1;
      break;
    }
  }
}

void run(const bytecode& cd){
  for(size_t pc = 0; pc < cd.size(); pc++){
    exec(cd, pc);
  }
}

int main(int argc, char* argv[]){
  if(argc < 2){
    cerr << "a file is required\n";
    return 1;
  }

  run(compile(argv[1]));
  return 0;
}
