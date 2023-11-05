# ifndef AST_H
# define AST_H
# pragma once //只引用一次

# include <memory>
# include <string>
# include <iostream>

class BaseAST {
    public:
    virtual ~BaseAST() = default;
    // Dump()是用来输出的函数
    virtual void Dump() const = 0;
};

// CompUnitAST 是 BaseAST
class CompUnitAST: public BaseAST {
    public:
    std::unique_ptr<BaseAST> func_def;
    void Dump() const override{
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
    };
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;
  void Dump() const override {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
  }
};

// 这个是必要的
class FuncTypeAST : public BaseAST {
    public:
    std::string func_type;
    void Dump() const override {
        std::cout << "FuncTypeAST {";
        std::cout << func_type;
        std::cout << "}";
    };
};

class BlockAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override{
        std::cout << "BlockAST {";
        stmt->Dump();
        std::cout << "}";
    };
};

class StmtAST : public BaseAST {
    public:
    std::unique_ptr<BaseAST> number;
    void Dump() const override {
        std::cout << "StmtAST {";
        number->Dump();
        std::cout << "}";
    };
};

// 这个也是必要的！这里是文档的typo！助教你没有心！
class NumberAST : public BaseAST {
    public:
    int int_const; //这里用整数形式
    void Dump() const override {
        std::cout << "NumberAST {";
        std::cout << int_const;
        std::cout << "}";
    };
};

# endif
