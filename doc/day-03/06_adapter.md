# Adapter

## 1. El problema

Estamos contentos con el Paint. Pero llega un requisito: el cliente quiere
usar una librería externa de gráficos que ya tiene, **GraphLib**, para
renderizar las figuras. La librería es buena, está optimizada, no
queremos reescribirla. Pero su interfaz **no encaja** con nuestro
`Shape`:

```cpp
// La librería externa, que no podemos modificar:
namespace GraphLib {
    class Renderable {
    public:
        virtual ~Renderable() = default;
        virtual void render(int x, int y) const = 0;
        virtual int width()  const = 0;
        virtual int height() const = 0;
    };

    class CircleRenderable : public Renderable {
    public:
        CircleRenderable(int radius);
        void render(int x, int y) const override;
        int width()  const override;
        int height() const override;
    };
}
```

Tres incompatibilidades evidentes:

- Métodos en inglés (`render`, `width`, `height`), nosotros en español
  (`dibujar`, `area`).
- `render` necesita `(x, y)`, nuestro `dibujar` no.
- `width()` y `height()` en vez de `area()`.

No podemos cambiar `GraphLib` (es de un tercero). No queremos cambiar
`Shape` (rompe todo el día 2). **Adapter** es la pieza que las conecta.

## 2. Intención (GoF)

> *Convertir la interfaz de una clase en otra interfaz que los clientes
> esperan. Permite que clases trabajen juntas cuando, por sus interfaces
> incompatibles, no podrían.*

En cristiano: un traductor entre dos mundos que no se entienden.

## 3. Bad — copiar y pegar la librería

La tentación: escribir nuestro propio `CirculoRender` que duplica la
lógica de `GraphLib::CircleRenderable`. Funciona, pero:

- Duplicación de código.
- Si la librería se actualiza con mejoras, no las heredamos.
- Cuando lleguen 30 figuras, son 30 duplicaciones.

## 4. Good — Adapter por composición (object adapter)

La variante más usada en C++ moderno: nuestro adaptador **contiene** un
objeto de la librería externa y traduce las llamadas:

```cpp
class CirculoAdaptado : public Shape {
    GraphLib::CircleRenderable circulo_externo;   // composición
    int x, y;                                       // datos extra que necesita render()
public:
    CirculoAdaptado(int radio, int x_, int y_)
        : circulo_externo(radio), x(x_), y(y_) {}

    void dibujar() const override {
        circulo_externo.render(x, y);              // traduce la llamada
    }

    double area() const override {
        // 'area' no existe en GraphLib; la calculamos aquí
        double w = circulo_externo.width();
        return 3.14159 * (w / 2.0) * (w / 2.0);
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<CirculoAdaptado>(*this);
    }
};
```

Uso:

```cpp
Lienzo::instancia().anadir(std::make_unique<CirculoAdaptado>(3, 10, 20));
Lienzo::instancia().dibujar();   // por dentro llama a GraphLib
```

Tres ganancias:

1. El cliente sigue trabajando con `Shape` (no sabe que hay una librería
   externa dentro).
2. Si `GraphLib` se actualiza, recompilamos y heredamos todas las
   mejoras gratis.
3. Si mañana cambiamos a `OtraGraphLib`, solo tocamos `CirculoAdaptado`.

## 5. UML rápido

```
┌───────────┐            ┌────────────────────┐
│   Shape   │            │ GraphLib::         │
├───────────┤            │   CircleRenderable │
│ +dibujar()│            ├────────────────────┤
│ +area()   │            │ +render(x,y)       │
└───────────┘            │ +width()           │
      ▲                  └────────────────────┘
      │                            ▲
      │                            │ tiene-un
┌──────────────────┐               │
│ CirculoAdaptado  │ ──── posee ───┘
├──────────────────┤
│ +dibujar()       │   ← internamente llama a render(x,y)
│ +area()          │   ← internamente calcula desde width()
└──────────────────┘
```

## 6. Variantes y trampas

### Class adapter (con herencia múltiple)

La variante "clásica" del GoF usa herencia múltiple: el adaptador
**hereda** de la interfaz nuestra y de la clase de la librería:

```cpp
class CirculoAdaptado : public Shape, private GraphLib::CircleRenderable {
public:
    CirculoAdaptado(int r) : CircleRenderable(r) {}
    void dibujar() const override { render(0, 0); }
    double area() const override   { /* ... */ }
};
```

En C++ es posible (herencia múltiple existe), pero **rara vez se usa**
porque:

- Acopla fuerte al adaptador con la implementación concreta.
- Si la librería externa tiene constructores complicados, sufres.
- La herencia privada para "implementar en términos de" es elegante
  pero confunde al equipo.

**Regla**: object adapter (composición) por defecto, class adapter solo
si tienes una razón muy concreta.

### Trampa: el adapter que hace demasiado

Si tu adaptador tiene 200 líneas de lógica, no es un adaptador, es un
wrapper con lógica de negocio. Sepáralo: que el adapter **solo traduzca**
y que la lógica viva en otro sitio.

## 7. Cuándo usar Adapter

- Librerías externas con interfaz incompatible.
- Código legacy que no podemos modificar.
- Cuando dos jerarquías hechas por equipos distintos tienen que hablar.

En el Paint nos sirve para integrar **cualquier renderer externo** (una
librería de SVG, OpenGL, un canvas web…) sin tocar `Shape`.

## 8. SOLID en acción

| Principio | Cómo lo aplica Adapter                                            |
|-----------|-------------------------------------------------------------------|
| SRP       | El adapter solo traduce.                                          |
| OCP       | **Cumplido.** Nueva librería externa = nuevo adapter, sin tocar `Shape`. |
| LSP       | Cumplido: el adapter cumple `Shape` como cualquier otra figura.   |
| ISP       | Neutro.                                                           |
| DIP       | **Cumplido.** El cliente sigue dependiendo de `Shape`.            |

## 9. Mantra

> *"Adapter no añade funcionalidad, traduce. Si tu adaptador piensa,
> es que ya no es un adaptador."*
