#include "RefactorTool.h"

#include <gtest/gtest.h>

#include <clang/Tooling/Tooling.h>

#include <string>

using namespace clang;
using namespace clang::tooling;

static std::string RunRefactor(const std::string &Code) {
    const auto AST = buildASTFromCodeWithArgs(Code, {"-std=c++20"});
    if (!AST) {
        return {};
    }

    auto &Ctx = AST->getASTContext();
    auto &SM = Ctx.getSourceManager();
    Rewriter R(SM, Ctx.getLangOpts());

    auto DiagOpts = std::make_unique<DiagnosticOptions>();
    auto Printer = std::make_unique<IgnoringDiagConsumer>();
    Ctx.getDiagnostics().setClient(Printer.release(), true);

    ComplexConsumer consumer(R);
    consumer.HandleTranslationUnit(Ctx);

    if (const auto *Buf = R.getRewriteBufferFor(SM.getMainFileID())) {
        return {Buf->begin(), Buf->end()};
    }
    return Code;
}

TEST(NonVirtualDtor, AddsVirtualToBaseDtor) {
    const auto Code = R"(
        struct Base {
            ~Base() {}
        };

        struct Derived : Base {};
    )";

    const auto Expected = R"(
        struct Base {
            virtual ~Base() {}
        };

        struct Derived : Base {};
    )";

    ASSERT_EQ(RunRefactor(Code), Expected);
}

TEST(NonVirtualDtor, NoChangeIfNoDerivedClasses) {
    const auto Code = R"(
        struct Base {
            ~Base() {}
        };
    )";

    ASSERT_EQ(RunRefactor(Code), Code);
}

TEST(MissingOverride, AddsOverrideKeyword) {
    const auto Code = R"(
        struct Base {
            virtual void foo();
        };

        struct Derived : Base {
            void foo();
        };
    )";

    const auto Expected = R"(
        struct Base {
            virtual void foo();
        };

        struct Derived : Base {
            void foo() override;
        };
    )";

    ASSERT_EQ(RunRefactor(Code), Expected);
}

TEST(MissingOverride, NoDuplicateOverride) {
    const auto Code = R"(
        struct Base {
            virtual void foo();
        };

        struct Derived : Base {
            void foo() override;
        };
    )";

    ASSERT_EQ(RunRefactor(Code), Code);
}

TEST(RangeFor, AddsConstReference) {
    const auto Code = R"(
        #include <vector>
        #include <string>

        void fun(const std::vector<std::string> &v) {
            for (const auto x : v) {
                auto x1 = x.size();
            }
        }
    )";

    const auto Expected = R"(
        #include <vector>
        #include <string>

        void fun(const std::vector<std::string> &v) {
            for (const auto &x : v) {
                auto x1 = x.size();
            }
        }
    )";

    ASSERT_EQ(RunRefactor(Code), Expected);
}

TEST(RangeFor, NoChangeForReferenceOrBuiltin) {
    const auto Code = R"(
        #include <vector>

        void fun(const std::vector<int> &v) {
            for (const int x : v) {
                auto x1 = x;
            }
        }
    )";

    ASSERT_EQ(RunRefactor(Code), Code);
}
