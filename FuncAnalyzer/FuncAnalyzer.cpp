/* Simple LibTool that analyzes functions in the source file. */
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Error.h"
#include <map>
#include <set>
#include <vector>

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory MyToolCategory("Function Analyzer options");

class BodyVisitor : public RecursiveASTVisitor<BodyVisitor> {
  ASTContext &Context;
public:
  unsigned numStatements = 0;
  unsigned numLoops = 0;
  explicit BodyVisitor(ASTContext &Context) : Context(Context) {}

  bool TraverseLambdaExpr(LambdaExpr *) { return true; }
  bool TraverseFunctionDecl(FunctionDecl *) { return true; }

  bool VisitStmt(Stmt *S) {
    if (isa<ForStmt>(S) || isa<WhileStmt>(S) || isa<DoStmt>(S) ||
        isa<CXXForRangeStmt>(S))
      numLoops++;
    if (isa<CompoundStmt>(S)) return true;
    if (!isa<Expr>(S)) {
      numStatements++;
      return true;
    }
    // Count expressions only when they are used as statements.
    for (const auto &P : Context.getParents(*S)) {
      const Stmt *Parent = P.get<Stmt>();
      if (!Parent) continue;
      bool isStatement = isa<CompoundStmt>(Parent);
      if (auto *I = dyn_cast<IfStmt>(Parent))
        isStatement = I->getThen() == S || I->getElse() == S;
      if (auto *F = dyn_cast<ForStmt>(Parent))
        isStatement = F->getBody() == S;
      if (auto *W = dyn_cast<WhileStmt>(Parent)) isStatement = W->getBody() == S;
      if (auto *D = dyn_cast<DoStmt>(Parent)) isStatement = D->getBody() == S;
      if (auto *R = dyn_cast<CXXForRangeStmt>(Parent)) isStatement = R->getBody() == S;
      if (auto *L = dyn_cast<LabelStmt>(Parent)) isStatement = L->getSubStmt() == S;
      if (auto *C = dyn_cast<CaseStmt>(Parent)) isStatement = C->getSubStmt() == S;
      if (auto *D = dyn_cast<DefaultStmt>(Parent)) isStatement = D->getSubStmt() == S;
      if (isStatement) { numStatements++; break; }
    }
    return true;
  }
};

class FuncAnalysisVisitor : public RecursiveASTVisitor<FuncAnalysisVisitor> {
  ASTContext *astContext;
  vector<FunctionDecl *> functions;
  set<const FunctionDecl *> seen;
  map<const FunctionDecl *, unsigned> calls;
public:
  explicit FuncAnalysisVisitor(CompilerInstance *CI)
      : astContext(&(CI->getASTContext())) {}

  bool inMainFile(SourceLocation Loc) {
    auto &SM = astContext->getSourceManager();
    return Loc.isValid() && SM.isWrittenInMainFile(SM.getExpansionLoc(Loc));
  }

  bool VisitFunctionDecl(FunctionDecl *func) {
    if (!func->isThisDeclarationADefinition() || !func->getBody() ||
        func->isImplicit() || !inMainFile(func->getLocation())) return true;
    if (seen.insert(func->getCanonicalDecl()).second) functions.push_back(func);
    return true;
  }

  bool VisitCallExpr(CallExpr *call) {
    if (inMainFile(call->getExprLoc()))
      if (auto *func = call->getDirectCallee()) calls[func->getCanonicalDecl()]++;
    return true;
  }

  void printResults() {
    for (auto *func : functions) {
      BodyVisitor body(*astContext);
      body.TraverseStmt(func->getBody());
      llvm::outs() << "Function name: " << func->getQualifiedNameAsString() << "\n"
                   << "Number of arguments: " << func->getNumParams() << "\n"
                   << "Number of statements: " << body.numStatements << "\n"
                   << "Number of loops: " << body.numLoops << "\n"
                   << "Number of calls: " << calls[func->getCanonicalDecl()] << "\n\n";
    }
    llvm::outs() << "Number of functions encountered " << functions.size() << "\n";
  }
};

class FuncAnalysisASTConsumer : public ASTConsumer {
  FuncAnalysisVisitor visitor;
public:
  explicit FuncAnalysisASTConsumer(CompilerInstance *CI) : visitor(CI) {}
  void HandleTranslationUnit(ASTContext &Context) override {
    visitor.TraverseDecl(Context.getTranslationUnitDecl());
    visitor.printResults();
  }
};

class FuncAnalysisFrontendAction : public ASTFrontendAction {
public:
  unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
    return make_unique<FuncAnalysisASTConsumer>(&CI);
  }
};

int main(int argc, const char **argv) {
  auto op = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!op) { llvm::errs() << llvm::toString(op.takeError()) << "\n"; return 1; }
  ClangTool Tool(op->getCompilations(), op->getSourcePathList());
  Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(
      "-resource-dir=" CLANG_RESOURCE_DIR,
      ArgumentInsertPosition::BEGIN));

  return Tool.run(newFrontendActionFactory<FuncAnalysisFrontendAction>().get());
}
