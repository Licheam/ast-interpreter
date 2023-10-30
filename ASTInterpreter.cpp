//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

#include "Environment.h"

class InterpreterVisitor : public EvaluatedExprVisitor<InterpreterVisitor>
{
public:
   explicit InterpreterVisitor(const ASTContext &context, Environment *env)
       : EvaluatedExprVisitor(context), mEnv(env), isReturned(false) {}
   virtual ~InterpreterVisitor() {}

   virtual void VisitIntegerLiteral(IntegerLiteral *literal)
   {
      if (isReturned)
         return;
      mEnv->intliteral(literal);
   }

   virtual void VisitUnaryOperator(UnaryOperator *uop)
   {
      if (isReturned)
         return;
      VisitStmt(uop);
      mEnv->unop(uop);
   }

   virtual void VisitBinaryOperator(BinaryOperator *bop)
   {
      if (isReturned)
         return;
      VisitStmt(bop);
      mEnv->binop(bop);
   }

   virtual void VisitDeclRefExpr(DeclRefExpr *expr)
   {
      if (isReturned)
         return;
      VisitStmt(expr);
      mEnv->declref(expr);
   }

   virtual void VisitCastExpr(CastExpr *expr)
   {
      if (isReturned)
         return;
      VisitStmt(expr);
      mEnv->cast(expr);
   }

   virtual void VisitCallExpr(CallExpr *call)
   {
      if (isReturned)
         return;
      VisitStmt(call);
      if (Stmt *body = mEnv->call(call))
      {
         VisitStmt(body);
         if (!isReturned)
         {
            mEnv->ret(nullptr);
         }
         isReturned = false;
      }
   }

   virtual void VisitDeclStmt(DeclStmt *declstmt)
   {
      if (isReturned)
         return;
      VisitStmt(declstmt);
      mEnv->decl(declstmt);
   }

   virtual void VisitReturnStmt(ReturnStmt *retstmt)
   {
      if (isReturned)
         return;
      VisitStmt(retstmt);
      mEnv->ret(retstmt);
      isReturned = true;
   }

   virtual void VisitIfStmt(IfStmt *ifstmt)
   {
      if (isReturned)
         return;
      Visit(ifstmt->getCond());
      if (mEnv->getStmtVal(ifstmt->getCond()))
      {
         Visit(ifstmt->getThen());
      }
      else if (Stmt *elsestmt = ifstmt->getElse())
      {
         Visit(elsestmt);
      }
   }

   virtual void VisitWhileStmt(WhileStmt *whilestmt)
   {
      if (isReturned)
         return;
      Visit(whilestmt->getCond());
      while (mEnv->getStmtVal(whilestmt->getCond()))
      {
         Visit(whilestmt->getBody());
         if (isReturned)
            return;
         Visit(whilestmt->getCond());
      }
   }

   virtual void VisitForStmt(ForStmt *forstmt)
   {
      if (isReturned)
         return;
      for (Visit(forstmt->getInit()), Visit(forstmt->getCond()); mEnv->getStmtVal(forstmt->getCond()); Visit(forstmt->getInc()), Visit(forstmt->getCond()))
      {
         Visit(forstmt->getBody());
         if (isReturned)
            return;
      }
   }

   virtual void VisitArraySubscriptExpr(ArraySubscriptExpr *expr)
   {
      if (isReturned)
         return;
      VisitStmt(expr);
      mEnv->arrsub(expr);
   }

   virtual void VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *expr)
   {
      if (isReturned)
         return;
      VisitStmt(expr);
      mEnv->uettop(expr);
   }

   virtual void VisitParenExpr(ParenExpr *expr)
   {
      if (isReturned)
         return;
      VisitStmt(expr);
      mEnv->paren(expr);
   }

   virtual void VisitStmt(Stmt *stmt)
   {
      if (isReturned)
         return;
      EvaluatedExprVisitor::VisitStmt(stmt);
   }

   virtual void Visit(Stmt *stmt)
   {
      if (isReturned || !stmt)
         return;
      EvaluatedExprVisitor::Visit(stmt);
   }

private:
   Environment *mEnv;
   bool isReturned;
};

class InterpreterConsumer : public ASTConsumer
{
public:
   explicit InterpreterConsumer(const ASTContext &context) : mEnv(),
                                                             mVisitor(context, &mEnv)
   {
   }
   virtual ~InterpreterConsumer() {}

   virtual void HandleTranslationUnit(clang::ASTContext &Context)
   {
      TranslationUnitDecl *decl = Context.getTranslationUnitDecl();
      mEnv.init(decl);

      FunctionDecl *entry = mEnv.getEntry();
      mVisitor.VisitStmt(entry->getBody());
   }

private:
   Environment mEnv;
   InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public ASTFrontendAction
{
public:
   virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
       clang::CompilerInstance &Compiler, llvm::StringRef InFile)
   {
      return std::unique_ptr<clang::ASTConsumer>(
          new InterpreterConsumer(Compiler.getASTContext()));
   }
};

int main(int argc, char **argv)
{
   if (argc > 1)
   {
      clang::tooling::runToolOnCode(std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction), argv[1]);
   }
}
