## Análise do `core/src` — Pontos de melhoria

### Visão geral da arquitetura
A engine é um framework de aplicação C++ multiplataforma (OpenGL, Vulkan, D3D12, EGL/Switch, WebGL) sobre SDL2, com modelo de cena estilo Unity (GameObject + componentes por composição, não ECS). A base é sólida, mas há dívidas técnicas acumuladas — várias já reconhecidas em `TODO` no próprio código.

---

### 🔴 Bugs / correção de posse (prioridade alta)

**1. Double-free latente do input (`engine_context.cpp` + `application.hpp`)**
O `Context` recebe `IInput*` com a documentação dizendo *"não assume a posse"* (`engine_context.hpp` linha ~20), mas `~Context()` faz `delete input`. Só que o dono real é `Application::inputMan` (um `std::unique_ptr<IInput>`). Ou seja, dois donos vão deletar o mesmo ponteiro. Hoje não quebra pela ordem/sorte de destruição, mas o contrato está furado.

Correção: ou o `Context` recebe apenas uma referência/observador e não deleta nada, ou passa a receber o `unique_ptr` e vira o dono único (e aí `Application` solta a posse).

---

### 🟠 Duplicação de código (prioridade alta — maior ganho de limpeza)

**2. Backends OpenGL e EGL são copy-paste um do outro**
`open_gl_renderer_backend.{hpp,cpp}` e `egl_renderer_backend.{hpp,cpp}` são quase idênticos: mesmos campos (SSBO de instâncias, VAO/VBO de sprite, UBOs de matrizes/material/luz), as mesmas structs internas `RenderKey`/`RenderKeyHash`/`InstanceGroup` copiadas, destrutor idêntico, e `createShaderProgram/createMeshBuffer/createShaderCompiler` idênticos. As únicas diferenças reais: o loader (`glew` vs `glad`), a extensão de shader (`.glsl` vs `.nxs`) e logs.

Sugestão: extrair uma base `GLRendererBackendBase` com toda a lógica comum, deixando virtuais só os pontos de variação. Elimina centenas de linhas duplicadas.

**3. As três factories são o mesmo esqueleto repetido**
`mesh_buffer_factory.cpp`, `shader_compiler_factory.cpp` e `shader_program_factory.cpp` têm estrutura idêntica: mesmo bloco `#ifdef __SWITCH__ / PLATFORM_WEBGL / else`, mesmo `switch(api)`, mesmos `static_cast<VulkanRendererBackend*>(context)`. Além disso, cada backend reimplementa `create*()` só para delegar à factory passando `getGraphicsAPI()` — boilerplate repetido em todo backend.

Sugestão: unificar num template de factory ou numa tabela de registro por backend.

**4. Inconsistência de estratégia de memória nas factories**
`RendererFactory` usa `new` + ponteiro cru (e `Renderer` dá `delete` no destrutor), enquanto `MeshBufferFactory`/`ShaderCompilerFactory`/`ShaderProgramFactory` usam `std::make_unique`. Padronizar tudo em `unique_ptr`.

~~**5. Criação de material duplicada 5x no `SceneLoader`**~~
~~`loadMeshRendererComponent`, `loadLodGroupComponent`, `loadSpriteRendererComponent`, `loadCameraComponent` (skybox) e `loadTextRendererComponent` repetem o mesmo bloco: criar `ShaderAsset` vertex+fragment, `setShaderCompiler(...)`, criar `Material`, `setShaderProgram(...)`, `setBaseColor`, `init()`.~~

~~Sugestão: extrair um helper `createMaterial(const MaterialData&)`.~~

---

### 🟡 Acoplamento e separação de responsabilidades (prioridade média)

**6. Vazamento de camadas no `RendererBackend`**
A interface de backend de GPU (camada baixa) inclui `components/camera.hpp`, `components/light.hpp`, `world_object.hpp`, `sprite.hpp` e tem `renderWorldObjects(const std::vector<WorldObject*>&, ...)`. O backend "conhece" o modelo de cena inteiro. O ideal é inverter: o backend recebe listas de dados de desenho (buffers, matrizes, material resolvido), sem saber o que é um `WorldObject`.

**7. `SceneLoader` é uma god-class**
Faz parse binário `.scnb` (header + validação), decodifica OBJ via tinyobj (inclusive cálculo de normais flat/smooth inline), cria materiais/texturas/skybox/fontes e faz o wiring dos componentes — tudo num arquivo. Dava para separar em `SceneBinaryReader`, `MeshLoader`/`ObjLoader` e `MaterialFactory`.

**8. Dois sistemas de cache de assets concorrentes**
Existe o `AssetManager` (cache por path), mas o caminho real de carregamento (`SceneLoader`) mantém caches próprios paralelos: `materialCache`, `meshCache`, `fontAtlasCache`. Resultado: o `AssetManager` está quase desconectado — só `ShaderAsset` herda de `Asset`; Mesh, Material, textura e FontAtlas não. Decidir: ou tudo passa pelo `AssetManager`, ou ele é removido. Bônus: o `AssetManager` faz busca linear (`find_if`) — se for mantido, usar `unordered_map`.

**9. `Application` acumula responsabilidades; `Context` subutilizado**
`Application` cuida de ciclo de vida + injeção manual de dependências + um `updateDebugOverlay` completo (FPS, toggle de frustum na tecla F, varrendo todos os objetos da cena duas vezes por frame procurando `TextRendererComponent`). Isso viola SRP. Enquanto isso, o `Context`, que deveria ser o ponto central de injeção, só guarda o input e quase ninguém usa. Sugestão: mover o overlay para um sistema de debug próprio e definir o papel do `Context` (service locator de verdade, ou remover).

---

### 🟡 Modelo de cena / componentes (prioridade média)

**10. Mesh e Sprite legados fora do sistema de componentes**
`WorldObject` guarda `shared_ptr<Mesh>` e `unique_ptr<Sprite>` diretamente, com vários `// TODO: remover suporte a legacy mesh/sprite`. Pior: `loadMeshRendererComponent` faz as duas coisas — `obj->setMesh(mesh)` **e** adiciona um `MeshRenderer` (que só guarda o Material). "O que é renderizável" fica dividido de forma inconsistente. Unificar: a mesh deveria viver dentro do `MeshRenderer`.

**11. `getComponent<T>()` via `dynamic_cast` linear por frame**
A busca percorre todos os componentes com `dynamic_cast`, e é chamada com frequência no render loop (Light, LodGroup, Camera, TextRendererComponent). Em cenas grandes isso pesa. Considerar índice por tipo (ex. `type_index → componente`).

**12. `Component` sem ciclo de vida**
A base só tem `owner`. Não há `init()`/`update()`/`onAttach()`, então toda lógica por frame vive fora dos componentes (no Renderer ou no `onUpdate` da Application). Um ciclo de vida mínimo deixaria o modelo mais coeso.

**13. `Scene` varre tudo a cada chamada**
`getLightObjects()` / `getRenderableObjects()` fazem varredura linear por frame. Dá para manter listas cacheadas atualizadas na inserção/remoção.

---

### 🟢 Organização e estilo (prioridade baixa)

**14. Namespaces inconsistentes**
`Math` está em `namespace Yume`, mas `Vector3`/`Vector4`/`Matrix3`/`Matrix4` estão no escopo global. Constantes em `namespace VECTOR3` e `COLOR` em CAPS (pouco idiomático). Padronizar tudo sob `Yume` (ex. `Yume::Math`, `Yume::Vector3`).

**15. API vetorial dividida**
`Vector3` define só `+`, `-`, `*escalar`. Faltam `/`, `==`, etc.

**16. `Math` como classe de métodos estáticos**
É efetivamente um namespace de funções livres embrulhado numa classe — poderia virar `namespace`.

~~**17. Pasta `core/src` plana**~~
~~\~60 arquivos soltos (assets, math, scene, utils todos juntos). Só `renderer/`, `components/`, `input/`, `window/` estão agrupados. Agrupar em `math/`, `assets/`, `scene/` ajudaria a navegação.~~

**18. Chave de cache de material frágil**
A chave é string concatenada `vs|fs|r g b a` com `std::to_string` sem separador entre os canais de cor — propenso a colisão (ex. `1.0`+`0.5` vs `1.00`+`.5`). Usar separador ou hash estruturado.

**19. `ShaderAsset` com acoplamento temporal**
Precisa chamar `setShaderCompiler()` antes de `load()`, senão quebra. Além disso guarda `void* shaderHandle` type-erased. Receber o compiler no construtor remove a ordem obrigatória implícita.

---
