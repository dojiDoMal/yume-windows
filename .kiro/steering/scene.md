---
inclusion: fileMatch
fileMatchPattern: ['core/src/scene/**', '**/*.scn']
---

# Guia do Módulo: Scene

Pipeline de cena baseado no pilar **autoria separada de runtime**: humanos
editam `.scn` (JSON); o build compila para `.scnb` (binário POD); a engine
carrega o `.scnb` em runtime.

## Pipeline em três estágios

```
.scn (JSON autoria)  ──scene_compiler──►  .scnb (POD binário)  ──SceneLoader──►  WorldObjects vivos
```

- **`scene_format.hpp`** — o **contrato POD compartilhado** entre a ferramenta de
  build e o runtime. Define `SceneHeader`, `WorldObjectData` (tamanho fixo),
  `ComponentData` (tagged union), o `enum class ComponentType` e as structs
  `*Data`. Arquivo mais importante do módulo.
- **`scene_compiler.cpp`** — ferramenta de **host** com `main()` próprio (não faz
  parte de `yume_core`). Lê o JSON com nlohmann, preenche as structs POD e grava
  `[SceneHeader][N × WorldObjectData]`. Funções `compileX(...)` por tipo de
  componente.
- **`scene_loader.{hpp,cpp}`** — runtime. `loadCompiledScene` lê o header + o
  bloco inteiro de uma vez; `loadWorldObjects` instancia em **duas passadas**:
  (1) cria objetos + componentes; (2) resolve `parentIndex → ponteiro` (o pai
  pode aparecer depois do filho). Mantém caches próprios (`meshCache`,
  `materialCache`, `fontAtlasCache`).
- **`world_object.{hpp,cpp}`** — nó do grafo: `Transform` embutido + vetor de
  `Component` + hierarquia parent/children. `getWorldMatrix()` combina a cadeia
  de pais (sem cache por ora).
- **`world_object_manager`**, **`scene`**, **`scene_manager`**, **`skybox`**,
  **`sprite`** — gerenciamento, cena ativa e seleção de câmera.

## INVARIANTE central: só caminhos, nunca bytes

O `.scn`/`.scnb` guardam apenas **PATHs** (`char[256]`) para recursos pesados
(`.obj`, `.png`, shaders, `.ys`) — **nunca** os bytes desses recursos. É isso que
permite tratar cada `WorldObjectData` como POD de tamanho fixo e ler/gravar o
array inteiro em bloco. Os recursos de verdade são carregados sob demanda pelo
`SceneLoader` em runtime. Não embuta dados variáveis no formato.

## `enum class ComponentType` (uint8_t)

`TRANSFORM=0, MESH_RENDERER=1, SPRITE_RENDERER=2, CAMERA=3, LIGHT=4,
TEXT_RENDERER=5, LOD_GROUP=6, SCRIPT=7`.

No **JSON**, a `transform` é um campo de topo do objeto (não entra em
`components`). Os `type` reconhecidos em `components` são os demais. Um `type`
desconhecido é **ignorado silenciosamente** pelo compilador.

### Shape do `.scn` (resumo)

Topo: `{ "worldObjects": [ ... ] }`. Cada objeto: `parent` (índice, opcional,
default raiz), `transform` ({position, rotation, scale}: arrays de 3 floats) e
`components` (máx. `MAX_COMPONENTS_PER_OBJECT = 8`). Cores no JSON são inteiros
**0–255** e o compilador normaliza para float `/255`.

## Como adicionar um novo tipo de componente (checklist)

1. **`scene_format.hpp`**: novo valor em `ComponentType` + struct correspondente
   dentro da union de `ComponentData` (campos POD de tamanho fixo; só paths).
2. **`scene_compiler.cpp`**: `void compileX(ComponentData&, const json&)` + um
   `else if (type == "X")` no dispatch de `compileWorldObjects`.
3. **`scene_loader.cpp`**: `loadXComponent(WorldObject*, const ComponentData&)` +
   um `case ComponentType::X:` no switch de `loadWorldObjects`.
4. A classe do componente em `components/` (ver `components.md`).
5. Se introduzir um novo **tipo de asset de arquivo**, adicione o glob em
   `yume_copy_assets` (`cmake/YumeProject.cmake`).

## Versionamento do formato

`SCENE_MAGIC = 0x53434E46` ('SCN' + byte de versão). Se uma mudança alterar o
`sizeof` de `WorldObjectData`/`ComponentData` (campo novo, reordenação), **bump do
byte de versão** e recompile todas as cenas — o loader rejeita `.scnb` com magic
antigo. `build.bat -c` apaga `.scnb` obsoletos.

## Convenções

Namespace **global**. Include guards `#ifndef`. Resolução de caminho via
`Yume::resolveAssetPath` (romfs:/ nos consoles) **uma única vez**, no ponto em que
o path vira abertura de arquivo. Logging com `#define CLASS_NAME` + `log_macros.hpp`.

> Lembre: `scene_compiler` e `scene_loader` precisam enxergar o **mesmo**
> `scene_format.hpp` e o **mesmo** `MAX_WORLD_OBJECTS`. Divergência corrompe a
> leitura.
