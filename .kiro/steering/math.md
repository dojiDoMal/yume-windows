---
inclusion: fileMatch
fileMatchPattern: ['core/src/math/**']
---

# Guia do Módulo: Math

Álgebra linear própria da engine. É a **folha** da árvore de dependências: não
importa nada de outro módulo (só STL/cmath). Mantenha assim.

## Tipos (namespace GLOBAL)

- `vector3.hpp` — `Vector3` (`union {struct{float x,y,z;}; float v[3];}`) +
  operadores. Namespace `::VECTOR3` com constantes `UP`(+Y), `DOWN`, `RIGHT`(+X),
  `LEFT`, `BACK`(+Z), `FRONT`(-Z), `ONES`, `ZEROS`. Sistema **right-handed**.
- `vector4.hpp` — `Vector4` (x,y,z,w / v[4]) + operadores (inclui `/`, `/=`, `[]`).
  Namespace `::VECTOR4` (`ONES`/`ZEROS`). Usado em coords homogêneas, cores e
  colunas de `Matrix4`.
- `matrix3.hpp` — `Matrix3` (`Vector3 cols[3]`, column-major, compat `glm::mat3`).
- `matrix4.hpp` — `Matrix4` (`Vector4 cols[4]`, **column-major**, compat
  `glm::mat4`). `Matrix4(1.0f)` = identidade; `data()` devolve os 16 floats
  contíguos (passável direto para a GPU, estilo `glm::value_ptr`); conversões
  `Matrix4(Matrix3)` e `toMatrix3()`. Namespace `::MATRIX4` com `IDENTITY`/`ZEROS`.

## Funções: classe `Yume::Math` (`math.{hpp,cpp}`)

É o **único** arquivo do módulo com namespace (`Yume`). Classe estática com:
- Vetor: `dot`, `cross`, `length`, `lengthSquared`, `normalize`, `min`, `max`,
  `radians`. Constante `PI`.
- Matrizes de transformação: `translate`, `rotate(m, angleRadians, axis)`,
  `scale`, `lookAt(eye, center, up)`, `perspective`, `ortho` (com e sem near/far).
- Variantes **RH_ZO** (right-handed, profundidade em `[0,1]`): `perspectiveRH_ZO`,
  `orthoRH_ZO` — para Vulkan/DirectX.

## Convenções que importam

- **Right-handed**, rotação em **Euler**, ângulos em **radianos** nas funções de
  matriz (a API de câmera usa graus e converte com `Math::radians`).
- Os **tipos** (`Vector*`/`Matrix*`) são globais; só a fachada `Math` está em
  `Yume`. Por isso o código escreve `Yume::Math::length(v)` mas `Vector3 v;` sem
  namespace.
- Compatibilidade com glm é **deliberada** (column-major, `mat4(1.0f)` identidade,
  `data()` como `value_ptr`, variantes RH_ZO, conversões mat3↔mat4) — preserve-a.
  Há `static_assert` garantindo o packing de `Matrix4`; não quebre o layout.
- Include guards `#ifndef` (`VECTOR3_HPP`, `MATRIX4_HPP`, `YUME_MATH_HPP`).
  Doxygen PT-BR.

Ao adicionar uma função nova, prefira colocá-la em `Yume::Math` em vez de
métodos nos tipos, seguindo o padrão atual.
