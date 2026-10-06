/**
 * @file harness_main.cpp
 * @brief Harness de teste isolado do YumeScript (Fase 1), sem a engine.
 *
 * Carrega um arquivo .ys (passado por argumento, ou tests/example.ys por
 * padrão), injeta um `transform` fictício e uma função nativa `log`, roda os
 * statements de topo, chama `start()` e algumas iterações de `update(dt)`, e
 * imprime o estado final do transform. Serve para validar lexer+parser+
 * interpretador end-to-end antes de integrar o ScriptComponent na engine.
 *
 * Compilado como alvo de host (ver CMakeLists), NÃO faz parte de yume_core.
 */
#include "yumescript/lexer.hpp"
#include "yumescript/parser.hpp"
#include "yumescript/value.hpp"
#include "yumescript/yumescript.hpp"

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

using namespace yumescript;

namespace {

/// @brief Estado numérico que o `transform` do script manipula.
struct MockTransform {
    double posX = 0, posY = 0, posZ = 0;
    double rotX = 0, rotY = 0, rotZ = 0;
};

/// @brief Cria um Value::Object que expõe {x,y,z} sobre três doubles.
Value makeVec3(double* x, double* y, double* z) {
    auto obj = std::make_shared<ObjectValue>();
    obj->members = {"x", "y", "z"};
    obj->get = [x, y, z](const std::string& name) -> Value {
        if (name == "x")
            return Value::makeNumber(*x);
        if (name == "y")
            return Value::makeNumber(*y);
        if (name == "z")
            return Value::makeNumber(*z);
        return Value::nil();
    };
    obj->set = [x, y, z](const std::string& name, const Value& v) {
        double n = (v.type == ValueType::Number) ? v.number : 0.0;
        if (name == "x")
            *x = n;
        else if (name == "y")
            *y = n;
        else if (name == "z")
            *z = n;
    };
    return Value::makeObject(obj);
}

/// @brief Cria o objeto `transform` com membros `position` e `rotation`.
Value makeTransform(MockTransform& t) {
    Value position = makeVec3(&t.posX, &t.posY, &t.posZ);
    Value rotation = makeVec3(&t.rotX, &t.rotY, &t.rotZ);
    auto obj = std::make_shared<ObjectValue>();
    obj->members = {"position", "rotation"};
    obj->get = [position, rotation](const std::string& name) -> Value {
        if (name == "position")
            return position;
        if (name == "rotation")
            return rotation;
        return Value::nil();
    };
    obj->set = [](const std::string&, const Value&) {
        // position/rotation são objetos; reatribuí-los inteiros não é suportado
        // nesta fase (o script escreve nos componentes .x/.y/.z).
    };
    return Value::makeObject(obj);
}

/// @brief Cria o objeto `this` (o objeto dono), expondo `this.transform`.
///        Espelha o que o ScriptComponent faz dentro da engine.
Value makeThis(MockTransform& t) {
    Value transform = makeTransform(t);
    auto obj = std::make_shared<ObjectValue>();
    obj->members = {"transform"};
    obj->get = [transform](const std::string& name) -> Value {
        if (name == "transform")
            return transform;
        return Value::nil();
    };
    obj->set = [](const std::string&, const Value&) {};
    return Value::makeObject(obj);
}

} // namespace

int main(int argc, char** argv) {
    const std::string path = (argc > 1) ? argv[1] : "core/src/yumescript/tests/example.ys";

    MockTransform transform;

    Script script;

    // API nativa: log(...) imprime os argumentos concatenados por espaço.
    script.defineGlobal("log", Value::makeNative([](std::vector<Value>& args) -> Value {
                            std::string line;
                            for (size_t i = 0; i < args.size(); i++) {
                                if (i)
                                    line += " ";
                                line += args[i].toString();
                            }
                            std::cout << "[log] " << line << "\n";
                            return Value::nil();
                        }));
    // print como alias de log, mas sem o prefixo "[log]".
    script.defineGlobal("print", Value::makeNative([](std::vector<Value>& args) -> Value {
                            std::string line;
                            for (size_t i = 0; i < args.size(); i++) {
                                if (i)
                                    line += " ";
                                line += args[i].toString();
                            }
                            std::cout << line << "\n";
                            return Value::nil();
                        }));

    // Objeto `this` (dono do script); o script manipula this.transform.
    script.defineGlobal("this", makeThis(transform));

    try {
        script.loadFile(path);
    } catch (const LexError& e) {
        std::cerr << "Erro léxico (linha " << e.line << ", col " << e.column << "): " << e.what()
                  << "\n";
        return 1;
    } catch (const ParseError& e) {
        std::cerr << "Erro de sintaxe (linha " << e.line << ", col " << e.column
                  << "): " << e.what() << "\n";
        return 1;
    } catch (const RuntimeError& e) {
        std::cerr << "Erro de execução (linha " << e.line << "): " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    std::cout << "--- carregado: " << path << " ---\n";

    try {
        if (script.hasFunction("start")) {
            std::cout << "--- start() ---\n";
            script.invoke("start");
        }

        if (script.hasFunction("update")) {
            std::cout << "--- update(dt) x3, dt=1.0 ---\n";
            for (int frame = 0; frame < 3; frame++) {
                std::cout << "frame " << frame << ":\n";
                script.invoke("update", {Value::makeNumber(1.0)});
            }
        }

        // Exercita uma função com retorno, se existir.
        if (script.hasFunction("dobro")) {
            Value r = script.invoke("dobro", {Value::makeNumber(21.0)});
            std::cout << "dobro(21) = " << r.toString() << "\n";
        }
    } catch (const RuntimeError& e) {
        std::cerr << "Erro de execução (linha " << e.line << "): " << e.what() << "\n";
        return 1;
    }

    std::cout << "--- estado final do transform ---\n";
    std::printf("rotation = (%.3f, %.3f, %.3f)\n", transform.rotX, transform.rotY, transform.rotZ);
    std::printf("position = (%.3f, %.3f, %.3f)\n", transform.posX, transform.posY, transform.posZ);

    return 0;
}
