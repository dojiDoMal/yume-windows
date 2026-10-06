/**
 * @file yumescript.cpp
 * @brief Implementação da fachada Script do YumeScript.
 */
#include "yumescript/yumescript.hpp"

#include "yumescript/lexer.hpp"
#include "yumescript/parser.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace yumescript {

Script::Script() = default;

void Script::defineGlobal(const std::string& name, const Value& value) {
    interp.defineGlobal(name, value);
}

void Script::loadSource(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    program = parser.parseProgram();

    interp.run(program);
}

void Script::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Não foi possível abrir o script: " + path);
    std::ostringstream ss;
    ss << file.rdbuf();
    loadSource(ss.str());
}

bool Script::hasFunction(const std::string& name) const {
    return interp.hasFunction(name);
}

Value Script::invoke(const std::string& name, std::vector<Value> args) {
    return interp.callFunction(name, std::move(args));
}

} // namespace yumescript
