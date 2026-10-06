/**
 * @file interpreter.cpp
 * @brief Implementação do interpretador tree-walking do YumeScript.
 */
#include "yumescript/interpreter.hpp"

#include "yumescript/token.hpp"

#include <utility>

namespace yumescript {

// --- Environment -------------------------------------------------------------

void Environment::define(const std::string& name, const Value& value) { values[name] = value; }

Value Environment::get(const std::string& name, int line) const {
    auto it = values.find(name);
    if (it != values.end())
        return it->second;
    if (enclosing)
        return enclosing->get(name, line);
    throw RuntimeError("Variável indefinida: '" + name + "'", line);
}

void Environment::assign(const std::string& name, const Value& value, int line) {
    auto it = values.find(name);
    if (it != values.end()) {
        it->second = value;
        return;
    }
    if (enclosing) {
        enclosing->assign(name, value, line);
        return;
    }
    throw RuntimeError("Atribuição a variável indefinida: '" + name + "'", line);
}

bool Environment::has(const std::string& name) const {
    if (values.find(name) != values.end())
        return true;
    return enclosing ? enclosing->has(name) : false;
}

// --- Interpreter: ciclo de vida ----------------------------------------------

Interpreter::Interpreter() {
    globals = std::make_shared<Environment>();
    environment = globals;
}

void Interpreter::defineGlobal(const std::string& name, const Value& value) {
    globals->define(name, value);
}

void Interpreter::run(const Program& program) {
    for (const auto& stmt : program.statements) {
        execute(*stmt);
    }
}

bool Interpreter::hasFunction(const std::string& name) const {
    if (!globals->has(name))
        return false;
    return globals->get(name, 0).isCallable();
}

Value Interpreter::callFunction(const std::string& name, std::vector<Value> args, int line) {
    Value callee = globals->get(name, line);
    if (!callee.isCallable())
        throw RuntimeError("'" + name + "' não é uma função", line);
    return call(callee, args, line);
}

Value Interpreter::call(const Value& callee, std::vector<Value>& args, int line) {
    if (callee.type == ValueType::Native) {
        return (*callee.native)(args);
    }
    if (callee.type == ValueType::Function) {
        const ScriptFunction& fn = *callee.function;
        const FunctionStmt& decl = *fn.decl;

        if (args.size() != decl.params.size()) {
            throw RuntimeError("Função '" + decl.name + "' espera " +
                                   std::to_string(decl.params.size()) + " argumento(s), recebeu " +
                                   std::to_string(args.size()),
                               line);
        }

        auto callEnv = std::make_shared<Environment>(fn.closure);
        for (size_t i = 0; i < decl.params.size(); i++) {
            callEnv->define(decl.params[i], args[i]);
        }

        returning = false;
        returnValue = Value::nil();
        executeBlock(decl.body, callEnv);

        Value out = returning ? returnValue : Value::nil();
        returning = false;
        returnValue = Value::nil();
        return out;
    }
    throw RuntimeError("Valor não é chamável", line);
}

// --- Interpreter: execução ---------------------------------------------------

Value Interpreter::evaluate(const Expr& expr) {
    expr.accept(*this);
    return result;
}

void Interpreter::execute(const Stmt& stmt) { stmt.accept(*this); }

void Interpreter::executeBlock(const Block& block, std::shared_ptr<Environment> env) {
    auto previous = environment;
    environment = std::move(env);
    // Garante restauração do escopo mesmo se um RuntimeError propagar.
    try {
        for (const auto& stmt : block) {
            execute(*stmt);
            if (returning)
                break;
        }
    } catch (...) {
        environment = previous;
        throw;
    }
    environment = previous;
}

// --- Statements --------------------------------------------------------------

void Interpreter::visitLet(const LetStmt& stmt) {
    Value value = stmt.initializer ? evaluate(*stmt.initializer) : Value::nil();
    environment->define(stmt.name, value);
}

void Interpreter::visitFunction(const FunctionStmt& stmt) {
    auto fn = std::make_shared<ScriptFunction>();
    fn->decl = &stmt;
    fn->closure = environment;
    environment->define(stmt.name, Value::makeFunction(fn));
}

void Interpreter::visitIf(const IfStmt& stmt) {
    Value cond = evaluate(*stmt.condition);
    if (cond.isTruthy()) {
        executeBlock(stmt.thenBranch, std::make_shared<Environment>(environment));
    } else if (stmt.hasElse) {
        executeBlock(stmt.elseBranch, std::make_shared<Environment>(environment));
    }
}

void Interpreter::visitWhile(const WhileStmt& stmt) {
    while (evaluate(*stmt.condition).isTruthy()) {
        executeBlock(stmt.body, std::make_shared<Environment>(environment));
        if (returning)
            break;
    }
}

void Interpreter::visitReturn(const ReturnStmt& stmt) {
    returnValue = stmt.value ? evaluate(*stmt.value) : Value::nil();
    returning = true;
}

void Interpreter::visitExpr(const ExprStmt& stmt) { evaluate(*stmt.expr); }

// --- Expressões --------------------------------------------------------------

void Interpreter::visitLiteral(const LiteralExpr& expr) {
    switch (expr.kind) {
    case LiteralExpr::Kind::Number:
        result = Value::makeNumber(expr.number);
        break;
    case LiteralExpr::Kind::String:
        result = Value::makeString(expr.str);
        break;
    case LiteralExpr::Kind::Bool:
        result = Value::makeBool(expr.boolean);
        break;
    case LiteralExpr::Kind::Nil:
        result = Value::nil();
        break;
    }
}

void Interpreter::visitIdentifier(const IdentifierExpr& expr) {
    result = environment->get(expr.name, expr.line);
}

void Interpreter::visitUnary(const UnaryExpr& expr) {
    Value operand = evaluate(*expr.operand);
    if (expr.op == TokenType::MINUS) {
        if (operand.type != ValueType::Number)
            throw RuntimeError("Operando de '-' deve ser número", expr.line);
        result = Value::makeNumber(-operand.number);
        return;
    }
    throw RuntimeError("Operador unário desconhecido", expr.line);
}

void Interpreter::visitBinary(const BinaryExpr& expr) {
    Value left = evaluate(*expr.left);
    Value right = evaluate(*expr.right);

    switch (expr.op) {
    case TokenType::PLUS:
        // '+' soma números ou concatena quando algum lado é string.
        if (left.type == ValueType::String || right.type == ValueType::String) {
            result = Value::makeString(left.toString() + right.toString());
        } else if (left.type == ValueType::Number && right.type == ValueType::Number) {
            result = Value::makeNumber(left.number + right.number);
        } else {
            throw RuntimeError("Operandos de '+' inválidos", expr.line);
        }
        return;
    case TokenType::MINUS:
    case TokenType::STAR:
    case TokenType::SLASH:
    case TokenType::LT:
    case TokenType::GT:
    case TokenType::LTE:
    case TokenType::GTE:
        if (left.type != ValueType::Number || right.type != ValueType::Number)
            throw RuntimeError("Operandos numéricos esperados", expr.line);
        switch (expr.op) {
        case TokenType::MINUS:
            result = Value::makeNumber(left.number - right.number);
            return;
        case TokenType::STAR:
            result = Value::makeNumber(left.number * right.number);
            return;
        case TokenType::SLASH:
            if (right.number == 0.0)
                throw RuntimeError("Divisão por zero", expr.line);
            result = Value::makeNumber(left.number / right.number);
            return;
        case TokenType::LT:
            result = Value::makeBool(left.number < right.number);
            return;
        case TokenType::GT:
            result = Value::makeBool(left.number > right.number);
            return;
        case TokenType::LTE:
            result = Value::makeBool(left.number <= right.number);
            return;
        case TokenType::GTE:
            result = Value::makeBool(left.number >= right.number);
            return;
        default:
            break;
        }
        return;
    case TokenType::EQ:
    case TokenType::NEQ: {
        bool equal = false;
        if (left.type == right.type) {
            switch (left.type) {
            case ValueType::Nil:
                equal = true;
                break;
            case ValueType::Bool:
                equal = left.boolean == right.boolean;
                break;
            case ValueType::Number:
                equal = left.number == right.number;
                break;
            case ValueType::String:
                equal = left.toString() == right.toString();
                break;
            default:
                equal = false;
                break;
            }
        }
        result = Value::makeBool(expr.op == TokenType::EQ ? equal : !equal);
        return;
    }
    default:
        throw RuntimeError("Operador binário desconhecido", expr.line);
    }
}

void Interpreter::visitCall(const CallExpr& expr) {
    Value callee = evaluate(*expr.callee);
    std::vector<Value> args;
    args.reserve(expr.args.size());
    for (const auto& arg : expr.args) {
        args.push_back(evaluate(*arg));
    }
    result = call(callee, args, expr.line);
}

void Interpreter::visitMember(const MemberExpr& expr) {
    Value object = evaluate(*expr.object);
    if (object.type != ValueType::Object || !object.object || !object.object->get)
        throw RuntimeError("'" + expr.name + "' não é membro de um objeto", expr.line);
    result = object.object->get(expr.name);
}

void Interpreter::visitAssign(const AssignExpr& expr) {
    Value value = evaluate(*expr.value);

    // Formas compostas (+=, -=, *=, /=): lê o alvo atual e combina.
    if (expr.op != TokenType::ASSIGN) {
        Value current = evaluate(*expr.target);
        if (current.type != ValueType::Number || value.type != ValueType::Number)
            throw RuntimeError("Atribuição composta requer números", expr.line);
        double a = current.number, b = value.number;
        double r = a;
        switch (expr.op) {
        case TokenType::PLUS_ASSIGN:
            r = a + b;
            break;
        case TokenType::MINUS_ASSIGN:
            r = a - b;
            break;
        case TokenType::STAR_ASSIGN:
            r = a * b;
            break;
        case TokenType::SLASH_ASSIGN:
            if (b == 0.0)
                throw RuntimeError("Divisão por zero", expr.line);
            r = a / b;
            break;
        default:
            break;
        }
        value = Value::makeNumber(r);
    }

    assignTo(*expr.target, value, expr.line);
    result = value;
}

void Interpreter::assignTo(const Expr& target, const Value& value, int line) {
    if (auto* id = dynamic_cast<const IdentifierExpr*>(&target)) {
        // Autovivificação: se não existe, cria no escopo atual.
        if (environment->has(id->name))
            environment->assign(id->name, value, line);
        else
            environment->define(id->name, value);
        return;
    }
    if (auto* member = dynamic_cast<const MemberExpr*>(&target)) {
        Value object = evaluate(*member->object);
        if (object.type != ValueType::Object || !object.object || !object.object->set)
            throw RuntimeError("Alvo de atribuição não é objeto gravável", line);
        object.object->set(member->name, value);
        return;
    }
    throw RuntimeError("Alvo de atribuição inválido", line);
}

} // namespace yumescript
