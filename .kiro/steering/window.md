---
inclusion: fileMatch
fileMatchPattern: ['core/src/window/**']
---

# Guia do Módulo: Window / MML

Janela, apresentação e camada de multimídia da plataforma. É o **ponto de
contato com SDL/libctru**; o resto da engine permanece agnóstico de plataforma.

## Peças

- `window_desc.hpp` — `WindowDesc` simples (title, width, height, extraFlags).
- `display_manager.{hpp,cpp}` — `DisplayManager` orquestra renderer + janela:
  `init(DisplayBackend&, WindowDesc)` cria renderer e janela; `render(Scene&)`,
  `present()`, `getWindow()` (`void*` opaco). **Usa** o `DisplayBackend` mas não é
  dono dele (a `MultimediaLayer` é).
- `mml/` — **MultiMedia Layer**, a abstração de janela/mídia:
  - `multimedia_layer.hpp` — `MultimediaLayer`: um único `init()`/`end()` que liga
    o subsistema da plataforma (ex.: `SDL_Init` vídeo+áudio) e **detém** três
    sub-backends: `display()` (`DisplayBackend`), `input()` (`Yume::IInput`),
    `audio()` (`AudioBackend`). A posse do ciclo de vida da plataforma
    (SDL_Init/SDL_Quit) vive aqui — os sub-backends não chamam isso sozinhos.
  - `multimedia_layer_factory.*` — escolhe a implementação por `GraphicsAPI`
    (CITRO3D → N3DS; demais → SDL2), em compile-time por `#ifdef`.
  - `display_backend.hpp` — `DisplayBackend`: o **único** ponto que fala com a
    plataforma sobre a janela. Expõe tudo por `void*` opaco (handle da janela,
    contexto GL, surface Vulkan, HWND nativo), para o renderer nunca ver um
    `SDL_Window`. Virtuais puros: `configureContext`, `createMainDisplay`,
    `destroyDisplay`; serviços por API (GL/Vulkan/nativo) têm default no-op/null.
  - `sdl2/` — implementações SDL2: `sdl2_multimedia_layer`, `sdl2_display_backend`
    (base) e um por API: `sdl2_gl_...`, `sdl2_vulkan_...`, `sdl2_d3d12_...`.
  - `3ds/` — implementação Nintendo 3DS (N3DS layer + backend citro3d).
  - `audio/` — `audio_backend.hpp` + `sdl2/` e `3ds/`.

## Princípios

- **O renderer é agnóstico de SDL.** Qualquer serviço de janela/contexto chega ao
  `RendererBackend` via `DisplayBackend` como `void*`. Não exponha tipos SDL para
  fora de `window/mml/sdl2/`.
- **A posse da plataforma mora na `MultimediaLayer`.** `SDL_Init`/`SDL_Quit` (ou
  equivalentes) só acontecem no `init()`/`end()` dela, de forma determinística.
- **Seleção por API em compile-time** (`#ifdef`) + por `GraphicsAPI` na factory,
  igual ao renderer. Um display backend por API espelha um renderer backend.

## Ao adicionar suporte de janela para um novo backend/API

1. Se usar SDL: crie `sdl2_<api>_display_backend.{hpp,cpp}` em `window/mml/sdl2/`,
   implementando os serviços de contexto/surface que a API precisa (os que não se
   aplicam ficam com o default).
2. Faça a `MultimediaLayerFactory`/`sdl2_multimedia_layer` instanciarem o backend
   certo para a nova `GraphicsAPI`.
3. No `CMakeLists.txt`, inclua/exclua o novo arquivo na poda por plataforma
   (os display backends específicos de API são removidos onde a API não existe).

## Convenções

Namespace **global** (as classes de janela não usam `Yume`; `IInput`, que a layer
expõe, está em `Yume`). Include guards `#ifndef`. `void*` opaco para handles de
plataforma. Doxygen PT-BR. Logging com `#define CLASS_NAME` + `log_macros.hpp`.
