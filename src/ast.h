# ifndef AST_H
# define AST_H
# pragma once //只引用一次
// todo: 对函数调用的调整

# include <memory>
# include <string>
# include <iostream>
# include <unordered_map> // 记录const情况用
# include <iostream>
# include <assert.h>
# include <limits.h> // 获取正无穷大
# include <stack>
# include <vector> // 制作func_param_list
# include <map> // 记录库函数的使用次数情况的map

// 尝试定义新的类型, 用于承接变量的信息
struct varinfo {
    int vartype; // 0为const, 1为var, 2为函数?
    std::string ident;  // 变量标号
    int layer; // 表明ident所属的层级, -1为函数参数变量, -2为全局变量(两个特殊层级)
    int value; // const的计算值
    std::string name;  // 这个是变量名称 这里可能有点问题. 先写着, 报错再说
    int ret_type; // 0为void(无返回), 1为int(有返回)
};

struct paraminfo {
    int paramtype; // 0为普通参数, 1用来处理数组
    std::string ident;
    std::string name;
};

// 记录const及其值的map
extern std::unordered_map<std::string, varinfo> var_map;
// 记录while情形的stack
extern std::stack<int> while_stack;
// 记录函数使用的参数信息
extern std::vector<paraminfo> func_param_list;
// 记录库函数的使用次数情况
extern std::map<std::string, int> sysy_use_time;
// 记录库函数的声明语句
extern std::map<std::string, std::string> sysy_decl;

class BaseAST {
    public:
    std::string fromup;
    int layer;  // 添加向下传递的layer标号
    // std::string upward = "";
    virtual ~BaseAST() = default;
    // Dump()是用来输出的函数
    //virtual void Dump() const = 0;
    virtual std::string Dump() const = 0;
    // GetUpward是收集数据向上传的函数
    // 有的数据需要上传使用, 用这个函数完成
    virtual std::string GetUpward() const = 0;
    // GetValue是收集数字向上传的函数
    // 目前用于编译期间计算
    virtual int GetValue() const = 0;
};

// CompUnitAST 是 BaseAST
class CompUnitAST: public BaseAST {
    public:
    std::unique_ptr<BaseAST> complist;
    int ast_state; // 0为多个值, 1为单个值
    std::string GetUpward() const override {return "";}
    std::string Dump() const override{
        std::string get_str;
        get_str += complist->Dump();
        // 在函数最前方添加SysY库函数的声明
        std::string func_decl_str = "";
        for (auto it=sysy_use_time.begin();it!=sysy_use_time.end();++it) {
            if (it->second > 0) func_decl_str += sysy_decl[it->first] + "\n";
        }
        if (func_decl_str.length() > 0) return func_decl_str + "\n" + get_str;
        else return get_str;
    };
    int GetValue() const override {return complist->GetValue();}
};

class CompListAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> list;
    std::unique_ptr<BaseAST> item;
    int ast_state; // 0为多个值, 1为单个值
    std::string Dump() const override {
        if (ast_state==0) return list->Dump() + "\n" + item->Dump();
        else if (ast_state==1) return item->Dump();
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) return list->GetUpward() + item->GetUpward();
        else if (ast_state==1) return item->GetUpward();
        else return "";
    }
    int GetValue() const override {return 0;}
};

class CompItemAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> content;
    int ast_state;  // 0为函数, 1为全局变量
    std::string Dump() const override {return content->Dump();}
    std::string GetUpward() const override {return content->GetUpward();}
    int GetValue() const override {return content->GetValue();}
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  std::string func_type;
  std::string ident;
  std::unique_ptr<BaseAST> p_list;  // 参数列表
  std::unique_ptr<BaseAST> block;
  int ast_state; // 0为无parameter, 1为有parameter
  std::string GetUpward() const override {return "";}
  std::string Dump() const override {
    std::string get_str = "";
    // 将函数信息放入符号表(要先放, 尝试支持递归操作)
    varinfo func_info;
    std::string name = "@" + ident;
    func_info.vartype = 2; // 表明为函数
    func_info.ident = ident; // 函数名称
    func_info.name = name; // koopa IR中要使用的名称
    func_info.layer = -2; // 函数是全局量
    if (func_type=="int") func_info.ret_type = 1;
    else func_info.ret_type = 0;
    var_map[name] = func_info; // 将函数信息放入var_map
    get_str += "fun " + name;
    // 添加函数参数信息
    if (ast_state==0) get_str += "()";
    else if (ast_state==1) get_str += "(" + p_list->Dump() + ")";
    else get_str += "()";
    // 函数类型标识
    if (func_type=="int") get_str += ": i32 ";
    else get_str += " ";
    // 函数起始
    get_str += "{\n\%entry:\n";
    // 函数变量的依次声明
    if (ast_state==1) {
        // 遍历函数变量表
        for(int i=0;i<func_param_list.size();++i){
            if (func_param_list[i].paramtype==0) {
                varinfo newinfo;
                std::string key = func_param_list[i].ident + "_param";
                newinfo.ident = func_param_list[i].ident;
                newinfo.layer = -1; // 表明为函数变量层
                newinfo.vartype = 1; // 视为变量
                newinfo.name = "%" + key; // 使用的名字
                var_map[key] = newinfo; // 放入函数符号表
                get_str += "  " + newinfo.name + " = alloc i32\n"; // 申请存储语句
                get_str += "  store " + func_param_list[i].name + ", " + newinfo.name + "\n"; // 放入存储语句
            }
            else;
        }
        func_param_list.clear(); // 清空变量列表
    }
    else get_str += "";
    // 函数本体
    get_str += block->Dump();
    // 函数结尾(如果有return就不添加ret语句, 否则添加ret语句)
    if (block->GetUpward()=="return") get_str += "";
    else {
        if (func_type=="int") get_str += "  ret 0\n";
        else get_str += "  ret\n";
    }
    get_str += "}\n";
    // 遍历var_map, 清空-1层的所有ident记录
    // 需要在var_map非空的条件下执行
    if (!var_map.empty()) {
        auto it = var_map.begin();
        while(it!=var_map.end()) {
            if (it->second.layer==-1) it = var_map.erase(it);
            else ++it;
        }
    }
    return get_str;
  }
  int GetValue() const override {
    // block->layer = layer; 
    return block->GetValue();
  }
};

class FuncFParamsAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> param;  // 参数
    std::unique_ptr<BaseAST> p_list;  // 参数列表
    int ast_state; // 0为有多个参数, 1为单个参数
    std::string Dump() const override {
        if (ast_state==0){
            std::string get_str = "";
            get_str += param->Dump() + ", " + p_list->Dump();
            get_str += "";
            return get_str;
        }
        else if (ast_state==1) return param->Dump();
        else return "";
    }
    std::string GetUpward() const override {return "";}
    int GetValue() const override {return 0;}
};

class FuncFParamAST : public BaseAST {
    public:
    std::string btype;
    std::string ident;
    std::string Dump() const override {
        if (btype=="int") {
            paraminfo pinfo; // 记录函数参数信息
            std::string name = "@"+ident;
            pinfo.paramtype = 0;
            pinfo.ident = ident;
            pinfo.name = name;
            func_param_list.push_back(pinfo);
            return name + ": i32";
        }
        else return "";
    }
    std::string GetUpward() const override {return "";}
    int GetValue() const override {return 0;}
};

// 这个是必要的
class FuncTypeAST : public BaseAST {
    public:
    std::string func_type;
    std::string GetUpward() const override {
        if (func_type == "int") {return "int";}
        else if (func_type=="void") {return "void";}
        else return "";
    }
    std::string Dump() const override {
        if (func_type == "int") {return "i32";}
        else if (func_type=="void") {return "";}
        else {return "";}
    };
    int GetValue() const override {return 0;}
};

class BlockAST : public BaseAST {
    public:
    // lv4修改: 改成blocklist以反映变化
    // std::unique_ptr<BaseAST> stmt;
    std::unique_ptr<BaseAST> blocklist;
    std::string GetUpward() const override {return blocklist->GetUpward();}
    std::string Dump() const override{
        std::string get_str;
        blocklist->layer = layer;
        get_str = blocklist->Dump();
        // 遍历var_map, 清空该层的所有ident记录
        auto it = var_map.begin();
        while(it!=var_map.end()) {
            if (it->second.layer==layer) it = var_map.erase(it);
            else ++it;
        }
        return get_str;
    };
    int GetValue() const override {return blocklist->GetValue();}
};

class BlockListAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> item;
    std::unique_ptr<BaseAST> blocklist;
    // 区分状态, 若空规则为1状态, 否则为0状态
    int ast_state;
    std::string Dump() const override {
        if (ast_state==0) {
            blocklist->layer = layer;
            item->layer = layer;
            // 添加对于list return后续的语句解读的截断
            // return blocklist->Dump() + item->Dump();
            if (blocklist->GetUpward()=="return") return blocklist->Dump();
            else return blocklist->Dump() + item->Dump();
        }
        else if (ast_state==1) return "";
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) {
            // 传递return信息
            if(item->GetUpward()=="return" || blocklist->GetUpward()=="return") return "return";
            else return "";
        }
        else if (ast_state==1) return "";
        else return "";
    }
    int GetValue() const override {return 0;}
};

class BlockItemAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> item;
    std::string Dump() const override {item->layer = layer; return item->Dump();}
    std::string GetUpward() const override {item->layer = layer; return item->GetUpward();}
    int GetValue() const override {item->layer = layer; return item->GetValue();}
};

class DeclAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> decl;
    std::string Dump() const override {decl->layer = layer; return decl->Dump();}
    std::string GetUpward() const override {decl->layer = layer; return decl->GetUpward();}
    int GetValue() const override {decl->layer = layer; return decl->GetValue();}
};

class ConstDeclAST : public BaseAST {
    public:
    std::string word;
    std::string btype;
    std::unique_ptr<BaseAST> mylist;
    std::string Dump() const override {
        mylist->layer = layer;
        mylist->fromup = btype;
        return mylist->Dump();
    }
    std::string GetUpward() const override {return "";}
    int GetValue() const override {mylist->layer = layer; return mylist->GetValue();}
};

class VarDeclAST : public BaseAST {
    public:
    std::string btype;
    std::unique_ptr<BaseAST> mylist;
    std::string Dump() const override {
        mylist->layer = layer;
        mylist->fromup = btype;
        return mylist->Dump();
    }
    std::string GetUpward() const override {return "";}
    int GetValue() const override {mylist->layer = layer; return mylist->GetValue();}
};

class VarListAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> mylist;
    std::unique_ptr<BaseAST> mydef;
    int ast_state;
    std::string Dump() const override {
        if (ast_state==0) {
            mylist->fromup = fromup;
            mydef->fromup = fromup;
            mylist->layer = layer;
            mydef->layer = layer;
            return mylist->Dump()+mydef->Dump();
        }
        else if (ast_state==1) {
            mydef->fromup = fromup;
            mydef->layer = layer;
            return mydef->Dump();
        }
        else return "";
    }
    std::string GetUpward() const override {return "";}
    int GetValue() const override {return 0;}
};

class VarDefAST : public BaseAST {
    public:
    std::string ident;
    std::unique_ptr<BaseAST> init;
    int ast_state; // 定义表达式状态, 0为有定义, 1为只声明
    std::string Dump() const override {
        // 压入map放在这个位置进行
        varinfo info;
        info.vartype = 1;
        std::string key = ident + "_" + std::to_string(layer);
        // 定义info的一系列信息
        info.ident = ident;
        info.layer = layer;
        info.name = "@" + ident + "_" + std::to_string(layer);
        // var_map[ident] = info;
        var_map[key] = info;
        if (ast_state==0) {
            if (fromup=="int") return "  " + info.name + " = alloc i32\n" + init->Dump() + "  store " + init->GetUpward() + ", " + info.name + "\n";
            else return "";
        }
        else if (ast_state==1) {
            //return info.name + " = alloc "
            if (fromup=="int") return "  " + info.name + " = alloc i32\n";
            else return "";
        }
        else return "";
    }
    std::string GetUpward() const override {return ident;}
    int GetValue() const override {return 0;}
};

class InitValAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> exp;
    std::string Dump() const override {return exp->Dump();}
    std::string GetUpward() const override {return exp->GetUpward();}
    int GetValue() const override {return exp->GetValue();}
};

class ConstListAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> mylist;
    std::unique_ptr<BaseAST> mydef;
    // std::unique_ptr<BaseAST> btype;
    int ast_state;
    std::string Dump() const override {
        if (ast_state==0) {
            // fromup传递数据类型给下方节点
            mydef->fromup = fromup;
            mylist->fromup = fromup;
            // layer传递层次信息给下方节点
            mydef->layer = layer;
            mylist->layer = layer;
            return mylist->Dump()+mydef->Dump();
        }
        else if (ast_state==1) {
            // fromup传递数据类型给下方节点
            // layer传递层次信息给下方节点
            mydef->layer = layer;
            mydef->fromup = fromup;
            return mydef->Dump();
        }
        else return "";
    }
    std::string GetUpward() const override {return fromup;}
    int GetValue() const override {mydef->layer = layer; return mydef->GetValue();}
};

class BTypeAST : public BaseAST {
    public:
    std::string btype;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return btype;}
    int GetValue() const override {return 0;}
};

class ConstDefAST : public BaseAST {
    public:
    std::string ident;
    std::unique_ptr<BaseAST> init;
    std::string Dump() const override {
        // 将const值的记录实现在这个位置
        if (fromup=="int") {
        varinfo constinfo;
        // 定义var_map的key
        std::string key = ident + "_" + std::to_string(layer);
        // 定义constinfo的一系列信息
        constinfo.vartype = 0;
        constinfo.ident = ident;
        constinfo.layer = layer;
        constinfo.value = init->GetValue();
        // var_map[ident] = constinfo;
        // 压入map
        var_map[key] = constinfo;
        }
        else;
        return "";
    }
    std::string GetUpward() const override {return ident;}
    int GetValue() const override {return init->GetValue();}
};

class ConstInitValAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> constexp;
    std::string Dump() const override {return constexp->Dump();}
    std::string GetUpward() const override {return constexp->GetUpward();}
    int GetValue() const override {
        return constexp->GetValue();
    }
};

class ConstExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> exp;
    std::string Dump() const override {return exp->Dump();}
    std::string GetUpward() const override {return exp->GetUpward();}
    int GetValue() const override {return exp->GetValue();}
};

class LValAST : public BaseAST {
    public:
    std::string ident;
    int place; // 表明变量占用的临时符号
    // 根据ident搜索变量信息的函数
    // 注意: 这里存在一个暂时无法解决的bug
    // 目前不修, 似乎不影响过样例
    varinfo IdentSearch() const {
        varinfo curr_info;
        // int check_layer = -1;
        int check_layer = INT_MAX;
        for (auto it=var_map.begin();it!=var_map.end();++it) {
            if (it->second.ident==ident) {
                // if (it->second.layer>check_layer) {
                if (it->second.layer<check_layer) {
                    curr_info = it->second;
                    check_layer = it->second.layer;
                }
            }
        }
        std::cout << curr_info.ident << "\n";
        if (check_layer==INT_MAX) {std::cout << ident << " not found!\n"; return curr_info;} // 找不到就报错
        else return curr_info;
    }
    std::string Dump() const override {
        varinfo found = IdentSearch();
        if (found.vartype==0) return "";
        else if (found.vartype==1) {
            if (fromup=="stmt") return "";
            else if (fromup=="exp") return "  %"+std::to_string(place)+" = load "+found.name+"\n";
            else return "";
        }
        else return "";
    }
    std::string GetUpward() const override {
        varinfo found = IdentSearch();
        if (found.vartype==0) return std::to_string(found.value);
        else if (found.vartype==1) {
            if (fromup=="stmt") return found.name;
            else if (fromup=="exp") return "%"+std::to_string(place); 
            else return "";
        }
        else return "";
    }
    int GetValue() const override {
        varinfo found = IdentSearch();
        if (found.vartype==0) return found.value;
        else if (found.vartype==1) return 0;
        else return 0;
    }
};

// lv6: 添加对于if语句的支持
class IfStmtAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> ifstmt;
    std::string Dump() const override {ifstmt->layer = layer; return ifstmt->Dump();}
    std::string GetUpward() const override {ifstmt->layer = layer; return ifstmt->GetUpward();} // 传递return信息
    int GetValue() const override {ifstmt->layer = layer; return ifstmt->GetValue();}
};

class IfOpenStmtAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> judge;
    std::unique_ptr<BaseAST> t_jump;
    std::unique_ptr<BaseAST> f_jump;
    int ast_state;
    int place; // if语句的编号
    std::string Dump() const override {
        std::string get_str;
        std::string then_str = "\%then_" + std::to_string(place);
        std::string end_str = "\%end_" + std::to_string(place);
        if (ast_state==0) {
            judge->layer = layer;
            t_jump->layer = layer;
            f_jump->layer = layer;
            std::string else_str = "\%else_" + std::to_string(place);
            get_str += judge->Dump();
            // 分支跳转指令
            get_str += "  br " + judge->GetUpward() + ", " + then_str + ", " + else_str + "\n\n";
            // then分支(放弃下述优化)
            // 修正: 如果分支中以ret句子结尾(这里暂时实现为存在ret句), 不进行跳转语句的生成
            // get_str += then_str + ":\n" + t_jump->Dump() + "  jump " + end_str + "\n\n";
            get_str += then_str + ":\n" + t_jump->Dump();
            if (t_jump->GetUpward()=="return") get_str += "\n";
            else get_str += "  jump " + end_str + "\n\n";
            // else分支
            // 做与then分支相似的修正(放弃优化)
            // get_str += else_str + ":\n" + f_jump->Dump() + "  jump " + end_str + "\n\n";
            get_str += else_str + ":\n" + f_jump->Dump();
            if (f_jump->GetUpward()=="return") get_str += "\n";
            else get_str += "  jump " + end_str + "\n\n";
            // 开end分支
            // get_str += end_str + ":\n";
            // 修正: 如果then和else分支都出现了ret, 那么就不需要继续生成代码了(暂时放弃这个修改)
            if (!(t_jump->GetUpward()=="return" && f_jump->GetUpward()=="return")) get_str += end_str + ":\n";
            return get_str;
        }
        else if (ast_state==1) {
            judge->layer = layer;
            t_jump->layer = layer;
            get_str += judge->Dump();
            // 分支跳转指令
            get_str += "  br " + judge->GetUpward() + ", " + then_str + ", " + end_str + "\n\n";
            // then分支(放弃优化)
            // 修正: 如果分支中以ret句子结尾(这里暂时实现为存在ret句), 不进行跳转语句的生成
            // get_str += then_str + ":\n" + t_jump->Dump() + "  jump " + end_str + "\n\n";
            get_str += then_str + ":\n" + t_jump->Dump();
            if (t_jump->GetUpward()=="return") get_str += "\n";
            else get_str += "  jump " + end_str + "\n\n";
            // 开end分支
            // 修正: 如果then分支出现了ret那么不需要继续生成代码了(戳啦!)
            get_str += end_str + ":\n";
            // if (t_jump->Dump().find("ret")==std::string::npos) get_str += end_str + ":\n";
            return get_str;
        }
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) {
            // 传递return信息
            if (t_jump->GetUpward()=="return" && f_jump->GetUpward()=="return") return "return";
            else return "";
        }
        else if (ast_state==1) return "";
        else return "";
    }
    int GetValue() const override {return 0;}
};

class IfCloseStmtAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> judge;
    std::unique_ptr<BaseAST> t_jump;
    std::unique_ptr<BaseAST> f_jump;
    int ast_state;  // 状态0是close的if语句, 状态1连接到Stmt
    int place; // if语句的编号
    std::string Dump() const override {
        if (ast_state==0) {
            judge->layer = layer;
            t_jump->layer = layer;
            f_jump->layer = layer;
            std::string get_str;
            std::string then_str = "\%then_" + std::to_string(place);
            std::string end_str = "\%end_" + std::to_string(place);
            std::string else_str = "\%else_" + std::to_string(place);
            get_str += judge->Dump();
            // 分支跳转指令
            get_str += "  br " + judge->GetUpward() + ", " + then_str + ", " + else_str + "\n\n";
            // then分支(放弃优化)
            // 修正: 如果分支中以ret句子结尾(这里暂时实现为存在ret句), 不进行跳转语句的生成
            // get_str += then_str + ":\n" + t_jump->Dump() + "  jump " + end_str + "\n\n";
            get_str += then_str + ":\n" + t_jump->Dump();
            if (t_jump->GetUpward()=="return") get_str += "\n";
            else get_str += "  jump " + end_str + "\n\n";
            // else分支(放弃优化)
            // 修正: 如果分支中以ret句子结尾(这里暂时实现为存在ret句), 不进行跳转语句的生成
            // get_str += else_str + ":\n" + f_jump->Dump() + "  jump " + end_str + "\n\n";
            get_str += else_str + ":\n" + f_jump->Dump();
            if (f_jump->GetUpward()=="return") get_str += "\n";
            else get_str += "  jump " + end_str + "\n\n";
            // 开end分支
            // 修正: 如果then和else分支都出现了ret, 那么就不需要继续生成代码了(放弃这个改动)
            // get_str += end_str + ":\n";
            if (!(t_jump->GetUpward()=="return" && f_jump->GetUpward()=="return")) get_str += end_str + ":\n";
            return get_str;
        }
        else if (ast_state==1) {t_jump->layer = layer; return t_jump->Dump();} // 直接往上传
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) {
            // 传递return信息
            if (t_jump->GetUpward()=="return" && f_jump->GetUpward()=="return") return "return";
            else return "";
        }
        else if (ast_state==1) {t_jump->layer = layer; return t_jump->GetUpward();}  // 传递return信息
        else return "";
    }
    int GetValue() const override {
        if (ast_state==0) return 0;
        else if (ast_state==1) {t_jump->layer = layer; return t_jump->GetValue();}
        else return 0;
    }
};

class StmtAST : public BaseAST {
    public:
    std::string word;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> expression;
    std::unique_ptr<BaseAST> statement;
    int ast_state;  // 0表示return语句, 1表示赋值语句, 2表示单纯求值语句(得扔), 3表示新的block
    // 4表示while语句, 5表示break语句, 6表示continue语句
    int while_place; // while的位置
    std::string GetUpward() const override {
        if (ast_state==0) // return expression->GetUpward(); 
        {
            // 尝试传递return信息用于后续计算
            return "return";
        }
        else if (ast_state==1) {
            lval->fromup = "stmt"; // 表明为表达式左值
            return lval->GetUpward();
        }
        else if (ast_state==2) return ""; // 副作用表达式不传递信息
        else if (ast_state==3) {
            // 出现了新的block, layer+1
            // expression->layer = layer + 1;
            return expression->GetUpward();
        }
        else if (ast_state==4) return expression->GetUpward();
        else if (ast_state==5) /*return "break";*/ return "return";
        else if (ast_state==6) /*return "continue";*/ return "return";
        else return "";
    }
    std::string Dump() const override {
        //std::cout << "StmtAST {";
        //number->Dump();
        //std::cout << "}";
        // std::string koopa_word;
        // if (word=="return") koopa_word = " ret ";
        // else return " ";
        // 收集两条路线的上传结果
        // todo: 此处的entry可能需要改变位置了
        // return "\%entry:\n" + expression->Dump() + koopa_word + expression->GetUpward();
        if (ast_state==0) return expression->Dump() + "  ret " + expression->GetUpward()+"\n";
        else if (ast_state==1) {
            lval->fromup = "stmt"; // 表明为表达式左值
            return expression->Dump() + "  store " + expression->GetUpward() + ", " + lval->GetUpward()+"\n";
        }
        else if (ast_state==2) return expression->Dump();
        else if (ast_state==3) {
            // expression->layer = layer + 1;
            return expression->Dump();
        }
        else if (ast_state==4) {
            // todo: 设计while语句的Dump函数
            std::string get_str = "";
            // entry块的名字
            std::string entry_str = "\%while_entry_" + std::to_string(while_place);
            // body块的名字
            std::string body_str = "\%while_body_" + std::to_string(while_place);
            // end块的名字
            std::string end_str = "\%while_end_" + std::to_string(while_place);
            // place入栈
            while_stack.push(while_place);
            // 跳入while_entry
            get_str += "  jump " + entry_str + "\n\n";
            // while_entry
            get_str += entry_str + ":\n";
            get_str += expression->Dump();
            get_str += "  br " + expression->GetUpward() + ", " + body_str + ", " + end_str + "\n\n";
            // while_body
            get_str += body_str + ":\n";
            get_str += statement->Dump();
            if (statement->GetUpward()=="return");
            else if (statement->GetUpward()=="break");
            else if (statement->GetUpward()=="continue");
            else get_str += "  jump " + entry_str + "\n";
            get_str += "\n";
            // while_end开头
            get_str += end_str + ":\n";
            // place出栈
            while_stack.pop();
            return get_str;
        }
        // break语句生成koopa
        else if (ast_state==5) {
            if (while_stack.empty()) assert(false);
            else {
                int curr_while_place = while_stack.top();
                return "  jump \%while_end_" + std::to_string(curr_while_place) + "\n";
            }
        }
        // continue语句生成koopa
        else if (ast_state==6) {
            if (while_stack.empty()) assert(false);
            else {
                int curr_while_place = while_stack.top();
                return "  jump \%while_entry_" + std::to_string(curr_while_place) + "\n";
            }
        }
        else return "";
    };
    int GetValue() const override {
        if (ast_state==0) return expression->GetValue();
        else if (ast_state==1) {
            lval->fromup = "stmt"; // 表明为表达式左值
            return lval->GetValue();
        }
        else if (ast_state==2) return expression->GetValue();
        else if (ast_state==3) {
            // expression->layer = layer + 1;
            return expression->GetValue();
        }
        else if (ast_state==4) return expression->GetValue();
        else if (ast_state==5) return 0;
        else if (ast_state==6) return 0;
        else return 0;
    }
};

class SideExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> exp;
    int ast_state; // 0为有exp, 1为没有exp
    std::string Dump() const override {
        if(ast_state==0) return exp->Dump();
        else return "";
    }
    std::string GetUpward() const override {
        if(ast_state==0) return exp->GetUpward();
        else return "";
    }
    int GetValue() const override {
        if(ast_state==0) return exp->GetValue();
        else return 0;
    }
};

class ExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> exp;
    // 用ast_state记录ast状态
    // 状态0代表处理一元表达式
    // int ast_state;
    std::string GetUpward() const override {return exp->GetUpward();}
    std::string Dump() const override {return exp->Dump();}
    int GetValue() const override {
        return exp->GetValue();
    }
};

class LOrExpAST :public BaseAST {
    public:
    std::unique_ptr<BaseAST> lor;
    std::unique_ptr<BaseAST> op;
    std::unique_ptr<BaseAST> land;
    // 在状态1下需要申请3个place
    int place1, place2, place3;
    int lor_place; // 分支基本块所使用的编号
    int ast_state;
    std::string Dump() const override {
        // 情况和LAnd差不太多
        // 转换方法
        // 先用2次ne运算, 若为0保持0, 若不为0转换为1
        // 再用or运算, 对0/1做按位与(逻辑与)
        // 需要申请3个place
        if (ast_state==0) {
            if (op->GetUpward()=="||") {
                // 实现短路求值, 修改实现方式
                std::string get_str = "";
                std::string r_block = "\%lor_right_" + std::to_string(lor_place);
                std::string zero_block = "\%lor_zero_" + std::to_string(lor_place);
                std::string one_block = "\%lor_one_" + std::to_string(lor_place);
                std::string successor_block = "\%lor_suc_" + std::to_string(lor_place);
                // 第一个ne运算
                // get_str += lor->Dump()+"  %" + std::to_string(place1) + " = ne 0, "+lor->GetUpward()+"\n";
                // 第二个ne运算
                // get_str += land->Dump()+"  %" + std::to_string(place2) + " = ne 0, "+land->GetUpward()+"\n";
                // 按位或运算
                // get_str += "  %" + std::to_string(place3) + " = or"+" %" + std::to_string(place1)+","+" %" + std::to_string(place2) + "\n";
                // 申存, 用于赋值
                get_str += "  %" + std::to_string(place1) + " = alloc i32\n";
                // 先考虑左值
                get_str += lor->Dump(); // 左值的运算过程
                get_str += "  br " + lor->GetUpward() + ", " + one_block + ", " + r_block + "\n\n";
                // 然后考虑右值
                // 新开基本块
                get_str += r_block + ":\n";
                get_str += land->Dump();
                get_str += "  br " + land->GetUpward() + ", " + one_block + ", " + zero_block + "\n\n";
                // 考虑0后续 请考虑如何给语句标号赋值？
                get_str += zero_block + ":\n";
                // get_str += "  %" + std::to_string(place3) + " = add 0, 1\n";
                get_str += "  store 0,  %" + std::to_string(place1) + "\n";
                get_str += "  jump " + successor_block + "\n\n";
                // 考虑1后续
                get_str += one_block + ":\n";
                // get_str += "  %" + std::to_string(place3) + " = add 0, 0\n";
                get_str += "  store 1, %" + std::to_string(place1) + "\n";
                get_str += "  jump " + successor_block + "\n\n";
                // 考虑后继
                get_str += successor_block + ":\n";
                get_str += "  %" + std::to_string(place3) + " = load" + " %" + std::to_string(place1) + "\n";
                return get_str;
            }
            else return "";
        }
        else if (ast_state==1) return land->Dump();
        else return "";
    };
    std::string GetUpward() const override {
        if (ast_state==0) return "%" + std::to_string(place3);
        else if (ast_state==1) return land->GetUpward();
        else return "";
    };
    int GetValue() const override {
        if (ast_state==0) {
            if (op->GetUpward()=="||") return lor->GetValue() || land->GetValue();
            else return 0;
        }
        else if (ast_state==1) return land->GetValue();
        else return 0;
    }
};

class LOrOpAST : public BaseAST {
    public:
    std::string op;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return op;}
    int GetValue() const override {return 0;}
};

class LAndExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> land;
    std::unique_ptr<BaseAST> op;
    std::unique_ptr<BaseAST> eq;
    // 在状态1下需要申请3个place
    int place1, place2, place3;
    int ast_state;
    int land_place;
    std::string Dump() const override {
        // 请注意, koopa IR只提供了按位与按位或运算, 需要运算的转换
        // 转换方法
        // 先用2次ne运算, 若为0保持0, 若不为0转换为1
        // 再用and运算, 对0/1做按位与(逻辑与)
        // 需要申请3个place
        // 实现短路求值, 修改实现方式
        std::string r_block = "\%land_right_" + std::to_string(land_place);
        std::string zero_block = "\%land_zero_" + std::to_string(land_place);
        std::string one_block = "\%land_one_" + std::to_string(land_place);
        std::string successor_block = "\%land_suc_" + std::to_string(land_place);
        if (ast_state==0) {
            if (op->GetUpward()=="&&") {
                std::string get_str = "";
                /*
                // 第一个ne运算, 包括预备的运算(也许可以短路求值?)
                get_str += land->Dump()+"  %" + std::to_string(place1) + " = ne 0, " + land->GetUpward() + "\n";
                // 第二个ne运算, 包括预备的运算
                get_str += eq->Dump()+"  %" + std::to_string(place2) + " = ne 0, " + eq->GetUpward()+"\n";
                // 做按位与运算
                get_str += " %" + std::to_string(place3) + " = and" +" %" + std::to_string(place1) + "," + " %" + std::to_string(place2) + "\n";
                */
                // 申存, 用于赋值
                get_str += "  %" + std::to_string(place1) + " = alloc i32\n";
                // 考虑左值
                get_str += land->Dump();
                get_str += "  br " + land->GetUpward() + ", " + r_block + ", " + zero_block + "\n\n";
                // 考虑右值
                get_str += r_block + ":\n";
                get_str += eq->Dump();
                get_str += "  br " + eq->GetUpward() + ", " + one_block + ", " + zero_block + "\n\n";
                // 考虑0后续
                get_str += zero_block + ":\n";
                // get_str += " %" + std::to_string(place3) + " = add 0, 0\n";
                get_str += "  store 0, %" + std::to_string(place1) + "\n";
                get_str += "  jump " + successor_block + "\n\n";
                // 考虑1后续
                get_str += one_block + ":\n";
                // get_str += " %" + std::to_string(place3) + " = add 0, 1\n";
                get_str += "  store 1, %" + std::to_string(place1) + "\n";
                get_str += "  jump " + successor_block + "\n\n";
                // 考虑后继
                get_str += successor_block + ":\n";
                get_str += "  %" + std::to_string(place3) + " = load" + "  %" + std::to_string(place1) + "\n";
                return get_str;
            }
            else return "";
        }
        else if (ast_state==1) return eq->Dump();
        else return "";
    };
    std::string GetUpward() const override {
        if (ast_state==0) return "%" + std::to_string(place3);
        else if (ast_state==1) return eq->GetUpward();
        else return "";
    };
    int GetValue() const override {
        if (ast_state==0) {
            if (op->GetUpward()=="&&") return land->GetValue() && eq->GetValue();
            else return 0;
        }
        else if (ast_state==1) return eq->GetValue();
        else return 0;
    }
};

class LAndOpAST : public BaseAST {
    public:
    std::string op;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return op;}
    int GetValue() const override {return 0;}
};

class EqExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> eq;
    std::unique_ptr<BaseAST> op;
    std::unique_ptr<BaseAST> rel;
    int place;
    int ast_state;
    std::string Dump() const override {
        if (ast_state==0) {
            if (op->GetUpward()=="==") return eq->Dump()+rel->Dump()+"  %" + std::to_string(place) + " = eq " +eq->GetUpward()+", "+rel->GetUpward()+"\n";
            else if (op->GetUpward()=="!=") return eq->Dump()+rel->Dump()+"  %" + std::to_string(place) + " = ne " +eq->GetUpward()+", "+rel->GetUpward()+"\n";
            else return "";
        }
        else if (ast_state==1) return rel->Dump();
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) return "%" + std::to_string(place);
        else if (ast_state==1) return rel->GetUpward();
        else return "";
    }
    int GetValue() const override {
        if (ast_state==0) {
            if (op->GetUpward()=="==") return eq->GetValue() == rel->GetValue();
            else if (op->GetUpward()=="!=") return eq->GetValue() != rel->GetValue(); 
            else return 0;
        }
        else if (ast_state==1) return rel->GetValue();
        else return 0;
    }
};

class EqOpAST : public BaseAST {
    public:
    std::string op;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return op;}
    int GetValue() const override {return 0;}
};

class RelExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> rel;
    std::unique_ptr<BaseAST> op;
    std::unique_ptr<BaseAST> add;
    int place;
    int ast_state;
    std::string Dump() const override {
        if (ast_state==0) {
            if (op->GetUpward()==">") return rel->Dump()+add->Dump()+"  %" + std::to_string(place) + " = gt " +rel->GetUpward()+", "+add->GetUpward()+"\n";
            else if (op->GetUpward()=="<") return rel->Dump()+add->Dump()+"  %" + std::to_string(place) + " = lt " +rel->GetUpward()+", "+add->GetUpward()+"\n";
            else if (op->GetUpward()==">=") return rel->Dump()+add->Dump()+"  %" + std::to_string(place) + " = ge " +rel->GetUpward()+", "+add->GetUpward()+"\n";
            else if (op->GetUpward()=="<=") return rel->Dump()+add->Dump()+"  %" + std::to_string(place) + " = le " +rel->GetUpward()+", "+add->GetUpward()+"\n";
            else return "";
        }
        else if (ast_state==1) return add->Dump();
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) return "%" + std::to_string(place);
        else if (ast_state==1) return add->GetUpward();
        else return "";
    }
    int GetValue() const override {
        if (ast_state==0) {
            if (op->GetUpward()==">") return rel->GetValue() > add->GetValue();
            else if (op->GetUpward()=="<") return rel->GetValue() < add->GetValue();
            else if (op->GetUpward()==">=") return rel->GetValue() >= add->GetValue();
            else if (op->GetUpward()=="<=") return rel->GetValue() <= add->GetValue();
            else return 0;
        }
        else if (ast_state==1) return add->GetValue();
        else return 0;
    }
};

class RelOpAST : public BaseAST {
    public:
    std::string op;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return op;}
    int GetValue() const override {return 0;}
};

class AddExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> add;
    std::string op;
    std::unique_ptr<BaseAST> mul;
    int ast_state;
    int place;
    std::string Dump() const override {
        if (ast_state==0) {
            if (op=="+") return add->Dump()+mul->Dump()+"  %" + std::to_string(place) + " = add " + add->GetUpward() + ", " + mul->GetUpward() +"\n";
            else if (op=="-") return add->Dump()+mul->Dump()+"  %" + std::to_string(place) + " = sub " + add->GetUpward() + ", " + mul->GetUpward() +"\n";
            else return "";
        }
        else if (ast_state==1) return mul->Dump();
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) return "%" + std::to_string(place);
        else if (ast_state==1) return mul->GetUpward();
        else return "";
    }
    int GetValue() const override {
        if (ast_state==0) {
            if (op=="+") return add->GetValue() + mul->GetValue();
            else if (op=="-") return add->GetValue() - mul->GetValue();
            else return 0;
        }
        else if (ast_state==1) return mul->GetValue();
        else return 0;
    }
};

class MulExpAST :public BaseAST {
    public:
    std::unique_ptr<BaseAST> mul;
    std::string op;
    std::unique_ptr<BaseAST> unary;
    // 设置表达式状态
    // 状态0为乘法表达式递归
    // 状态1为进入一元表达式
    int ast_state;
    // 占用的标识符号
    int place;
    std::string Dump() const override {
        if (ast_state==0) {
            if (op=="*") return mul->Dump() + unary->Dump() + "  %" + std::to_string(place) + " = mul " + mul->GetUpward() + ", " + unary->GetUpward() + "\n";
            else if (op=="/") return mul->Dump() + unary->Dump() + "  %" + std::to_string(place) + " = div " + mul->GetUpward() + ", " + unary->GetUpward() + "\n";
            else if (op=="%") return mul->Dump() + unary->Dump() + "  %" + std::to_string(place) + " = mod " + mul->GetUpward() + ", "+ unary->GetUpward() + "\n";
            else return "";
        }
        else if (ast_state==1) return unary->Dump();
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) return "%" + std::to_string(place);
        else if (ast_state==1) return unary->GetUpward();
        else return "";
    }
    int GetValue() const override {
        if (ast_state==0) {
            if (op=="*") return mul->GetValue() * unary->GetValue();
            else if (op=="/") return mul->GetValue() / unary->GetValue();
            else if (op=="%") return mul->GetValue() % unary->GetValue();
            else return 0;
        }
        else if (ast_state==1) return unary->GetValue();
        else return 0;
    }
};

class UnaryExpAST : public BaseAST {
    public:
    // 存放一元运算符
    // std::unique_ptr<BaseAST> unary_op;
    std::string op;
    // 存放一元表达式或者基础表达式
    std::unique_ptr<BaseAST> son_tree;
    // 设置表达式状态
    // 状态0为一元表达式递归
    // 状态1为进入基础表达式
    // 状态2为有参数function call
    // 状态3为无参数function call
    int ast_state;
    // 占用的标识符号
    int place;
    // 调用的函数标号
    std::string ident;
    // 设置表达式要用到的koopa标识符
    std::string GetUpward() const override {
        if (ast_state==0) {
            // return "%" + std::to_string(place);
            // 若为+则沿用之前传来的GetUpward
            if (op=="+") return son_tree->GetUpward();
            // 若为-!则新建地址向上传
            else if(op=="-"||op=="!") return "%" + std::to_string(place);
            else return "";
        }
        else if (ast_state==1) return son_tree->GetUpward();
        else if (ast_state==2 || ast_state==3) {
            varinfo using_func = SearchMap();
            if (using_func.ret_type==1) return "\%" + std::to_string(place);
            else return "";
        }
        else return "";
    };
    std::string Dump() const override {
        if (ast_state==0) {
            // todo: 如何调整?
            // return son_tree->Dump() + "\n " + "%" + std::to_string(place) + " = " + unary_op->Dump() + son_tree->GetUpward();
            // 若为+则保持结果字符串不动
            if (op=="+") return son_tree->Dump();
            // 若为-!则添加式子
            else if(op=="-") {
                return son_tree->Dump() + "  %" + std::to_string(place) + " = sub 0, " + son_tree->GetUpward() + "\n";
            }
            else if (op=="!") {
                return son_tree->Dump() + "  %" + std::to_string(place) + " = eq 0, " + son_tree->GetUpward() + "\n";
            }
            else return "";
        }
        else if (ast_state==1) {
            return son_tree->Dump();
        }
        else if (ast_state==2) {
            std::string get_str;
            get_str += son_tree->Dump();
            varinfo using_func = SearchMap();
            // call指令
            if (using_func.ret_type==1) get_str += "  \%" + std::to_string(place) + " = call " + using_func.name + "(" + son_tree->GetUpward() + ")\n";
            else get_str += "  call " + using_func.name + "(" + son_tree->GetUpward() + ")\n";
            // 增加函数的调用记录
            if (sysy_use_time.find(ident)!=sysy_use_time.end()) sysy_use_time[ident]++;
            return get_str;
        }
        else if (ast_state==3) {
            std::string get_str;
            varinfo using_func = SearchMap();
            // call指令
            if (using_func.ret_type==1) get_str += "  \%" + std::to_string(place) + " = call " + using_func.name + "()\n";
            else get_str += "  call " + using_func.name + "()\n";
            // 增加函数的调用记录
            if (sysy_use_time.find(ident)!=sysy_use_time.end()) sysy_use_time[ident]++;
            return get_str;
        }
        else return " ";
    };
    int GetValue() const override {
        if (ast_state==0) {
            if (op=="+") return son_tree->GetValue();
            else if (op=="-") return -(son_tree->GetValue());
            else if (op=="!") return !(son_tree->GetValue());
            else return 0;
        }
        else if (ast_state==1) return son_tree->GetValue();
        else return 0;
    }
    // 查找函数符号表函数
    varinfo SearchMap() const {
        varinfo found;
        for (auto it=var_map.begin();it!=var_map.end();++it) {
            if (it->second.ident==ident && it->second.vartype==2) {
                found = it->second;
                return found;
            }
        }
        std::cout << "function not found!";
        assert(false);
    }
};

class PrimaryExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> son_tree;
    int ast_state;  // 0表示为number or exp, 1表示lval
    // 2表示function call
    // 将子树的upward直接上传
    std::string GetUpward() const override {
        if (ast_state==0) return son_tree->GetUpward();
        else if (ast_state==1) {
            son_tree->fromup = "exp"; // 表明为表达式右值
            return son_tree->GetUpward();
        }
        else return "";
    };
    std::string Dump() const override {
        // 保留子孙的dump结果不变
        if (ast_state==0) return son_tree->Dump();
        else if (ast_state==1) {
            son_tree->fromup = "exp"; // 表明为表达式右值
            return son_tree->Dump();
        }
        else return "";
    };
    int GetValue() const override {
        if (ast_state==0) return son_tree->GetValue();
        else if (ast_state==1) {
            son_tree->fromup = "exp"; // 表明为表达式右值
            return son_tree->GetValue();
        }
        else return 0;
    }
};

class FuncRParamsAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> p_list;
    int ast_state; // 0为多个参数, 1为单个参数
    std::string Dump() const override {
        if (ast_state==0) return exp->Dump() + p_list->Dump();
        else if (ast_state==1) return exp->Dump();
        else return "";
    }
    std::string GetUpward() const override {
        if (ast_state==0) return exp->GetUpward() + ", " + p_list->GetUpward();
        else if (ast_state==1) return exp->GetUpward();
        else return "";
    }
    int GetValue() const override {return 0;}
};

// 这个也是必要的！这里是文档的typo！助教你没有心！
class NumberAST : public BaseAST {
    public:
    int int_const; //这里用整数形式
    // upward = std::to_string(int_const);
    std::string GetUpward() const override {
        return std::to_string(int_const);
    };
    std::string Dump() const override {
        //std::cout << "NumberAST {";
        //std::cout << int_const;
        //std::cout << "}";
        //std::string const_str = std::to_string(int_const);
        //return const_str;
        // 利用upward参数传递值, 这一函数废弃
        // 将数作为upward值上传
        return "";
    };
    int GetValue() const override {return int_const;}
};

# endif
