# Bridge

## 1. El problema

El cliente nos pide tres cosas a la vez:

- Que el Paint pueda dibujar en consola (lo que ya hace).
- Que pueda exportar a SVG.
- Que en el futuro renderice con OpenGL.

Y a la vez, la jerarquía de figuras sigue creciendo: `Circulo`,
`Cuadrado`, `Triangulo`, `Poligono`, y mañana llegará `Estrella`.

Si lo resolvemos por herencia, la explosión es inmediata:

```
CirculoConsola, CirculoSVG, CirculoOpenGL,
CuadradoConsola, CuadradoSVG, CuadradoOpenGL,
TrianguloConsola, TrianguloSVG, TrianguloOpenGL,
PoligonoConsola, PoligonoSVG, PoligonoOpenGL,
...
```

**N figuras × M backends = NxM clases**. Cada figura nueva obliga a
escribir tres clases; cada backend nuevo, cuatro. Y peor: la lógica
geométrica de `Circulo` se repite en `CirculoConsola`, `CirculoSVG` y
`CirculoOpenGL`, porque las tres saben qué es un círculo pero pintan
distinto.

Esto huele a Abstract Factory, ¿no? Recuerda: ayer resolvimos algo
parecido con temas claro/oscuro. La diferencia es sutil pero importante,
y la veremos al final.

## 2. Intención (GoF)

> *Desacoplar una abstracción de su implementación para que ambas puedan
> variar independientemente.*

Dos jerarquías paralelas que se conectan por composición. La
abstracción (`Shape`) **tiene** una implementación (`Renderer`), no
**es** una implementación.

## 3. Bad: la herencia múltiple por backend

```cpp
class CirculoConsola : public Shape {
    double radio;
public:
    void dibujar() const override {
        std::cout << "Circulo " << radio << "\n";   // backend metido aquí
    }
    double area() const override { return 3.14159 * radio * radio; }
};

class CirculoSVG : public Shape {
    double radio;
public:
    void dibujar() const override {
        std::cout << "<circle r=\"" << radio << "\"/>\n";   // backend distinto
    }
    double area() const override { return 3.14159 * radio * radio; }
    // ↑ area() duplicada respecto a CirculoConsola
};
```

Añadir OpenGL: triplica las clases. Añadir `Estrella`: hay que escribir
`EstrellaConsola`, `EstrellaSVG`, `EstrellaOpenGL`. Y la geometría de
`Estrella` se repite en las tres.

## 4. Good: dos jerarquías y un puente

Separamos las dos cosas que varían: **qué es una figura** y **cómo se
pinta**.

```cpp
// Jerarquía de implementación: cómo se pinta
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void dibujarCirculo(double r) const = 0;
    virtual void dibujarRectangulo(double w, double h) const = 0;
    virtual void dibujarLinea(double x1, double y1, double x2, double y2) const = 0;
};

class RendererConsola : public Renderer {
public:
    void dibujarCirculo(double r) const override {
        std::cout << "Circulo de radio " << r << "\n";
    }
    void dibujarRectangulo(double w, double h) const override {
        std::cout << "Rectangulo " << w << "x" << h << "\n";
    }
    void dibujarLinea(double x1, double y1, double x2, double y2) const override {
        std::cout << "Linea (" << x1 << "," << y1 << ")-(" << x2 << "," << y2 << ")\n";
    }
};

class RendererSVG : public Renderer {
public:
    void dibujarCirculo(double r) const override {
        std::cout << "<circle r=\"" << r << "\"/>\n";
    }
    void dibujarRectangulo(double w, double h) const override {
        std::cout << "<rect width=\"" << w << "\" height=\"" << h << "\"/>\n";
    }
    void dibujarLinea(double x1, double y1, double x2, double y2) const override {
        std::cout << "<line x1=\"" << x1 << "\" y1=\"" << y1
                  << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\"/>\n";
    }
};
```

Y ahora las figuras delegan al renderer:

```cpp
class Shape {
protected:
    std::shared_ptr<Renderer> renderer;   // el puente
public:
    explicit Shape(std::shared_ptr<Renderer> r) : renderer(std::move(r)) {}
    virtual ~Shape() = default;
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
};

class Circulo : public Shape {
    double radio;
public:
    Circulo(double r, std::shared_ptr<Renderer> rend)
        : Shape(std::move(rend)), radio(r) {}
    void dibujar() const override { renderer->dibujarCirculo(radio); }
    double area() const override   { return 3.14159 * radio * radio; }
};

class Cuadrado : public Shape {
    double lado;
public:
    Cuadrado(double l, std::shared_ptr<Renderer> rend)
        : Shape(std::move(rend)), lado(l) {}
    void dibujar() const override { renderer->dibujarRectangulo(lado, lado); }
    double area() const override   { return lado * lado; }
};
```

Cliente:

```cpp
auto consola = std::make_shared<RendererConsola>();
auto svg     = std::make_shared<RendererSVG>();

Lienzo::instancia().anadir(std::make_unique<Circulo>(3.0, consola));
Lienzo::instancia().anadir(std::make_unique<Cuadrado>(2.0, svg));
```

Añadir OpenGL: **una sola clase** (`RendererOpenGL`). Añadir `Estrella`:
**una sola clase**, con su geometría, que sabe pintarse llamando al
renderer que le pongan.

N figuras + M backends = N + M clases, no NxM.

## 5. UML rápido

```
       Shape  ◇─────────►  Renderer
        ▲                    ▲
        │                    │
  ┌─────┼─────┐         ┌────┼────┐
  │     │     │         │    │    │
Circulo Cuadrado ...  Consola SVG OpenGL
```

Dos jerarquías paralelas. La línea ◇ es la composición (Shape *tiene*
un Renderer).

## 6. Bridge vs Abstract Factory

Resuelven problemas parecidos pero diferentes:

| Aspecto | Abstract Factory (ayer) | Bridge (hoy) |
|---------|------------------------|--------------|
| Qué varía | Familias de productos relacionados | Una abstracción y su implementación |
| Cuándo se decide | En construcción (cambias fábrica) | En construcción o en runtime |
| Granularidad | Toda la familia a la vez | Una figura, un backend |
| Mezcla | No (claro o oscuro, no ambos) | Sí (una figura consola, otra SVG) |
| Acoplamiento | Cliente acoplado a una fábrica | Cliente acoplado a Shape |

Regla práctica: si las variantes son **familias homogéneas que deben
combinarse entre sí** (todas claras, todas oscuras), Abstract Factory.
Si las variantes son **dos ejes que deben poder mezclarse libremente**
(esta figura en SVG, esa otra en consola), Bridge.

## 7. Variantes y trampas

- **shared_ptr vs raw pointer**: si varias figuras comparten el mismo
  renderer (lo normal), `shared_ptr`. Si cada figura tiene el suyo,
  `unique_ptr`. En el Paint compartimos.
- **Cambiar de renderer en runtime**: el Bridge lo permite. Un
  `setRenderer()` y la figura cambia de backend. Útil para
  previsualización.
- **Estado vs comportamiento**: el Renderer no debe guardar estado de
  la figura. Si lo hace, Bridge se degrada en Adapter mal pensado.
- **No confundir con Strategy**: Strategy intercambia un **algoritmo
  concreto** (cómo dibujar líneas: alámbrico vs relleno). Bridge
  intercambia una **implementación completa** (toda la API gráfica).
  Strategy es local; Bridge es estructural.

## 8. SOLID

| Principio | Cómo lo cumple Bridge |
|-----------|----------------------|
| SRP       | `Shape` sabe de geometría; `Renderer` sabe de pintar. |
| OCP       | Nuevas figuras y nuevos backends sin tocar lo existente. |
| LSP       | Cualquier `Renderer` sirve a cualquier `Shape`. |
| ISP       | `Renderer` expone solo primitivas de pintar. |
| DIP       | `Shape` depende de la abstracción `Renderer`. |

## 9. Mantra

> *"Si dos cosas varían, no las hereden a la vez. Hereden cada una por
> su lado y pónganlas en composición."*
