---
inclusion: fileMatch
fileMatchPattern: ['core/src/input/**']
---

# Guia do Módulo: Input

Entrada de teclado/botões multiplataforma por trás de uma interface abstrata.
Tudo neste módulo está em `namespace Yume`.

## Peças

- `i_input.hpp` — interface `IInput` (virtual pura):
  - `processEvents(float deltaTime)` — uma vez por frame; processa a fila e
    dispara os callbacks (o `dt` vai para ações `KeyHold`).
  - `bindKey(KeyCode, std::function<void(float)> cb, KeyEventType = KeyDown)`.
  - `getQuitEvent()`, `requestQuit()`.
  - `isKeyPressed(KeyCode)` (estado contínuo), `wasKeyPressed` (borda down),
    `wasKeyReleased` (borda up).
  - A interface só usa `std::function`/`KeyCode`; SDL é incluído só fora do 3DS.
- `input_key.hpp` — `using KeyCode = intptr_t` (largo o bastante para qualquer
  backend, sem acoplar a engine a uma lib). `enum class KeyEventType {KeyDown,
  KeyHold, KeyUp}`.
- `i_input_factory.{hpp,cpp}` — `IInputFactory::create()` escolhe por `#ifdef`:
  `N3DSInput` sob `__3DS__`, senão `DesktopInput`. Escolha em **compile-time**.
- `desktop_input.{hpp,cpp}` — implementação SDL. Guarda estado do teclado,
  bindings e conjuntos de bordas (pressed/released neste frame).
- `n3ds_input.{hpp,cpp}` — implementação libctru (hid). KeyCode = máscaras `KEY_*`.
- `key_names.{hpp,cpp}` — `bool resolveKeyName(const std::string&, KeyCode&)`:
  traduz nomes lógicos estilo JS (`"ArrowUp"`, `"KeyA"`, `"Space"`) para o
  `KeyCode` **nativo** da plataforma. A tradução é isolada por plataforma.

## Padrão multiplataforma

Interface única (`IInput`) + factory por macro + `KeyCode` opaco (`intptr_t`) +
tradução de nomes isolada (`key_names`). O resto da engine só lida com **nomes
lógicos** e `IInput`; nunca com `SDL_Keycode` ou `KEY_*` diretos.

## Quem consome

- `Application::mainLoop` chama `processEvents(dt)` e checa `getQuitEvent()`.
- `MultimediaLayer::input()` expõe a instância; `Yume::Context::getInputSystem()`
  a repassa.
- **Scripts** acessam via `Yume.InputSystem` (`onAction`/`isPressed`/...), que
  resolve aliases de `project.conf` por `ScriptServices` + `key_names` (ver
  `yumescript.md`). Os aliases mapeiam um nome lógico (ex.: `"moveUp"`) para um
  nome de tecla + `KeyEventType`.

## Como adicionar uma nova tecla/nome lógico

Adicione o mapeamento em `key_names.cpp` para **cada** plataforma relevante
(desktop e 3DS têm tabelas distintas), traduzindo o nome lógico para o `KeyCode`
nativo. Se `resolveKeyName` falhar, o binding é ignorado com `LOG_WARN`.

## Convenções

Namespace `Yume`. Include guards `#ifndef I_INPUT_HPP` etc. Interface com prefixo
`I`. `KeyCode`/`KeyEventType` são os tipos de troca. Doxygen PT-BR. Logging com
`#define CLASS_NAME` + `log_macros.hpp`.
