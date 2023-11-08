# ifndef AST_H
# define AST_H
# pragma once //只引用一次
// todo: IR生成代码(试图输出string)

# include <memory>
# include <string>
# include <iostream>

class BaseAST {
    public:
    virtual ~BaseAST() = default;
    // Dump()是用来输出的函数
    //virtual void Dump() const = 0;
    virtual std::string Dump() const = 0;
};

// CompUnitAST 是 BaseAST
class CompUnitAST: public BaseAST {
    public:
    std::unique_ptr<BaseAST> func_def;
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
    std::unique_ptr<BaseAST> number;
    std::string Dump() const override {
        //std::cout << "StmtAST {";
        //number->Dump();
        //std::cout << "}";
        std::string koopa_word;
        if (word=="return") koopa_word = "ret ";
        else return " ";
        return "\%entry:\n  " + koopa_word + number->Dump();
    };
};

// 这个也是必要的！这里是文档的typo！助教你没有心！
class NumberAST : public BaseAST {
    public:
    int int_const; //这里用整数形式
    std::string Dump() const override {
        //std::cout << "NumberAST {";
        //std::cout << int_const;
        //std::cout << "}";
        std::string const_str = std::to_string(int_const);
        return const_str;
    };
};

# endif
