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
#include <unordered_map> // 用unordered_map类型存已经分配了的内存
#include <cmath> // 引入绝对值函数

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

// 尝试在此处定义exp_counter
// 还是不要在这里定义了, 放到y文件去吧
//int exp_counter = 0;

// 保存常量名称-值对, 用于编译期间求值
unordered_map<std::string, varinfo> var_map;

// 声明0号寄存器名称
const string ZERO_REGISTER = "x0";
string registers[15] = {"t0", "t1", "t2", "t3", "t4", "t5", 
"t6", "a1", "a2", "a3", "a4", "a5", "a6", "a0"};

// 存放已经分配好的register和指针的关系
// 暂时废除
// unordered_map<koopa_raw_value_t, string> register_distribution;
// 使用的register的全局计数器
// 暂时废除
// int register_counter = 0;
// 存放函数中各指针与栈位置的关系
// 目前的设计是只支持一个函数使用, 用完就clear
unordered_map<koopa_raw_value_t, int> stack_place;
// 一样的, 表明目前还没有用到的位置在哪里
// 只支持一个函数使用, 用完就归零
int unused_tag = 0;
// 表明函数的大小
// 仅支持一个函数使用, 用完就归零
int function_size = 0;

// 声明Visit函数的用法
string Visit(const koopa_raw_program_t &program);
string Visit(const koopa_raw_slice_t &slice);
string Visit(const koopa_raw_function_t &func);
string Visit(const koopa_raw_basic_block_t &bb);
string Visit(const koopa_raw_value_t &value);
string Visit(const koopa_raw_return_t inst);
string Visit(const koopa_raw_integer_t inst);
// 增加声明识别二元运算符的函数
string Visit(const koopa_raw_binary_t inst, const koopa_raw_value_t &value);
// 增加访问load store指令的Visit函数
string Visit(const koopa_raw_load_t load, const koopa_raw_value_t &value);
string Visit(const koopa_raw_store_t store);
// 增加访问br(branch)分支指令的函数
string Visit(const koopa_raw_branch_t branch);
// 增加访问jump跳转指令的函数
string Visit(const koopa_raw_jump_t jump);
// 增加计算函数所用栈空间和基本块所用栈空间的函数
int CountStackSpace(const koopa_raw_slice_t &slice);
// 增加计算类型占据栈空间的函数
// 通过类型判断需要给多大存储空间的函数
int CountStackSpace(const koopa_raw_type_t type);
string MoveFuncPointer(int func_size);

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
  // get_str用于接住要输出的字符串
  string get_str = func_name.substr(1, func_name.length()) + ":\n";
  // 获知需要分配的栈空间情况
  int use_space = CountStackSpace(func->bbs);
  use_space = use_space%16==0 ? use_space : (use_space / 16 + 1)*16;
  function_size = use_space; // 将函数大小记录到全局
  get_str += MoveFuncPointer(-use_space);
  // 访问所有基本块
  string blocks_str = Visit(func->bbs);
  // 检查基本块情况
  /*
  if (blocks_str.size() >= 3) {
    if (blocks_str.substr(blocks_str.size()-3, 3)=="ret"){
      get_str += blocks_str.substr(0, blocks_str.size()-3);
      // get_str += "addi sp, sp, " + to_string(use_space) + "\n";
      if (use_space==0);  // 0的场合不用栈
      else if (use_space>=2048) {
        get_str += "  li   " + registers[0] + ", " + to_string(use_space) + "\n";
        get_str += "  add  sp, sp, " + registers[0] + "\n";
      }
      else get_str += "addi sp, sp, " + to_string(use_space) + "\n";
      get_str += "  ret\n";
    }
    else {
      cout << "no return!";
      assert(false);
    }
  }
  else {
  // 否则直接报错
    cout << "string too short!";
    assert(false);
  }*/
  get_str += blocks_str;
  // 返回之前先把栈中标识符记录报销了
  stack_place.clear();
  // 还得报销一个未使用记录位置
  unused_tag = 0;
  // 还得报销一个函数大小
  function_size = 0;
  // 返回函数的riscv表示形式
  return get_str;
  // return func_name.substr(1, func_name.length()) + ":\n" + blocks_str;
}

// 访问基本块
string Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  string get_str = "";
  // 主要需要输出块的名称
  string block_name = bb->name;
  if (block_name!="\%entry") get_str += block_name.substr(1, block_name.length()) + ":\n";
  // 访问所有指令, 获得其字符表示形式
  get_str += Visit(bb->insts);
  // 返回之
  return get_str;
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
    case KOOPA_RVT_BINARY:
      // 11-22: 添加类型 二元运算符
      // 访问 binary 指令并获得返回的字符串
      get_str = Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_ALLOC:
      // 11-30: 添加类型 alloc
      // alloc类型什么也不做(不行, 得进存储)
      stack_place[value] = unused_tag;
      unused_tag += 4;
      break;
    case KOOPA_RVT_LOAD:
      // 11-30: 添加类型 load
      get_str = Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      // 11-30: 添加类型 store
      get_str = Visit(kind.data.store);
      break;
    case KOOPA_RVT_BRANCH:
      // 12-07: 添加类型 分支跳转 br
      get_str = Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      // 12-07: 添加类型 无条件跳转 jump
      get_str = Visit(kind.data.jump);
      break;
    default:
      // cout << "unknown type";
      // 其他类型暂时遇不到
      assert(false);
  }
  // 需要访问那些使用该值的值吗?这个东西怎么使用?
  return get_str;
}

string Visit(const koopa_raw_return_t inst){
  string ret_str, get_str = "";
  // 确定返回值指针
  koopa_raw_value_t ret_value = inst.value;
  const auto &kind = ret_value->kind;
  // 判断指针类型
  switch (kind.tag)
  {
  case KOOPA_RVT_INTEGER:
    ret_str = Visit(ret_value);
    get_str += "  li   a0, " + ret_str + "\n";
    get_str += MoveFuncPointer(function_size);
    get_str += "  ret\n\n";
    break;
  case KOOPA_RVT_BINARY:
    // ret_str = register_distribution[ret_value];
    ret_str = to_string(stack_place[ret_value]);
    get_str += "  lw   a0, " + ret_str + "(sp)\n";
    get_str += MoveFuncPointer(function_size);
    get_str += "  ret\n\n";
    break;
  case KOOPA_RVT_LOAD:
    ret_str = to_string(stack_place[ret_value]);
    get_str += "  lw   a0, " + ret_str + "(sp)\n";
    get_str += MoveFuncPointer(function_size);
    get_str += "  ret\n\n";
    break;
  default:
    break;
  }
  return get_str;
  // string ret_val_str = Visit(ret_value);
  // 暂时在此处实现将立即数0放入a0存储器的操作
  // 返回riscv return指令的字符串形式
  // return "  li a0, " + ret_val_str + "\n  ret";
}

string Visit(const koopa_raw_integer_t inst){
  // 获得整数的值
  int32_t int_val = inst.value;
  // 尝试转为字符串
  string val_str = to_string(int_val);
  // 返回获得值的字符串形式
  return val_str;
}


string Visit(const koopa_raw_binary_t inst, const koopa_raw_value_t &value){
  // 测试结果是: 左值右值作为koopa_raw_value_t(指针)有两种可能性: 
  // 一个是具体数值指针 kind==KOOPA_RVT_INTEGER
  // 一个是二元运算指针 kind==KOOPA_RVT_BINARY
  // 需要判断指针相同来确定寄存器分配(大概)
  // 需要引入指针来管理寄存器分配
  // 设计左值 右值 左值寄存器 右值寄存器 本运算寄存器 结果值
  // 需要为reg设计初值, 以验证reg是否有分到寄存器 
  string left_str, right_str, left_reg, right_reg, reg="flag", get_str="";
  int register_counter = 0; // 计数, 我怎么用寄存器的
  // 为新建的二元运算分配新的寄存器
  // reg = registers[register_counter];
  // 放入对应值表中
  // register_distribution[value] = reg;
  // 寄存器计数器+1
  // register_counter++;
  // 获知左值右值的类型
  const auto &left_kind = inst.lhs->kind;
  const auto &right_kind = inst.rhs->kind;
  // 断言: 不能出现两个整数值, 不然寄存器分不了一点
  // 放弃断言
  // assert(!(left_kind.tag==KOOPA_RVT_INTEGER && right_kind.tag==KOOPA_RVT_INTEGER));
  // 检查左值
  switch (left_kind.tag)
  {
  case KOOPA_RVT_INTEGER:
  // 若为数字, 则看数字的值
    left_str = Visit(inst.lhs);
    if (left_str=="0") left_str = ZERO_REGISTER;
    else {
      // 非0左值直接放进分来的寄存器
      // get_str += "  li   " + reg + ", " + left_str;
      // left_str = reg;
      // 为非0左值分配一个新的寄存器
      left_reg = registers[register_counter];
      // 把这个非0左值放进去
      get_str += "  li   " + left_reg + ", " + left_str + "\n";
      // left_str更新为寄存器地址
      left_str = left_reg;
      // 本次运算的结果寄存器更新为left的寄存器
      reg = left_reg;
      // 寄存器计数器+1
      register_counter++;
      // 将reg设为这个新分配的
    }
    break;
  case KOOPA_RVT_BINARY:
  // 若为已有的二元运算, 则要找出该二元运算用的寄存器
  // 12-1修改: 修正为二元运算结果的栈上位置
    // left_str = register_distribution[inst.lhs];
    left_str = to_string(stack_place[inst.lhs]);
    // 为非0左值分配一个新的寄存器
    left_reg = registers[register_counter];
    // 计数器自增
    register_counter++;
    get_str += "  lw   " + left_reg + ", " + left_str + "(sp)\n";
    // 更新reg寄存器
    reg = left_reg;
    // 更新left_str
    left_str = left_reg;
    break;
  case KOOPA_RVT_LOAD:
    left_str = to_string(stack_place[inst.lhs]);
    // 为非0左值分配一个新的寄存器
    left_reg = registers[register_counter];
    // 计数器自增
    register_counter++;
    get_str += "  lw   " + left_reg + ", " + left_str + "(sp)\n";
    // 更新reg寄存器
    reg = left_reg;
    // 更新left_str
    left_str = left_reg;
    break;
  default:
  // 否则报错
    assert(false);
  }
  // 同样的检查右值类型
  switch (right_kind.tag)
  {
  case KOOPA_RVT_INTEGER:
  // 若为数字, 则看数字的值
    right_str = Visit(inst.rhs);
    if (right_str=="0") right_str = ZERO_REGISTER;
    else {
      // 非0右值直接放进分来的寄存器
      // get_str += "  li   " + reg + ", " + right_str + "\n";
      // right_str = reg;
      // 逻辑与左值相同
      right_reg = registers[register_counter];
      get_str += "  li   " + right_reg + ", " + right_str + "\n";
      right_str = right_reg;
      reg = right_reg;
      register_counter++;
    }
    break;
  case KOOPA_RVT_BINARY:
  // 若为已有的二元运算, 则要找出该二元运算用的寄存器
    // right_str = register_distribution[inst.rhs];
    right_str = to_string(stack_place[inst.rhs]);
    right_reg = registers[register_counter];
    register_counter++;
    get_str += "  lw   " + right_reg + ", " + right_str + "(sp)\n";
    // 更新right寄存器
    reg = right_reg;
    // 更新right_str
    right_str = right_reg;
    break;
  case KOOPA_RVT_LOAD:
    right_str = to_string(stack_place[inst.rhs]);
    right_reg = registers[register_counter];
    register_counter++;
    get_str += "  lw   " + right_reg + ", " + right_str + "(sp)\n";
    // 更新right寄存器
    reg = right_reg;
    // 更新right_str
    right_str = right_reg;
    break;
  default:
  // 否则报错
    assert(false);
  }
  // reg放入对应值表中
  // register_distribution[value] = reg;
  // 查看reg
  if (reg=="flag") {
  // 此时说明reg还是初值, 可能是两个即时值之类的, 没有分到寄存器
  // 为新建的二元运算分配新的寄存器
  reg = registers[register_counter];
  // 放入对应值表中
  // register_distribution[value] = reg;
  // 寄存器计数器+1
  register_counter++;
  }
  else {;
  // 此时reg已经不是初值了
  // 把reg放入对应值表中就好
  // register_distribution[value] = reg;
  }
  switch (inst.op)
  {
  case KOOPA_RBO_EQ:
  // 等于(相等的数按位异或的结果是0, 否则是1; 若按位异或的结果是0, 返回1(等于0), 否则返回0)
    get_str += "  xor  " + reg + ", " + right_str + ", " + left_str + "\n";
    get_str += "  seqz " + reg + ", " + reg + "\n";
    break;
  case KOOPA_RBO_NOT_EQ:
  // 不等于(相等的数按位异或的结果是0, 否则是1; 若按位异或的结果是0, 返回0(不等于0), 否则返回1)
    get_str += "  xor  " + reg + ", " + right_str + ", " + left_str + "\n";
    get_str += "  snez " + reg + ", " + reg + "\n";
    break;
  case KOOPA_RBO_ADD:
  // 加法
    get_str += "  add  " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  case KOOPA_RBO_SUB:
  // 减法
    get_str += "  sub  " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  case KOOPA_RBO_MUL:
  // 乘法
    get_str += "  mul  " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  case KOOPA_RBO_DIV:
  // 除法
    get_str += "  div  " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  case KOOPA_RBO_MOD:
  // 取余数运算
    get_str += "  rem  " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  case KOOPA_RBO_GT:
  // 大于
    get_str += "  sgt  " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  case KOOPA_RBO_LT:
  // 小于
    get_str += "  slt  " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  case KOOPA_RBO_GE:
  // 大于等于, 就是“不”“小于”
    // 判断左小于右
    get_str += "  slt  " + reg + ", " + left_str + ", " + right_str + "\n";
    // 判断结果是否是0, 若是, 则左大于等于右=1; 若否, 则左大于等于右=0
    get_str += "  seqz " + reg + ", " + reg + "\n";
    break;
  case KOOPA_RBO_LE:
  // 小于等于, 就是“不”“大于”
    get_str += "  sgt  " + reg + ", " + left_str + ", " + right_str + "\n";
    get_str += "  seqz " + reg + ", " + reg + "\n";
    break;
  case KOOPA_RBO_AND:
  // 按位与运算
    get_str += "  and  " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  case KOOPA_RBO_OR:
  // 按位或运算
    get_str += "  or   " + reg + ", " + left_str + ", " + right_str + "\n";
    break;
  default:
    cout << "unknown type!";
    break;
  }
  // 放入stack上
  stack_place[value] = unused_tag;
  get_str += "  sw   " + reg + ", " + to_string(unused_tag) + "(sp)\n";
  unused_tag += CountStackSpace(value->ty);
  return get_str;
}

// 计算栈需要的空间S'
int CountStackSpace(const koopa_raw_slice_t &slice) {
  int count_space = 0;
  koopa_raw_basic_block_t block_ptr;
  koopa_raw_value_t value_ptr;
  // cout << slice.len << "\n";
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    switch (slice.kind) {
      case KOOPA_RSIK_BASIC_BLOCK:
        block_ptr = reinterpret_cast<koopa_raw_basic_block_t>(ptr);
        count_space += CountStackSpace(block_ptr->insts);
        break;
      case KOOPA_RSIK_VALUE:
        value_ptr = reinterpret_cast<koopa_raw_value_t>(ptr);
        // 在这个位置尝试阻截branch和jump指令; 不需要这么做
        count_space += CountStackSpace(value_ptr->ty);
        break;
      default:
      // 其他情况报错
        cout << "unknown kind!";
        assert(false);
        break;
    }
  }
  switch (slice.kind) {
    case KOOPA_RSIK_BASIC_BLOCK:
      return count_space;
      break;
    case KOOPA_RSIK_VALUE:
      return count_space;
      break;
    default:
      cout << "problem!";
      assert(false);
      break;
  }
}

// 访问加载指令
string Visit(const koopa_raw_load_t load, const koopa_raw_value_t &value) {
  string get_str = "";  // 需要返回的操作字符串
  int register_counter = 0; // 计数, 我怎么用寄存器的
  int src_place; // source的位置
  int value_place; // 本指令对应的value指针在栈中的位置
  const auto src_kind = load.src->kind;  // 加载值的类型
  // 先进行读存操作
  switch (src_kind.tag) {
    case KOOPA_RVT_ALLOC:
    // 局部申存, 这意味着是@x形式的标号, 需要找到位置
      if (stack_place.find(load.src)==stack_place.end()) {
        // 找不到就报错
        cout << "symbol not defined!";
        assert(false);
    }
      else {
        // 否则记录位置, 执行读存操作
        src_place = stack_place[load.src];
        get_str += "  lw   " + registers[register_counter] + ", " + to_string(src_place) + "(sp)\n";
    }
    break;
    default:
      assert(false);
      break;
  }
  // 然后是写存操作
  value_place = unused_tag;
  stack_place[value] = value_place;
  get_str += "  sw   " + registers[register_counter] + ", " + to_string(value_place) + "(sp)\n";
  unused_tag += CountStackSpace(load.src->ty);
  return get_str;
}

// 访问存储指令
string Visit(const koopa_raw_store_t store) {
  string get_str = "";  // 需要返回的操作字符串
  int register_counter = 0; // 计数, 我怎么用寄存器的
  string value_str; // value值的字符串形式
  int value_place; // value在栈上的位置
  int dest_place; // dest在栈上的位置
  const auto value_kind = store.value->kind;  // 值的类型
  const auto dest_kind = store.dest->kind; // 存储标识符的类型?
  switch (value_kind.tag)
  {
  case KOOPA_RVT_INTEGER:
    /* 若为数字, 就将数字的值写到寄存器里去 */
    value_str = Visit(store.value);  // 获得值的字符串表示
    get_str += "  li   " + registers[register_counter] + ", " + value_str + "\n";
    break;
  case KOOPA_RVT_BINARY:
    /* 若为二元表达式, 这可能是%x形式的标号, 需要找到栈上位置 */
    if (stack_place.find(store.value)==stack_place.end()) {
      cout << "symbol not defined!";
      assert(false);
    }
    else {
      value_place = stack_place[store.value];
      get_str += "  lw   " + registers[register_counter] + ", " + to_string(value_place) + "(sp)\n";
    }
    break;
  case KOOPA_RVT_ALLOC:
    /* 若为局部申存, 这意味着是@x形式的标号, 需要找到栈上位置 */
    if (stack_place.find(store.value)==stack_place.end()) {
      cout << "symbol not defined!";
      assert(false);
    }
    else {
      value_place = stack_place[store.value];
      get_str += "  lw   " + registers[register_counter] + ", " + to_string(value_place) + "(sp)\n";
    }
    break;
  case KOOPA_RVT_LOAD:
  // 加载指令的操作?
    if (stack_place.find(store.value)==stack_place.end()) {
      cout << "symbol not defined!";
      assert(false);
    }
    else {
      value_place = stack_place[store.value];
      get_str += "  lw   " + registers[register_counter] + ", " + to_string(value_place) + "(sp)\n";
    }
    break;
  default:
    /* 其他情况报错 */
    cout << value_kind.tag;
    // assert(false);
    break;
  }
  switch (dest_kind.tag)
  {
  case KOOPA_RVT_ALLOC:
    /* 若为局部申存, 这意味着是@x形式的标号, 此时给一个位置 */
    if (stack_place.find(store.dest)==stack_place.end()){
      // 找不到的场合, 分一个新位置
      dest_place = unused_tag;
      unused_tag += CountStackSpace(store.value->ty);  // 这里需要改吗?
      get_str += "  sw   " + registers[register_counter] + ", " + to_string(dest_place) + "(sp)\n";
    }
    else {
      // 找到了的场合, 分一个旧位置
      dest_place = stack_place[store.dest];
      get_str += "  sw   " + registers[register_counter] + ", " + to_string(dest_place) + "(sp)\n";
    }
    break;
  default:
    // 否则报错
    assert(false);
    break;
  }
  return get_str;
}

// 通过类型判断需要给多大存储空间的函数
int CountStackSpace(const koopa_raw_type_t type){
  int count_space = 0;
  switch (type->tag) {
    case KOOPA_RTT_INT32:
      count_space += 4;
      break;
    case KOOPA_RTT_UNIT:
      // 若为void则分配0-不分配
      break;
    case KOOPA_RTT_POINTER:
      // alloc好像是pointer类型, 注意pointer类型操作
      count_space += CountStackSpace(type->data.pointer.base);
      break;
    default:
      cout << "unknown value type!";
      assert(false);
      break;
  }
  return count_space;
}
// 访问br(branch)指令的函数
string Visit(const koopa_raw_branch_t branch) {
  string get_str = "";
  int register_counter = 0; // 计数, 我怎么用寄存器的
  string cond_register = registers[register_counter];
  // 注意branch的cond的种类
  switch (branch.cond->kind.tag) {
    case KOOPA_RVT_INTEGER:
      get_str += "  li   " + cond_register + ", " + Visit(branch.cond) + "\n";
      register_counter++;
      break;
    default:
      // 处理条件
      get_str += "  lw   " + cond_register + ", " + to_string(stack_place[branch.cond]) + "(sp)\n";
      register_counter++;
      break;
  }
  // 处理向true方向的跳转
  string t_name = branch.true_bb->name;  // true块的名字
  get_str += "  bnez " + cond_register + ", " + t_name.substr(1, t_name.length()) + "\n";
  // 向false方向无条件跳转
  string f_name = branch.false_bb->name; // false块的名字
  get_str += "  j    " + f_name.substr(1, f_name.length()) + "\n\n";
  return get_str;
}

// 访问jump指令的函数
string Visit(const koopa_raw_jump_t jump) {
  string get_str = "";
  // int register_counter = 0; // 计数, 我怎么用寄存器的
  // 跳转到target
  string target_name = jump.target->name;
  get_str += "  j    " + target_name.substr(1, target_name.length()) + "\n\n";
  return get_str;
}

// 获取移动函数栈指针的操作字符串
string MoveFuncPointer(int func_size) {
  string get_str = "";
  if (func_size==0);
  else if (abs(func_size)<2048) {
    get_str += "  addi sp, sp, " + to_string(func_size) + "\n";
  }
  else {
    get_str += "  li   " + registers[0] + ", " + to_string(func_size) + "\n";
    get_str += "  add  sp, sp, " + registers[0] + "\n";
  }
  return get_str;
}