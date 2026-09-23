# Día 1 — Fundamentos de C++ para Patrones de Diseño

Material de apoyo para la primera jornada del curso (≈6 horas efectivas).

## Estructura del día

| # | Bloque                       | Duración orientativa | Fichero                              |
|---|------------------------------|----------------------|--------------------------------------|
| 1 | Punteros inteligentes + RAII | ~2 h                 | `01_punteros_inteligentes.md`        |
| 2 | Templates (lo esencial)      | ~1,5 h               | `02_templates.md`                    |
| 3 | POO en C++ moderno           | ~2,5 h               | `03_poo.md`                          |

## Por qué este orden

1. **Punteros inteligentes primero** porque condicionan cómo se escribe el
   resto del curso. Sin `unique_ptr`/`shared_ptr` claros, los patrones acaban
   llenos de `new`/`delete` que distraen del concepto.

2. **Templates en medio** y solo lo justo: deducción, class templates,
   por qué van en headers, una pincelada de CRTP. Sin SFINAE ni concepts.

3. **POO al final** porque al cerrar el día con composición/delegación y el
   mantra *"composición sobre herencia"*, dejamos el terreno preparado para
   SOLID y patrones creacionales mañana.

## Contenido transversal

- **Constructores de copia y move** se introducen formalmente en el bloque 1
  (sección 7) y se recuerdan en el bloque 3 (sección 8) al hablar de la
  regla de los 5/0.

- Todos los ejemplos están pensados para conectar con el **ejemplo del Paint
  sin GUI** que será el hilo conductor del curso. La jerarquía `Shape`
  aparece desde el primer bloque.

## Versión de C++

Los ejemplos compilan con **C++14** en adelante (`g++ -std=c++14`).
Para `std::make_unique` se necesita C++14 mínimo; si el alumnado estuviera
en C++11, sustituir por `std::unique_ptr<T>(new T(...))`.

Algunos ejemplos puntuales con `[[nodiscard]]`, `if constexpr` o concepts
requerirían C++17/20, pero se han evitado en este día para no confundir.

## Cierre del día

El bloque 3 termina con un mini-refactor en vivo: pasamos de una `class Shape`
con `enum Tipo` y `switch` a una jerarquía polimórfica con `unique_ptr<Shape>`
y `vector` polimórfico. Esa clase es la semilla del Paint que iremos
desarrollando los próximos tres días.
