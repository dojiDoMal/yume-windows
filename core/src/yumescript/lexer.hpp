/**
 * @file lexer.hpp
 * @brief Tokenizador do YumeScript, com rastreio de indentação estilo Python.
 *
 * Converte o texto de um arquivo .ys numa sequência de Tokens. Mantém uma pilha
 * de níveis de indentação para emitir INDENT/DEDENT nas trocas de bloco e
 * NEWLINE ao fim de cada linha lógica. Linhas em branco e comentários (iniciados
 * por `#`) não geram NEWLINE nem afetam a indentação.
 */
#ifndef YUMESCRIPT_LEXER_HPP
#define YUMESCRIPT_LEXER_HPP

#include "yumescript/token.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace yumescript {

/// @brief Erro de análise léxica (ex.: string não terminada, char inválido).
class LexError : public std::runtime_error {
  public:
    LexError(const std::string& msg, int line, int column)
        : std::runtime_error(msg), line(line), column(column) {}
    int line;   ///< Linha do erro.
    int column; ///< Coluna do erro.
};

/**
 * @brief Lexer do YumeScript.
 *
 * Uso: construa com o fonte e chame tokenize(), que devolve todos os tokens
 * terminando em END_OF_FILE. Lança LexError em entrada malformada.
 */
class Lexer {
  public:
    explicit Lexer(std::string source);

    /** @brief Tokeniza todo o fonte. @return Vetor de tokens (termina em EOF). */
    std::vector<Token> tokenize();

  private:
    std::string source;  ///< Texto-fonte completo.
    size_t pos = 0;      ///< Índice do próximo caractere a consumir.
    int line = 1;        ///< Linha atual (1-based).
    int column = 1;      ///< Coluna atual (1-based).

    std::vector<Token> tokens;    ///< Tokens acumulados.
    std::vector<int> indentStack; ///< Pilha de níveis de indentação (começa com 0).

    bool atEnd() const;
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);

    void lexLine();                 ///< Lê o conteúdo de uma linha lógica.
    int consumeIndentation();       ///< Mede a indentação no início da linha.
    void emitIndentation(int width);///< Emite INDENT/DEDENT comparando com a pilha.
    void lexToken();                ///< Lê um único token dentro de uma linha.
    void lexNumber();
    void lexString();
    void lexIdentifier();

    void add(TokenType type, const std::string& lexeme, int ln, int col);
};

} // namespace yumescript

#endif // YUMESCRIPT_LEXER_HPP
