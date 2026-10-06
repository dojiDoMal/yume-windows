/**
 * @file yumescript.hpp
 * @brief Fachada de alto nível do YumeScript: carrega e roda um script .ys.
 *
 * Reúne lexer, parser e interpretador num único objeto conveniente. É o ponto
 * de entrada que o host usa (o harness de teste agora, o ScriptComponent na
 * integração com a engine). Guarda a AST (Program) viva enquanto o script
 * existir, porque o interpretador referencia nós dela (ex.: declarações de
 * função apontam para FunctionStmt da AST).
 */
#ifndef YUMESCRIPT_YUMESCRIPT_HPP
#define YUMESCRIPT_YUMESCRIPT_HPP

#include "yumescript/ast.hpp"
#include "yumescript/interpreter.hpp"
#include "yumescript/value.hpp"

#include <memory>
#include <string>
#include <vector>

namespace yumescript {

/**
 * @brief Um script YumeScript carregado e pronto para executar.
 *
 * Fluxo: construa, (opcionalmente) injete globais com defineGlobal(), chame
 * loadSource()/loadFile() para analisar e rodar os statements de topo (isso
 * declara as funções do script), e então invoque funções por nome com
 * invoke()/hasFunction().
 */
class Script {
  public:
    Script();

    /** @brief Injeta um valor global (API nativa, `transform`, etc.). */
    void defineGlobal(const std::string& name, const Value& value);

    /**
     * @brief Analisa e executa os statements de topo de um fonte em memória.
     * @throws LexError, ParseError, RuntimeError em caso de erro.
     */
    void loadSource(const std::string& source);

    /**
     * @brief Lê um arquivo .ys do disco e o executa (ver loadSource()).
     * @throws std::runtime_error se o arquivo não puder ser lido.
     */
    void loadFile(const std::string& path);

    /** @brief Indica se o script definiu uma função com esse nome. */
    bool hasFunction(const std::string& name) const;

    /** @brief Chama uma função do script pelo nome. */
    Value invoke(const std::string& name, std::vector<Value> args = {});

    /** @brief Acesso ao interpretador (para injeções avançadas). */
    Interpreter& interpreter() { return interp; }

  private:
    Interpreter interp;
    Program program; ///< AST mantida viva (o interpretador aponta para ela).
};

} // namespace yumescript

#endif // YUMESCRIPT_YUMESCRIPT_HPP
