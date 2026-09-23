# Día 3 — Patrones Creacionales (+ 3 estructurales prometidos)

Material de apoyo para la tercera jornada del curso (≈5 horas efectivas,
8:45–15:00).

## Estructura del día

| # | Bloque                          | Duración orientativa | Fichero                  |
|---|---------------------------------|----------------------|--------------------------|
| 1 | Singleton                       | ~35 min              | `01_singleton.md`        |
| 2 | Prototype                       | ~35 min              | `02_prototype.md`        |
| 3 | Factory Method                  | ~35 min              | `03_factory.md`          |
| 4 | Builder                         | ~35 min              | `04_builder.md`          |
| 5 | Abstract Factory                | ~35 min              | `05_abstract_factory.md` |
| 7 | Adapter                         | ~35 min              | `07_adapter.md`          |
| 8 | Decorator                       | ~35 min              | `08_decorator.md`        |
| 9 | Composite                       | ~35 min              | `09_composite.md`        |
| 6 | Cierre                          | ~20 min              | `06_cierre_paint.md`     |

Total: ~5h efectivas. Cubre los **5 creacionales** del temario y **3
estructurales** que ya prometí cumplir en el día 2 (Adapter, Decorator,
Composite).

Mañana (día 4) cierra el curso con los 4 estructurales restantes
(Bridge, Facade, Proxy, Flyweight), los 9 patrones de comportamiento,
y un recorrido por los patrones enterprise como mapa final (sin código,
porque no aplican a C++).

## Decisión de ritmo

Los markdowns están escritos con material extendido (variantes, trampas,
SOLID, mantras). En clase **se da a ritmo más rápido** del que el
markdown sugiere: lo extenso es para el material que se llevan, no
para el discurso oral. Pensar en ~30-35 min reales por patrón en clase.

## Cómo enfocar el bloque

Hoy cambiamos de modo. Ayer fueron **principios** (formas de pensar); hoy
son **patrones** (soluciones concretas con nombre propio). Cada patrón
sigue el mismo esquema:

1. **El problema** que resuelve, en una frase.
2. **Intención** (la definición del GoF, traducida).
3. **Bad**: cómo lo resolveríamos sin el patrón. Que se vea el dolor.
4. **Good**: aplicación del patrón sobre el Paint.
5. **UML rápido** (en ASCII, para fijar la estructura).
6. **Variantes** y trampas en C++ moderno.
7. **Qué SOLID estamos aplicando** (porque cada patrón es SOLID en acción).
8. **Mantra** del patrón.

## Decisión de diseño del día

**Todos los ejemplos van sobre el Paint del día 2.** No usamos ejemplos
variados como ayer. La razón: ayer cada principio se entendía mejor con
un dominio fresco, pero hoy lo que queremos es **ver cómo el mismo
diseño va creciendo** patrón a patrón sin reescribirse. Es la prueba de
que ayer aplicamos bien SOLID.

Al final del día, el alumno habrá visto el mismo `Shape` / `Lienzo`
crecer en cinco direcciones distintas, cada una con un patrón.

## Punto de partida

Recuperamos el Paint donde lo dejamos ayer:

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
};

class Circulo : public Shape { /* ... */ };
class Cuadrado : public Shape { /* ... */ };
class Triangulo : public Shape { /* ... */ };

class Lienzo {
    std::vector<std::unique_ptr<Shape>> figuras;
public:
    void anadir(std::unique_ptr<Shape> s) { figuras.push_back(std::move(s)); }
    void dibujar() const { for (const auto& f : figuras) f->dibujar(); }
};
```

Este es el **estado base**. Cada patrón del día va a tocar una parte
distinta de este diseño sin destrozar el resto.

## Conexión con días anteriores

- **Día 1** nos dio `unique_ptr<Shape>`, destructores virtuales y la
  noción de composición. Sin eso, los patrones de hoy serían un festival
  de `new`/`delete`.
- **Día 2** nos dio SOLID. Hoy comprobamos en vivo que los patrones
  creacionales son **SOLID congelado en una solución reutilizable**.

## Mantra del día

> *"Crear objetos no es invocar `new`. Crear objetos es una decisión de
> diseño."*
