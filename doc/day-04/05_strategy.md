# Strategy ⭐ (prometido el día 2)

## 1. El problema

El usuario del Paint quiere elegir cómo se dibujan las figuras:

- **Alámbrico** (solo el contorno).
- **Relleno** (interior macizo).
- **Texturizado** (interior con un patrón).

Es la misma figura: un círculo es un círculo. Pero el algoritmo que lo
pinta cambia. Y queremos que **un mismo círculo** pueda dibujarse hoy
alámbrico y mañana relleno, sin crear tres clases distintas.

Si lo resolvemos por herencia, el resultado es feo:

```
CirculoAlambrico, CirculoRelleno, CirculoTexturizado,
CuadradoAlambrico, CuadradoRelleno, CuadradoTexturizado,
...
```

Es Bridge mal aplicado: la "implementación" aquí no es un backend
completo (consola/SVG), es **un único algoritmo** de pintado. Para algo
tan localizado, Bridge es excesivo.

Si lo resolvemos con un `if/switch` dentro de la figura, peor:

```cpp
void Circulo::dibujar() const {
    switch (modo) {
        case Alambrico: /* ... */ break;
        case Relleno:    /* ... */ break;
        case Texturizado:/* ... */ break;
    }
}
```

OCP roto: añadir un modo nuevo (sombreado, punteado, degradado) obliga
a tocar el `switch` de **cada figura**. Y se repite en todas.

## 2. Intención (GoF)

> *Definir una familia de algoritmos, encapsular cada uno, y hacerlos
> intercambiables. Strategy permite que el algoritmo varíe
> independientemente de los clientes que lo usan.*

Un algoritmo, una clase. La figura compone una estrategia y delega.

## 3. Bad: el switch dentro de la figura

```cpp
enum class ModoPintado { Alambrico, Relleno, Texturizado };

class Circulo : public Shape {
    double radio;
    ModoPintado modo;
public:
    Circulo(double r, ModoPintado m) : radio(r), modo(m) {}

    void dibujar() const override {
        switch (modo) {
            case ModoPintado::Alambrico:
                std::cout << "Circulo alambrico r=" << radio << "\n";
                break;
            case ModoPintado::Relleno:
                std::cout << "Circulo relleno r=" << radio << "\n";
                break;
            case ModoPintado::Texturizado:
                std::cout << "Circulo texturizado r=" << radio << "\n";
                break;
        }
    }
    double area() const override { return 3.14159 * radio * radio; }
};
```

Añadir un modo: tocar `Circulo`, `Cuadrado`, `Triangulo`, `Poligono`.
Cuatro ediciones por cada modo nuevo.

## 4. Good: Strategy

Una jerarquía paralela de **estrategias de pintado**:

```cpp
class EstrategiaPintado {
public:
    virtual ~EstrategiaPintado() = default;
    virtual void pintar(const Shape& s) const = 0;
};

class PintadoAlambrico : public EstrategiaPintado {
public:
    void pintar(const Shape& s) const override {
        std::cout << "[alambrico] ";
        s.dibujarContorno();
    }
};

class PintadoRelleno : public EstrategiaPintado {
public:
    void pintar(const Shape& s) const override {
        std::cout << "[relleno] ";
        s.dibujarContorno();
        std::cout << " (rellenado)\n";
    }
};

class PintadoTexturizado : public EstrategiaPintado {
public:
    void pintar(const Shape& s) const override {
        std::cout << "[texturizado] ";
        s.dibujarContorno();
        std::cout << " (con textura)\n";
    }
};
```

`Shape` expone `dibujarContorno()` (lo que toda figura sabe hacer) y
compone una estrategia:

```cpp
class Shape {
protected:
    std::shared_ptr<EstrategiaPintado> estrategia;
public:
    explicit Shape(std::shared_ptr<EstrategiaPintado> e)
        : estrategia(std::move(e)) {}
    virtual ~Shape() = default;

    void dibujar() const { estrategia->pintar(*this); }
    void setEstrategia(std::shared_ptr<EstrategiaPintado> e) {
        estrategia = std::move(e);
    }

    virtual void dibujarContorno() const = 0;
    virtual double area() const = 0;
};

class Circulo : public Shape {
    double radio;
public:
    Circulo(double r, std::shared_ptr<EstrategiaPintado> e)
        : Shape(std::move(e)), radio(r) {}
    void dibujarContorno() const override {
        std::cout << "Circulo r=" << radio;
    }
    double area() const override { return 3.14159 * radio * radio; }
};
```

Cliente:

```cpp
auto alambrico = std::make_shared<PintadoAlambrico>();
auto relleno   = std::make_shared<PintadoRelleno>();

auto c = std::make_unique<Circulo>(3.0, alambrico);
c->dibujar();                       // alambrico
c->setEstrategia(relleno);
c->dibujar();                       // ahora relleno, sin recrear el círculo
```

Añadir `PintadoPunteado`: una clase nueva, cero ediciones.

## 5. UML rápido

```
       Shape  ◇──────► EstrategiaPintado
        ▲                    ▲
        │                    │
  Circulo, Cuadrado    Alambrico, Relleno, Texturizado
```

## 6. Strategy vs Bridge

Mismo esquema estructural, distinta intención y granularidad:

| | Strategy | Bridge |
|--|----------|--------|
| Qué encapsula | Un **algoritmo** | Una **implementación completa** |
| Cuándo se cambia | Frecuente, en runtime | Raro, casi siempre en construcción |
| Tamaño de la abstracción | Pequeña, 1-2 métodos | Grande, toda una API |
| Pareja | Una figura, una estrategia, intercambiable | Una abstracción, una implementación, estable |

Ejemplo en el Paint: el **modo de pintado** (alámbrico/relleno) es
Strategy. El **backend gráfico** (consola/SVG/OpenGL) es Bridge.

## 7. Strategy con `std::function`

En C++ moderno, si la estrategia es un solo método sin estado, no hace
falta una jerarquía:

```cpp
class Shape {
protected:
    std::function<void(const Shape&)> pintar;
public:
    explicit Shape(std::function<void(const Shape&)> p) : pintar(std::move(p)) {}
    void dibujar() const { pintar(*this); }
    virtual void dibujarContorno() const = 0;
    virtual double area() const = 0;
};

// Estrategias como lambdas:
auto alambrico = [](const Shape& s) {
    std::cout << "[alambrico] ";
    s.dibujarContorno();
};

auto c = std::make_unique<Circulo>(3.0, alambrico);
```

Más ligero, sin jerarquía. La pega: si la estrategia tiene **estado**
(parámetros, configuración), una clase sigue siendo más limpia.

## 8. Strategy y los días anteriores

- **Day 1 — Composición**: Strategy es composición pura. La figura
  *tiene* una estrategia, no *es* una estrategia.
- **Day 2 — OCP**: cumplir OCP es **tener un Strategy**. Si tu OCP usa
  `switch` con enum, has hecho Strategy de pobres.
- **Day 3 — Abstract Factory**: si las estrategias vienen en familias
  coordinadas (todas claras / todas oscuras), Abstract Factory crea
  conjuntos de estrategias.

## 9. SOLID

| Principio | Cómo lo cumple Strategy |
|-----------|------------------------|
| SRP       | Cada estrategia un algoritmo. La figura no sabe pintar. |
| OCP       | Nueva estrategia = nueva clase, cero ediciones. |
| LSP       | Todas las estrategias cumplen la misma interfaz. |
| ISP       | La interfaz de estrategia tiene un método (o pocos). |
| DIP       | La figura depende de `EstrategiaPintado`, no de la concreta. |

## 10. Mantra

> *"Si tu OCP es un `switch`, te falta un Strategy."*
