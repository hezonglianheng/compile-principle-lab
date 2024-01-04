# hezonglianheng的编译原理作业

## 说明
- 2023.11.8 创建此文档
- 2023.11.8 通过lv1.5
- 2023.11.15 通过lv2
- 2023.11.23 通过lv3.1
- 2023.11.24 通过lv3.2
- 2023.11.27 通过lv3
- 2023.11.29 通过lv4.1
- 2023.12.1 通过lv4
- 2023.12.5 通过lv5
- 2023.12.9 通过lv6
- 2023.12.26 通过lv7
- 2024.01.04 通过lv8.1

## 一些笔记
- 本实践课作业的说明文档存在一些typo
- 注意向AST父节点传值的方法(定义新的函数并调用之)
- 注意当前使用的内存分配方法
- 注意：可以使用左递归文法
- 注意：有时候可以多定义符号，有助于缓解文法上的冲突
- 注意：注意逻辑与、逻辑或的实现逻辑
- 注意：在实现编译期间求值的时候，需要非常小心各函数的实现方式。格外注意类LValAST(从map中取出来的值应该放进GetUpward函数中，对应Number类)和类ConstDefAST(Dump中做将const压入map的操作)
- 注意：注意alloc指令的返回值类型. alloc指令的返回值是pointer类型, 需要查询pointer类型的内容才能确定大小, 而且这个指令是有返回的, 这点需要注意. 另外, 能否实现一点简单的寄存器分配?
- 注意：在EBNF范式中，"[]"符号对应的是出现0次或多次；直到lv5才检查出这个错误。因此，需要注意，这里可能涉及空规则的使用与实现，参见BlockList的语法规则，就使用了空规则

## 有关悬空else问题
在说明文档中，语法
```ebnf
Stmt ::= ...
        | "if" '(' Exp ')' Stmt ["else" Stmt]
        | ...
```
是具有二义性的文法. 我们参照龙书第四章对于悬空`"else"`的分析, 对于具有`"else"`和不具有`"else"`的情况做分析, 可以写出（伪）ebnf语法如下.
```ebnf
IfStmt ::= Open | Close | Others
Open ::= "if" '(' Exp ')' Open 
        | "if" '(' Exp ')' Close 
        | "if" '(' Exp ')' Others
Close ::= "if" '(' Exp ')' Close "else" Open
        | "if" '(' Exp ')' Close "else" Close
        | "if" '(' Exp ')' Close "else" Others
        | "if" '(' Exp ')' Others "else" Open
        | "if" '(' Exp ')' Others "else" Close
        | "if" '(' Exp ')' Others "else" Others
Others ::= ...
```
由上述文法, 显然, 我们可以使得`Close`推导`Others`. 这样, 我们就得到了文法
```ebnf
IfStmt ::= Open | Close
Open ::= "if" '(' Exp ')' Open 
        | "if" '(' Exp ')' Close
Close ::= "if" '(' Exp ')' Close "else" Open
        | "if" '(' Exp ')' Close "else" Close
        | Others
Others ::= ...
```
这是一个看上去非常漂亮的文法, 然而事实上它仍然具有二义性. 注意到以下代码
```cpp
if (exp) if (exp) if (exp) others else if (exp) if (exp) 
others else others else others else others
```
如果按照上述文法解析, 那么将有一种错误的解析方式
```cpp
IfStmt -> Open
       -> if (exp) Close
       -> if (exp) if (exp) Close else others
       -> if (exp) if (exp) if (exp) others else Open
       else others
       -> if (exp) if (exp) if (exp) others else (
       if (exp) if (exp) others else others) else others
```
与规范中要求`else`永远匹配其距离最近的`if`相冲突. 由此可知, 位于两个`else`之间的代码块中, `else`和`if`的数量应该一致. 而上述推导的问题点在于规则
```ebnf
Close ::= "if" '(' Exp ')' Close "else" Open
```
那么, 改写为规则
```ebnf
Open ::= "if" '(' Exp ')' Close "else" Open
```
可行吗? 答案是可以的. 这样的文法同样可以解析类似于下述的代码
```cpp
if (exp) others else if (exp) if (exp) others
```
最终, 我们需要实现的语法规则为
```ebnf
IfStmt ::= Open | Close
Open ::= "if" '(' Exp ')' IfStmt 
        | "if" '(' Exp ')' Close "else" Open
Close ::= Others
        | "if" '(' Exp ')' Close "else" Close
Others ::= ...
```
这就是悬空`"else"`问题的解决方案.

## 一个暂时不修的bug
目前发现, 在ast.h文件的LValAST的实现中, 如果代码为
```c
int a = 4;
a = a + 1;
return a;
```
则在执行时, 其`IdentSearch()`函数会在解析语句`a = a + 1`之前执行一次, 且此时无法找到`a`. 但若没有第二行计算语句, 则不会报错. 目前由于不影响过样例, 故暂时不修改. 
## 有关语句`break;`和`continue;`
引自ChatGPT3.5:

在Bison中，以“|”分隔的多条产生式的顺序可以影响解析结果。Bison按照产生式的顺序来尝试匹配输入，它会按照产生式的定义顺序从上到下进行匹配。当存在多个产生式可以匹配输入时，Bison将选择第一个匹配成功的产生式。

考虑以下的例子：

```bison
expr : expr '+' expr
     | expr '-' expr
     | NUMBER
     ;
```

在这个例子中，如果输入是 `1 + 2 - 3`，Bison 将尝试按照产生式的顺序来匹配。如果它首先尝试匹配 `expr '+' expr`，那么它将匹配 `1 + 2`，然后再尝试匹配 `- 3`。如果它首先尝试匹配 `expr '-' expr`，那么它将匹配 `1 + 2 - 3`。产生式的顺序决定了解析的优先级和结合性。

因此，在设计语法时，你需要考虑产生式的顺序以确保正确的解析结果。如果你想要改变优先级或者结合性，你可以通过重新排列产生式来实现。

对于语句`break;`和`continue;`, 情况是类似的. 需要设计单独的符号或者安排语句的顺序问题, 使得`break;`和`continue;`不会被解析为`Exp`.

# 有关类型问题
在SysY语言规范中, 关于类型有如下定义:
```ebnf
Decl          ::= ConstDecl | VarDecl;
ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
BType         ::= "int";
ConstDef      ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal;
VarDef        ::= IDENT {"[" ConstExp "]"}
                | IDENT {"[" ConstExp "]"} "=" InitVal;
FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;
FuncType      ::= "void" | "int";
FuncFParams   ::= FuncFParam {"," FuncFParam};
FuncFParam    ::= BType IDENT ["[" "]" {"[" ConstExp "]"}];
```
由于`int`可以被规约为`BType`和`FuncType`两种类型, 因此会发生归约-归约冲突. 因此自行实现时, 需要取消这两种类型, 而直接书写函数定义/全局变量定义的表达式.