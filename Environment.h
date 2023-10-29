//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include <stdio.h>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

/// Heap maps address to a value
class Heap
{
	/// Heap maps Variable Declaration to Addresses (represented using an Integer value)
	std::map<Decl *, int> mVars;
	/// Heap maps Addresses to Values
	std::vector<char> mValues;
	/// FreeList maps Addresses to Intervals
	std::vector<std::pair<int, int>> mFreeList;

public:
	Heap() : mVars(), mValues(), mFreeList()
	{
	}

	int Malloc(int size)
	{
		for (int i = 0; i < mFreeList.size(); i++)
		{
			if (mFreeList[i].second - mFreeList[i].first >= size)
			{
				int addr = mFreeList[i].first;
				mFreeList[i].first += size;
				if (mFreeList[i].first == mFreeList[i].second)
				{
					mFreeList.erase(mFreeList.begin() + i);
				}
				return addr;
			}
		}
		int addr = mValues.size();
		mValues.resize(mValues.size() + size);
		return addr;
	}
	//    void Free (int addr) ;
	void Update(int addr, int val)
	{
		*(int *)&mValues[addr] = val;
	}

	int get(int addr)
	{
		return *(int *)&mValues[addr];
	}

	void bindDecl(Decl *decl, int val)
	{
		if (mVars.find(decl) == mVars.end())
		{
			int addr = Malloc(sizeof(int));
		}
		Update(mVars[decl], val);
	}

	int getDeclVal(Decl *decl)
	{
		assert(mVars.find(decl) != mVars.end());
		return get(mVars[decl]);
	}
};

class StackFrame
{
	/// StackFrame maps Variable Declaration to Value
	/// Which are either integer or addresses (also represented using an Integer value)
	std::map<Decl *, int> mVars;
	std::map<Stmt *, int> mExprs;
	/// The current stmt
	Stmt *mPC;
	Heap *mHeap;

public:
	StackFrame(Heap *mHeap) : mVars(), mExprs(), mPC(), mHeap(mHeap)
	{
	}

	void initDecl(Decl *decl, int val)
	{
		mVars[decl] = val;
	}

	void bindDecl(Decl *decl, int val)
	{
		if (mVars.find(decl) == mVars.end())
		{
			mHeap->bindDecl(decl, val);
		}
		else
			mVars[decl] = val;
	}

	int getDeclVal(Decl *decl)
	{
		if (mVars.find(decl) == mVars.end())
			return mHeap->getDeclVal(decl);
		else
			return mVars.find(decl)->second;
	}

	void bindStmt(Stmt *stmt, int val)
	{
		mExprs[stmt] = val;
	}

	int getStmtVal(Stmt *stmt)
	{
		assert(mExprs.find(stmt) != mExprs.end());
		return mExprs[stmt];
	}

	void setPC(Stmt *stmt)
	{
		mPC = stmt;
	}
	Stmt *getPC()
	{
		return mPC;
	}
};

class Environment
{
	std::vector<StackFrame> mStack;
	Heap mHeap;

	FunctionDecl *mFree; /// Declartions to the built-in functions
	FunctionDecl *mMalloc;
	FunctionDecl *mInput;
	FunctionDecl *mOutput;

	FunctionDecl *mEntry;

public:
	/// Get the declartions to the built-in functions
	Environment() : mStack(), mHeap(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL)
	{
	}

	/// Initialize the Environment
	void init(TranslationUnitDecl *unit)
	{
		for (TranslationUnitDecl::decl_iterator i = unit->decls_begin(), e = unit->decls_end(); i != e; ++i)
		{
			if (FunctionDecl *fdecl = dyn_cast<FunctionDecl>(*i))
			{
				if (fdecl->getName().equals("FREE"))
					mFree = fdecl;
				else if (fdecl->getName().equals("MALLOC"))
					mMalloc = fdecl;
				else if (fdecl->getName().equals("GET"))
					mInput = fdecl;
				else if (fdecl->getName().equals("PRINT"))
					mOutput = fdecl;
				else if (fdecl->getName().equals("main"))
					mEntry = fdecl;
			}
			else if (VarDecl *vdecl = dyn_cast<VarDecl>(*i))
			{
				if (vdecl->hasInit())
				{
					Expr *expr = vdecl->getInit();
					if (IntegerLiteral *literal = dyn_cast<IntegerLiteral>(expr))
					{
						int val = literal->getValue().getSExtValue();
						mHeap.bindDecl(vdecl, val);
					}
				}
				else
					mHeap.bindDecl(vdecl, 0);
			}
		}
		mStack.push_back(StackFrame(&mHeap));
	}

	FunctionDecl *getEntry()
	{
		return mEntry;
	}

	void intliteral(IntegerLiteral *literal)
	{
		mStack.back().bindStmt(literal, literal->getValue().getSExtValue());
	}

	void unop(UnaryOperator *uop)
	{
		Expr *expr = uop->getSubExpr();
		int val = mStack.back().getStmtVal(expr);
		if (uop->getOpcode() == UO_Minus)
			val = -val;
		mStack.back().bindStmt(uop, val);
	}

	void binop(BinaryOperator *bop)
	{
		Expr *left = bop->getLHS();
		Expr *right = bop->getRHS();

		int val = 0;
		if (bop->isAssignmentOp())
		{
			val = mStack.back().getStmtVal(right);
			mStack.back().bindStmt(left, val);
			if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(left))
			{
				Decl *decl = declexpr->getFoundDecl();
				mStack.back().bindDecl(decl, val);
			}
		}
		else if (bop->isAdditiveOp())
		{
			if (bop->getOpcode() == BO_Add)
				val = mStack.back().getStmtVal(left) + mStack.back().getStmtVal(right);
			else if (bop->getOpcode() == BO_Sub)
				val = mStack.back().getStmtVal(left) - mStack.back().getStmtVal(right);
		}
		else if (bop->isMultiplicativeOp())
		{
			if (bop->getOpcode() == BO_Mul)
				val = mStack.back().getStmtVal(left) * mStack.back().getStmtVal(right);
			else if (bop->getOpcode() == BO_Div)
				val = mStack.back().getStmtVal(left) / mStack.back().getStmtVal(right);
			else if (bop->getOpcode() == BO_Rem)
				val = mStack.back().getStmtVal(left) % mStack.back().getStmtVal(right);
			mStack.back().bindStmt(bop, val);
		}
		else if (bop->isRelationalOp())
		{
			if (bop->getOpcode() == BO_LT)
				val = mStack.back().getStmtVal(left) < mStack.back().getStmtVal(right);
			else if (bop->getOpcode() == BO_GT)
				val = mStack.back().getStmtVal(left) > mStack.back().getStmtVal(right);
			else if (bop->getOpcode() == BO_LE)
				val = mStack.back().getStmtVal(left) <= mStack.back().getStmtVal(right);
			else if (bop->getOpcode() == BO_GE)
				val = mStack.back().getStmtVal(left) >= mStack.back().getStmtVal(right);
		}
		else if (bop->isEqualityOp())
		{
			if (bop->getOpcode() == BO_EQ)
				val = mStack.back().getStmtVal(left) == mStack.back().getStmtVal(right);
			else if (bop->getOpcode() == BO_NE)
				val = mStack.back().getStmtVal(left) != mStack.back().getStmtVal(right);
		}
		mStack.back().bindStmt(bop, val);
	}

	void decl(DeclStmt *declstmt)
	{
		for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
			 it != ie; ++it)
		{
			Decl *decl = *it;
			if (VarDecl *vardecl = dyn_cast<VarDecl>(decl))
			{
				if (vardecl->hasInit())
				{
					Expr *expr = vardecl->getInit();
					int val = mStack.back().getStmtVal(expr);
					mStack.back().initDecl(vardecl, val);
				}
				else
					mStack.back().initDecl(vardecl, 0);
			}
		}
	}

	void declref(DeclRefExpr *declref)
	{
		mStack.back().setPC(declref);
		if (declref->getType()->isIntegerType())
		{
			Decl *decl = declref->getFoundDecl();

			int val = mStack.back().getDeclVal(decl);
			mStack.back().bindStmt(declref, val);
		}
	}

	void cast(CastExpr *castexpr)
	{
		mStack.back().setPC(castexpr);
		if (castexpr->getType()->isIntegerType())
		{
			Expr *expr = castexpr->getSubExpr();
			int val = mStack.back().getStmtVal(expr);
			mStack.back().bindStmt(castexpr, val);
		}
	}

	/// !TODO Support Function Call
	Stmt *call(CallExpr *callexpr)
	{
		mStack.back().setPC(callexpr);
		int val = 0;
		FunctionDecl *callee = callexpr->getDirectCallee();
		if (callee == mInput)
		{
			llvm::errs() << "Please Input an Integer Value : ";
			scanf("%d", &val);

			mStack.back().bindStmt(callexpr, val);
			return nullptr;
		}
		else if (callee == mOutput)
		{
			Expr *decl = callexpr->getArg(0);
			val = mStack.back().getStmtVal(decl);
			llvm::errs() << val;
			return nullptr;
		}
		else
		{
			/// You could add your code here for Function call Return
			StackFrame stack(&mHeap);
			for (int i = 0; i < callexpr->getNumArgs(); i++)
			{
				Expr *arg = callexpr->getArg(i);
				int val = mStack.back().getStmtVal(arg);
				ParmVarDecl *param = callee->getParamDecl(i);
				stack.initDecl(param, val);
			}
			mStack.push_back(stack);
			return callee->getBody();
		}
	}

	void ret(ReturnStmt *retstmt)
	{
		int val = 0;
		if (retstmt)
		{
			if (Expr *expr = retstmt->getRetValue())
			{
				val = mStack.back().getStmtVal(expr);
			}
		}
		mStack.pop_back();
		if (!mStack.empty())
		{
			mStack.back().bindStmt(mStack.back().getPC(), val);
		}
	}

	Stmt *ifel(IfStmt *ifstmt)
	{
		Expr *cond = ifstmt->getCond();
		int val = mStack.back().getStmtVal(cond);
		if (val)
			return ifstmt->getThen();
		else
			return ifstmt->getElse();
	}

	int getStmtVal(Stmt *stmt)
	{
		return mStack.back().getStmtVal(stmt);
	}
};
