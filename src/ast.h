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
        if (word=="return") koopa_word = "ret ";
        else return " ";
        // 收集两条路线的上传结果
        return "\%entry:\n  " + expression->Dump() + "\n " + koopa_word + expression->GetUpward();
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
        if (ast_state==0) {
            return exp->Dump();
        }
        else return " ";
    };
};

class UnaryExpAST : public BaseAST {
    public:
    // 存放一元运算符
    std::unique_ptr<BaseAST> unary_op;
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
            if (unary_op->GetUpward()=="+") return son_tree->GetUpward();
            // 若为-!则新建地址向上传
            else if(unary_op->GetUpward()=="-"||unary_op->GetUpward()=="!") return "%" + std::to_string(place);
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
            if (unary_op->GetUpward()=="+") return son_tree->Dump();
            // 若为-!则添加式子
            else if(unary_op->GetUpward()=="-"||unary_op->GetUpward()=="!") {
                return son_tree->Dump() + "\n " + "%" + std::to_string(place) + " = " + unary_op->Dump() + son_tree->GetUpward();
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
