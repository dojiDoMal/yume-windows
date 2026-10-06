/**
 * @file parser.cpp
 * @brief Implementação do Parser recursivo-descendente do YumeScript.
 */
#include "yumescript/parser.hpp"

#include "yumescript/lexer.hpp" // tokenTypeName

#include <utility>

namespace yumescript {

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

// --- helpers de token --------------------------------------------------------

const Token& Parser::peek() const { return tokens[current]; }
const Token& Parser::previous() const { return tokens[current - 1]; }
bool Parser::atEnd() const { return peek().type == TokenType::END_OF_FILE; }

bool Parser::check(TokenType type) const { return !atEnd() && peek().type == type; }

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::advance() {
    if (!atEnd())
        current++;
    return previous();
}

const Token& Parser::expect(TokenType type, const std::string& what) {
    if (check(type))
        return advance();
    error(peek(),
          "Esperava " + what + ", encontrei '" + std::string(tokenTypeName(peek().type)) + "'");
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE))
        advance();
}

void Parser::error(const Token& tok, const std::string& msg) {
    throw ParseError(msg, tok.line, tok.column);
}

// --- programa e statements ---------------------------------------------------

Program Parser::parseProgram() {
    Program program;
    skipNewlines();
    while (!atEnd()) {
        program.statements.push_back(declaration());
        skipNewlines();
    }
    return program;
}

StmtPtr Parser::declaration() {
    if (check(TokenType::LET))
        return letStatement();
    if (check(TokenType::FUNCTION))
        return functionStatement();
    if (check(TokenType::IF))
        return ifStatement();
    if (check(TokenType::WHILE))
        return whileStatement();
    if (check(TokenType::RETURN))
        return returnStatement();
    return expressionStatement();
}

StmtPtr Parser::letStatement() {
    int line = peek().line;
    advance(); // 'let'
    const Token& name = expect(TokenType::IDENTIFIER, "nome de variável após 'let'");
    auto stmt = std::make_unique<LetStmt>();
    stmt->name = name.lexeme;
    stmt->line = line;
    if (match(TokenType::ASSIGN)) {
        stmt->initializer = expression();
    }
    expect(TokenType::NEWLINE, "fim de linha após declaração 'let'");
    return stmt;
}

StmtPtr Parser::functionStatement() {
    int line = peek().line;
    advance(); // 'function'
    const Token& name = expect(TokenType::IDENTIFIER, "nome da função");
    auto stmt = std::make_unique<FunctionStmt>();
    stmt->name = name.lexeme;
    stmt->line = line;
    // Parâmetros sem parênteses: uma sequência de identificadores até o ':'.
    while (check(TokenType::IDENTIFIER)) {
        stmt->params.push_back(advance().lexeme);
    }
    stmt->body = block();
    return stmt;
}

StmtPtr Parser::ifStatement() {
    int line = peek().line;
    advance(); // 'if'
    auto stmt = std::make_unique<IfStmt>();
    stmt->line = line;
    stmt->condition = expression();
    stmt->thenBranch = block();

    // Um 'else' pode vir depois do DEDENT do bloco 'then'.
    skipNewlines();
    if (check(TokenType::ELSE)) {
        advance();
        stmt->hasElse = true;
        stmt->elseBranch = block();
    }
    return stmt;
}

StmtPtr Parser::whileStatement() {
    int line = peek().line;
    advance(); // 'while'
    auto stmt = std::make_unique<WhileStmt>();
    stmt->line = line;
    stmt->condition = expression();
    stmt->body = block();
    return stmt;
}

StmtPtr Parser::returnStatement() {
    int line = peek().line;
    advance(); // 'return'
    auto stmt = std::make_unique<ReturnStmt>();
    stmt->line = line;
    if (!check(TokenType::NEWLINE)) {
        stmt->value = expression();
    }
    expect(TokenType::NEWLINE, "fim de linha após 'return'");
    return stmt;
}

StmtPtr Parser::expressionStatement() {
    int line = peek().line;
    auto stmt = std::make_unique<ExprStmt>();
    stmt->line = line;
    stmt->expr = expression();
    expect(TokenType::NEWLINE, "fim de linha após expressão");
    return stmt;
}

Block Parser::block() {
    expect(TokenType::COLON, "':' para iniciar um bloco");
    expect(TokenType::NEWLINE, "quebra de linha após ':'");
    expect(TokenType::INDENT, "bloco indentado");

    Block stmts;
    skipNewlines();
    while (!check(TokenType::DEDENT) && !atEnd()) {
        stmts.push_back(declaration());
        skipNewlines();
    }
    expect(TokenType::DEDENT, "fim do bloco (dedent)");
    return stmts;
}

// --- expressões --------------------------------------------------------------

ExprPtr Parser::expression() { return assignment(); }

ExprPtr Parser::assignment() {
    ExprPtr expr = equality();

    if (check(TokenType::ASSIGN) || check(TokenType::PLUS_ASSIGN) ||
        check(TokenType::MINUS_ASSIGN) || check(TokenType::STAR_ASSIGN) ||
        check(TokenType::SLASH_ASSIGN)) {
        const Token& opTok = advance();
        ExprPtr value = assignment(); // associatividade à direita

        // O alvo precisa ser atribuível: identificador ou acesso a membro.
        if (dynamic_cast<IdentifierExpr*>(expr.get()) || dynamic_cast<MemberExpr*>(expr.get())) {
            auto assign = std::make_unique<AssignExpr>();
            assign->target = std::move(expr);
            assign->op = opTok.type;
            assign->value = std::move(value);
            assign->line = opTok.line;
            return assign;
        }
        error(opTok, "Alvo de atribuição inválido");
    }
    return expr;
}

ExprPtr Parser::equality() {
    ExprPtr expr = comparison();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        const Token& opTok = advance();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = opTok.type;
        bin->line = opTok.line;
        bin->left = std::move(expr);
        bin->right = comparison();
        expr = std::move(bin);
    }
    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = term();
    while (check(TokenType::LT) || check(TokenType::GT) || check(TokenType::LTE) ||
           check(TokenType::GTE)) {
        const Token& opTok = advance();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = opTok.type;
        bin->line = opTok.line;
        bin->left = std::move(expr);
        bin->right = term();
        expr = std::move(bin);
    }
    return expr;
}

ExprPtr Parser::term() {
    ExprPtr expr = factor();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        const Token& opTok = advance();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = opTok.type;
        bin->line = opTok.line;
        bin->left = std::move(expr);
        bin->right = factor();
        expr = std::move(bin);
    }
    return expr;
}

ExprPtr Parser::factor() {
    ExprPtr expr = unary();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        const Token& opTok = advance();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = opTok.type;
        bin->line = opTok.line;
        bin->left = std::move(expr);
        bin->right = unary();
        expr = std::move(bin);
    }
    return expr;
}

ExprPtr Parser::unary() {
    if (check(TokenType::MINUS)) {
        const Token& opTok = advance();
        auto un = std::make_unique<UnaryExpr>();
        un->op = opTok.type;
        un->line = opTok.line;
        un->operand = unary();
        return un;
    }
    return call();
}

ExprPtr Parser::call() {
    ExprPtr expr = primary();
    // Encadeia chamadas `()` e acessos a membro `.nome` à esquerda.
    while (true) {
        if (check(TokenType::LPAREN)) {
            expr = finishCall(std::move(expr));
        } else if (check(TokenType::DOT)) {
            const Token& dot = advance();
            const Token& name = expect(TokenType::IDENTIFIER, "nome do membro após '.'");
            auto member = std::make_unique<MemberExpr>();
            member->object = std::move(expr);
            member->name = name.lexeme;
            member->line = dot.line;
            expr = std::move(member);
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::finishCall(ExprPtr callee) {
    const Token& paren = advance(); // '('
    auto callExpr = std::make_unique<CallExpr>();
    callExpr->line = paren.line;
    callExpr->callee = std::move(callee);
    if (!check(TokenType::RPAREN)) {
        do {
            callExpr->args.push_back(expression());
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, "')' para fechar a lista de argumentos");
    return callExpr;
}

ExprPtr Parser::primary() {
    const Token& tok = peek();
    switch (tok.type) {
    case TokenType::NUMBER:
        advance();
        return LiteralExpr::makeNumber(tok.number);
    case TokenType::STRING:
        advance();
        return LiteralExpr::makeString(tok.lexeme);
    case TokenType::TRUE:
        advance();
        return LiteralExpr::makeBool(true);
    case TokenType::FALSE:
        advance();
        return LiteralExpr::makeBool(false);
    case TokenType::NIL:
        advance();
        return LiteralExpr::makeNil();
    case TokenType::IDENTIFIER:
        advance();
        return std::make_unique<IdentifierExpr>(tok.lexeme, tok.line);
    case TokenType::LPAREN: {
        advance();
        ExprPtr expr = expression();
        expect(TokenType::RPAREN, "')' para fechar a expressão agrupada");
        return expr;
    }
    default:
        error(tok, "Expressão esperada, encontrei '" + std::string(tokenTypeName(tok.type)) + "'");
    }
}

} // namespace yumescript
