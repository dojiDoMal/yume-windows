/**
 * @file lexer.cpp
 * @brief Implementação do Lexer do YumeScript.
 */
#include "yumescript/lexer.hpp"

#include "yumescript/token.hpp"

#include <cctype>
#include <unordered_map>

namespace yumescript {

const char* tokenTypeName(TokenType type) {
    switch (type) {
    case TokenType::NUMBER:
        return "NUMBER";
    case TokenType::STRING:
        return "STRING";
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::LET:
        return "LET";
    case TokenType::FUNCTION:
        return "FUNCTION";
    case TokenType::IF:
        return "IF";
    case TokenType::ELSE:
        return "ELSE";
    case TokenType::WHILE:
        return "WHILE";
    case TokenType::RETURN:
        return "RETURN";
    case TokenType::TRUE:
        return "TRUE";
    case TokenType::FALSE:
        return "FALSE";
    case TokenType::NIL:
        return "NIL";
    case TokenType::PLUS:
        return "PLUS";
    case TokenType::MINUS:
        return "MINUS";
    case TokenType::STAR:
        return "STAR";
    case TokenType::SLASH:
        return "SLASH";
    case TokenType::ASSIGN:
        return "ASSIGN";
    case TokenType::PLUS_ASSIGN:
        return "PLUS_ASSIGN";
    case TokenType::MINUS_ASSIGN:
        return "MINUS_ASSIGN";
    case TokenType::STAR_ASSIGN:
        return "STAR_ASSIGN";
    case TokenType::SLASH_ASSIGN:
        return "SLASH_ASSIGN";
    case TokenType::EQ:
        return "EQ";
    case TokenType::NEQ:
        return "NEQ";
    case TokenType::LT:
        return "LT";
    case TokenType::GT:
        return "GT";
    case TokenType::LTE:
        return "LTE";
    case TokenType::GTE:
        return "GTE";
    case TokenType::LPAREN:
        return "LPAREN";
    case TokenType::RPAREN:
        return "RPAREN";
    case TokenType::COMMA:
        return "COMMA";
    case TokenType::COLON:
        return "COLON";
    case TokenType::DOT:
        return "DOT";
    case TokenType::NEWLINE:
        return "NEWLINE";
    case TokenType::INDENT:
        return "INDENT";
    case TokenType::DEDENT:
        return "DEDENT";
    case TokenType::END_OF_FILE:
        return "EOF";
    }
    return "?";
}

namespace {
const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"let", TokenType::LET},   {"function", TokenType::FUNCTION}, {"if", TokenType::IF},
        {"else", TokenType::ELSE}, {"while", TokenType::WHILE},       {"return", TokenType::RETURN},
        {"true", TokenType::TRUE}, {"false", TokenType::FALSE},       {"null", TokenType::NIL},
    };
    return kw;
}
} // namespace

Lexer::Lexer(std::string source) : source(std::move(source)) { indentStack.push_back(0); }

bool Lexer::atEnd() const { return pos >= source.size(); }

char Lexer::peek() const { return atEnd() ? '\0' : source[pos]; }

char Lexer::peekNext() const { return (pos + 1 >= source.size()) ? '\0' : source[pos + 1]; }

char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (atEnd() || source[pos] != expected)
        return false;
    advance();
    return true;
}

void Lexer::add(TokenType type, const std::string& lexeme, int ln, int col) {
    tokens.emplace_back(type, lexeme, ln, col);
}

std::vector<Token> Lexer::tokenize() {
    while (!atEnd()) {
        lexLine();
    }

    // Fecha blocos abertos no fim do arquivo e emite EOF.
    if (!tokens.empty() && tokens.back().type != TokenType::NEWLINE) {
        add(TokenType::NEWLINE, "\\n", line, column);
    }
    while (indentStack.size() > 1) {
        indentStack.pop_back();
        add(TokenType::DEDENT, "", line, column);
    }
    add(TokenType::END_OF_FILE, "", line, column);
    return tokens;
}

int Lexer::consumeIndentation() {
    int width = 0;
    while (!atEnd() && (peek() == ' ' || peek() == '\t')) {
        // Tab conta como 4 espaços; mistura é tolerada mas desencorajada.
        width += (peek() == '\t') ? 4 : 1;
        advance();
    }
    return width;
}

void Lexer::emitIndentation(int width) {
    if (width > indentStack.back()) {
        indentStack.push_back(width);
        add(TokenType::INDENT, "", line, column);
    } else {
        while (width < indentStack.back()) {
            indentStack.pop_back();
            add(TokenType::DEDENT, "", line, column);
        }
        if (width != indentStack.back()) {
            throw LexError("Indentação inconsistente", line, column);
        }
    }
}

void Lexer::lexLine() {
    int indentWidth = consumeIndentation();

    // Linha em branco ou só comentário: não gera NEWLINE nem muda indentação.
    if (atEnd())
        return;
    if (peek() == '\n') {
        advance();
        return;
    }
    if (peek() == '#') {
        while (!atEnd() && peek() != '\n')
            advance();
        if (!atEnd())
            advance();
        return;
    }

    emitIndentation(indentWidth);

    // Lê os tokens até o fim da linha.
    while (!atEnd() && peek() != '\n') {
        char c = peek();
        if (c == ' ' || c == '\t') {
            advance();
            continue;
        }
        if (c == '#') { // comentário até o fim da linha
            while (!atEnd() && peek() != '\n')
                advance();
            break;
        }
        lexToken();
    }

    int ln = line, col = column;
    if (!atEnd())
        advance(); // consome o '\n'
    add(TokenType::NEWLINE, "\\n", ln, col);
}

void Lexer::lexToken() {
    int startLine = line, startCol = column;
    char c = peek();

    if (std::isdigit(static_cast<unsigned char>(c))) {
        lexNumber();
        return;
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        lexIdentifier();
        return;
    }
    if (c == '"') {
        lexString();
        return;
    }

    advance(); // consome o primeiro char do operador/pontuação
    switch (c) {
    case '+':
        add(match('=') ? TokenType::PLUS_ASSIGN : TokenType::PLUS, "+", startLine, startCol);
        return;
    case '-':
        add(match('=') ? TokenType::MINUS_ASSIGN : TokenType::MINUS, "-", startLine, startCol);
        return;
    case '*':
        add(match('=') ? TokenType::STAR_ASSIGN : TokenType::STAR, "*", startLine, startCol);
        return;
    case '/':
        add(match('=') ? TokenType::SLASH_ASSIGN : TokenType::SLASH, "/", startLine, startCol);
        return;
    case '=':
        add(match('=') ? TokenType::EQ : TokenType::ASSIGN, "=", startLine, startCol);
        return;
    case '!':
        if (match('=')) {
            add(TokenType::NEQ, "!=", startLine, startCol);
            return;
        }
        throw LexError("Caractere inesperado '!'", startLine, startCol);
    case '<':
        add(match('=') ? TokenType::LTE : TokenType::LT, "<", startLine, startCol);
        return;
    case '>':
        add(match('=') ? TokenType::GTE : TokenType::GT, ">", startLine, startCol);
        return;
    case '(':
        add(TokenType::LPAREN, "(", startLine, startCol);
        return;
    case ')':
        add(TokenType::RPAREN, ")", startLine, startCol);
        return;
    case ',':
        add(TokenType::COMMA, ",", startLine, startCol);
        return;
    case ':':
        add(TokenType::COLON, ":", startLine, startCol);
        return;
    case '.':
        add(TokenType::DOT, ".", startLine, startCol);
        return;
    default:
        throw LexError(std::string("Caractere inesperado '") + c + "'", startLine, startCol);
    }
}

void Lexer::lexNumber() {
    int startLine = line, startCol = column;
    size_t start = pos;
    while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek())))
        advance();
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance(); // o '.'
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek())))
            advance();
    }
    std::string text = source.substr(start, pos - start);
    Token tok(TokenType::NUMBER, text, startLine, startCol);
    tok.number = std::stod(text);
    tokens.push_back(tok);
}

void Lexer::lexString() {
    int startLine = line, startCol = column;
    advance(); // aspa de abertura
    std::string value;
    while (!atEnd() && peek() != '"') {
        char c = advance();
        if (c == '\\' && !atEnd()) {
            char esc = advance();
            switch (esc) {
            case 'n':
                value += '\n';
                break;
            case 't':
                value += '\t';
                break;
            case '"':
                value += '"';
                break;
            case '\\':
                value += '\\';
                break;
            default:
                value += esc;
                break;
            }
        } else {
            value += c;
        }
    }
    if (atEnd())
        throw LexError("String não terminada", startLine, startCol);
    advance(); // aspa de fechamento
    add(TokenType::STRING, value, startLine, startCol);
}

void Lexer::lexIdentifier() {
    int startLine = line, startCol = column;
    size_t start = pos;
    while (!atEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
        advance();
    std::string text = source.substr(start, pos - start);
    auto it = keywords().find(text);
    TokenType type = (it != keywords().end()) ? it->second : TokenType::IDENTIFIER;
    add(type, text, startLine, startCol);
}

} // namespace yumescript
