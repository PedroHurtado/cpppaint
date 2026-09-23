# Día 2 — Principios SOLID en C++

Material de apoyo para la segunda jornada del curso (≈6 horas efectivas).

## Estructura del día

| # | Bloque                                  | Duración orientativa | Fichero                          |
|---|-----------------------------------------|----------------------|----------------------------------|
| 1 | SRP — Single Responsibility Principle   | ~45 min              | `01_SRP.md`                      |
| 2 | OCP — Open/Closed Principle             | ~45 min              | `02_OCP.md`                      |
| 3 | LSP — La regla de Liskov                | ~50 min              | `03_LSP.md`                      |
| 4 | ISP — Interface Segregation Principle   | ~40 min              | `04_ISP.md`                      |
| 5 | DIP — Dependency Inversion Principle    | ~50 min              | `05_DIP.md`                      |
| 6 | Combinando principios                   | ~30 min              | `06_combinando.md`               |
| 7 | Cierre: refactor en vivo sobre el Paint | ~40 min              | `07_cierre_paint.md`             |

Total: ~5 h efectivas. El resto del día queda para dudas, pausas y arrancar
patrones creacionales (Singleton) si va sobrado.

## Cómo enfocar el bloque

Cada principio sigue el mismo esquema:

1. **El problema** que resuelve, en una frase.
2. **Definición formal** (lo que aparece en cualquier libro).
3. **Bad**: código que viola el principio. Que se vea por qué duele.
4. **Good**: refactor aplicando el principio.
5. **Qué hemos ganado** y qué hemos pagado.
6. **Conexión con patrones** que vendrán después.
7. **Mantra** del principio.

Los ejemplos **Bad/Good** son variados (cuentas bancarias, reportes,
notificaciones, pájaros, switches) para que el principio se vea como una
herramienta general, no algo "que solo aplica al Paint".

## El Paint sigue ahí

El Paint del día 1 reaparece en el **ejercicio de cierre** (`07_cierre_paint.md`)
ya como refactor guiado: aplicamos los 5 principios sobre el Paint y dejamos
una versión de la jerarquía `Shape` que aguantará todos los patrones de los
próximos dos días.

## Conexión con el día 1

- **Composición sobre herencia** (mantra del día 1) es la columna vertebral
  del OCP y del DIP.
- **Delegación** (día 1) es el mecanismo que aplicamos en SRP, OCP, DIP.
- **Destructor virtual + `override`** (día 1) son obligatorios en todos los
  ejemplos con jerarquías.
- **`unique_ptr<Base>`** (día 1) es cómo inyectamos dependencias en DIP.

## Conexión con el día 3 y 4

Al cerrar SOLID, casi cualquier patrón GoF se reduce a *"aplicar uno o dos
principios SOLID con un nombre propio"*. Esto se hace explícito al final
de `06_combinando.md`.

## Versión de C++

Igual que el día 1: **C++14** mínimo. Sin `concepts`, sin `if constexpr`.
