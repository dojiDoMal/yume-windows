/**
 * @file interpreter.hpp
 * @brief Interpretador tree-walking do YumeScript e o Environment de escopos.
 *
 * O Interpreter percorre a AST produzida pelo Parser e a executa diretamente
 * (sem bytecode — ver DESIGN.md, Fase 1). Mantém um escopo global onde ficam as
 * variáveis/funções de topo do script e os valores injetados pelo host (ex.:
 * `transform`, `deltaTime`, `log`). Depois de carregar um script, o host pode
 * chamar funções definidas nele por nome, como `start()` e `update(dt)`.
 */
#ifndef YUMESCRIPT_INTERPRETER_HPP
#define YUMESCRIPT_INTERPRETER_HPP

#include "yumescript/ast.hpp"
#include "yumescript/value.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace yumescript {

/// @brief Erro em tempo de execução (variável indefinida, tipo inválido, etc.).
class RuntimeError : public std::runtime_error {
  public:
    RuntimeError(const std::string& msg, int line)
        : std::runtime_error(msg), line(line) {}
    int line; ///< Linha aproximada do erro no script.
};

/**
 * @brief Escopo de variáveis encadeado.
 *
 * Cada chamada de função e cada bloco cria um Environment filho que aponta para
 * o pai (enclosing). A busca por um nome sobe a cadeia até o escopo global.
 */
class Environment {
  public:
    explicit Environment(std::shared_ptr<Environment> enclosing = nullptr)
        : enclosing(std::move(enclosing)) {}

    /** @brief Declara (ou redeclara) uma variável no escopo atual. */
    void define(const std::string& name, const Value& value);
    /** @brief Lê uma variável, subindo a cadeia. Lança se não existir. */
    Value get(const std::string& name, int line) const;
    /** @brief Atribui a uma variável existente. Lança se não existir. */
    void assign(const std::string& name, const Value& value, int line);
    /** @brief Indica se o nome existe neste escopo ou em algum ancestral. */
    bool has(const std::string& name) const;

  private:
    std::shared_ptr<Environment> enclosing;
    std::unordered_map<std::string, Value> values;
};

/**
 * @brief Interpretador tree-walking.
 *
 * Uso típico:
 * @code
 * Interpreter interp;
 * interp.defineGlobal("log", Value::makeNative(...));
 * interp.run(program);          // executa statements de topo (declara funções)
 * if (interp.hasFunction("update"))
 *     interp.callFunction("update", { Value::makeNumber(dt) });
 * @endcode
 */
class Interpreter : public ExprVisitor, public StmtVisitor {
  public:
    Interpreter();

    /** @brief Injeta um valor no escopo global (API nativa, transform, etc.). */
    void defineGlobal(const std::string& name, const Value& value);

    /** @brief Executa os statements de topo do programa (declara let/function). */
    void run(const Program& program);

    /** @brief Indica se o escopo global tem uma função chamável com esse nome. */
    bool hasFunction(const std::string& name) const;

    /** @brief Chama uma função global pelo nome com os argumentos dados. */
    Value callFunction(const std::string& name, std::vector<Value> args, int line = 0);

    /** @brief Chama um Value chamável (função de script ou nativa). */
    Value call(const Value& callee, std::vector<Value>& args, int line);

  private:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;

    // Resultado da última expressão avaliada (os visitors escrevem aqui).
    Value result;
    // Sinaliza um 'return' em andamento, carregando o valor retornado.
    bool returning = false;
    Value returnValue;

    Value evaluate(const Expr& expr);
    void execute(const Stmt& stmt);
    void executeBlock(const Block& block, std::shared_ptr<Environment> env);

    // Alvos de atribuição.
    void assignTo(const Expr& target, const Value& value, int line);

    // ExprVisitor
    void visitLiteral(const LiteralExpr&) override;
    void visitIdentifier(const IdentifierExpr&) override;
    void visitUnary(const UnaryExpr&) override;
    void visitBinary(const BinaryExpr&) override;
    void visitCall(const CallExpr&) override;
    void visitMember(const MemberExpr&) override;
    void visitAssign(const AssignExpr&) override;

    // StmtVisitor
    void visitLet(const LetStmt&) override;
    void visitFunction(const FunctionStmt&) override;
    void visitIf(const IfStmt&) override;
    void visitWhile(const WhileStmt&) override;
    void visitReturn(const ReturnStmt&) override;
    void visitExpr(const ExprStmt&) override;
};

} // namespace yumescript

#endif // YUMESCRIPT_INTERPRETER_HPP
