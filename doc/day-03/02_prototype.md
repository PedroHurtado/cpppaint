# Prototype

## 1. El problema

En el Paint, el usuario tiene un círculo dibujado y quiere **duplicarlo**.
Necesitamos copiarlo. Si `figura` es un `std::unique_ptr<Shape>`, ¿cómo
hago la copia?

```cpp
std::unique_ptr<Shape> figura = ...;       // apunta a un Circulo en realidad
std::unique_ptr<Shape> copia = ...;        // ¿qué pongo aquí?
```

No puedo escribir:

```cpp
auto copia = std::make_unique<Shape>(*figura);   // error: Shape es abstracta
```

No puedo escribir:

```cpp
auto copia = std::make_unique<Circulo>(*figura); // ¿y si no es un Circulo?
```

El polimorfismo dinámico me dice **qué hacer** pero no me dice **qué soy**.
Para copiar, necesitaría saber el tipo concreto en tiempo de ejecución.
Eso me llevaría a `dynamic_cast` por cada subclase y un `if/else` que
crece cada vez que añadimos una figura.

**Prototype** invierte el problema: en vez de que el cliente averigüe qué
tipo es, le decimos al propio objeto "clónate".

## 2. Intención (GoF)

> *Especificar los tipos de objetos a crear usando una instancia
> prototípica, y crear nuevos objetos copiando este prototipo.*

En cristiano: que cada objeto sepa fabricar un clon de sí mismo. La
fábrica está dentro del objeto.

## 3. Bad — el switch por tipo

```cpp
std::unique_ptr<Shape> clonar(const Shape& s) {
    if (auto c = dynamic_cast<const Circulo*>(&s))
        return std::make_unique<Circulo>(*c);
    if (auto c = dynamic_cast<const Cuadrado*>(&s))
        return std::make_unique<Cuadrado>(*c);
    if (auto c = dynamic_cast<const Triangulo*>(&s))
        return std::make_unique<Triangulo>(*c);
    throw std::runtime_error("tipo desconocido");
}
```

Síntomas:

- Cada figura nueva obliga a tocar esta función. **Viola OCP**.
- Si te olvidas de añadir un caso, te enteras en runtime con la excepción.
- `dynamic_cast` por cadena es lento y huele a código que evita pensar.

## 4. Good — clone() virtual

Añadimos un método virtual `clone()` a `Shape`. Cada subclase devuelve
una copia de sí misma:

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
    virtual std::unique_ptr<Shape> clone() const = 0;   // ← Prototype
};

class Circulo : public Shape {
    double radio;
public:
    explicit Circulo(double r) : radio(r) {}
    void dibujar() const override { std::cout << "Circulo " << radio << "\n"; }
    double area() const override   { return 3.14159 * radio * radio; }
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Circulo>(*this);        // copia con el constructor de copia
    }
};

class Cuadrado : public Shape {
    double lado;
public:
    explicit Cuadrado(double l) : lado(l) {}
    void dibujar() const override { std::cout << "Cuadrado " << lado << "\n"; }
    double area() const override   { return lado * lado; }
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Cuadrado>(*this);
    }
};
```

Uso:

```cpp
std::unique_ptr<Shape> figura = std::make_unique<Circulo>(3.0);
std::unique_ptr<Shape> copia  = figura->clone();   // sin saber qué es

copia->dibujar();    // "Circulo 3"
```

El cliente no sabe ni le importa qué tipo concreto hay dentro. La
responsabilidad de clonar la tiene **el propio objeto**.

## 5. UML rápido

```
        ┌─────────────────────────────┐
        │           Shape             │
        ├─────────────────────────────┤
        │ + dibujar()                 │
        │ + area()                    │
        │ + clone() : unique_ptr<Shape>│   ← el método clave
        └─────────────────────────────┘
                    ▲
        ┌───────────┼───────────────┐
        │           │               │
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   Circulo    │ │   Cuadrado   │ │   Triangulo  │
├──────────────┤ ├──────────────┤ ├──────────────┤
│ + clone()    │ │ + clone()    │ │ + clone()    │
│   → Circulo  │ │   → Cuadrado │ │   → Triangulo│
└──────────────┘ └──────────────┘ └──────────────┘
```

## 6. Variantes y trampas en C++

### Trampa 1: el slicing si devuelves por valor

¿Por qué no `virtual Shape clone() const`? Porque devolvería un `Shape`
por valor, y `Shape` es abstracta. Pero aunque no lo fuera, **slicing**:
la parte derivada se perdería. Por eso `clone()` siempre devuelve
**puntero o referencia inteligente**.

### Trampa 2: clone() y herencia profunda

Si `CirculoRelleno` hereda de `Circulo`, debe **sobrescribir** `clone()`,
o estaríamos clonando como si fuera un `Circulo` pelado:

```cpp
class CirculoRelleno : public Circulo {
    Color color;
public:
    CirculoRelleno(double r, Color c) : Circulo(r), color(c) {}
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<CirculoRelleno>(*this);   // ¡importante!
    }
};
```

Regla: **toda subclase concreta debe reimplementar `clone()`**. Si
alguien se olvida, el `clone()` heredado de la clase padre se ejecutará y
se perderá la parte hija.

### Variante moderna: covariant return types

C++ permite que una subclase devuelva un tipo derivado del que declara la
base. Útil para evitar casts en algunos contextos:

```cpp
class Shape {
public:
    virtual Shape* clone() const = 0;
};

class Circulo : public Shape {
public:
    Circulo* clone() const override {        // ← devuelve Circulo*, no Shape*
        return new Circulo(*this);
    }
};
```

Pero con `unique_ptr<Shape>` esto **no funciona** directamente (los
tipos de retorno no son covariantes con templates). En la práctica
moderna, `unique_ptr<Shape>` en toda la jerarquía y olvidarse de
covariancia.

### Variante: CRTP para evitar boilerplate

Si te cansa repetir `return std::make_unique<X>(*this);` en cada
subclase, puedes usar CRTP (que tocamos por encima el día 1):

```cpp
template<typename Derived>
class Clonable : public Shape {
public:
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }
};

class Circulo : public Clonable<Circulo> {
    double radio;
public:
    explicit Circulo(double r) : radio(r) {}
    void dibujar() const override { /* ... */ }
    double area() const override   { /* ... */ }
    // ya no hace falta declarar clone()
};
```

Más elegante, pero **más opaco**. Decide según el equipo.

## 7. Cuándo usar Prototype

- Cuando los objetos son **caros de construir** y es más barato copiar.
- Cuando el tipo concreto se conoce solo en runtime (lectura de fichero,
  selección del usuario, undo/redo).
- Cuando quieres evitar una **jerarquía paralela de fábricas**: en vez
  de una `CirculoFactory`, una `CuadradoFactory`, etc., basta con tener
  una instancia prototípica de cada figura y clonarla.

En el Paint nos sirve para:

- **Duplicar figuras** seleccionadas (Ctrl+D).
- **Implementar undo/redo** clonando el estado.
- Preparar el terreno para **Memento** (día 5), que también clona pero
  con otra intención.

## 8. SOLID en acción

| Principio | Cómo lo aplica Prototype                                          |
|-----------|-------------------------------------------------------------------|
| SRP       | Cada figura es responsable de copiarse a sí misma.                |
| OCP       | **Cumplido.** Añadir `Triangulo` no toca el código cliente.       |
| LSP       | Cumplido si toda subclase concreta sobrescribe `clone()`.         |
| ISP       | Neutro.                                                           |
| DIP       | Cumplido: el cliente depende de `Shape`, no de tipos concretos.   |

Compara con Singleton: Prototype cumple lo que Singleton rompe (DIP).
Cada patrón tiene su precio y su recompensa.

## 9. Mantra

> *"No preguntes qué tipo soy para copiarme. Pídeme que me copie yo."*
