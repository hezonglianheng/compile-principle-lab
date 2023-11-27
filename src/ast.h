# ifndef AST_H
# define AST_H
# pragma once //只引用一次
// todo: IR生成代码(试图输出string)

# include <memory>
# include <string>
# include <iostream>

class BaseAST {
    public:
    // std::string upward = "";
    virtual ~BaseAST() = default;
    // Dump()是用来输出的函数
    //virtual void Dump() const = 0;
    virtual std::string Dump() const = 0;
    // GetUpward是收集数据向上传的函数
    // 有的数据需要上传使用, 用这个函数完成
    virtual std::string GetUpward() const = 0;
    // virtual int Place() const = 0;
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
    return "fun @" + ident + "(): " + func_type->Dump() + block->Dump();
  }
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
};

class BlockAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> stmt;
    std::string GetUpward() const override {return stmt->GetUpward();}
    std::string Dump() const override{
        //std::cout << "BlockAST {";
        //stmt->Dump();
        //std::cout << "}";
        return "{\n" + stmt->Dump() + "\n}";
    };
};

class StmtAST : public BaseAST {
    public:
    std::string word;
    std::unique_ptr<BaseAST> expression;
    std::string GetUpward() const override {return expression->GetUpward();}
    std::string Dump() const override {
        //std::cout << "StmtAST {";
        //number->Dump();
        //std::cout << "}";
        std::string koopa_word;
        if (word=="return") koopa_word = " ret ";
        else return " ";
        // 收集两条路线的上传结果
        return "\%entry:\n" + expression->Dump() + koopa_word + expression->GetUpward();
    };
};

class ExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> exp;
    // 用ast_state记录ast状态
    // 状态0代表处理一元表达式
    int ast_state;
    std::string GetUpward() const override {return exp->GetUpward();}
    std::string Dump() const override {
        return exp->Dump();
    };
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
                get_str += lor->Dump()+" %" + std::to_string(place1) + " = ne 0, "+lor->GetUpward()+"\n";
                // 第二个ne运算
                get_str += land->Dump()+" %" + std::to_string(place2) + " = ne 0, "+land->GetUpward()+"\n";
                // 按位或运算
                get_str += " %" + std::to_string(place3) + " = or"+" %" + std::to_string(place1)+","+" %" + std::to_string(place2) + "\n";
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
};

class LOrOpAST : public BaseAST {
    public:
    std::string op;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return op;}
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
                get_str += land->Dump()+" %" + std::to_string(place1) + " = ne 0, " + land->GetUpward() + "\n";
                // 第二个ne运算, 包括预备的运算
                get_str += eq->Dump()+" %" + std::to_string(place2) + " = ne 0, " + eq->GetUpward()+"\n";
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
};

class LAndOpAST : public BaseAST {
    public:
    std::string op;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return op;}
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
            if (op->GetUpward()=="==") return eq->Dump()+rel->Dump()+" %" + std::to_string(place) + " = eq " +eq->GetUpward()+", "+rel->GetUpward()+"\n";
            else if (op->GetUpward()=="!=") return eq->Dump()+rel->Dump()+" %" + std::to_string(place) + " = ne " +eq->GetUpward()+", "+rel->GetUpward()+"\n";
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
};

class EqOpAST : public BaseAST {
    public:
    std::string op;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return op;}
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
            if (op->GetUpward()==">") return rel->Dump()+add->Dump()+" %" + std::to_string(place) + " = gt " +rel->GetUpward()+", "+add->GetUpward()+"\n";
            else if (op->GetUpward()=="<") return rel->Dump()+add->Dump()+" %" + std::to_string(place) + " = lt " +rel->GetUpward()+", "+add->GetUpward()+"\n";
            else if (op->GetUpward()==">=") return rel->Dump()+add->Dump()+" %" + std::to_string(place) + " = ge " +rel->GetUpward()+", "+add->GetUpward()+"\n";
            else if (op->GetUpward()=="<=") return rel->Dump()+add->Dump()+" %" + std::to_string(place) + " = le " +rel->GetUpward()+", "+add->GetUpward()+"\n";
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
};

class RelOpAST : public BaseAST {
    public:
    std::string op;
    std::string Dump() const override {return "";}
    std::string GetUpward() const override {return op;}
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
            if (op=="+") return add->Dump()+mul->Dump()+" %" + std::to_string(place) + " = add " + add->GetUpward() + ", " + mul->GetUpward() +"\n";
            else if (op=="-") return add->Dump()+mul->Dump()+" %" + std::to_string(place) + " = sub " + add->GetUpward() + ", " + mul->GetUpward() +"\n";
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
            if (op=="*") return mul->Dump() + unary->Dump() + " %" + std::to_string(place) + " = mul " + mul->GetUpward() + ", " + unary->GetUpward() + "\n";
            else if (op=="/") return mul->Dump() + unary->Dump() + " %" + std::to_string(place) + " = div " + mul->GetUpward() + ", " + unary->GetUpward() + "\n";
            else if (op=="%") return mul->Dump() + unary->Dump() + " %" + std::to_string(place) + " = mod " + mul->GetUpward() + ", "+ unary->GetUpward() + "\n";
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
                return son_tree->Dump() + " %" + std::to_string(place) + " = sub 0, " + son_tree->GetUpward() + "\n";
            }
            else if (op=="!") {
                return son_tree->Dump() + " %" + std::to_string(place) + " = eq 0, " + son_tree->GetUpward() + "\n";
            }
            else return "";
        }
        else if (ast_state==1) {
            return son_tree->Dump();
        }
        else return " ";
    };
};

class PrimaryExpAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> son_tree;
    // 将子树的upward直接上传
    // upward = son_tree->upward;
    std::string GetUpward() const override {
        return son_tree->GetUpward();
    };
    std::string Dump() const override {
        // 保留子孙的dump结果不变
        return son_tree->Dump();
    };
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
};

# endif
