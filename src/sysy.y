%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"  // include的用法, 先尝试
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

int exp_counter = 0;
// lv6增加的对if语句的计数
int if_counter = 0;
// lv5增加Stmt的第一层常数
// const int FIRST_LAYER = 1;
int layer_count = 1;
// lv7增加的对while语句的计数
int while_counter = 0;
// lv8增加的对编译单元的计数
int unit_counter = 0;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
// 添加关键字CONST(const)
// lv6添加关键字IF ELSE
// lv8添加关键字VOID
%token INT RETURN CONST IF ELSE VOID
// lv7添加关键字WHILE BREAK CONTINUE
%token WHILE BREAK CONTINUE
// 尝试添加 str_val EQ_CONST REL_CONST
%token <str_val> IDENT EQ_CONST REL_CONST LAND_CONST LOR_CONST
// 试图将Number token 的类型调整为int_val, 不行, 还是要作为ast_val
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Stmt Number Exp AddExp MulExp UnaryExp PrimaryExp RelExp RelOp EqExp EqOp LAndExp LAndOp LOrExp LOrOp
// lv4定义一批新的类型
%type <ast_val> Decl ConstDecl ConstDef ConstInitVal BlockItem LVal ConstExp VarDecl VarDef InitVal 
// 尝试定义类型ConstList BlockList VarList
%type <ast_val> ConstList BlockList VarList
// lv5定义新类型
%type <ast_val> SideExp
// lv6定义新类型IfStmt IfOpenStmt IfCloseStmt
%type <ast_val> IfStmt IfOpenStmt IfCloseStmt
// lv8定义新类型
%type <ast_val> FuncFParams FuncFParam FuncRParams CompItem CompList
// lv9 定义新类型
%type <ast_val> ArrayIdx ArrayList ConstInitValList InitValList

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值

CompUnit
  : CompList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->complist = unique_ptr<BaseAST>($1);
    comp_unit->layer = layer_count;
    // layer_count++;
    ast = move(comp_unit);
  }
  ;

CompList
  : CompItem {
    auto ast = new CompListAST();
    ast->item = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | CompList CompItem {
    auto ast = new CompListAST();
    ast->list = unique_ptr<BaseAST>($1);
    ast->item = unique_ptr<BaseAST>($2);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

// 单独的全局符号
CompItem
  : Decl {
    auto ast = new CompItemAST();
    ast->content = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | FuncDef {
    auto ast = new CompItemAST();
    ast->content = unique_ptr<BaseAST>($1);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
  : INT IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "int";
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    ast->ast_state = 0;
    $$ = ast;
  }
  | VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "void";
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    ast->ast_state = 0;
    $$ = ast;
  }
  | INT IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "int";
    ast->ident = *unique_ptr<string>($2);
    ast->p_list = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    ast->params_layer = layer_count;
    layer_count++;
    ast->ast_state = 1;
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = "void";
    ast->ident = *unique_ptr<string>($2);
    ast->p_list = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    ast->params_layer = layer_count;
    layer_count++;
    ast->ast_state = 1;
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam {
    auto ast = new FuncFParamsAST();
    ast->param = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | FuncFParam ',' FuncFParams {
    auto ast = new FuncFParamsAST();
    ast->param = unique_ptr<BaseAST>($1);
    ast->p_list = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

FuncFParam
  : INT IDENT {
    auto ast = new FuncFParamAST();
    ast->btype = "int";
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

Block
  : '{' BlockList '}' {
    auto ast = new BlockAST();
    ast->blocklist = unique_ptr<BaseAST>($2);
    ast->layer = layer_count;
    layer_count++;
    $$ = ast;
    //auto stmt = unique_ptr<string>($2);
    //$$ = new string("{ " + *stmt + " }");
  }
  ;

// 增加list以表示重复的问题
BlockList
  : /*empty rule*/ {
    auto ast = new BlockListAST();
    ast->ast_state = 1;
    $$ = ast;
  }
  | BlockList BlockItem {
    auto ast = new BlockListAST();
    ast->blocklist = unique_ptr<BaseAST>($1);
    ast->item = unique_ptr<BaseAST>($2);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->item = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | /*Stmt*/ IfStmt {
    auto ast = new BlockItemAST();
    ast->item = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// todo: 解决匹配多个token0次或多次的问题
// 尝试: 设计新节点, 转换为左递归文法
ConstDecl 
  : CONST INT ConstList ';' {
    auto ast = new ConstDeclAST();
    ast->word = "const";
    ast->btype = "int";
    ast->mylist = unique_ptr<BaseAST>($3);
    // ast->mylist->fromup = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

VarDecl
  : INT VarList ';' {
    auto ast = new VarDeclAST();
    ast->btype = "int";
    ast->mylist = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

VarList
  : VarDef {
    auto ast = new VarListAST();
    ast->mydef = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | VarList ',' VarDef {
    auto ast = new VarListAST();
    ast->mylist = unique_ptr<BaseAST>($1);
    ast->mydef = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

VarDef
  : IDENT ArrayList {
    // lv9修改: 支持数组定义
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->list = unique_ptr<BaseAST>($2);
    ast->ast_state = 1;
    $$ = ast;
  }
  | IDENT ArrayList '=' InitVal {
    // lv9修改: 支持数组定义
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->list = unique_ptr<BaseAST>($2);
    ast->init = unique_ptr<BaseAST>($4);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ast_state = 0;
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    ast->ast_state = 1;
    $$ = ast;
  }
  | '{' InitValList '}' {
    auto ast = new InitValAST();
    ast->list = unique_ptr<BaseAST>($2);
    ast->ast_state = 2;
    $$ = ast;
  }
  ;

InitValList
  : InitVal {
    auto ast = new InitValListAST();
    ast->init = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | InitValList ',' InitVal {
    auto ast = new InitValListAST();
    ast->list = unique_ptr<BaseAST>($1);
    ast->init = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

ConstList
  : ConstDef {
    auto ast = new ConstListAST();
    ast->mydef = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | ConstList ',' ConstDef {
    auto ast = new ConstListAST();
    ast->mylist = unique_ptr<BaseAST>($1);
    ast->mydef = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

ConstDef 
  : IDENT ArrayList '=' ConstInitVal {
    // lv9调整: 添加ArrayList以支持数组
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->list = unique_ptr<BaseAST>($2);
    ast->init = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->constexp = unique_ptr<BaseAST>($1);
    ast->ast_state = 0;
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    ast->ast_state = 1;
    $$ = ast;
  }
  | '{' ConstInitValList '}' {
    auto ast = new ConstInitValAST();
    ast->list = unique_ptr<BaseAST>($2);
    ast->ast_state = 2;
    $$ = ast;
  }
  ;

ConstInitValList
  : ConstInitVal {
    auto ast = new ConstInitValListAST();
    ast->init = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | ConstInitValList ',' ConstInitVal {
    auto ast = new ConstInitValListAST();
    ast->list = unique_ptr<BaseAST>($1);
    ast->init = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT ArrayList {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->list = unique_ptr<BaseAST>($2);
    ast->place = exp_counter;
    exp_counter++;
    $$ = ast;
  }
  ;

// 数组定义或使用中的
ArrayList
  : /*empty rule*/ {
    auto ast = new ArrayListAST();
    ast->ast_state = 1;  // 1表示空规则
    $$ = ast;
  }
  | ArrayList ArrayIdx {
    auto ast = new ArrayListAST();
    ast->list = unique_ptr<BaseAST>($1);
    ast->idx = unique_ptr<BaseAST>($2);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

// 数组定义或使用中的单个元素
ArrayIdx
  : '[' Exp ']' {
    auto ast = new ArrayIdxAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

// lv6增加的对if语句的支持
IfStmt
  : IfOpenStmt {
    auto ast = new IfStmtAST();
    ast->ifstmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IfCloseStmt {
    auto ast = new IfStmtAST();
    ast->ifstmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

IfOpenStmt
  : IF '(' Exp ')' IfStmt {
    auto ast = new IfOpenStmtAST();
    ast->judge = unique_ptr<BaseAST>($3);
    ast->t_jump = unique_ptr<BaseAST>($5);
    ast->ast_state = 1;
    ast->place = if_counter;
    if_counter++;
    $$ = ast;
  }
  | IF '(' Exp ')' IfCloseStmt ELSE IfOpenStmt {
    auto ast = new IfOpenStmtAST();
    ast->judge = unique_ptr<BaseAST>($3);
    ast->t_jump = unique_ptr<BaseAST>($5);
    ast->f_jump = unique_ptr<BaseAST>($7);
    ast->ast_state = 0;
    ast->place = if_counter;
    if_counter++;
    $$ = ast;
  }
  ;

IfCloseStmt
  : Stmt {
    auto ast = new IfCloseStmtAST();
    ast->t_jump = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | IF '(' Exp ')' IfCloseStmt ELSE IfCloseStmt {
    auto ast = new IfCloseStmtAST();
    ast->judge = unique_ptr<BaseAST>($3);
    ast->t_jump = unique_ptr<BaseAST>($5);
    ast->f_jump = unique_ptr<BaseAST>($7);
    ast->ast_state = 0;
    ast->place = if_counter;
    if_counter++;
    $$ = ast;
  }
  ;

// lv5增加Stmt的[Exp];和Block两种分支
Stmt
  : LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->expression = unique_ptr<BaseAST>($3);
    ast->ast_state = 1;
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->ast_state = 5;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->ast_state = 6;
    $$ = ast;
  }
  | SideExp {
    auto ast = new StmtAST();
    ast->expression = unique_ptr<BaseAST>($1);
    ast->ast_state = 2;
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->expression = unique_ptr<BaseAST>($1);
    ast->ast_state = 3;
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->expression = unique_ptr<BaseAST>($3);
    ast->statement = unique_ptr<BaseAST>($5);
    ast->ast_state = 4;
    /*申请一个while_place*/
    ast->while_place = while_counter;
    while_counter++; // 自增
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->word = "return";
    ast->expression = unique_ptr<BaseAST>($2);
    ast->ast_state = 0;
    $$ = ast;
    //auto number = unique_ptr<string>($2);
    //$$ = new string("return " + *number + ";");
  }
  ;

SideExp
  : ';' {
    auto ast = new SideExpAST();
    ast->ast_state = 1;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new SideExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

// 需要增加UnaryExp, PrimaryExp等符号
Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->land = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | LOrExp LOrOp LAndExp {
    auto ast = new LOrExpAST();
    ast->lor = unique_ptr<BaseAST>($1);
    ast->op = unique_ptr<BaseAST>($2);
    ast->land = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    /*连续三次申请标识符*/
    ast->place1 = exp_counter;
    exp_counter++;
    ast->place2 = exp_counter;
    exp_counter++;
    ast->place3 = exp_counter;
    exp_counter++;
    /*申请一个分支跳转标识符*/
    ast->lor_place = if_counter;
    if_counter++;
    $$ = ast;
  }
  ;

LOrOp
  : LOR_CONST {
    auto ast = new LOrOpAST();
    ast->op = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | LAndExp LAndOp EqExp {
    auto ast = new LAndExpAST();
    ast->land = unique_ptr<BaseAST>($1);
    ast->op = unique_ptr<BaseAST>($2);
    ast->eq = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    /*连续三次申请标识符*/
    ast->place1 = exp_counter;
    exp_counter++;
    ast->place2 = exp_counter;
    exp_counter++;
    ast->place3 = exp_counter;
    exp_counter++;
    /*申请分支跳转标识符*/
    ast->land_place = if_counter;
    if_counter++;
    $$ = ast;
  }
  ;

LAndOp
  : LAND_CONST {
    auto ast = new LAndOpAST();
    ast->op = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | EqExp EqOp RelExp {
    auto ast = new EqExpAST();
    ast->eq = unique_ptr<BaseAST>($1);
    ast->op = unique_ptr<BaseAST>($2);
    ast->rel = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    ast->place = exp_counter;
    exp_counter++;
    $$ = ast;
  }
  ;

EqOp
  : EQ_CONST {
    auto ast = new EqOpAST();
    ast->op = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | RelExp RelOp AddExp {
    auto ast = new RelExpAST();
    ast->rel = unique_ptr<BaseAST>($1);
    ast->op = unique_ptr<BaseAST>($2);
    ast->add = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    ast->place = exp_counter;
    exp_counter++;
    $$ = ast;
  }
  ;

RelOp
  : REL_CONST {
    auto ast = new RelOpAST();
    ast->op = *unique_ptr<string>($1);
    $$ = ast;
  }

// 增加AddExp
AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->add = unique_ptr<BaseAST>($1);
    ast->op = "+";
    ast->mul = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    ast->place = exp_counter;
    exp_counter++;
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->add = unique_ptr<BaseAST>($1);
    ast->op = "-";
    ast->mul = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    ast->place = exp_counter;
    exp_counter++;
    $$ = ast;
  }
  ;

// 增加MulExp
MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul = unique_ptr<BaseAST>($1);
    ast->op = "*";
    ast->unary = unique_ptr<BaseAST>($3);
    ast->place = exp_counter;
    exp_counter++;
    ast->ast_state = 0;
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul = unique_ptr<BaseAST>($1);
    ast->op = "/";
    ast->unary = unique_ptr<BaseAST>($3);
    ast->place = exp_counter;
    exp_counter++;
    ast->ast_state = 0;
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->mul = unique_ptr<BaseAST>($1);
    ast->op = "%";
    ast->unary = unique_ptr<BaseAST>($3);
    ast->place = exp_counter;
    exp_counter++;
    ast->ast_state = 0;
    $$ = ast;
  }

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->son_tree = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->place = exp_counter;
    exp_counter++;
    ast->ast_state = 3;
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->son_tree = unique_ptr<BaseAST>($3);
    ast->place = exp_counter;
    exp_counter++;
    ast->ast_state = 2;
    $$ = ast;
  }
  | '+' UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = "+";
    ast->son_tree = unique_ptr<BaseAST>($2);
    ast->place = exp_counter;
    exp_counter++;
    ast->ast_state = 0;
    $$ = ast;
  }
  | '-' UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = "-";
    ast->son_tree = unique_ptr<BaseAST>($2);
    ast->place = exp_counter;
    exp_counter++;
    ast->ast_state = 0;
    $$ = ast;

  }
  | '!' UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = "!";
    ast->son_tree = unique_ptr<BaseAST>($2);
    ast->place = exp_counter;
    exp_counter++;
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

PrimaryExp
  : Number {
    auto ast = new PrimaryExpAST();
    ast->son_tree = unique_ptr<BaseAST>($1);
    ast->ast_state = 0;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->son_tree = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->son_tree = unique_ptr<BaseAST>($2);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

FuncRParams
  : Exp {
    auto ast = new FuncRParamsAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->ast_state = 1;
    $$ = ast;
  }
  | Exp ',' FuncRParams {
    auto ast = new FuncRParamsAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->p_list = unique_ptr<BaseAST>($3);
    ast->ast_state = 0;
    $$ = ast;
  }
  ;

// 接下来还请考虑是否使用Number这个种类的token
Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->int_const = $1;
    $$ = ast;
    //$$ = new string(to_string($1));
    // 试图调整类型，不再使用str，而是int
    //$$ = $1;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
