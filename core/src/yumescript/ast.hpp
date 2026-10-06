/**
 * @file ast.hpp
 * @brief Nós da árvore de sintaxe abstrata (AST) do YumeScript.
 *
 * O parser produz uma AST de posse exclusiva (std::unique_ptr). Os nós são
 * visitados por um padrão Visitor (ExprVisitor/StmtVisitor), para que o
 * interpretador e, futuramente (Fase 2), um emissor de bytecode possam percorrer
 * a mesma árvore sem a árvore saber quem a consome.
 *
 * Divisão em duas hierarquias:
 *  - Expr: produz um valor (literais, binárias, chamadas, acesso a membro...).
 *  - Stmt: executa um efeito (let, function, if, while, return, expressão solta).
 */
#ifndef YUMESCRIPT_AST_HPP
#define YUMESCRIPT_AST_HPP

#include "yumescript/token.hpp"

#include <memory>
#include <string>
#include <vector>

namespace yumescript {

// Forward declarations das expressões.
struct LiteralExpr;
struct IdentifierExpr;
struct UnaryExpr;
struct BinaryExpr;
struct CallExpr;
struct MemberExpr;
struct AssignExpr;

/// @brief Visitante das expressões. Implementado pelo interpretador.
struct ExprVisitor {
    virtual ~ExprVisitor() = default;
    virtual void visitLiteral(const LiteralExpr&) = 0;
    virtual void visitIdentifier(const IdentifierExpr&) = 0;
    virtual void visitUnary(const UnaryExpr&) = 0;
    virtual void visitBinary(const BinaryExpr&) = 0;
    virtual void visitCall(const CallExpr&) = 0;
    virtual void visitMember(const MemberExpr&) = 0;
    virtual void visitAssign(const AssignExpr&) = 0;
};

/// @brief Base de todas as expressões.
struct Expr {
    virtual ~Expr() = default;
    virtual void accept(ExprVisitor& v) const = 0;
};
using ExprPtr = std::unique_ptr<Expr>;

/// @brief Literal: número, string ou booleano.
struct LiteralExpr : Expr {
    enum class Kind { Number, String, Bool, Nil } kind;
    double number = 0.0;
    std::string str;
    bool boolean = false;

    static ExprPtr makeNumber(double n) {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::Number;
        e->number = n;
        return e;
    }
    static ExprPtr makeString(std::string s) {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::String;
        e->str = std::move(s);
        return e;
    }
    static ExprPtr makeBool(bool b) {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::Bool;
        e->boolean = b;
        return e;
    }
    static ExprPtr makeNil() {
        auto e = std::make_unique<LiteralExpr>();
        e->kind = Kind::Nil;
        return e;
    }
    void accept(ExprVisitor& v) const override { v.visitLiteral(*this); }
};

/// @brief Referência a uma variável pelo nome.
struct IdentifierExpr : Expr {
    std::string name;
    int line = 0;
    explicit IdentifierExpr(std::string n, int ln) : name(std::move(n)), line(ln) {}
    void accept(ExprVisitor& v) const override { v.visitIdentifier(*this); }
};

/// @brief Operador unário prefixo (`-x`).
struct UnaryExpr : Expr {
    TokenType op;
    ExprPtr operand;
    int line = 0;
    void accept(ExprVisitor& v) const override { v.visitUnary(*this); }
};

/// @brief Operador binário infixo (`a + b`, `a < b`, `a == b`...).
struct BinaryExpr : Expr {
    TokenType op;
    ExprPtr left;
    ExprPtr right;
    int line = 0;
    void accept(ExprVisitor& v) const override { v.visitBinary(*this); }
};

/// @brief Chamada de função: `callee(args...)`.
struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    int line = 0;
    void accept(ExprVisitor& v) const override { v.visitCall(*this); }
};

/// @brief Acesso a membro: `object.name` (encadeável: `a.b.c`).
struct MemberExpr : Expr {
    ExprPtr object;
    std::string name;
    int line = 0;
    void accept(ExprVisitor& v) const override { v.visitMember(*this); }
};

/**
 * @brief Atribuição a um alvo (`target = value`, `target += value`, ...).
 *
 * O alvo (@c target) é uma IdentifierExpr ou MemberExpr. @c op guarda a forma
 * composta original (ASSIGN para `=`, PLUS_ASSIGN para `+=`, etc.).
 */
struct AssignExpr : Expr {
    ExprPtr target;
    TokenType op;
    ExprPtr value;
    int line = 0;
    void accept(ExprVisitor& v) const override { v.visitAssign(*this); }
};

// --- Statements ---------------------------------------------------------------

struct LetStmt;
struct FunctionStmt;
struct IfStmt;
struct WhileStmt;
struct ReturnStmt;
struct ExprStmt;

/// @brief Visitante dos statements. Implementado pelo interpretador.
struct StmtVisitor {
    virtual ~StmtVisitor() = default;
    virtual void visitLet(const LetStmt&) = 0;
    virtual void visitFunction(const FunctionStmt&) = 0;
    virtual void visitIf(const IfStmt&) = 0;
    virtual void visitWhile(const WhileStmt&) = 0;
    virtual void visitReturn(const ReturnStmt&) = 0;
    virtual void visitExpr(const ExprStmt&) = 0;
};

/// @brief Base de todos os statements.
struct Stmt {
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& v) const = 0;
};
using StmtPtr = std::unique_ptr<Stmt>;

/// @brief Lista de statements (corpo de bloco / programa).
using Block = std::vector<StmtPtr>;

/// @brief Declaração de variável: `let nome = valor`.
struct LetStmt : Stmt {
    std::string name;
    ExprPtr initializer;
    int line = 0;
    void accept(StmtVisitor& v) const override { v.visitLet(*this); }
};

/// @brief Declaração de função: `function nome param1 param2: <bloco>`.
struct FunctionStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    Block body;
    int line = 0;
    void accept(StmtVisitor& v) const override { v.visitFunction(*this); }
};

/// @brief Condicional: `if cond: <bloco>` com `else` opcional.
struct IfStmt : Stmt {
    ExprPtr condition;
    Block thenBranch;
    Block elseBranch; ///< Vazio quando não há else.
    bool hasElse = false;
    int line = 0;
    void accept(StmtVisitor& v) const override { v.visitIf(*this); }
};

/// @brief Laço: `while cond: <bloco>`.
struct WhileStmt : Stmt {
    ExprPtr condition;
    Block body;
    int line = 0;
    void accept(StmtVisitor& v) const override { v.visitWhile(*this); }
};

/// @brief Retorno de função: `return` ou `return expr`.
struct ReturnStmt : Stmt {
    ExprPtr value; ///< nullptr quando `return` sem valor.
    int line = 0;
    void accept(StmtVisitor& v) const override { v.visitReturn(*this); }
};

/// @brief Expressão usada como statement (ex.: uma chamada `log("oi")`).
struct ExprStmt : Stmt {
    ExprPtr expr;
    int line = 0;
    void accept(StmtVisitor& v) const override { v.visitExpr(*this); }
};

/// @brief Raiz da AST: o programa é um bloco de statements de topo.
struct Program {
    Block statements;
};

} // namespace yumescript

#endif // YUMESCRIPT_AST_HPP
