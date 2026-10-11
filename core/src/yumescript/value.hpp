/**
 * @file value.hpp
 * @brief Representação de um valor em tempo de execução no YumeScript.
 *
 * Um Value é dinamicamente tipado e pode ser: nil, booleano, número (double),
 * string, função (declarada no script ou nativa em C++) ou objeto. Objetos
 * expõem membros nomeados com get/set; é assim que o `transform` do objeto dono
 * é apresentado ao script (`transform.rotation.x`), sem o script saber que por
 * baixo há um Transform em C++.
 */
#ifndef YUMESCRIPT_VALUE_HPP
#define YUMESCRIPT_VALUE_HPP

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace yumescript {

struct FunctionStmt; // declarada em ast.hpp
struct FunctionExpr; // declarada em ast.hpp
class Environment;   // declarada em interpreter.hpp

class Value;

/// @brief Assinatura de uma função nativa (implementada em C++).
using NativeFn = std::function<Value(std::vector<Value>&)>;

/**
 * @brief Objeto com membros nomeados acessíveis pelo script.
 *
 * As operações de leitura/escrita são callbacks, para que um objeto possa ser
 * um simples saco de propriedades OU uma fachada sobre dados nativos (ex.: o
 * Transform do WorldObject). `members` lista os nomes conhecidos (apenas para
 * mensagens de erro/depuração).
 */
struct ObjectValue {
    std::function<Value(const std::string&)> get;              ///< Lê um membro.
    std::function<void(const std::string&, const Value&)> set; ///< Escreve um membro.
    std::vector<std::string> members;                          ///< Nomes conhecidos.
};

/**
 * @brief Função definida no script: nó da AST + ambiente de captura (closure).
 *
 * Pode ser uma de duas formas, nunca ambas:
 *  - @c decl: declaração nomeada (`function nome ...:` com corpo em bloco).
 *  - @c lambda: função anônima (`function <params> : <expressão>` inline).
 * O interpretador escolhe como executar conforme qual ponteiro está presente.
 */
struct ScriptFunction {
    const FunctionStmt* decl = nullptr;   ///< Declaração nomeada (corpo em bloco), ou nullptr.
    const FunctionExpr* lambda = nullptr; ///< Função anônima (corpo inline), ou nullptr.
    std::shared_ptr<Environment> closure;
};

/// @brief Discriminante do tipo dinâmico de um Value.
enum class ValueType { Nil, Bool, Number, String, Function, Native, Object };

/// @brief Um valor dinâmico do YumeScript.
class Value {
  public:
    ValueType type = ValueType::Nil;

    bool boolean = false;
    double number = 0.0;
    std::shared_ptr<std::string> str;
    std::shared_ptr<ScriptFunction> function;
    std::shared_ptr<NativeFn> native;
    std::shared_ptr<ObjectValue> object;

    Value() = default;

    static Value nil() { return Value(); }
    static Value makeBool(bool b) {
        Value v;
        v.type = ValueType::Bool;
        v.boolean = b;
        return v;
    }
    static Value makeNumber(double n) {
        Value v;
        v.type = ValueType::Number;
        v.number = n;
        return v;
    }
    static Value makeString(std::string s) {
        Value v;
        v.type = ValueType::String;
        v.str = std::make_shared<std::string>(std::move(s));
        return v;
    }
    static Value makeFunction(std::shared_ptr<ScriptFunction> f) {
        Value v;
        v.type = ValueType::Function;
        v.function = std::move(f);
        return v;
    }
    static Value makeNative(NativeFn fn) {
        Value v;
        v.type = ValueType::Native;
        v.native = std::make_shared<NativeFn>(std::move(fn));
        return v;
    }
    static Value makeObject(std::shared_ptr<ObjectValue> o) {
        Value v;
        v.type = ValueType::Object;
        v.object = std::move(o);
        return v;
    }

    bool isNil() const { return type == ValueType::Nil; }
    bool isCallable() const { return type == ValueType::Function || type == ValueType::Native; }

    /**
     * @brief Valor-verdade do estilo da linguagem.
     * nil e false são falsos; 0 é falso; string vazia é falsa; o resto é verdade.
     */
    bool isTruthy() const {
        switch (type) {
        case ValueType::Nil:
            return false;
        case ValueType::Bool:
            return boolean;
        case ValueType::Number:
            return number != 0.0;
        case ValueType::String:
            return str && !str->empty();
        default:
            return true;
        }
    }

    /// @brief Representação textual (para log/print e depuração).
    std::string toString() const {
        switch (type) {
        case ValueType::Nil:
            return "nil";
        case ValueType::Bool:
            return boolean ? "true" : "false";
        case ValueType::Number: {
            // Imprime inteiros sem o ".0" supérfluo.
            double intpart;
            if (std::modf(number, &intpart) == 0.0 && std::abs(number) < 1e15) {
                return std::to_string(static_cast<long long>(number));
            }
            std::string s = std::to_string(number);
            // Remove zeros à direita.
            size_t dot = s.find('.');
            if (dot != std::string::npos) {
                size_t last = s.find_last_not_of('0');
                if (last == dot)
                    last--;
                s.erase(last + 1);
            }
            return s;
        }
        case ValueType::String:
            return str ? *str : std::string();
        case ValueType::Function:
            return "<function>";
        case ValueType::Native:
            return "<native fn>";
        case ValueType::Object:
            return "<object>";
        }
        return "nil";
    }
};

} // namespace yumescript

#endif // YUMESCRIPT_VALUE_HPP
