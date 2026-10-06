# YumeScript (.ys) — Design e Plano de Implementação

Linguagem de scripting embutida da Yume Engine. Sintaxe híbrida: keywords no
estilo JavaScript (`let`, `function`, `if`, `else`, `return`, `true`/`false`),
com a sintaxe enxuta do Python (sem parênteses obrigatórios, sem chaves, sem
ponto e vírgula; blocos definidos por indentação).

Status: planejamento aprovado. Fase 1 em aberto para implementação.

---

## 1. Decisão de arquitetura: faseamento (parse em runtime -> compilado)

A Yume separa autoria (`.scn` JSON, editado por humanos) de runtime (`.scnb`
binário POD, lido pela engine). Todo artefato que a engine carrega em runtime
já é pré-processado: `.scn` -> `.scnb`, shaders HLSL -> `.glsl`/`.cso`. A longo
prazo, scripts devem seguir a mesma filosofia (compilar `.ys` -> `.ysb` no
build). Esse é o destino.

Mas o faseamento abaixo foi decidido para destravar a linguagem rápido sem
pagar o custo de manter um formato serializado enquanto a gramática ainda muda.

### Fase 1 — parse em runtime, interpretador tree-walking (ATUAL)
- O `.ys` é copiado verbatim para a pasta de build (como `.obj`/`.png`).
- Em runtime, o `ScriptComponent` lê o texto e roda tokenizer -> parser -> AST.
- A AST é interpretada diretamente (tree-walking). Sem VM, sem bytecode.
- Vantagem: um único pipeline, dentro da engine; itera a linguagem rápido sem
  nenhum formato intermediário para manter sincronizado.

### Fase 2 — compilar para bytecode `.ysb` no build (FUTURO)
- Só quando a gramática estabilizar.
- Reusa o MESMO tokenizer/parser da fase 1 dentro de uma host tool, que emite
  bytecode `.ysb`.
- Runtime passa a carregar `.ysb` e executar numa VM enxuta.
- Vantagem: erros de sintaxe pegos no build; startup mais rápido; runtime sem
  tokenizer/parser.

### Invariante que torna a migração segura
A escolha fase 1 vs fase 2 NÃO vaza para o `.scn`. O autor sempre escreve:

```json
{ "type": "SCRIPT", "path": "rotate.ys" }
```

Na fase 2, o `scene_compiler` reescreve esse path para `"rotate.ysb"` dentro do
`.scnb`. O `.scn` que o humano edita nunca muda. Migrar da fase 1 para a 2 não
quebra nenhuma cena nem a API de autoria.

### O que NÃO fazer
Nunca embutir o código/bytecode do script dentro dos bytes do `WorldObjectData`.
O formato POD tem campos de tamanho fixo (`char[256]`) e é lido em bloco;
scripts não têm tamanho fixo. O `.scnb` guarda apenas o PATH do script, igual
faz com `cube.obj` e os shaders.

---

## 2. Escopo mínimo da linguagem (Fase 1)

Superfície pequena de propósito, mas com a estrutura completa (tokenizer,
parser, AST, interpreter) para crescer sem reescrever.

Tokens:
- Números, strings, identificadores
- Keywords: `let`, `function`, `if`, `else`, `return`, `true`, `false`
- Operadores: `+ - * / = == != < > <= >=` e compostos (`+=` etc.)
- NEWLINE + INDENT/DEDENT (o tokenizer rastreia indentação, estilo Python)

AST:
- Statements: `Program`, `LetStmt`, `FunctionDecl`, `IfStmt`, `ReturnStmt`,
  `ExpressionStmt`
- Expressões: `Binary`, `Unary`, `Literal`, `Identifier`, `Call`,
  `MemberAccess` (para `this.transform.rotation.x`), `Assignment`

Interpreter:
- Tree-walking com `Environment` (escopo de variáveis encadeado)
- Chama `start()` e `update(dt)` definidos no script
- Injeta no escopo global: `this` (o objeto dono; expõe `this.transform`),
  `deltaTime`, e nativas (`log` / `print`)

Fora de escopo na Fase 1 (vem depois): loops, mais tipos, bindings de input,
acesso a assets/renderer.

### Exemplo alvo (`rotate.ys`)
```
let speed = 45

function update dt:
    this.transform.rotation.x += speed * dt
    this.transform.rotation.y += speed * dt
```

`this` é o objeto dono do script (injetado pelo ScriptComponent). Hoje expõe
apenas `this.transform`; novos membros entram aí conforme a API crescer.

---

## 3. Pontos de integração na engine

### 3.1 Débito técnico: ciclo de vida de Component (pré-requisito)
Hoje `core/src/components/component.hpp` só guarda `owner`; não há `update()`.
Toda lógica por frame vive no `onUpdate` da `Application` (ver
`yume-examples/rotating-cube/src/main.cpp`). Débito técnico já conhecido.

Mudança necessária na classe base:
```cpp
class Component {
protected:
    WorldObject* owner = nullptr;
public:
    virtual ~Component() = default;
    virtual void start() {}            // uma vez, após carregar a cena
    virtual void update(float dt) {}   // por frame
    void setOwner(WorldObject* obj) { owner = obj; }
    WorldObject* getOwner() const { return owner; }
};
```
E `Application::mainLoop` passa a percorrer os objetos da cena chamando `update`
em cada componente, antes do `render`.

### 3.2 Registrar o tipo SCRIPT (3 pontos + arquivos novos)
1. `core/src/scene/scene_format.hpp`: `SCRIPT = 7` no enum `ComponentType` +
   struct no union: `struct { char scriptPath[256]; } script;`
2. `core/src/scene/scene_compiler.cpp`: `else if (type == "SCRIPT")
   compileScript(...)` copiando `comp["path"]` para `scriptPath`.
3. `core/src/scene/scene_loader.cpp`: `case ComponentType::SCRIPT:` que
   instancia o `ScriptComponent`, passa o path, carrega/parseia o `.ys` e faz
   `addComponent`.
4. Novo: `core/src/components/script_component.hpp` (herda `Component`).
5. `cmake/YumeProject.cmake`: adicionar `*.ys` ao glob de `yume_copy_assets`
   (hoje copia `.obj`/`.png`/`project.conf`).

### 3.3 Ponte para o engine (fase 1 limitada)
- `this.transform`: fácil, via `getOwner()->getTransform()`. O `this` é um objeto
  injetado pelo ScriptComponent que embrulha o objeto dono.
- input/assets/renderer: adiados. O `Yume::Context` hoje só expõe input e é
  privado da `Application`. Expandi-lo e expor aos scripts é trabalho de fase 2.

---

## 4. Layout de arquivos (core/src/yumescript/)
- `token.hpp`       — tipos de token
- `lexer.{hpp,cpp}` — tokenizer com rastreio de indentação
- `ast.hpp`         — nós da AST
- `parser.{hpp,cpp}`— parser recursivo-descendente
- `value.hpp`       — valor em runtime (number/string/bool/nil/function)
- `interpreter.{hpp,cpp}` — tree-walking + Environment

Namespace: global (consistente com components/scene/assets, que não usam
`namespace Yume`). Include guards `#ifndef`. Doxygen em português.

---

## 5. Ordem de implementação recomendada
1. Linguagem isolada (lexer -> parser -> AST -> interpreter) com um harness de
   teste que roda um `.ys` e imprime o resultado. Reduz risco; não depende da
   engine.
2. Débito técnico do ciclo de vida de Component (`start`/`update` + loop na
   Application).
3. `ScriptComponent` + registro do tipo SCRIPT nos 3 pontos + regra de CMake.
4. Validar end-to-end no rotating-cube trocando o `onUpdate` em C++ por um
   `rotate.ys`.
