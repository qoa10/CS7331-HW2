/* Replace integer multiplication and division by powers of two with shifts. */
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Error.h"
#include <algorithm>
#include <vector>

using namespace std;
using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory MyToolCategory("Strength Reduction options");

class StrengthReductionVisitor : public RecursiveASTVisitor<StrengthReductionVisitor> {
  ASTContext &Context;
  Rewriter &Rewrite;
  struct Edit { CharSourceRange range; string replacement; unsigned begin, end; };
  vector<Edit> edits;

  bool nonnegative(const Expr *E) {
    if (E->getType()->isUnsignedIntegerType()) return true;
    Expr::EvalResult R;
    return E->EvaluateAsInt(R, Context) && !R.Val.getInt().isNegative();
  }
public:
  StrengthReductionVisitor(ASTContext &C, Rewriter &R) : Context(C), Rewrite(R) {}

  bool VisitBinaryOperator(BinaryOperator *B) {
    if (B->getOpcode() != BO_Mul && B->getOpcode() != BO_Div) return true;
    if (!B->getType()->isIntegerType() || B->getType()->isBooleanType()) return true;
    auto &SM = Context.getSourceManager();
    if (B->getBeginLoc().isMacroID() || B->getEndLoc().isMacroID() ||
        B->getOperatorLoc().isMacroID() || !SM.isWrittenInMainFile(B->getBeginLoc()) ||
        !SM.isWrittenInMainFile(B->getEndLoc())) return true;
    const Expr *value = B->getLHS();
    const Expr *constant = B->getRHS();
    auto power = [&](const Expr *E, unsigned &shift) {
      if (!isa<IntegerLiteral>(E->IgnoreParenImpCasts()))
        return false;
      Expr::EvalResult R;
      if (!E->EvaluateAsInt(R, Context) || E->HasSideEffects(Context)) return false;
      const auto &N = R.Val.getInt();
      if (N.isNegative() || !N.isPowerOf2()) return false;
      shift = N.logBase2();
      return shift < Context.getIntWidth(B->getType());
    };
    unsigned shift;
    if (!power(constant, shift)) {
      if (B->getOpcode() != BO_Mul || !power(value, shift)) return true;
      swap(value, constant);
    }
    if (shift == 0) return true;
    bool signedResult = B->getType()->isSignedIntegerType();
    bool needsCorrection = signedResult && !nonnegative(value);

    if (B->getOpcode() == BO_Div && needsCorrection &&
        value->HasSideEffects(Context)) return true;
    auto range = CharSourceRange::getTokenRange(B->getSourceRange());
    auto operandRange = CharSourceRange::getTokenRange(value->IgnoreParenImpCasts()->getSourceRange());
    bool invalid = false;
    auto operand = Lexer::getSourceText(operandRange, SM, Context.getLangOpts(), &invalid);
    if (invalid || operand.empty()) return true;
    auto end = Lexer::getLocForEndOfToken(B->getEndLoc(), 0, SM, Context.getLangOpts());
    if (end.isInvalid()) return true;
    string type = B->getType().getCanonicalType().getUnqualifiedType().getAsString();
    const Expr *original = value->IgnoreParenImpCasts();
    string text = operand.str();
    QualType promoted = original->getType().getCanonicalType().getUnqualifiedType();
    if (Context.isPromotableIntegerType(promoted))
      promoted = Context.getPromotedIntegerType(promoted);
    if (!Context.hasSameType(promoted, B->getType().getCanonicalType().getUnqualifiedType()))
      text = "(" + type + ")(" + text + ")";
    else if (!isa<DeclRefExpr>(original) && !isa<IntegerLiteral>(original))
      text = "(" + text + ")";
    string replacement = "(" + text +
                         (B->getOpcode() == BO_Mul ? " << " : " >> ") +
                         to_string(shift) + ")";
    if (needsCorrection) {
      if (B->getOpcode() == BO_Mul) {
        QualType unsignedType =
            Context.getCorrespondingUnsignedType(B->getType());
        replacement = "((" + type + ")((" +
            unsignedType.getAsString() + ")(" + text + ") << " +
            to_string(shift) + "))";
      } else {
        string mask = "(((" + type + ")1 << " +
            to_string(shift) + ") - 1)";
        replacement = "((" + text + " + ((" + text +
            " < 0) ? " + mask + " : 0)) >> " +
            to_string(shift) + ")";
      }
    }
    edits.push_back({range, replacement, SM.getFileOffset(B->getBeginLoc()), SM.getFileOffset(end)});
    return true;
  }

  void applyEdits() {
    // Apply inner expressions first; skip overlapping edits in this pass.
    std::sort(edits.begin(), edits.end(), [](const Edit &A, const Edit &B) {
      return A.end - A.begin < B.end - B.begin;
    });
    vector<Edit> applied;
    for (const auto &E : edits) {
      bool overlap = false;
      for (const auto &A : applied)
        if (E.begin < A.end && A.begin < E.end) { overlap = true; break; }
      if (!overlap && !Rewrite.ReplaceText(E.range, E.replacement)) applied.push_back(E);
    }
  }
};

class StrengthReductionASTConsumer : public ASTConsumer {
  StrengthReductionVisitor visitor;
public:
  StrengthReductionASTConsumer(ASTContext &C, Rewriter &R) : visitor(C, R) {}
  void HandleTranslationUnit(ASTContext &Context) override {
    visitor.TraverseDecl(Context.getTranslationUnitDecl());
    visitor.applyEdits();
  }
};

class StrengthReductionFrontendAction : public ASTFrontendAction {
  Rewriter Rewrite;
public:
  unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
    Rewrite.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return make_unique<StrengthReductionASTConsumer>(CI.getASTContext(), Rewrite);
  }
  void EndSourceFileAction() override {
    Rewrite.getEditBuffer(Rewrite.getSourceMgr().getMainFileID()).write(llvm::outs());
  }
};

int main(int argc, const char **argv) {
  auto op = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!op) { llvm::errs() << llvm::toString(op.takeError()) << "\n"; return 1; }
  ClangTool Tool(op->getCompilations(), op->getSourcePathList());
  Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(
      "-resource-dir=" CLANG_RESOURCE_DIR,
      ArgumentInsertPosition::BEGIN));

  return Tool.run(newFrontendActionFactory<StrengthReductionFrontendAction>().get());
}
