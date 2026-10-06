/**
 * @file script_component.cpp
 * @brief Implementação do ScriptComponent (ponte engine <-> YumeScript).
 */
#define CLASS_NAME "ScriptComponent"
#include "log_macros.hpp"

#include "components/script_component.hpp"

#include "components/transform.hpp"
#include "math/vector3.hpp"
#include "scene/world_object.hpp"
#include "yumescript/lexer.hpp"
#include "yumescript/parser.hpp"

#include <functional>
#include <iostream>
#include <utility>

using yumescript::ObjectValue;
using yumescript::Value;
using yumescript::ValueType;

namespace {

/**
 * @brief Expõe um Vector3 "virtual" ao script como objeto com {x,y,z}.
 *
 * As leituras e escritas passam por callbacks `read`/`write` que convertem
 * entre o Vector3 do engine (get/set por valor no Transform) e os números do
 * script. Escrever `transform.rotation.x` lê o Vector3 atual, troca só o
 * componente e grava o Vector3 de volta — necessário porque o Transform não
 * expõe referência mutável.
 */
Value makeVectorFacade(std::function<Vector3()> read, std::function<void(const Vector3&)> write) {
    auto obj = std::make_shared<ObjectValue>();
    obj->members = {"x", "y", "z"};
    obj->get = [read](const std::string& name) -> Value {
        Vector3 v = read();
        if (name == "x")
            return Value::makeNumber(v.x);
        if (name == "y")
            return Value::makeNumber(v.y);
        if (name == "z")
            return Value::makeNumber(v.z);
        return Value::nil();
    };
    obj->set = [read, write](const std::string& name, const Value& val) {
        Vector3 v = read();
        float n = (val.type == ValueType::Number) ? static_cast<float>(val.number) : 0.0f;
        if (name == "x")
            v.x = n;
        else if (name == "y")
            v.y = n;
        else if (name == "z")
            v.z = n;
        write(v);
    };
    return Value::makeObject(obj);
}

} // namespace

ScriptComponent::ScriptComponent(std::string scriptPath) : scriptPath(std::move(scriptPath)) {}

Value ScriptComponent::buildTransformObject() {
    Transform& t = getOwner()->getTransform();

    Value position = makeVectorFacade([&t]() { return t.getPosition(); },
                                      [&t](const Vector3& v) { t.setPosition(v); });
    Value rotation = makeVectorFacade([&t]() { return t.getRotation(); },
                                      [&t](const Vector3& v) { t.setRotation(v); });
    Value scale = makeVectorFacade([&t]() { return t.getScale(); },
                                   [&t](const Vector3& v) { t.setScale(v); });

    auto obj = std::make_shared<ObjectValue>();
    obj->members = {"position", "rotation", "scale"};
    obj->get = [position, rotation, scale](const std::string& name) -> Value {
        if (name == "position")
            return position;
        if (name == "rotation")
            return rotation;
        if (name == "scale")
            return scale;
        return Value::nil();
    };
    // Reatribuir position/rotation/scale inteiros não é suportado na Fase 1;
    // o script escreve nos componentes .x/.y/.z.
    obj->set = [](const std::string&, const Value&) {};
    return Value::makeObject(obj);
}

Value ScriptComponent::buildThisObject() {
    Value transform = buildTransformObject();

    auto obj = std::make_shared<ObjectValue>();
    obj->members = {"transform"};
    obj->get = [transform](const std::string& name) -> Value {
        if (name == "transform")
            return transform;
        return Value::nil();
    };
    // `this` não é reatribuível em nenhum de seus membros nesta fase.
    obj->set = [](const std::string&, const Value&) {};
    return Value::makeObject(obj);
}

void ScriptComponent::start() {
    if (!getOwner()) {
        LOG_ERROR("ScriptComponent sem owner; script não carregado: " + scriptPath);
        return;
    }

    script = std::make_unique<yumescript::Script>();

    // API nativa mínima da Fase 1: log/print.
    auto logFn = [](std::vector<Value>& args) -> Value {
        std::string line;
        for (size_t i = 0; i < args.size(); i++) {
            if (i)
                line += " ";
            line += args[i].toString();
        }
        std::cout << "[ys] " << line << "\n";
        return Value::nil();
    };
    script->defineGlobal("log", Value::makeNative(logFn));
    script->defineGlobal("print", Value::makeNative(logFn));

    // O objeto dono do script, acessível como `this`. Hoje expõe apenas
    // `this.transform`; novos membros (nome, componentes...) entram aqui.
    script->defineGlobal("this", buildThisObject());

    try {
        script->loadFile(scriptPath);
    } catch (const yumescript::LexError& e) {
        LOG_ERROR("Erro léxico em " + scriptPath + " (linha " + std::to_string(e.line) +
                  "): " + e.what());
        return;
    } catch (const yumescript::ParseError& e) {
        LOG_ERROR("Erro de sintaxe em " + scriptPath + " (linha " + std::to_string(e.line) +
                  "): " + e.what());
        return;
    } catch (const yumescript::RuntimeError& e) {
        LOG_ERROR("Erro de execução em " + scriptPath + " (linha " + std::to_string(e.line) +
                  "): " + e.what());
        return;
    } catch (const std::exception& e) {
        LOG_ERROR("Falha ao carregar script " + scriptPath + ": " + e.what());
        return;
    }

    loaded = true;
    hasUpdate = script->hasFunction("update");

    if (script->hasFunction("start")) {
        try {
            script->invoke("start");
        } catch (const yumescript::RuntimeError& e) {
            LOG_ERROR("Erro em start() de " + scriptPath + " (linha " + std::to_string(e.line) +
                      "): " + e.what());
        }
    }
}

void ScriptComponent::update(float deltaTime) {
    if (!loaded || !hasUpdate)
        return;
    try {
        script->invoke("update", {Value::makeNumber(deltaTime)});
    } catch (const yumescript::RuntimeError& e) {
        // Loga uma vez e desabilita o update para não floodar o console.
        LOG_ERROR("Erro em update() de " + scriptPath + " (linha " + std::to_string(e.line) +
                  "): " + e.what());
        hasUpdate = false;
    }
}
