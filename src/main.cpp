#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <string.h>
#include <cstdint> // 提供int32_t类型支持的库
#include "ast.h"
#include "koopa.h" // 引用koopa头文件

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
// 尝试使用yyout文件, 失败，你不需要使用yyout文件
// extern FILE *yyout;
extern int yyparse(unique_ptr<BaseAST> &ast);

// 声明Visit函数的用法
string Visit(const koopa_raw_program_t &program);
string Visit(const koopa_raw_slice_t &slice);
string Visit(const koopa_raw_function_t &func);
string Visit(const koopa_raw_basic_block_t &bb);
string Visit(const koopa_raw_value_t &value);
string Visit(const koopa_raw_return_t inst);
string Visit(const koopa_raw_integer_t inst);

int main(int argc, const char *argv[]) {
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  // 用fstream来写字符串
  std::ofstream out_file (output);

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  // 输出解析得到的 AST, 其实就是个字符串
  //ast->Dump();
  // ofstream to_koopa(output);
  //cout << ast->Dump();
  //cout << endl;
  // 将转换得来的Koopa IR写入文件中备用
  // 将换来的koopa字符串形式存储起来
  std::string koopa_string = ast->Dump();
  // 如果为koopa模式时将其输出到.koopa文件中保存起来. 请注意C++中的字符串比较方法
  if (strcmp(mode, "-koopa")==0) out_file << koopa_string << endl;
  // 如果为riscv模式或perf模式待处理
  if (strcmp(mode, "-riscv")==0 || strcmp(mode, "-perf")==0) {
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    // 注意.c_str()的用法, 它将string转换为char
    koopa_error_code_t ret = koopa_parse_from_string(koopa_string.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 搜索输出文件得到汇编代码字符串
    string riscv_string = Visit(raw);
    // 写入输出文件
    out_file << riscv_string << endl;

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
  };
  return 0;
}

// 访问 raw program
string Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // todo: 遍历全局变量得到一些信息
  // 遍历全部函数, 得到信息
  string program_str = "  .text\n";
  for (size_t i = 0; i < program.funcs.len; ++i){
    auto ptr = program.funcs.buffer[i];
    // 断言是function
    assert(program.funcs.kind==KOOPA_RSIK_FUNCTION);
    // 重新解释指针的类型
    auto new_ptr = reinterpret_cast<koopa_raw_function_t>(ptr);
    string func_name = new_ptr->name;
    program_str += "  .global " + func_name.substr(1, func_name.length()) + "\n";
  }
  // 访问所有全局变量
  string glob_var_str = Visit(program.values);
  // 访问所有函数
  string funcs_str = Visit(program.funcs);
  program_str = program_str + glob_var_str;
  program_str = program_str + funcs_str;
  return program_str;
}

// 访问 raw slice
string Visit(const koopa_raw_slice_t &slice) {
  string slice_str = "";
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        slice_str = slice_str + Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        slice_str = slice_str + Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        slice_str = slice_str + Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
  return slice_str;
}

// 访问函数
string Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  // 获得函数的名字作为标识符
  string func_name = func->name;
  // 访问所有基本块
  string blocks_str = Visit(func->bbs);
  // 返回函数的riscv表示形式
  return func_name.substr(1, func_name.length()) + ":\n" + blocks_str;
}

// 访问基本块
string Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令, 获得其字符表示形式
  string inst_str = Visit(bb->insts);
  // 返回之
  return inst_str;
}

// 访问指令
string Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  string get_str;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令并获得返回的字符串
      get_str = Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令并获得返回的字符串
      get_str = Visit(kind.data.integer);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
  return get_str;
}

string Visit(const koopa_raw_return_t inst){
  // 确定返回值指令并访问之
  koopa_raw_value_t ret_value = inst.value;
  string ret_val_str = Visit(ret_value);
  // 暂时在此处实现将立即数0放入a0存储器的操作
  // 返回riscv return指令的字符串形式
  return "  li a0, " + ret_val_str + "\n  ret";
}

string Visit(const koopa_raw_integer_t inst){
  // 获得整数的值
  int32_t int_val = inst.value;
  // 尝试转为字符串
  string val_str = to_string(int_val);
  // 返回获得值的字符串形式
  return val_str;
}