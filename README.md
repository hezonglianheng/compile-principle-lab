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

## 一些笔记
- 本实践课作业的说明文档存在一些typo
- 注意向AST父节点传值的方法(定义新的函数并调用之)
- 注意当前使用的内存分配方法
- 注意：可以使用左递归文法
- 注意：有时候可以多定义符号，有助于缓解文法上的冲突
- 注意：注意逻辑与、逻辑或的实现逻辑
- 注意：在实现编译期间求值的时候，需要非常小心各函数的实现方式。格外注意类LValAST(从map中取出来的值应该放进GetUpward函数中，对应Number类)和类ConstDefAST(Dump中做将const压入map的操作)
- 注意：注意alloc指令的返回值类型. alloc指令的返回值是pointer类型, 需要查询pointer类型的内容才能确定大小, 而且这个指令是有返回的, 这点需要注意. 另外, 能否实现一点简单的寄存器分配?