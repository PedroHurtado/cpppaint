# Cierre — El Paint con los cinco creacionales

## Punto de partida (estado al cerrar el día 2)

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
};

class Circulo    : public Shape { /* ... */ };
class Cuadrado   : public Shape { /* ... */ };
class Triangulo  : public Shape { /* ... */ };

class Lienzo {
    std::vector<std::unique_ptr<Shape>> figuras;
public:
    void anadir(std::unique_ptr<Shape> s) { figuras.push_back(std::move(s)); }
    void dibujar() const { for (const auto& f : figuras) f->dibujar(); }
};
```

Limpio, SOLID, y todavía sin un solo patrón aplicado. Vamos a meter los
cinco creacionales **uno a uno**, sin romper el diseño existente.

## Iteración 1 — Singleton: el Lienzo único

El primer cambio: `Lienzo` es único en la aplicación. Constructor privado,
método de acceso, copia y movimiento borrados.

```cpp
class Lienzo {
public:
    static Lienzo& instancia() {
        static Lienzo unica;
        return unica;
    }

    void anadir(std::unique_ptr<Shape> s) {
        figuras.push_back(std::move(s));
        // TODO día 5 (Observer): notificar a los suscriptores que ha cambiado el lienzo.
    }
    void dibujar() const { for (const auto& f : figuras) f->dibujar(); }
    void limpiar() {
        figuras.clear();
        // TODO día 5 (Observer): notificar.
    }
    size_t numFiguras() const { return figuras.size(); }

    Lienzo(const Lienzo&)            = delete;
    Lienzo& operator=(const Lienzo&) = delete;
    Lienzo(Lienzo&&)                 = delete;
    Lienzo& operator=(Lienzo&&)      = delete;

private:
    Lienzo() = default;
    ~Lienzo() = default;
    std::vector<std::unique_ptr<Shape>> figuras;
};
```

Dos cosas importantes que ayer prometimos y que **no estamos resolviendo
hoy**, pero que dejamos el hueco preparado:

- La barra de herramientas se enterará de los cambios vía **Observer**
  (día 5). Por eso `anadir` y `limpiar` llevan un `TODO`: cuando metamos
  Observer, ahí se notifica. Hoy nadie está suscrito todavía.
- Las acciones del usuario (añadir, mover, eliminar) se canalizarán por
  **Command** (día 5), no llamando a `anadir` desde la GUI directamente.
  Ese Command es el que invocará `anadir`; así implementamos
  deshacer/rehacer.

Es decir, **el código de hoy no es el final**. El Singleton aguanta lo
que viene porque sus puntos de entrada y sus puntos de cambio ya están
identificados.

Recordamos el aviso del bloque: en producción esto haría los tests más
difíciles. En el Paint nos lo permitimos.

## Iteración 2 — Prototype: duplicar figuras

Añadimos `clone()` a `Shape`. Cada subclase devuelve copia de sí misma.

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
    virtual std::unique_ptr<Shape> clone() const = 0;   // ← nuevo
};

class Circulo : public Shape {
    double radio;
public:
    explicit Circulo(double r) : radio(r) {}
    void dibujar() const override { std::cout << "Circulo " << radio << "\n"; }
    double area() const override   { return 3.14159 * radio * radio; }
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Circulo>(*this);
    }
};

// Cuadrado y Triangulo, igual.
```

Y un método de conveniencia en el `Lienzo`:

```cpp
void duplicarUltima() {
    if (figuras.empty()) return;
    figuras.push_back(figuras.back()->clone());
}
```

El cliente puede duplicar la última figura **sin saber qué tipo era**.

## Iteración 3 — Builder: el Polígono configurable

Añadimos una figura compleja, `Poligono`, que el Builder construye:

```cpp
class Poligono : public Shape {
    std::vector<Punto> vertices;
    Color borde = Color::Negro;
    double opacidad = 1.0;

    Poligono() = default;

public:
    void dibujar() const override { std::cout << "Poligono de " << vertices.size() << " vértices\n"; }
    double area() const override   { /* shoelace */ return 0.0; }
    std::unique_ptr<Shape> clone() const override {
        return std::unique_ptr<Poligono>(new Poligono(*this));
    }

    class Builder;
    friend class Builder;
};

class Poligono::Builder {
    Poligono p;
public:
    Builder& con_vertices(std::vector<Punto> v) { p.vertices = std::move(v); return *this; }
    Builder& con_borde(Color c)                 { p.borde = c;               return *this; }
    Builder& con_opacidad(double o)             { p.opacidad = o;            return *this; }

    std::unique_ptr<Poligono> build() {
        if (p.vertices.size() < 3)
            throw std::invalid_argument("polígono necesita ≥3 vértices");
        if (p.opacidad < 0.0 || p.opacidad > 1.0)
            throw std::invalid_argument("opacidad fuera de [0,1]");
        return std::unique_ptr<Poligono>(new Poligono(std::move(p)));
    }
};
```

Cliente:

```cpp
auto tri = Poligono::Builder{}
    .con_vertices({{0,0}, {3,0}, {3,4}})
    .con_borde(Color::Rojo)
    .build();

Lienzo::instancia().anadir(std::move(tri));
```

## Iteración 4 — Factory: crear desde texto

La GUI parsea lo que teclea el usuario. La fábrica con registro hace el
trabajo:

```cpp
class FabricaFiguras {
public:
    using Constructor = std::function<std::unique_ptr<Shape>(std::istringstream&)>;

    static void registrar(const std::string& nombre, Constructor c) {
        registro()[nombre] = std::move(c);
    }

    static std::unique_ptr<Shape> crear(const std::string& linea) {
        std::istringstream in(linea);
        std::string tipo;
        in >> tipo;
        auto it = registro().find(tipo);
        if (it == registro().end())
            throw std::runtime_error("figura desconocida: " + tipo);
        return it->second(in);
    }

private:
    static std::map<std::string, Constructor>& registro() {
        static std::map<std::string, Constructor> m;
        return m;
    }
};

// Registro inicial (en su propio .cpp, o en main):
void registrarFigurasBasicas() {
    FabricaFiguras::registrar("circulo", [](std::istringstream& in) {
        double r; in >> r;
        return std::make_unique<Circulo>(r);
    });
    FabricaFiguras::registrar("cuadrado", [](std::istringstream& in) {
        double l; in >> l;
        return std::make_unique<Cuadrado>(l);
    });
    FabricaFiguras::registrar("triangulo", [](std::istringstream& in) {
        double a, b, c; in >> a >> b >> c;
        return std::make_unique<Triangulo>(a, b, c);
    });
}
```

Uso:

```cpp
Lienzo::instancia().anadir(FabricaFiguras::crear("circulo 3.0"));
```

Añadir un tipo nuevo es **una llamada a `registrar`** en otro fichero.
Cero ediciones en `FabricaFiguras`.

## Iteración 5 — Abstract Factory: temas claro y oscuro

Reorganizamos las figuras en dos familias paralelas (`CirculoClaro`,
`CirculoOscuro`, etc.) y una fábrica abstracta:

```cpp
class FabricaTema {
public:
    virtual ~FabricaTema() = default;
    virtual std::unique_ptr<Shape> circulo(double r) const = 0;
    virtual std::unique_ptr<Shape> cuadrado(double l) const = 0;
    virtual std::unique_ptr<Shape> triangulo(double a, double b, double c) const = 0;
};

class FabricaTemaClaro : public FabricaTema {
public:
    std::unique_ptr<Shape> circulo(double r) const override {
        return std::make_unique<CirculoClaro>(r);
    }
    // ... resto
};

class FabricaTemaOscuro : public FabricaTema { /* ... */ };
```

Y el `Editor` que las consume:

```cpp
class Editor {
    std::unique_ptr<FabricaTema> tema;
public:
    explicit Editor(std::unique_ptr<FabricaTema> t) : tema(std::move(t)) {}

    void escenaDePrueba() {
        auto& lienzo = Lienzo::instancia();
        lienzo.anadir(tema->circulo(3.0));
        lienzo.anadir(tema->cuadrado(2.0));
        lienzo.anadir(tema->triangulo(3, 4, 5));
    }
};
```

## La foto final

```
Lienzo (Singleton)
  └── vector<unique_ptr<Shape>>

Shape
  ├── Circulo  (Prototype: clone())
  ├── Cuadrado (Prototype: clone())
  ├── Triangulo(Prototype: clone())
  ├── Poligono (Builder)
  ├── CirculoClaro / CirculoOscuro       ┐
  ├── CuadradoClaro / CuadradoOscuro     ├── familias de Abstract Factory
  └── TrianguloClaro / TrianguloOscuro   ┘

FabricaFiguras (Factory con registro)
  ├── crear("circulo 3.0") → Circulo
  └── ...

FabricaTema (Abstract Factory)
  ├── FabricaTemaClaro
  └── FabricaTemaOscuro

Editor
  └── FabricaTema   (DIP: depende de la abstracta, no de las concretas)
```

## Diagnóstico SOLID, otra vez

| Principio | Cómo lo cumple el Paint creacional                                 |
|-----------|--------------------------------------------------------------------|
| SRP       | Cada fábrica una responsabilidad; el Builder construye, el objeto representa. |
| OCP       | Factory con registro y Abstract Factory permiten ampliar sin tocar lo existente. |
| LSP       | Toda figura cumple `Shape`, toda fábrica cumple su interfaz.       |
| ISP       | Las fábricas exponen solo los métodos de creación que necesitan.   |
| DIP       | Editor depende de `FabricaTema`; clientes dependen de `Shape`.     |

**Excepción honesta**: el `Lienzo` Singleton viola DIP en todo el código
que lo invoca directamente. Lo aceptamos por dominio, pero conviene
recordarlo.

## Qué hemos hecho hoy

Cinco patrones encajados en el mismo diseño **sin reescribir nada del
día 2**. Eso es la prueba experimental de que SOLID se aplicó bien:
los patrones encajan como guantes porque el diseño ya respiraba en la
dirección correcta.

## Conexión con el día 4 — último día del curso

Mañana cerramos el curso en una sola jornada de 8:45 a 15:00 (~5h
efectivas). Queda mucho material y vamos a ir con ritmo, pero **sin
saltarnos las promesas**.

### Qué hemos hecho hoy

Cinco creacionales **a fondo** (Singleton, Prototype, Factory, Builder,
Abstract Factory) y tres estructurales **a fondo** (Adapter, Decorator,
Composite). Los tres estructurales eran promesas concretas del día 2,
así que las cumplimos hoy mismo.

### Qué queda mañana

**Estructurales restantes (≈1h 15m):**

- **Bridge** — separar `Shape` del backend de renderizado (consola,
  SVG, OpenGL). Resuelve un problema parecido al de Abstract Factory
  pero por otra vía: dos ejes que varían independientemente.
- **Facade** — esconder toda la maquinaria del Paint detrás de una
  interfaz simple para la GUI.
- **Proxy** — lazy-loading de figuras pesadas y control de acceso.
- **Flyweight** — compartir representación entre figuras idénticas para
  ahorrar memoria.

**Comportamiento — los 5 prometidos del día 2, a fondo (≈3h):**

- **Strategy** ⭐ — algoritmos de dibujado intercambiables (alámbrico,
  relleno, texturizado) sin tocar la figura.
- **Command** ⭐ — cada acción del usuario es un objeto invocable y
  reversible. Aquí entra **deshacer/rehacer**. Los `TODO` que dejamos
  hoy en `Lienzo::anadir` y `Lienzo::limpiar` se rellenan porque las
  llamadas vendrán desde Commands.
- **Observer** ⭐ — la barra de herramientas, el panel de capas y
  cualquier vista se suscriben al `Lienzo` y se enteran de los cambios.
  Los `TODO` que dejamos hoy se rellenan aquí.
- **Memento** ⭐ — snapshots del lienzo para undo a nivel de estado
  completo (alternativa o complemento a Command).
- **Visitor** ⭐ — añadir operaciones nuevas (exportar, contar,
  transformar) sin tocar las figuras. Resuelve el OCP "al revés".

**Comportamiento — los 4 restantes, como mapa breve (≈30 min):**

- **Chain of Responsibility** — cadena de manejadores, útil para
  eventos de ratón con filtros sucesivos.
- **Mediator** — coordinador que evita acoplamientos todos-contra-todos.
- **State** — máquina de estados del editor (selección, dibujo,
  arrastre).
- **Template Method** — esqueleto fijo, pasos variables. Conecta con
  CRTP que vimos el día 1.

Estos cuatro **no** tenían promesa concreta del día 2, así que se ven
con definición, intención y ejemplo corto. El alumno se lleva el
markdown como referencia.

**Cierre del curso (≈15 min):**

- **Trabajando con patrones** (extensibilidad, encapsulación,
  combinación) — repasamos el Paint completo y vemos dónde se combinan
  patrones: Singleton + Observer, Composite + Visitor, Command + Memento…
- **Patrones Enterprise** (MVC, Repository, Entity, Aggregate, DTO,
  Mapper, Business Delegate, Reactive Services, Observables) — son
  patrones de Java EE / .NET, **no aplican a C++**. Los citamos como
  mapa conceptual para que el alumno los reconozca, sin código.

### Las 13 promesas del día 2, cuenta final

| Prometido día 2  | Día    | Estado       |
|------------------|--------|--------------|
| Singleton        | hoy    | ✅ cumplido  |
| Prototype        | hoy    | ✅ cumplido  |
| Factory          | hoy    | ✅ cumplido  |
| Abstract Factory | hoy    | ✅ cumplido  |
| Builder          | hoy    | ✅ cumplido  |
| Adapter          | hoy    | ✅ cumplido  |
| Decorator        | hoy    | ✅ cumplido  |
| Composite        | hoy    | ✅ cumplido  |
| Strategy         | mañana | pendiente    |
| Command          | mañana | pendiente    |
| Observer         | mañana | pendiente    |
| Memento          | mañana | pendiente    |
| Visitor          | mañana | pendiente    |

Ocho cumplidas, cinco pendientes. Mañana cerramos las cinco **y** los
otros ocho GoF que quedan **y** el mapa enterprise.

## Mantra de cierre

> *"Hoy hemos creado y empezado a componer. Mañana terminamos de
> componer y coordinamos. Todo sobre el mismo `Shape`. Si el día 2
> hicimos bien los deberes, mañana no tendremos que reescribir nada."*
