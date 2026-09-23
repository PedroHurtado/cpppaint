# Factory Method

## 1. El problema

En el Paint, el usuario teclea en una caja de texto el nombre de la figura
y sus parámetros:

```
circulo 3.0
cuadrado 2.0
triangulo 4.0 5.0 6.0
```

El parser tiene que convertir esa cadena en un `Shape*`. La primera
versión, sin pensar mucho:

```cpp
std::unique_ptr<Shape> crearDesdeTexto(const std::string& linea) {
    if (linea.starts_with("circulo"))   return std::make_unique<Circulo>(...);
    if (linea.starts_with("cuadrado"))  return std::make_unique<Cuadrado>(...);
    if (linea.starts_with("triangulo")) return std::make_unique<Triangulo>(...);
    throw std::runtime_error("figura desconocida");
}
```

Funciona. Pero cada nueva figura nos obliga a editar esta función. Y la
función parsea **y** decide qué tipo crear **y** llama al constructor.
Tres responsabilidades en una. **SRP llorando, OCP llorando.**

**Factory Method** separa "qué crear" de "quién lo crea".

## 2. Intención (GoF)

> *Definir una interfaz para crear un objeto, pero dejar que las subclases
> decidan qué clase instanciar. Factory Method permite que una clase delegue
> la instanciación en sus subclases.*

Esa es la definición clásica, **y es confusa** porque mezcla dos cosas
que en la práctica se llaman igual:

1. **Factory Method estricto (GoF)**: un método virtual en una clase
   abstracta que las subclases sobrescriben para decidir qué crear.
2. **Static / simple factory**: una función o método estático que
   centraliza la creación. No es GoF en sentido puro, pero todo el mundo
   le llama "factory" igual.

Vamos a ver las dos.

## 3. Bad — constructor que decide

El antipatrón clásico: un constructor o función que decide internamente
qué tipo crear, con un `enum` o un string.

```cpp
class Figura {
public:
    enum Tipo { CIRCULO, CUADRADO, TRIANGULO };
    Figura(Tipo t, double a, double b = 0, double c = 0) : tipo(t) {
        // ... lógica monstruosa según el tipo
    }
private:
    Tipo tipo;
    // parámetros para todos los tipos posibles, la mayoría sin usar
};
```

Síntomas:

- Una clase con todos los parámetros de todas las figuras posibles.
- `switch` en cada método según `tipo`.
- Es **exactamente el Bad del día 1**, que ya refactorizamos a herencia.

## 4. Good (a) — Factory simple estático

La versión que más se usa en la práctica. Centralizamos la creación en una
función o clase, sin herencia de fábricas:

```cpp
class FabricaFiguras {
public:
    static std::unique_ptr<Shape> crear(const std::string& linea) {
        std::istringstream in(linea);
        std::string tipo;
        in >> tipo;

        if (tipo == "circulo") {
            double r; in >> r;
            return std::make_unique<Circulo>(r);
        }
        if (tipo == "cuadrado") {
            double l; in >> l;
            return std::make_unique<Cuadrado>(l);
        }
        if (tipo == "triangulo") {
            double a, b, c; in >> a >> b >> c;
            return std::make_unique<Triangulo>(a, b, c);
        }
        throw std::runtime_error("figura desconocida: " + tipo);
    }
};
```

Uso:

```cpp
auto figura = FabricaFiguras::crear("circulo 3.0");
Lienzo::instancia().anadir(std::move(figura));
```

**Esto sigue teniendo el `if/else`**. ¿Hemos ganado algo?

Sí, dos cosas:

1. La lógica de "leer texto y construir" está **en un único sitio**. Si
   cambia el formato, toco esta clase y nada más.
2. El **cliente** (`main`, GUI, parser) ya no sabe nada de tipos
   concretos. Depende solo de `Shape` (DIP) y de `FabricaFiguras`.

Para que el `if/else` desaparezca, pasamos al registro dinámico.

## 5. Good (b) — Factory con registro

Versión más sofisticada: cada figura se **registra** en la fábrica en
tiempo de ejecución (o de inicialización). Añadir una figura nueva ya
no toca el `if/else` porque no hay `if/else`.

```cpp
class FabricaFiguras {
public:
    using Constructor = std::function<std::unique_ptr<Shape>(std::istringstream&)>;

    // Registro de un nuevo tipo
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
    // Singleton de Meyers escondido para el mapa
    static std::map<std::string, Constructor>& registro() {
        static std::map<std::string, Constructor> m;
        return m;
    }
};
```

Registro de cada tipo, típicamente al inicio del programa:

```cpp
FabricaFiguras::registrar("circulo", [](std::istringstream& in) {
    double r; in >> r;
    return std::make_unique<Circulo>(r);
});

FabricaFiguras::registrar("cuadrado", [](std::istringstream& in) {
    double l; in >> l;
    return std::make_unique<Cuadrado>(l);
});
```

Añadir `Triangulo`: una línea de `registrar` en su propio `.cpp`. **No
tocamos `FabricaFiguras`**. OCP perfecto.

## 6. Good (c) — Factory Method estricto (GoF)

La versión original del GoF, con jerarquía de fábricas:

```cpp
class CreadorDeFigura {
public:
    virtual ~CreadorDeFigura() = default;
    virtual std::unique_ptr<Shape> crear() const = 0;   // ← Factory Method

    // Operación de plantilla que usa el factory method:
    void anadirAlLienzo() {
        Lienzo::instancia().anadir(crear());
    }
};

class CreadorCirculo : public CreadorDeFigura {
    double radio;
public:
    explicit CreadorCirculo(double r) : radio(r) {}
    std::unique_ptr<Shape> crear() const override {
        return std::make_unique<Circulo>(radio);
    }
};

class CreadorCuadrado : public CreadorDeFigura {
    double lado;
public:
    explicit CreadorCuadrado(double l) : lado(l) {}
    std::unique_ptr<Shape> crear() const override {
        return std::make_unique<Cuadrado>(lado);
    }
};
```

¿Cuándo merece la pena esto? Cuando **el creador tiene su propia lógica**
(validación, logging, configuración) además de crear. Si solo creas, el
factory simple (b) sobra.

En el Paint, el factory simple con registro es lo que usaremos en el
cierre. La versión jerárquica la dejamos preparada por si se necesita
cuando lleguen los plugins.

## 7. UML rápido

**Factory simple (b):**

```
┌─────────────────────────────┐
│      FabricaFiguras         │
├─────────────────────────────┤
│ - registro : map<str, fn>   │
├─────────────────────────────┤
│ + registrar(nombre, fn)     │
│ + crear(linea) : Shape*     │
└─────────────────────────────┘
            │ crea
            ▼
        ┌─────────┐
        │  Shape  │ ◀── Circulo, Cuadrado, Triangulo
        └─────────┘
```

**Factory Method GoF (c):**

```
┌──────────────────────┐         ┌─────────┐
│   CreadorDeFigura    │ ──crea─▶│  Shape  │
├──────────────────────┤         └─────────┘
│ + crear() = 0        │              ▲
│ + anadirAlLienzo()   │              │
└──────────────────────┘              │
        ▲                ┌────────────┼────────────┐
        │                │            │            │
┌───────────────┐   ┌──────────┐ ┌──────────┐ ┌──────────┐
│ CreadorCirculo│   │ Circulo  │ │ Cuadrado │ │Triangulo │
└───────────────┘   └──────────┘ └──────────┘ └──────────┘
        │ crea
        └────────────▶ Circulo
```

Dos jerarquías paralelas. Por eso decimos que el factory simple suele
ser suficiente.

## 8. Variantes y trampas

### Trampa: confundir Factory con Builder

Si la creación es **una sola llamada** con todos los parámetros, es
Factory. Si la creación se hace en **varios pasos** con configuración
intermedia, es Builder. Lo vemos en el siguiente bloque.

### Variante: factory con templates

```cpp
template<typename T, typename... Args>
std::unique_ptr<Shape> crear(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}
```

Útil para infraestructura, pero pierde la idea de "ocultar el tipo
concreto" porque el cliente todavía tiene que escribir `T`. Más un
helper que un patrón.

### Trampa: el factory que es Singleton

Muy frecuente: `FabricaFiguras::crear(...)` con todo estático es,
*de facto*, un Singleton sin reconocerlo. Hereda los problemas del
Singleton (estado global, difícil de testear). En proyectos grandes,
inyecta la fábrica como dependencia: vuelve a SOLID.

## 9. SOLID en acción

| Principio | Cómo lo aplica Factory                                            |
|-----------|-------------------------------------------------------------------|
| SRP       | Cumplido: la fábrica solo crea, el cliente solo usa.              |
| OCP       | **Cumplido especialmente bien** en la variante (b) con registro.  |
| LSP       | Cumplido: todas las figuras cumplen el contrato `Shape`.          |
| ISP       | Neutro.                                                           |
| DIP       | **Cumplido.** El cliente depende de `Shape`, no de tipos concretos.|

Es el patrón **más SOLID-friendly** de los creacionales. Por eso casi
cualquier código serio acaba teniendo factories por todas partes.

## 10. Mantra

> *"El cliente no llama a `new`. Pide a la fábrica."*
