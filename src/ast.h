# ifndef AST_H
# define AST_H
# pragma once //只引用一次
// todo: IR生成代码(试图输出string)

# include <memory>
# include <string>
# include <iostream>
# include <unordered_map> // 记录const情况用
# include <iostream>
# include <assert.h>
// # include <vector> // 接不定长参数用的

// 尝试定义新的类型, 用于承接变量的信息
struct varinfo {
    int vartype; // 0为const, 1为var
    std::string ident;  // 变量标号
    int layer; // 表明ident所属的层级
    int value; // const的计算值
    std::string name;  // 这个是变量名称 这里可能有点问题. 先写着, 报错再说
};

// 记录const及其值的map
extern std::unordered_map<std::string, varinfo> var_map;

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
    // GetUpValue是收集数字向上传的函数
    // 目前用于编译期间计算
    virtual int GetValue() const = 0;
};

// CompUnitAST 是 BaseAST
class CompUnitAST: public BaseAST {
    public:
    std::unique_ptr<BaseAST> func_def;
    std::string GetUpward() const override {return "";}
    std::string Dump() const override{
    // std::cout << "CompUnitAST { ";
    return func_def->Dump();
    // std::cout << " }";
    };
    int GetValue() const override {return func_def->GetValue();}
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  std::string GetUpward() const override {return "";}
  std::string Dump() const override {
    //std::cout << "FuncDefAST { ";
    //func_type->Dump();
    //std::cout << ", " << ident << ", ";
    //block->Dump();
    //std::cout << " }";
    // layer值向下传递
    block->layer = layer;
    return "fun @" + ident + "(): " + func_type->Dump() + "{\n\%entry:\n" + block->Dump() + "}\n";
  }
  int GetValue() const override {block->layer = layer; return block->GetValue();}
};

// 这个是必要的
class FuncTypeAST : public BaseAST {
    public:
    std::string func_type;
    std::string GetUpward() const override {return "";}
    std::string Dump() const override {
        //std::cout << "FuncTypeAST {";
        //std::cout << func_type;
        //std::cout << "}";
        if (func_type == "int") {return "i32";}
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
        //std::cout << "BlockAST {";
        //stmt->Dump();
        //std::cout << "}";
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
            return blocklist->Dump() + item->Dump();
        }
        else if (ast_state==1) return "";
        else return "";
    }
    std::string GetUpward() const override {return "";}
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
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<BaseAST> mylist;
    std::string Dump() const override {
        mylist->layer = layer;
        mylist->fromup = btype->GetUpward();
        return mylist->Dump();
    }
    std::string GetUpward() const override {return "";}
    int GetValue() const override {mylist->layer = layer; return mylist->GetValue();}
};

class VarDeclAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<BaseAST> mylist;
    std::string Dump() const override {
        mylist->layer = layer;
        mylist->fromup = btype->GetUpward();
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
    varinfo IdentSearch() const {
        varinfo curr_info;
        int check_layer = -1;
        for (auto it=var_map.begin();it!=var_map.end();++it) {
            if (it->second.ident==ident) {
                if (it->second.layer>check_layer) {
                    curr_info = it->second;
                    check_layer = it->second.layer;
                }
            }
        }
        if (check_layer<0) {std::cout << ident << " not found!"; return curr_info;} // 找不到就报错
        else return curr_info;
    }
    std::string Dump() const override {
        /*
        if (var_map.find(ident)==var_map.end()) return "";
        else {
            if (var_map[ident].vartype==0) return "";
            else if (var_map[ident].vartype==1) {
                if (fromup=="stmt") return "";
                else if (fromup=="exp") return " %"+std::to_string(place)+" = load "+var_map[ident].name+"\n";
                else return "";
            }
            else return "";
        }
        */
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
        // 把值从map中取出来应该放在这个位置
        // if (const_map.find(ident)==const_map.end()) std::cout << "not found!";
        // else std::cout << "found!";
        // return std::to_string(var_map[ident]);
        /*
        if (var_map.find(ident)==var_map.end()) return "";
        else {
            if (var_map[ident].vartype==0) return std::to_string(var_map[ident].value);
            else if (var_map[ident].vartype==1) {
                if (fromup=="stmt") return var_map[ident].name;
                else if (fromup=="exp") return "%"+std::to_string(place); 
                else return "";
            }
            else return "";
        }
        */
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
        // return var_map[ident];
        /*
        if (var_map.find(ident)==var_map.end()) return 0;
        else {
            if (var_map[ident].vartype==0) return var_map[ident].value;
            else if (var_map[ident].vartype==1) return 0;
            else return 0;
        }
        */
        varinfo found = IdentSearch();
        if (found.vartype==0) return found.value;
        else if (found.vartype==1) return 0;
        else return 0;
    }
};

class StmtAST : public BaseAST {
    public:
    std::string word;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> expression;
    int ast_state;  // 0表示return语句, 1表示赋值语句, 2表示单纯求值语句(得扔), 3表示新的block
    std::string GetUpward() const override {
        if (ast_state==0) return expression->GetUpward();
        else if (ast_state==1) {
            lval->fromup = "stmt"; // 表明为表达式左值
            return lval->GetUpward();
        }
        else if (ast_state==2) return expression->GetUpward();
        else if (ast_state==3) {
            // 出现了新的block, layer+1
            expression->layer = layer + 1;
            return expression->GetUpward();
        }
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
            expression->layer = layer + 1;
            return expression->Dump();
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
            expression->layer = layer + 1;
            return expression->GetValue();
        }
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
    int ast_state;
    std::string Dump() const override {
        // 情况和LAnd差不太多
        // 转换方法
        // 先用2次ne运算, 若为0保持0, 若不为0转换为1
        // 再用or运算, 对0/1做按位与(逻辑与)
        // 需要申请3个place
        if (ast_state==0) {
            if (op->GetUpward()=="||") {
                std::string get_str = "";
                // 第一个ne运算
                get_str += lor->Dump()+"  %" + std::to_string(place1) + " = ne 0, "+lor->GetUpward()+"\n";
                // 第二个ne运算
                get_str += land->Dump()+"  %" + std::to_string(place2) + " = ne 0, "+land->GetUpward()+"\n";
                // 按位或运算
                get_str += "  %" + std::to_string(place3) + " = or"+" %" + std::to_string(place1)+","+" %" + std::to_string(place2) + "\n";
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
    std::string Dump() const override {
        // 请注意, koopa IR只提供了按位与按位或运算, 需要运算的转换
        // 转换方法
        // 先用2次ne运算, 若为0保持0, 若不为0转换为1
        // 再用and运算, 对0/1做按位与(逻辑与)
        // 需要申请3个place
        if (ast_state==0) {
            if (op->GetUpward()=="&&") {
                std::string get_str = "";
                // 第一个ne运算, 包括预备的运算(也许可以短路求值?)
                get_str += land->Dump()+"  %" + std::to_string(place1) + " = ne 0, " + land->GetUpward() + "\n";
                // 第二个ne运算, 包括预备的运算
                get_str += eq->Dump()+"  %" + std::to_string(place2) + " = ne 0, " + eq->GetUpward()+"\n";
                // 做按位与运算
                get_str += " %" + std::to_string(place3) + " = and" +" %" + std::to_string(place1) + "," + " %" + std::to_string(place2) + "\n";
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
    int ast_state;
    // 占用的标识符号
    int place;
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
};

class PrimaryExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> son_tree;
    int ast_state;  // 0表示为number or exp, 1表示lval
    // 将子树的upward直接上传
    // upward = son_tree->upward;
    std::string GetUpward() const override {
        // return son_tree->GetUpward();
        if (ast_state==0) return son_tree->GetUpward();
        else if (ast_state==1) {
            son_tree->fromup = "exp"; // 表明为表达式右值
            return son_tree->GetUpward();
        }
        else return "";
    };
    std::string Dump() const override {
        // 保留子孙的dump结果不变
        // return son_tree->Dump();
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

// 添加AST节点UnaryOpAST用来翻译一元运算符
/*尝试放弃
class UnaryOpAST : public BaseAST {
    public:
    std::string str_const;
    std::string GetUpward() const override {return str_const;}
    std::string Dump() const override {
        if (str_const=="+") {return "";}
        else if (str_const=="-") {return "sub 0, ";}
        else if (str_const=="!") {return "eq 0, ";}
        else {return "";}
    };
};
*/

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
