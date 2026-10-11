/**
 * @file script_component.cpp
 * @brief Implementação do ScriptComponent (ponte engine <-> YumeScript).
 */
#define CLASS_NAME "ScriptComponent"
#include "log_macros.hpp"

#include "components/script_component.hpp"

#include "components/script_services.hpp"
#include "components/transform.hpp"
#include "input/i_input.hpp"
#include "input/input_key.hpp"
#include "input/key_names.hpp"
#include "math/vector3.hpp"
#include "scene/world_object.hpp"
#include "yumescript/lexer.hpp"
#include "yumescript/parser.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <utility>

using yumescript::ObjectValue;
using yumescript::Value;
using yumescript::ValueType;

namespace {

/// @brief Qual componente (eixo) de um Vector3 um acessor lê/escreve.
enum class Axis { X, Y, Z };

float axisOf(const Vector3& v, Axis a) { return a == Axis::X ? v.x : (a == Axis::Y ? v.y : v.z); }
void setAxis(Vector3& v, Axis a, float n) {
    if (a == Axis::X)
        v.x = n;
    else if (a == Axis::Y)
        v.y = n;
    else
        v.z = n;
}

/**
 * @brief Expõe um Vector3 "virtual" ao script com acessores local/global.
 *
 * Membros expostos (para position e rotation):
 *  - localX/localY/localZ: valor LOCAL (relativo ao pai). Leitura e escrita.
 *  - globalX/globalY/globalZ: valor de MUNDO. Leitura sempre; escrita só quando
 *    o objeto é raiz (sem pai), onde local == global. Em objeto com pai, a
 *    escrita global exigiria inverter a transformação do pai e é bloqueada por
 *    ora (ver TODO) — e, para rotação, a escrita global não é suportada (Euler).
 *  - x/y/z: atalho para a forma GLOBAL (x == globalX), como `position` na Unity.
 *
 * @param readLocal/writeLocal   acessam o valor local (no Transform do dono).
 * @param readGlobal             lê o valor de mundo (via WorldObject).
 * @param hasParent              se o objeto tem pai (define se global-write é ok).
 * @param allowGlobalWrite       false para rotação (global é só leitura).
 */
Value makeTransformFacade(std::function<Vector3()> readLocal,
                          std::function<void(const Vector3&)> writeLocal,
                          std::function<Vector3()> readGlobal, std::function<bool()> hasParent,
                          bool allowGlobalWrite, const char* fieldName) {
    auto obj = std::make_shared<ObjectValue>();
    obj->members = {"x", "y", "z", "localX", "localY", "localZ", "globalX", "globalY", "globalZ"};

    // Mapeia nome -> (eixo, isGlobal). x/y/z são atalhos para global*.
    auto parse = [](const std::string& name, Axis& axis, bool& global) -> bool {
        if (name == "x" || name == "globalX") {
            axis = Axis::X;
            global = true;
            return true;
        }
        if (name == "y" || name == "globalY") {
            axis = Axis::Y;
            global = true;
            return true;
        }
        if (name == "z" || name == "globalZ") {
            axis = Axis::Z;
            global = true;
            return true;
        }
        if (name == "localX") {
            axis = Axis::X;
            global = false;
            return true;
        }
        if (name == "localY") {
            axis = Axis::Y;
            global = false;
            return true;
        }
        if (name == "localZ") {
            axis = Axis::Z;
            global = false;
            return true;
        }
        return false;
    };

    obj->get = [readLocal, readGlobal, parse](const std::string& name) -> Value {
        Axis axis;
        bool global;
        if (!parse(name, axis, global))
            return Value::nil();
        Vector3 v = global ? readGlobal() : readLocal();
        return Value::makeNumber(axisOf(v, axis));
    };

    std::string field = fieldName;
    obj->set = [readLocal, writeLocal, hasParent, allowGlobalWrite, parse,
                field](const std::string& name, const Value& val) {
        Axis axis;
        bool global;
        if (!parse(name, axis, global))
            return;
        float n = (val.type == ValueType::Number) ? static_cast<float>(val.number) : 0.0f;

        if (!global) {
            // Escrita local: direta no Transform.
            Vector3 v = readLocal();
            setAxis(v, axis, n);
            writeLocal(v);
            return;
        }

        // Escrita global.
        if (!allowGlobalWrite) {
            // Rotação: global é só leitura por ora (Euler não decompõe trivial).
            LOG_WARN("Escrita de " + field + ".global* não suportada (use local*)");
            return;
        }
        if (hasParent()) {
            // TODO: escrever posição global num objeto com pai exige inverter a
            // world matrix do pai (não há Matrix4::inverse ainda). Bloqueado para
            // não gravar valor incorreto silenciosamente; use local* por enquanto.
            LOG_WARN("Escrita de " + field +
                     ".global* em objeto com pai ainda não suportada (use local*)");
            return;
        }
        // Objeto raiz: local == global, grava direto no local.
        Vector3 v = readLocal();
        setAxis(v, axis, n);
        writeLocal(v);
    };
    return Value::makeObject(obj);
}

} // namespace

ScriptComponent::ScriptComponent(std::string scriptPath) : scriptPath(std::move(scriptPath)) {}

Value ScriptComponent::buildTransformObject() {
    WorldObject* owner = getOwner();
    Transform& t = owner->getTransform();

    auto hasParent = [owner]() { return owner->getParent() != nullptr; };

    // Posição: local* escreve/lê o Transform; global*/x,y,z usam a world matrix.
    Value position = makeTransformFacade([&t]() { return t.getPosition(); },
                                         [&t](const Vector3& v) { t.setPosition(v); },
                                         [owner]() { return owner->getWorldPosition(); }, hasParent,
                                         /*allowGlobalWrite=*/true, "position");

    // Rotação: local* lê/escreve; global*/x,y,z leem a rotação de mundo
    // (soma Euler pela cadeia). Escrita global desabilitada (ver TODO Euler).
    Value rotation = makeTransformFacade([&t]() { return t.getRotation(); },
                                         [&t](const Vector3& v) { t.setRotation(v); },
                                         [owner]() { return owner->getWorldRotation(); }, hasParent,
                                         /*allowGlobalWrite=*/false, "rotation");

    // Escala: mantida como local (global de escala raramente é útil); global*
    // espelha o local aqui.
    Value scale = makeTransformFacade([&t]() { return t.getScale(); },
                                      [&t](const Vector3& v) { t.setScale(v); },
                                      [&t]() { return t.getScale(); }, hasParent,
                                      /*allowGlobalWrite=*/true, "scale");

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

Value ScriptComponent::buildYumeObject() {
    using yumescript::ScriptFunction;

    // Interpretador deste script: usado para chamar de volta os callbacks de
    // input registrados (que são Values chamáveis do próprio script).
    yumescript::Interpreter* interp = &script->interpreter();

    // Yume.InputSystem.onAction(alias, callback)
    //   alias:    nome lógico da AÇÃO definida em project.conf (ex.: "moveUp").
    //             O tipo de evento (keydown/keyhold/keyup) vem do alias, não da
    //             chamada. Se o alias não existir, trata a string como nome de
    //             tecla estilo JS, com evento padrão KeyDown.
    //   callback: função chamada quando a ação dispara; recebe o dt do frame.
    //
    // Deve ser chamada uma vez (ex.: no start): registra o callback e o engine
    // passa a dispará-lo conforme o tipo do alias, sem necessidade de re-bindar
    // a cada frame.
    auto onActionFn = [interp](std::vector<Value>& args) -> Value {
        if (args.size() != 2 || args[0].type != ValueType::String || !args[1].isCallable()) {
            LOG_WARN("Yume.InputSystem.onAction espera (string alias, function callback)");
            return Value::nil();
        }

        Yume::IInput* input = Yume::ScriptServices::input;
        if (!input) {
            LOG_WARN("Yume.InputSystem.onAction chamado sem input disponível");
            return Value::nil();
        }

        const std::string& alias = *args[0].str;
        const auto& aliases = Yume::ScriptServices::inputAliases;
        auto aliasIt = aliases.find(alias);
        std::string keyName = (aliasIt != aliases.end()) ? aliasIt->second.value : alias;
        Yume::KeyEventType eventType =
            (aliasIt != aliases.end()) ? aliasIt->second.eventType : Yume::KeyEventType::KeyDown;

        Yume::KeyCode code{};
        if (!Yume::resolveKeyName(keyName, code)) {
            LOG_WARN("Tecla desconhecida para ação '" + alias + "' (nome '" + keyName +
                     "') nesta plataforma; binding ignorado");
            return Value::nil();
        }

        // Aridade do callback do script: 0 (ignora dt) ou 1 (recebe dt). O
        // engine entrega o dt do frame; preenchemos os args de acordo.
        Value callback = args[1];
        size_t arity = 0;
        if (callback.type == ValueType::Function && callback.function) {
            const ScriptFunction& sf = *callback.function;
            if (sf.lambda)
                arity = sf.lambda->params.size();
            else if (sf.decl)
                arity = sf.decl->params.size();
        }

        input->bindKey(
            code,
            [interp, callback, arity](float dt) {
                try {
                    std::vector<Value> callArgs;
                    callArgs.reserve(arity);
                    // Primeiro parâmetro recebe o dt do frame; demais (se o
                    // script declarar mais) ficam nil.
                    for (size_t i = 0; i < arity; i++)
                        callArgs.push_back(i == 0 ? Value::makeNumber(dt) : Value::nil());
                    Value cb = callback; // call() recebe por referência não-const
                    interp->call(cb, callArgs, 0);
                } catch (const yumescript::RuntimeError& e) {
                    LOG_ERROR(std::string("Erro no callback de input: ") + e.what());
                }
            },
            eventType);
        return Value::nil();
    };

    // Polling: resolve o alias (ou nome de tecla cru) para KeyCode e devolve
    // true/false. Compartilhado por isPressed/wasPressed/wasReleased, que só
    // diferem na função de estado consultada no backend.
    auto makePollFn = [](bool (Yume::IInput::*query)(Yume::KeyCode)) {
        return [query](std::vector<Value>& args) -> Value {
            if (args.size() != 1 || args[0].type != ValueType::String) {
                LOG_WARN("Função de polling de input espera (string alias)");
                return Value::makeBool(false);
            }
            Yume::IInput* input = Yume::ScriptServices::input;
            if (!input)
                return Value::makeBool(false);

            const std::string& alias = *args[0].str;
            const auto& aliases = Yume::ScriptServices::inputAliases;
            auto aliasIt = aliases.find(alias);
            std::string keyName = (aliasIt != aliases.end()) ? aliasIt->second.value : alias;

            Yume::KeyCode code{};
            if (!Yume::resolveKeyName(keyName, code))
                return Value::makeBool(false);
            return Value::makeBool((input->*query)(code));
        };
    };

    Value onActionValue = Value::makeNative(onActionFn);
    Value isPressedValue = Value::makeNative(makePollFn(&Yume::IInput::isKeyPressed));
    Value wasPressedValue = Value::makeNative(makePollFn(&Yume::IInput::wasKeyPressed));
    Value wasReleasedValue = Value::makeNative(makePollFn(&Yume::IInput::wasKeyReleased));

    auto inputSystem = std::make_shared<ObjectValue>();
    inputSystem->members = {"onAction", "isPressed", "wasPressed", "wasReleased"};
    inputSystem->get = [onActionValue, isPressedValue, wasPressedValue,
                        wasReleasedValue](const std::string& name) -> Value {
        if (name == "onAction")
            return onActionValue;
        if (name == "isPressed")
            return isPressedValue;
        if (name == "wasPressed")
            return wasPressedValue;
        if (name == "wasReleased")
            return wasReleasedValue;
        return Value::nil();
    };
    inputSystem->set = [](const std::string&, const Value&) {};
    Value inputSystemValue = Value::makeObject(inputSystem);

    auto yume = std::make_shared<ObjectValue>();
    yume->members = {"InputSystem"};
    yume->get = [inputSystemValue](const std::string& name) -> Value {
        if (name == "InputSystem")
            return inputSystemValue;
        return Value::nil();
    };
    yume->set = [](const std::string&, const Value&) {};
    return Value::makeObject(yume);
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

    // API global `Yume`, hoje com `Yume.InputSystem.on(alias, fn)` para ligar
    // teclas a callbacks do script usando os aliases de project.conf.
    script->defineGlobal("Yume", buildYumeObject());

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
