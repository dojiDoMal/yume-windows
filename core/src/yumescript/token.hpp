/**
 * @file token.hpp
 * @brief Tipos de token do YumeScript e a struct Token produzida pelo lexer.
 *
 * O lexer converte o texto-fonte de um arquivo .ys numa sequência de Tokens.
 * Além dos tokens usuais (literais, identificadores, operadores), o YumeScript
 * usa blocos por indentação ao estilo Python: o lexer emite NEWLINE ao fim de
 * uma linha lógica e INDENT/DEDENT quando o nível de indentação sobe/desce.
 */
#ifndef YUMESCRIPT_TOKEN_HPP
#define YUMESCRIPT_TOKEN_HPP

#include <string>

namespace yumescript {

/// @brief Categoria de um token.
enum class TokenType {
    // Literais e nomes.
    NUMBER,     ///< Literal numérico (ex.: 45, 3.14).
    STRING,     ///< Literal de string entre aspas duplas.
    IDENTIFIER, ///< Nome de variável/função.

    // Keywords.
    LET,      ///< `let`
    FUNCTION, ///< `function`
    IF,       ///< `if`
    ELSE,     ///< `else`
    WHILE,    ///< `while`
    RETURN,   ///< `return`
    TRUE,     ///< `true`
    FALSE,    ///< `false`
    NIL,      ///< `null`

    // Operadores.
    PLUS,         ///< `+`
    MINUS,        ///< `-`
    STAR,         ///< `*`
    SLASH,        ///< `/`
    ASSIGN,       ///< `=`
    PLUS_ASSIGN,  ///< `+=`
    MINUS_ASSIGN, ///< `-=`
    STAR_ASSIGN,  ///< `*=`
    SLASH_ASSIGN, ///< `/=`
    EQ,           ///< `==`
    NEQ,          ///< `!=`
    LT,           ///< `<`
    GT,           ///< `>`
    LTE,          ///< `<=`
    GTE,          ///< `>=`

    // Pontuação.
    LPAREN, ///< `(`
    RPAREN, ///< `)`
    COMMA,  ///< `,`
    COLON,  ///< `:`
    DOT,    ///< `.`

    // Estrutura de linha/bloco.
    NEWLINE, ///< Fim de uma linha lógica.
    INDENT,  ///< Aumento de nível de indentação (início de bloco).
    DEDENT,  ///< Redução de nível de indentação (fim de bloco).

    END_OF_FILE ///< Fim do arquivo.
};

/// @brief Um token: tipo, lexema original e posição no fonte.
struct Token {
    TokenType type;      ///< Categoria do token.
    std::string lexeme;  ///< Texto exato lido do fonte.
    double number = 0.0; ///< Valor numérico, quando type == NUMBER.
    int line = 0;        ///< Linha (1-based) onde o token começa.
    int column = 0;      ///< Coluna (1-based) onde o token começa.

    Token() : type(TokenType::END_OF_FILE) {}
    Token(TokenType t, std::string lex, int ln, int col)
        : type(t), lexeme(std::move(lex)), line(ln), column(col) {}
};

/// @brief Nome legível de um TokenType (para mensagens de erro/depuração).
const char* tokenTypeName(TokenType type);

} // namespace yumescript

#endif // YUMESCRIPT_TOKEN_HPP
