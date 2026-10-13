---
inclusion: fileMatch
fileMatchPattern: ['core/src/components/**']
---

# Guia do Módulo: Components

Dados e comportamento anexados a um `WorldObject`, no estilo entidade-componente.

> O `ScriptComponent`/`ScriptServices` têm um guia próprio — ver `yumescript.md`.

## Classe base `Component` (`component.hpp`, namespace global)

- Membro protegido: `WorldObject* owner` (não-dono).
- Ciclo de vida (ganchos virtuais, no-op por padrão):
  - `virtual void start()` — chamado **uma vez** após a cena carregar, antes do
    primeiro update e antes de `onInit` do projeto.
  - `virtual void update(float deltaTime)` — chamado **todo frame**, antes da
    renderização e antes de `onUpdate` do projeto.
- `setOwner()` / `getOwner()`.

## Quem dirige o ciclo de vida

`Yume::Application` orquestra, delegando ao `WorldObject`:

- `boot()` → `startSceneComponents()` → `obj->startComponents()` → `comp->start()`.
- `mainLoop()` → `updateSceneComponents(dt)` → `obj->updateComponents(dt)` →
  `comp->update(dt)` — **antes** de `onUpdate(dt)` e do render.

**Exceção importante:** `LodGroup` **não usa** o `update(float)` virtual. Ele tem
um `update(objPos, radius, camPos, fov, height)` com assinatura própria, chamado
pelo **Renderer** durante a frame, não pelo loop de componentes. Não confunda os
dois.

## Ligação com `WorldObject` (em `scene/world_object.hpp`)

- `WorldObject` guarda `std::vector<std::unique_ptr<Component>>` (posse exclusiva)
  + um `Transform` embutido (que **não** é um Component).
- `addComponent<T>(unique_ptr<T>)` — faz `static_assert(is_base_of<Component,T>)`,
  seta owner, move para o vetor, devolve observador.
- `getComponent<T>()` / `hasComponent<T>()` — busca por tipo via `dynamic_cast`.

## Componentes concretos

- `transform.{hpp,cpp}` — position/rotation(Euler)/scale, com `Matrix4` cacheado
  (recálculo lazy via flag `dirty`). Não herda de Component.
- `camera.{hpp,cpp}` — fov, near/far, viewport, ortho/orthoSize, backgroundColor,
  Skybox opcional. Posição/orientação vêm do `Transform` do owner.
- `light.{hpp,cpp}` — `enum class LightType {DIRECTIONAL, POINT, SPOT}`, direção,
  cor, intensidade.
- `mesh_renderer.hpp` (header-only) — segura `shared_ptr<Material>`.
- `sprite_renderer.hpp` (header-only) — segura `unique_ptr<Material>`.
- `text_renderer_component.hpp` (header-only) — `FontAtlas` + `TextRenderer`.
- `lod_group.hpp` (header-only) — níveis de LOD; `update` próprio (ver acima).
- `script_component.*`, `script_services.*` — ver `yumescript.md`.

## Como criar um componente novo

1. `class X : public Component` em `components/x.{hpp[,cpp]}`. Header-only se for
   só dados; `.hpp+.cpp` se tiver lógica. Sobrescreva `start()`/`update(dt)` só se
   precisar de ciclo de vida.
2. Anexe/consulte via `obj->addComponent<X>(...)` / `getComponent<X>()`.
3. Se o componente deve ser **declarável em cena**, integre no pipeline de cena
   (`ComponentType` + compiler + loader) — ver `scene.md`.

## Convenções

Namespace **global**. Include guards `#ifndef X_HPP`. Classes PascalCase, membros
camelCase. Doxygen PT-BR. Componentes simples ficam header-only; os com lógica têm
`.cpp`. Logging com `#define CLASS_NAME` + `log_macros.hpp` nos `.cpp`.
