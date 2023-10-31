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
	/// OccupiedList maps Addresses to Interval Size
	std::map<int, int> mOccupied;

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
					mFreeList.erase(mFreeList.begin() + i);
				mOccupied[addr] = size;
				return addr;
			}
		}
		int addr = mValues.size();
		mValues.resize(mValues.size() + size);
		mOccupied[addr] = size;
		return addr;
	}
	void Free(int addr)
	{
		assert(mOccupied.find(addr) != mOccupied.end());
		int size = mOccupied[addr];
		mOccupied.erase(addr);
		for (int i = 0; i < mFreeList.size(); i++)
		{
			if (mFreeList[i].first == addr + size)
			{
				mFreeList[i].first = addr;
				return;
			}
			else if (mFreeList[i].second == addr)
			{
				mFreeList[i].second = addr + size;
				if (addr + size == mValues.size())
				{
					mValues.resize(mFreeList.back().second - mFreeList.back().first);
					mFreeList.pop_back();
				}
				return;
			}
			else if (mFreeList[i].first > addr + size)
			{
				mFreeList.insert(mFreeList.begin() + i, std::make_pair(addr, addr + size));
				return;
			}
		}
		mFreeList.push_back(std::make_pair(addr, addr + size));
		if (addr + size == mValues.size())
		{
			mValues.resize(mFreeList.back().second - mFreeList.back().first);
			mFreeList.pop_back();
		}
	}
	void Update(int addr, int val)
	{
		*(int *)&mValues[addr] = val;
	}
	void Update(int addr, char val)
	{
		*(char *)&mValues[addr] = val;
	}

	int getInt(int addr)
	{
		return *(int *)&mValues[addr];
	}

	char getChar(int addr)
	{
		return *(char *)&mValues[addr];
	}

	void bindDecl(Decl *decl, int val)
	{
		if (mVars.find(decl) == mVars.end())
		{
			if (VarDecl *vardecl = dyn_cast<VarDecl>(decl))
			{
				if (vardecl->getType()->isCharType())
				{
					mVars[decl] = Malloc(sizeof(char));
					Update(mVars[decl], (char)val);
				}
				else if (vardecl->getType()->isIntegerType())
				{
					mVars[decl] = Malloc(sizeof(int));
					Update(mVars[decl], val);
				}
				else if (vardecl->getType()->isPointerType())
				{
					mVars[decl] = Malloc(sizeof(int *));
					Update(mVars[decl], val);
				}
			}
		}
		else
		{
			if (VarDecl *vardecl = dyn_cast<VarDecl>(decl))
			{
				if (vardecl->getType()->isCharType())
					Update(mVars[decl], (char)val);
				else if (vardecl->getType()->isIntegerType())
					Update(mVars[decl], val);
				else if (vardecl->getType()->isPointerType())
					Update(mVars[decl], val);
			}
		}
	}

	int getDeclVal(Decl *decl)
	{
		assert(mVars.find(decl) != mVars.end());
		if (VarDecl *vardecl = dyn_cast<VarDecl>(decl))
		{
			if (vardecl->getType()->isCharType())
				return getChar(mVars[decl]);
			else if (vardecl->getType()->isIntegerType())
				return getInt(mVars[decl]);
			else if (vardecl->getType()->isPointerType())
				return getInt(mVars[decl]);
		}
		assert(false);
	}
};

class StackFrame
{
	/// StackFrame maps Variable Declaration to Value
	/// Which are either integer or addresses (also represented using an Integer value)
	std::map<Decl *, int> mVars;
	std::map<Stmt *, int> mExprs;
	std::vector<char> mValues;
	/// The current stmt
	Stmt *mPC;
	Heap *mHeap;

public:
	StackFrame(Heap *mHeap) : mVars(), mExprs(), mValues(), mPC(), mHeap(mHeap)
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

	bool hasDeclVal(Decl *decl)
	{
		return mVars.find(decl) != mVars.end();
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

	bool hasStmtVal(Stmt *stmt)
	{
		return mExprs.find(stmt) != mExprs.end();
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

	int Malloc(int size)
	{
		int addr = mValues.size();
		mValues.resize(addr + size);
		return addr;
	}

	void Update(int addr, int val)
	{
		*(int *)&mValues[addr] = val;
	}

	int get(int addr)
	{
		return *(int *)&mValues[addr];
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
					else if (CharacterLiteral *literal = dyn_cast<CharacterLiteral>(expr))
					{
						int val = literal->getValue();
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

	void charliteral(CharacterLiteral *literal)
	{
		mStack.back().bindStmt(literal, literal->getValue());
	}

	void unop(UnaryOperator *uop)
	{
		Expr *expr = uop->getSubExpr();
		int val = mStack.back().getStmtVal(expr);
		if (uop->getOpcode() == UO_Minus)
			val = -val;
		else if (uop->getOpcode() == UO_Deref)
		{
			assert(expr->getType()->isPointerType());
			if (expr->getType()->getPointeeType()->isCharType())
				val = mHeap.getChar(val);
			else if (expr->getType()->getPointeeType()->isIntegerType())
				val = mHeap.getInt(val);
			else if (expr->getType()->getPointeeType()->isPointerType())
				val = mHeap.getInt(val);
		}
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
			while (isa<ParenExpr>(left))
			{
				ParenExpr *paren = dyn_cast<ParenExpr>(left);
				left = paren->getSubExpr();
			}
			if (DeclRefExpr *declexpr = dyn_cast<DeclRefExpr>(left))
			{
				Decl *decl = declexpr->getFoundDecl();
				mStack.back().bindDecl(decl, val);
			}
			else if (ArraySubscriptExpr *arrsub = dyn_cast<ArraySubscriptExpr>(left))
			{
				Expr *base = arrsub->getBase();
				Expr *idx = arrsub->getIdx();
				int addr = mStack.back().getStmtVal(base);
				int idxval = mStack.back().getStmtVal(idx);
				mStack.back().Update(addr + idxval * sizeof(int), val);
			}
			else if (UnaryOperator *unaryop = dyn_cast<UnaryOperator>(left))
			{
				assert(unaryop->getOpcode() == UO_Deref);
				Expr *expr = unaryop->getSubExpr();
				int addr = mStack.back().getStmtVal(expr);
				if (expr->getType()->getPointeeType()->isCharType())
					mHeap.Update(addr, (char)val);
				else if (expr->getType()->getPointeeType()->isIntegerType())
					mHeap.Update(addr, val);
				else if (expr->getType()->getPointeeType()->isPointerType())
					mHeap.Update(addr, val);
			}
		}
		else
		{
			while (isa<ParenExpr>(left))
			{
				ParenExpr *paren = dyn_cast<ParenExpr>(left);
				left = paren->getSubExpr();
			}
			while (isa<ParenExpr>(right))
			{
				ParenExpr *paren = dyn_cast<ParenExpr>(right);
				right = paren->getSubExpr();
			}
			if (bop->isAdditiveOp())
			{
				int leftval = mStack.back().getStmtVal(left);
				int rightval = mStack.back().getStmtVal(right);
				if (left->getType()->isPointerType())
				{
					if (left->getType()->getPointeeType()->isCharType())
						rightval *= sizeof(char);
					else if (left->getType()->getPointeeType()->isIntegerType())
						rightval *= sizeof(int);
					else if (left->getType()->getPointeeType()->isPointerType())
						rightval *= sizeof(int *);
				}
				else if (right->getType()->isPointerType())
				{
					if (right->getType()->getPointeeType()->isCharType())
						leftval *= sizeof(char);
					else if (right->getType()->getPointeeType()->isIntegerType())
						leftval *= sizeof(int);
					else if (right->getType()->getPointeeType()->isPointerType())
						leftval *= sizeof(int *);
				}

				if (bop->getOpcode() == BO_Add)
					val = leftval + rightval;
				else if (bop->getOpcode() == BO_Sub)
					val = leftval - rightval;
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
				if (vardecl->getType()->isCharType())
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
				else if (vardecl->getType()->isIntegerType())
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
				else if (const VariableArrayType *vararrtype = dyn_cast<VariableArrayType>(vardecl->getType()))
				{
					Expr *expr = vararrtype->getSizeExpr();
					int size = mStack.back().getStmtVal(expr);
					int addr = mStack.back().Malloc(size * sizeof(int));
					mStack.back().initDecl(vardecl, addr);
				}
				else if (const ConstantArrayType *constarrtype = dyn_cast<ConstantArrayType>(vardecl->getType()))
				{
					int size = constarrtype->getSize().getSExtValue();
					int addr = mStack.back().Malloc(size * sizeof(int));
					mStack.back().initDecl(vardecl, addr);
				}
				else if (vardecl->getType()->isPointerType())
				{
					if (vardecl->hasInit())
					{
						Expr *expr = vardecl->getInit();
						int addr = mStack.back().getStmtVal(expr);
						mStack.back().initDecl(vardecl, addr);
					}
					else
						mStack.back().initDecl(vardecl, 0);
				}
			}
		}
	}

	void declref(DeclRefExpr *declref)
	{
		mStack.back().setPC(declref);
		if (declref->getType()->isCharType())
		{
			Decl *decl = declref->getFoundDecl();
			int val = mStack.back().getDeclVal(decl);
			mStack.back().bindStmt(declref, val);
		}
		else if (declref->getType()->isIntegerType())
		{
			Decl *decl = declref->getFoundDecl();
			int val = mStack.back().getDeclVal(decl);
			mStack.back().bindStmt(declref, val);
		}
		else if (declref->getType()->isArrayType())
		{
			Decl *decl = declref->getFoundDecl();
			int addr = mStack.back().getDeclVal(decl);
			mStack.back().bindStmt(declref, addr);
		}
		else if (declref->getType()->isPointerType())
		{
			Decl *decl = declref->getFoundDecl();
			if (mStack.back().hasDeclVal(decl))
			{
				int addr = mStack.back().getDeclVal(decl);
				mStack.back().bindStmt(declref, addr);
			}
		}
	}

	void cast(CastExpr *castexpr)
	{
		mStack.back().setPC(castexpr);
		if (castexpr->getType()->isCharType())
		{
			Expr *expr = castexpr->getSubExpr();
			int val = mStack.back().getStmtVal(expr);
			mStack.back().bindStmt(castexpr, val);
		}
		else if (castexpr->getType()->isIntegerType())
		{
			Expr *expr = castexpr->getSubExpr();
			int val = mStack.back().getStmtVal(expr);
			mStack.back().bindStmt(castexpr, val);
		}
		else if (castexpr->getType()->isPointerType())
		{
			Expr *expr = castexpr->getSubExpr();
			if (mStack.back().hasStmtVal(expr))
			{
				int addr = mStack.back().getStmtVal(expr);
				mStack.back().bindStmt(castexpr, addr);
			}
		}
	}

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
		else if (callee == mMalloc)
		{
			Expr *expr = callexpr->getArg(0);
			val = mStack.back().getStmtVal(expr);
			int addr = mHeap.Malloc(val);
			mStack.back().bindStmt(callexpr, addr);
			return nullptr;
		}
		else if (callee == mFree)
		{
			Expr *expr = callexpr->getArg(0);
			val = mStack.back().getStmtVal(expr);
			mHeap.Free(val);
			return nullptr;
		}
		else
		{
			/// You could add your code here for Function call Return
			callee = callee->getDefinition();
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

	void arrsub(ArraySubscriptExpr *arrsub)
	{
		Expr *base = arrsub->getBase();
		Expr *idx = arrsub->getIdx();
		int addr = mStack.back().getStmtVal(base);
		int idxval = mStack.back().getStmtVal(idx);
		int val = mStack.back().get(addr + idxval * sizeof(int));
		mStack.back().bindStmt(arrsub, val);
	}

	void uettop(UnaryExprOrTypeTraitExpr *expr)
	{
		if (expr->getArgumentType()->isCharType())
		{
			int val = sizeof(char);
			mStack.back().bindStmt(expr, val);
		}
		else if (expr->getArgumentType()->isIntegerType())
		{
			int val = sizeof(int);
			mStack.back().bindStmt(expr, val);
		}
		else if (expr->getArgumentType()->isPointerType())
		{
			int val = sizeof(int *);
			mStack.back().bindStmt(expr, val);
		}
		else if (const VariableArrayType *vararrtype = dyn_cast<VariableArrayType>(expr->getArgumentType()))
		{
			Expr *szexpr = vararrtype->getSizeExpr();
			int size = mStack.back().getStmtVal(szexpr);
			int val = size * sizeof(int);
			mStack.back().bindStmt(expr, val);
		}
		else if (const ConstantArrayType *constarrtype = dyn_cast<ConstantArrayType>(expr->getArgumentType()))
		{
			int size = constarrtype->getSize().getSExtValue();
			int val = size * sizeof(int);
			mStack.back().bindStmt(expr, val);
		}
	}

	void paren(ParenExpr *paren)
	{
		Expr *expr = paren->getSubExpr();
		int val = mStack.back().getStmtVal(expr);
		mStack.back().bindStmt(paren, val);
	}

	int getStmtVal(Stmt *stmt)
	{
		return mStack.back().getStmtVal(stmt);
	}
};
