/**
 * @file parser.hpp
 * @brief Parser recursivo-descendente do YumeScript.
 *
 * Consome a lista de Tokens produzida pelo Lexer e constrói um Program (AST).
 * Blocos são delimitados por `:` seguido de NEWLINE INDENT ... DEDENT (estilo
 * Python). `function`, `if`, `else` e `while` dispensam parênteses; chamadas de
 * função usam parênteses. Lança ParseError em entrada malformada.
 */
#ifndef YUMESCRIPT_PARSER_HPP
#define YUMESCRIPT_PARSER_HPP

#include "yumescript/ast.hpp"
#include "yumescript/token.hpp"

#include <stdexcept>
#include <vector>

namespace yumescript {

/// @brief Erro de análise sintática.
class ParseError : public std::runtime_error {
  public:
    ParseError(const std::string& msg, int line, int column)
        : std::runtime_error(msg), line(line), column(column) {}
    int line;   ///< Linha do erro.
    int column; ///< Coluna do erro.
};

/// @brief Parser do YumeScript.
class Parser {
  public:
    explicit Parser(std::vector<Token> tokens);

    /** @brief Analisa o programa inteiro. @return A AST raiz. */
    Program parseProgram();

  private:
    std::vector<Token> tokens;
    size_t current = 0;

    // --- helpers de token ---
    const Token& peek() const;
    const Token& previous() const;
    bool atEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& advance();
    const Token& expect(TokenType type, const std::string& what);
    void skipNewlines();

    // --- statements ---
    StmtPtr declaration();
    StmtPtr letStatement();
    StmtPtr functionStatement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr returnStatement();
    StmtPtr expressionStatement();
    Block block(); ///< `: NEWLINE INDENT <stmts> DEDENT`

    // --- expressões (por precedência) ---
    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr primary();

    ExprPtr finishCall(ExprPtr callee);

    [[noreturn]] void error(const Token& tok, const std::string& msg);
};

} // namespace yumescript

#endif // YUMESCRIPT_PARSER_HPP
