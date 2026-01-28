#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/Tooling.h"

#include <unordered_set>

#include "RefactorTool.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

// Метод run вызывается для каждого совпадения с матчем.
// Мы проверяем тип совпадения по bind-именам и применяем рефакторинг.
void RefactorHandler::run(const MatchFinder::MatchResult &Result) {
    auto &Diag = Result.Context->getDiagnostics();
    auto &SM = *Result.SourceManager;  // Получаем SourceManager для проверки isInMainFile

    if (const auto *Dtor = Result.Nodes.getNodeAs<CXXDestructorDecl>("nonVirtualDtor")) {
        handle_nv_dtor(Dtor, Diag, SM);
    }

    if (const auto *Method = Result.Nodes.getNodeAs<CXXMethodDecl>("missingOverride");
        Method && Method->size_overridden_methods() > 0 && !Method->hasAttr<OverrideAttr>()) {
        handle_miss_override(Method, Diag, SM);
    }

    if (const auto *LoopVar = Result.Nodes.getNodeAs<VarDecl>("loopVar")) {
        handle_crange_for(LoopVar, Diag, SM);
    }
}

void RefactorHandler::handle_nv_dtor(const CXXDestructorDecl *Dtor, DiagnosticsEngine &Diag, SourceManager &SM) {
    if (!SM.isInMainFile(Dtor->getLocation())) {
        return;
    }

    const auto Loc = Dtor->getLocation();
    if (Loc.isInvalid()) {
        return;
    }

    if (!virtualDtorLocations.emplace(SM.getFileOffset(Loc)).second) {
        return;
    }

    Rewrite.InsertTextBefore(Loc, "virtual ");

    const unsigned DiagID = Diag.getCustomDiagID(DiagnosticsEngine::Remark, "Объявлен деструктор");
    Diag.Report(Dtor->getLocation(), DiagID);
}

void RefactorHandler::handle_miss_override(const CXXMethodDecl *Method, DiagnosticsEngine &Diag, SourceManager &SM) {
    if (!SM.isInMainFile(Method->getLocation())) {
        return;
    }

    const auto Loc = SM.getFileLoc(Method->getTypeSpecEndLoc());
    if (Loc.isInvalid()) {
        return;
    }

    Rewrite.InsertTextAfterToken(Loc, " override");

    const unsigned DiagID = Diag.getCustomDiagID(DiagnosticsEngine::Remark, "Объявлен метод");
    Diag.Report(Method->getLocation(), DiagID);
}

void RefactorHandler::handle_crange_for(const VarDecl *LoopVar, DiagnosticsEngine &Diag, SourceManager &SM) {
    if (!SM.isInMainFile(LoopVar->getLocation())) {
        return;
    }

    const auto Loc = SM.getFileLoc(LoopVar->getLocation());
    if (Loc.isInvalid()) {
        return;
    }

    Rewrite.InsertTextBefore(Loc, "&");

    const unsigned DiagID = Diag.getCustomDiagID(DiagnosticsEngine::Remark, "Объявлена переменная");
    Diag.Report(LoopVar->getLocation(), DiagID);
}

auto NvDtorMatcher() {
    return cxxRecordDecl(hasAnyBase(hasType(
        cxxRecordDecl(has(cxxDestructorDecl(unless(isVirtual()), unless(isImplicit())).bind("nonVirtualDtor"))))));
}

auto NoOverrideMatcher() {
    return cxxMethodDecl(isVirtual(), unless(hasAttr(attr::Override)), unless(isImplicit()),
                         unless(cxxDestructorDecl()))
        .bind("missingOverride");
}

auto NoRefConstVarInRangeLoopMatcher() {
    return cxxForRangeStmt(
        hasLoopVariable(varDecl(hasType(qualType(isConstQualified(), unless(referenceType()), unless(builtinType()))))
                            .bind("loopVar")));
}

// Конструктор принимает Rewriter для изменения кода.
ComplexConsumer::ComplexConsumer(Rewriter &Rewrite) : Handler(Rewrite) {
    // Создаем MatchFinder и добавляем матчеры.
    Finder.addMatcher(NvDtorMatcher(), &Handler);
    Finder.addMatcher(NoOverrideMatcher(), &Handler);
    Finder.addMatcher(NoRefConstVarInRangeLoopMatcher(), &Handler);
}

// Метод HandleTranslationUnit вызывается для каждого файла.
void ComplexConsumer::HandleTranslationUnit(ASTContext &Context) { Finder.matchAST(Context); }

std::unique_ptr<ASTConsumer> CodeRefactorAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    RewriterForCodeRefactor.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<ComplexConsumer>(RewriterForCodeRefactor);
}

bool CodeRefactorAction::BeginSourceFileAction(CompilerInstance &CI) {
    // Инициализируем Rewriter для рефакторинга.
    RewriterForCodeRefactor.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return true;  // Возвращаем true, чтобы продолжить обработку файла.
}

void CodeRefactorAction::EndSourceFileAction() {
    // Применяем изменения в файле.
    if (RewriterForCodeRefactor.overwriteChangedFiles()) {
        llvm::errs() << "Error applying changes to files.\n";
    }
}
