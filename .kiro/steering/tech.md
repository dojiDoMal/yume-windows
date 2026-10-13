# Yume Engine — Stack Técnico, Build e Convenções

## Linguagem e padrões globais

- **C++17** (`CMAKE_CXX_STANDARD 17`). `CMAKE_EXPORT_COMPILE_COMMANDS ON`.
- Build default é **Debug** (`-g -O0`) quando nenhum é passado. Debug define
  `YUME_DEBUG` (via `$<$<CONFIG:Debug>:YUME_DEBUG>`), público em `yume_core`.
- `MAX_WORLD_OBJECTS` é definido globalmente: **64 no Web**, **10240 no resto**.
  Ele dimensiona a validação da cena e precisa ser **o mesmo** na engine e no
  `scene_compiler` (ambos recebem o define via CMake).

## Como compilar

A engine é uma biblioteca; o build standalone só gera `yume_core` + ferramentas
de host. Use `build.bat` (Windows) com um alvo:

```powershell
build.bat                 # desktop nativo (= -t native)
build.bat -t native       # desktop: MinGW Makefiles + MSYS2 UCRT64 gcc + vcpkg
build.bat -t web          # WebGL: emcmake cmake --preset web
build.bat -t switch        # Switch: delega para build_switch.sh no bash devkitPro
build.bat -c              # limpa o build dir e os *.scnb obsoletos
build.bat -t native -c    # limpa e recompila
```

Observações importantes:
- `-r/--run` **não tem efeito** (a engine não tem app para rodar).
- O alvo **native NÃO usa o preset `native`**: o script configura à mão com
  `-G "MinGW Makefiles"`, gcc/g++ do MSYS2 UCRT64 e toolchain do vcpkg (triplet
  `x64-mingw-dynamic`), saída em `build\pc`.
- **Web** e **Switch** usam os presets (`CMakePresets.json`): `build/web`,
  `build/switch`. O Switch precisa de `DEVKITPRO_BIN` e das ferramentas de host
  `HOST_DXC` / `HOST_SPIRV_CROSS` para gerar os shaders `.nxs`.
- **3DS não tem atalho** no `build.bat`; configure via toolchain devkitARM 3DS
  pelo CMake diretamente.

Após mudar código, compile o alvo relevante para validar antes de dar por pronto.

## Seleção de fontes e de plataforma (CMake)

- `CMakeLists.txt` coleta fontes com `file(GLOB_RECURSE core/src/*.cpp)`. **Um
  `.cpp` novo é compilado automaticamente** — não é preciso listá-lo. (Reconfigure
  o CMake para o glob pegar o arquivo novo.)
- Removidos do `yume_core`: `scene/scene_compiler.cpp` (ferramenta host) e
  `yumescript/tests/*` (harness host).
- Backends são **podados por plataforma**: cada ramo (`IS_3DS`, `EMSCRIPTEN`,
  `IS_SWITCH`, desktop) remove os diretórios de `renderer/backends/*` e os display
  backends que não existem naquela plataforma, via `foreach(BACKEND ...)`.
- Deps gráficas por plataforma ficam em `cmake/{PC,Switch,N3DS,Web}.cmake`,
  anexadas como PUBLIC a `yume_core`.

## Pipeline de assets (cmake/YumeProject.cmake)

`yume_add_project()` é o ponto de entrada do projeto consumidor. Ele cria o
executável, linka `yume_core` e transforma os assets de autoria nos artefatos de
runtime, colocados ao lado do executável (ou num `romfs/` de staging nos
consoles):

- `yume_compile_shaders` — HLSL (`*.vxs`/`*.pxs`) → SPIR-V (dxc) → GLSL/`.nxs`
  (spirv-cross); no Windows também gera DXIL `.cso`.
- `yume_compile_pica_shaders` — só 3DS: `*.v.pica` → `.shbin` (picasso).
- `yume_compile_scenes` — `*.scn` → `*.scnb` (host tool `scene_compiler`).
- `yume_copy_assets` — copia verbatim: `*.obj`, `*.png`, `*.ys`, `project.conf`.

Consoles empacotam ao final: Switch → `.nro` (elf2nro + romfs), 3DS → `.3dsx`
(3dsxtool + romfs). A extensão de shader que o build gera **tem que casar** com o
`getShaderExtension()` do backend em runtime.

## Convenções de código (SEGUIR SEMPRE)

- **Namespace seletivo.** Use `namespace Yume` apenas no que já está nele:
  `Application`, `Context`, `platform_paths`, `input/*` (`IInput`), a classe
  `Math`, `ScriptServices`. O resto é **global, sem namespace**: `components/*`,
  `scene/*`, `assets/*`, `renderer/*`, `window/*`, `Logger`, `Timer`,
  `GraphicsAPI`, os tipos `Vector*/Matrix*`. A linguagem de script usa
  `namespace yumescript` (minúsculo). Ao criar um arquivo novo, **siga o módulo
  onde ele mora**, não invente um namespace.
- **Include guards** `#ifndef NOME_HPP / #define / #endif` (NOME em MAIÚSCULAS).
  Não use `#pragma once` (há uma exceção legada em `timer.hpp`; não propague).
- **Includes** sempre a partir de `core/src` (ex.: `"components/camera.hpp"`).
- **Formatação** pelo `.clang-format` (base LLVM): indent 4, coluna máx. 100,
  `PointerAlignment: Left` (`Tipo* ptr`). Rode o clang-format no que editar.
- **Nomenclatura:** classes/tipos em `PascalCase`; métodos e membros em
  `camelCase` sem prefixo; `enum class` com membros `PascalCase` ou `UPPER_CASE`
  conforme o enum vizinho; arquivos em `snake_case`; arquivos de backend com
  prefixo da API (`open_gl_`, `vulkan_`, `d3d12_`, `web_gl_`, `citro3d_`);
  interfaces com prefixo `I` (`IInput`); macros em `UPPER_SNAKE`.
- **Logging:** no topo de cada `.cpp`, antes de incluir `log_macros.hpp`:
  ```cpp
  #define CLASS_NAME "MinhaClasse"
  #include "log_macros.hpp"
  ```
  e use `LOG_INFO(msg)` / `LOG_WARN(msg)` / `LOG_ERROR(msg)` (recebem `std::string`).
- **Posse de memória:** `unique_ptr` para posse exclusiva, `shared_ptr` para
  recursos compartilhados/cacheados (Mesh/Material), ponteiro cru para
  observadores não-donos (`owner`, `parent`, retorno de `getComponent`).
- **Documentação Doxygen em português** (`@brief`, `@param`, `@return`, `@see`),
  mantida pelo agente `doc-maintainer`.
- **Gates de plataforma:** `#ifdef __SWITCH__`, `#ifdef __3DS__`,
  `#ifdef PLATFORM_WEBGL`, `#ifdef _WIN32`, `#ifdef YUME_DEBUG`. Essas macros vêm
  do toolchain/CMake; não as redefina.
- **Config resiliente:** ao ler config/cena, nunca aborte por valor inválido —
  caia num default seguro e emita `LOG_WARN`.

## Testes

- Único alvo de teste automatizável: `yumescript_test` (apenas builds nativos).
  Compile com `cmake --build <build-dir> --target yumescript_test` e rode
  `tools/yumescript_test [arquivo.ys]`. É um smoke test (sem framework de assert).
- Não há suíte de testes para a engine inteira nem integração com CTest. **Não
  adicione testes a menos que seja pedido.**
