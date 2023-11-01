# AST Interpreter (Advanced Compiling Techniques Assignment 1)

基于llvm-10的Clang语法树实现的简单C语言解释器，见[作业要求](#作业要求)。

如果你想尝试运行本项目，可以参考[使用帮助](#使用帮助)。

如果你也要完成该作业，可以参考[Tips](#tips)来避坑。

## 使用帮助

### 环境

以下几种方案

1. 使用作者的环境可以通过[Dockerfile](Dockerfile)构建
```bash
docker buildx build -t ast-interpreter .
```

2. 使用课程提供的预编译好的llvm-10环境
```bash
docker pull lczxxx123/llvm_10_hw:0.2
```

3. 自行编译需要的llvm-10环境，请参考[Dockerfile](Dockerfile).

### 编译

```bash
mkdir build
cd build
cmake -DLLVM_DIR="<path to your llvm-10 dir>" ..
make
```

### 运行

```bash
./ast-interpreter "`cat <path to your c file>`"
```

### 测试

```bash
make test
```

### 打分

```bash
LLVM_DIR="<path to your llvm-10 dir>" ./grade.sh
LLVM_DIR="<path to your llvm-10 dir>" ./grade.sh extests
LLVM_DIR="<path to your llvm-10 dir>" ./grade.sh optests
```

## Tips

### 编译llvm-10

如果你打算使用自己编译的llvm-10作为实验环境，可能会遇到以下的问题：

1. llvm-10在高版本的gcc下编译出现问题（如头文件缺失等）。

如果不想打补丁，可以使用gcc-9及以下版本编译llvm-10。[Dockerfile](Dockerfile)中的debian:buster环境下使用的是gcc-8。

2. llvm-10编译之后，编译课程框架代码出现`ld`链接错误。

这应该是因为编译时没有启用`RTTI`。在编译llvm-10时，需要在`cmake`时加入`-DLLVM_ENABLE_RTTI=ON`选项。

3. llvm-10编译过慢，占用过高内存。

可以参考[Dockerfile](Dockerfile)中的编译选项。其中`-DCMAKE_BUILD_TYPE=Release`将减少大量的编译时间和内存占用；`-DLLVM_TARGETS_TO_BUILD=X86`可以只编译X86架构的代码，如果你的机器不支持X86架构，可以将其改为`-DLLVM_TARGETS_TO_BUILD=AArch64`等；`-DLLVM_USE_LINKER=lld`可以使用`lld`链接器，加快编译时链接速度。也可以使用`gold`，但是需要保证系统中安装了`lld`或者`gold`链接器。

### 测试用例

今年作业本身提供的测试用例存放在[tests](tests)目录下。我在做作业过程中额外增加了一些子测试用例存放在[extests](extests)目录下。额外我收集了一些来自前人的测试用例存放在[optests](optests)目录下。

我修改了[CMakeLists.txt](CMakeLists.txt)以支持自动化测试。这个测试里对比了你的输出和我提前设置的标准输出，如果不一致则会输出错误信息（[optests](optests)里并没设置标准输出）。你可以通过`make test`或`ctest`来运行所有测试用例。如果你想运行某个子测试用例，可以通过`ctest -R <test name>`来运行，具体可以参考[CMakeLists.txt](CMakeLists.txt)中的`add_test`部分。

我也从前人那里收集到了自动化评分脚本，你可以通过`./grade.sh`来运行[tests](tests)目录下所有测试用例。这个脚本将输出与`gcc`编译器实际编译的程序输出进行对比，如果不一致则会输出错误信息。如果你想运行特定文件夹，可以通过`./grade.sh <test folder>`来运行，具体可以参考[grade.sh](grade.sh)。

## 作业要求

Requirements: Implement a basic interpreter based on Clang

Marking: 25 testcases are provided and each test case counts for 1 mark

Supported Language: We support a subset of C language constructs, as follows: 

```c
Type: int | char | void | *
Operator: * | + | - | * | / | < | > | == | = | [ ] 
Statements: IfStmt | WhileStmt | ForStmt | DeclStmt 
Expr : BinaryOperator | UnaryOperator | DeclRefExpr | CallExpr | CastExpr 
```

We also need to support 4 external functions int GET(), void * MALLOC(int), void FREE (void *), void PRINT(int), the semantics of the 4 funcions are self-explanatory. 

A skelton implemnentation ast-interpreter.tgz is provided, and you are welcome to make any changes to the implementation. The provided implementation is able to interpreter the simple program like : 
```c
extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = GET();
   PRINT(a);
}
```

We provide a more formal definition of the accepted language, as follows: 

```c
Program : DeclList
DeclList : Declaration DeclList | empty
Declaration : VarDecl FuncDecl
VarDecl : Type VarList;
Type : BaseType | QualType
BaseType : int | char | void
QualType : Type * 
VarList : ID, VarList |  | ID[num], VarList | emtpy
FuncDecl : ExtFuncDecl | FuncDefinition
ExtFuncDecl : extern int GET(); | extern void * MALLOC(int); | extern void FREE(void *); | extern void PRINT(int);
FuncDefinition : Type ID (ParamList) { StmtList }
ParamList : Param, ParamList | empty
Param : Type ID
StmtList : Stmt, StmtList | empty
Stmt : IfStmt | WhileStmt | ForStmt | DeclStmt | CompoundStmt | CallStmt | AssignStmt | 
IfStmt : if (Expr) Stmt | if (Expr) Stmt else Stmt
WhileStmt : while (Expr) Stmt
DeclStmt : Type VarList;
AssignStmt : DeclRefExpr = Expr;
CallStmt : CallExpr;
CompoundStmt : { StmtList }
ForStmt : for ( Expr; Expr; Expr) Stmt
Expr : BinaryExpr | UnaryExpr | DeclRefExpr | CallExpr | CastExpr | ArrayExpr | DerefExpr | (Expr) | num
BinaryExpr : Expr BinOP Expr
BinaryOP : + | - | * | / | < | > | ==
UnaryExpr : - Expr
DeclRefExpr : ID
CallExpr : DeclRefExpr (ExprList)
ExprList : Expr, ExprList | empty
CastExpr : (Type) Expr
ArrayExpr : DeclRefExpr [Expr]
DerefExpr : * DeclRefExpr
```

## Special Credits

本项目独立完成。目录[optests](optests)内收集的测试用例和[grade.sh](grade.sh)来自于前人的作业，具体如下：

- https://github.com/ChinaNuke/ast-interpreter
- https://github.com/plusls/ast-interpreter
- https://github.com/ycdxsb/ast-interpreter
- https://github.com/enochii/AST-Interpreter
