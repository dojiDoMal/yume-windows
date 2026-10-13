---
inclusion: fileMatch
fileMatchPattern: ['core/src/yumescript/**', 'core/src/components/script_component.*', 'core/src/components/script_services.*', '**/*.ys']
---

# Guia do Módulo: YumeScript

Linguagem de script embutida (`.ys`). Sintaxe híbrida: keywords estilo JS
(`let`, `function`, `if`, `else`, `while`, `return`, `true`/`false`, `null`) com
blocos por indentação estilo Python (NEWLINE/INDENT/DEDENT, sem chaves nem `;`).

> Existe um design doc em `core/src/yumescript/DESIGN.md`. **O código atual já
> ultrapassou a Fase 1 descrita lá** — confie no código, não só no design.
> Referência: #[[file:core/src/yumescript/DESIGN.md]]

## Pipeline

`Lexer → Parser → AST → Interpreter` (tree-walking, `Environment` encadeado). A
fachada é `yumescript::Script` (`yumescript.{hpp,cpp}`), que encadeia tudo e expõe
`loadFile/loadSource`, `defineGlobal`, `hasFunction`, `invoke`.

- `token.hpp` — `enum class TokenType`.
- `lexer.{hpp,cpp}` — tokenizer com rastreio de indentação; `keywords()` mapeia
  lexemas → keyword.
- `ast.hpp` — nós + visitors (`ExprVisitor`/`StmtVisitor`).
- `parser.{hpp,cpp}` — recursivo-descendente. Precedência:
  `assignment → equality → comparison → term → factor → unary → call → primary`.
- `value.hpp` — `Value` dinâmico: nil/bool/number/string/function/native/object.
- `interpreter.{hpp,cpp}` — execução; `call()` trata funções nomeadas, lambdas
  (inline e bloco) e nativas.

## O que existe hoje

- **Statements:** `Let`, `Function`, `If`/`else`, `While`, `Return`, `Expr`.
- **Expressões:** `Literal`, `Identifier`, `Unary` (só `-`), `Binary`, `Call`,
  `Member` (`a.b.c`), `Assign` (`=`, `+=`, `-=`, `*=`, `/=`), `Function` (lambda).
- **Lambdas arrow:** `(params) => expr` (inline, pode ser argumento) e
  `(params) => :` + bloco indentado (só em contexto de statement).
- `+` concatena se um lado é string; autovivificação em atribuição a identificador.

**Ainda não há** (Fase 2 / não implementado): operadores lógicos `and`/`or`/`!`,
arrays/mapas, `elif`, acesso a assets/renderer, e o bytecode `.ysb` + VM.

## Integração com a engine (`ScriptComponent`)

`components/script_component.{hpp,cpp}` (namespace **global**) herda `Component`:

- `start()`: cria o `Script`, injeta globais **antes** de carregar o fonte —
  `log`/`print` (nativas), `this` (`buildThisObject`, expõe `this.transform`),
  `Yume` (`buildYumeObject`, expõe `Yume.InputSystem`) — carrega o `.ys` e chama
  `start()` do script se existir.
- `update(dt)`: chama `update` do script passando `dt` como **argumento** (não é
  global injetada).
- `this.transform` é uma fachada sobre o `Transform` do owner com membros
  `x/y/z` (global), `localX/Y/Z`, `globalX/Y/Z`. Escrita global só funciona em
  objeto raiz; rotação global é só leitura (Euler não decompõe trivialmente).
- `Yume.InputSystem`: `onAction(alias, cb)`, `isPressed/wasPressed/wasReleased(alias)`.
  Resolve aliases de `project.conf` via `ScriptServices` + `key_names`.

`components/script_services.{hpp,cpp}` (namespace `Yume`): holder **estático** com
`IInput* input` e os `inputAliases`. `Application::boot` chama
`ScriptServices::configure(...)` antes do `start()` dos componentes.

## Pontos de extensão

- **Novo operador/token:** valor em `TokenType` → case em `tokenTypeName()` →
  emitir no lexer (`match()` p/ 2 chars, como `+=`/`=>`) → consumir no parser no
  nível de precedência certo → tratar no interpreter.
- **Nova keyword:** registrar em `keywords()` e rotear em `declaration()`/`primary()`.
- **Novo statement:** struct `XStmt : Stmt` (com `accept`) → `visitX` no
  `StmtVisitor` → forward-decl → `xStatement()` no parser → `visitX` no interpreter
  (use `executeBlock(..., make_shared<Environment>(env))` para escopo; cheque
  `returning` em loops).
- **Função nativa:** `script->defineGlobal("nome", Value::makeNative([](std::vector<Value>& a){...}))`.
  Nativas validam aridade/tipos **manualmente**.
- **Objeto nativo:** `make_shared<ObjectValue>()`, preencher `members` e os
  callbacks `get`/`set`; `Value::makeObject(obj)`. Padrão em `makeTransformFacade`,
  `buildYumeObject`.

## Testes

Alvo CMake `yumescript_test` (só builds nativos), compilado standalone (sem
`yume_core`). Rode: `cmake --build <build> --target yumescript_test` e depois
`tools/yumescript_test [arquivo.ys]`. É smoke test (sem framework de assert); sem
argumento carrega `core/src/yumescript/tests/example.ys`.

## Convenções

Namespace `yumescript` (minúsculo) na linguagem; `ScriptComponent` global;
`ScriptServices` em `Yume`. Include guards `YUMESCRIPT_<FILE>_HPP`. Doxygen PT-BR.
Erros são classes derivadas de `std::runtime_error` com `line`/`column`
(`LexError`, `ParseError`, `RuntimeError`).
