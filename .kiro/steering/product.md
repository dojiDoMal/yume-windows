# Yume Engine — Visão do Produto

A **Yume Engine** é um game engine **C++17 multiplataforma**, com foco em rodar o
mesmo conteúdo em desktop e consoles portáteis a partir de uma única base de
código. A engine é distribuída como **biblioteca estática** (`yume_core`); ela
**não tem executável próprio**.

## O que a engine é (e o que não é)

- **É uma biblioteca.** Um build standalone produz só `yume_core` + ferramentas
  de host (`scene_compiler`, `yumescript_test`). Não há "app" nem janela quando
  você compila só a engine.
- **Quem roda é um projeto consumidor.** Um projeto (ex.: `../yume-examples/rotating-cube`)
  linka `yume_core`, fornece seu próprio `main.cpp` e chama `yume_add_project()`
  no CMake. O executável/`.nro`/`.3dsx` é artefato do projeto, não da engine.
- O executável final (in-repo) previsto é o **editor**, não um jogo de exemplo.

## Plataformas-alvo

| Plataforma        | API gráfica        | Toolchain              |
|-------------------|--------------------|------------------------|
| Desktop (Win/Linux) | OpenGL, Vulkan, DirectX 12 (Windows) | MinGW/MSYS2 + vcpkg |
| Nintendo Switch   | OpenGL ES (EGL/glad) | devkitPro/libnx       |
| Nintendo 3DS      | Citro3D (PICA200)  | devkitARM/libctru      |
| Web               | WebGL (Emscripten) | emsdk (em hiato)       |

Portabilidade é um requisito de primeira classe, não um detalhe. Praticamente
toda decisão de arquitetura (interfaces abstratas + fábricas por `GraphicsAPI`,
caminhos de asset resolvidos por plataforma, seleção de código por macro) existe
para manter essas quatro famílias funcionando a partir do mesmo núcleo.

## Pilares de design

1. **Autoria separada de runtime.** Humanos editam formatos legíveis (`.scn`
   JSON, shaders HLSL, scripts `.ys`); o build pré-processa tudo para o formato
   que a engine carrega em runtime (`.scnb` binário, GLSL/`.cso`/`.shbin`,
   `.ys` copiado). A engine em runtime nunca faz o trabalho pesado de parsing de
   autoria.
2. **O engine sempre inicia.** Parsing de configuração/cena é resiliente: valor
   ausente ou inválido cai em um default seguro e emite `LOG_WARN`, em vez de
   abortar o boot.
3. **Agnóstico de plataforma por interface.** O núcleo fala com interfaces
   abstratas (`RendererBackend`, `IInput`, `DisplayBackend`, `MultimediaLayer`);
   o código específico de SDL/libctru/API gráfica fica isolado atrás delas.

## Quem usa a engine

- **Autores de cena/projeto:** escrevem `.scn`, `project.conf`, shaders e `.ys`.
- **Desenvolvedores de jogo:** herdam de `Yume::Application`, sobrescrevem
  `onInit()`/`onUpdate(dt)` e, cada vez mais, movem a lógica por frame para
  componentes e scripts YumeScript.
- **Mantenedores da engine:** trabalham dentro de `core/src/`, respeitando as
  fronteiras de módulo e as convenções descritas em `structure.md` e `tech.md`.
