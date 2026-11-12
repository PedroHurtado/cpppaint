# Patrones de diseño en el proyecto `paint`

Qué patrones usa ya el Paint de consola, en qué fichero y en qué método.
Después, tres mejoras que **no añaden funcionalidad**: solo quitan código
repetido. Cada una muestra el código actual y cómo quedaría.

---

## 1. Patrones implementados

| Patrón | Fichero | Método / función |
|---|---|---|
| **Singleton** (de Meyers) | [Canvas.h](../../paint/include/paint/Canvas.h), [Canvas.cpp](../../paint/src/Canvas.cpp) | `Canvas::Instance()`: una estática local. Constructor y destructor privados; copia y movimiento borrados. |
| | [ICanvas.h](../../paint/include/paint/ICanvas.h) | `~ICanvas()` es `protected` y no virtual: `delete &Canvas::Instance()` no compila. |
| | [main.cpp](../../paint/src/main.cpp) | `main()`: el único sitio que llama a `Canvas::Instance()`. |
| **Prototype** | [IShape.h](../../paint/include/paint/IShape.h) | `IShape::Clone()` (virtual pura) |
| | [Circle.cpp](../../paint/src/Circle.cpp), [Square.cpp](../../paint/src/Square.cpp) | `Circle::Clone()`, `Square::Clone()` |
| | [DuplicateShapeCommand.cpp](../../paint/src/DuplicateShapeCommand.cpp) | `DuplicateShapeCommand::Execute()`: clona sin conocer el tipo concreto. |
| **Factory** (fábrica con registro) | [ShapeFactory.h](../../paint/include/paint/ShapeFactory.h), [ShapeFactory.cpp](../../paint/src/ShapeFactory.cpp) | `ShapeFactory::Register()`, `Knows()`, `Create()`, tipo `Builder` |
| | [ShapeRegistration.cpp](../../paint/src/ShapeRegistration.cpp) | `RegisterBuiltinShapes()`: builders de `circle` y `square`. |
| | [App.cpp](../../paint/src/App.cpp) | `App::Run()`: el cliente de la fábrica. |
| **Command** (con deshacer) | [ICommand.h](../../paint/include/paint/ICommand.h) | `ICommand::Execute()` / `Undo()` |
| | [AddShapeCommand.cpp](../../paint/src/AddShapeCommand.cpp), [MoveShapeCommand.cpp](../../paint/src/MoveShapeCommand.cpp), [DuplicateShapeCommand.cpp](../../paint/src/DuplicateShapeCommand.cpp) | `Execute()` / `Undo()` de cada comando. Receptor: `ICanvas` / `IShape`. |
| | [CommandManager.cpp](../../paint/src/CommandManager.cpp) | `CommandManager::Execute()`, `Undo()`, `Redo()`: el invocador con el historial en dos pilas. |
| **Strategy** (con `std::function`) + **Registry** | [CommandRegistry.h](../../paint/include/paint/CommandRegistry.h), [CommandRegistry.cpp](../../paint/src/CommandRegistry.cpp) | `CommandRegistry::Register()` / `Run()`: cada verbo es una `Action` intercambiable. |
| | [CommandRegistration.cpp](../../paint/src/CommandRegistration.cpp) | `RegisterBuiltinCommands()`: las lambdas son las estrategias. |
| **Internal Iterator** (variante del Iterator de GoF) | [CommandRegistry.cpp](../../paint/src/CommandRegistry.cpp), [ShapeFactory.cpp](../../paint/src/ShapeFactory.cpp) | `ForEachListed(visit)`: recorre las entradas sin enseñar el `std::map`. |
| **Parameter / Context Object** | [CommandRegistry.h](../../paint/include/paint/CommandRegistry.h) | `struct AppContext`, que recibe cada `Action`. |
| **Ports & Adapters** | [IReader.h](../../paint/include/paint/IReader.h), [IWriter.h](../../paint/include/paint/IWriter.h) | Puertos `IReader::ReadLine()` / `IWriter::Write()` |
| | [ConsoleReader.cpp](../../paint/src/ConsoleReader.cpp), [ConsoleWriter.cpp](../../paint/src/ConsoleWriter.cpp) | Adaptan `std::cin` / `std::cout` a esos puertos. |
| **Dependency Injection + Composition Root** | [main.cpp](../../paint/src/main.cpp), [App.cpp](../../paint/src/App.cpp) | `main()` conecta las piezas; `App::App(ICanvas&, IReader&, IWriter&)` las recibe. |

> `CommandManager` es también el *Command Processor* de POSA: el cliente no
> lleva la cuenta del historial, la lleva él.

---

## 2. Mejoras propuestas

| Patrón | Fichero | Método / función afectado | Por qué |
|---|---|---|---|
| **Template Method** | [Circle.cpp](../../paint/src/Circle.cpp), [Square.cpp](../../paint/src/Square.cpp) | `Color()`, `Position()`, `MoveTo()`, `ToString()`, campos `color_` / `position_` | Son idénticos en las dos figuras. Solo cambian `Area()` y el nombre. |
| **Prototype con CRTP** | [Circle.cpp](../../paint/src/Circle.cpp), [Square.cpp](../../paint/src/Square.cpp) | `Clone()` | Es la misma línea en cada figura: `make_unique<T>(*this)`. |
| **Registry genérico** (`Registry<T>`) | [ShapeFactory.cpp](../../paint/src/ShapeFactory.cpp), [CommandRegistry.cpp](../../paint/src/CommandRegistry.cpp) | `Register()`, `Knows()`, `ForEachListed()`, `struct Entry` | Las dos clases son el mismo `map` nombre → `{args, description, callable}`. |

Las tres se han compilado con `g++ -std=c++17 -Wall -Wextra -Wpedantic`, sin
avisos, y la salida es la misma que la del código actual:

```
circle <radio> <color> <x> <y> -> anade un circulo
square <lado> <color> <x> <y> -> anade un cuadrado
Circle | area: 3.141593 | color: 3 | pos: (4, 5)
Circle | area: 3.141593 | color: 3 | pos: (9, 9)
Square | area: 4.000000 | color: 1 | pos: (0, 0)
figura desconocida: triangle
```

---

### 2.1 Template Method: una base común para las figuras

**El problema en una frase:** `Circle` y `Square` repiten cuatro métodos y dos
campos; solo se diferencian en el área y en el nombre.

**Definición:** el *Template Method* fija el esqueleto de un algoritmo en la
clase base y deja que las subclases rellenen solo los pasos que cambian (los
*hooks*).

#### Código actual

```cpp
// Circle.cpp
int   Circle::Color() const          { return color_; }       // ← igual en Square
Point Circle::Position() const       { return position_; }    // ← igual en Square
void  Circle::MoveTo(Point position) { position_ = position; } // ← igual en Square

std::string Circle::ToString() const {
    return "Circle | area: " + std::to_string(Area()) +       // ← solo cambia "Circle"
           " | color: " + std::to_string(color_) +
           " | pos: (" + std::to_string(position_.x) + ", " +
           std::to_string(position_.y) + ")";
}

// Square.cpp
int   Square::Color() const          { return color_; }       // ← copia
Point Square::Position() const       { return position_; }    // ← copia
void  Square::MoveTo(Point position) { position_ = position; } // ← copia

std::string Square::ToString() const {
    return "Square | area: " + std::to_string(Area()) +       // ← copia con otro nombre
           " | color: " + std::to_string(color_) +
           " | pos: (" + std::to_string(position_.x) + ", " +
           std::to_string(position_.y) + ")";
}
```

Si mañana cambia el formato de `ToString()`, hay que tocarlo en cada figura.
Si alguien se olvida de una, la salida deja de ser uniforme.

#### Con la propuesta

```cpp
// ShapeBase.h
class ShapeBase : public IShape {
public:
    int   Color() const final          { return color_; }
    Point Position() const final       { return position_; }
    void  MoveTo(Point position) final { position_ = position; }

    // Template Method: el formato se escribe una sola vez.
    std::string ToString() const final {
        return std::string(Name()) + " | area: " + std::to_string(Area()) +
               " | color: " + std::to_string(color_) +
               " | pos: (" + std::to_string(position_.x) + ", " +
               std::to_string(position_.y) + ")";
    }

protected:
    ShapeBase(int color, Point position) : color_(color), position_(position) {}

private:
    virtual const char* Name() const = 0;  // hook: lo pone cada figura
    // Area() es el otro hook; ya viene como virtual pura de IShape.

    int   color_;
    Point position_;
};
```

Cada figura queda reducida a lo que de verdad la distingue (ver 2.2 para la
versión final de `Circle` y `Square`).

| Qué ganamos | Qué pagamos |
|---|---|
| Un solo `ToString()`, un solo `MoveTo()`. | Un nivel más de herencia. |
| `final` impide que una figura rompa el formato. | Si una figura futura necesitase otro formato, habría que quitar el `final`. |
| `IShape` no cambia: los tests y los mocks siguen igual. | |

**Mantra:** *lo que es igual sube a la base; lo que cambia se queda como hook.*

#### Revisión crítica: ¿estamos metiendo estado en la jerarquía?

La objeción es razonable: *"conviertes `IShape` en una clase abstracta con
estado"*. La respuesta tiene dos partes.

**Lo que no ocurre.** `IShape` no cambia: sigue sin datos, con destructor
virtual y solo métodos virtuales puros. El estado vive en una clase nueva,
`ShapeBase`:

```
IShape        ← interfaz pura, intacta (Canvas, comandos y mocks dependen de ella)
  └ ShapeBase ← abstracta CON estado (lo nuevo)
      ├ Circle
      └ Square
```

DIP se mantiene y los mocks de los tests siguen implementando `IShape` sin
enterarse. Es el esquema *interfaz + base abstracta opcional* (como `List` y
`AbstractList` en Java).

**Lo que sí es verdad.** La propuesta mezcla dos cosas bajo el nombre de
Template Method:

| Parte | ¿Es Template Method? | ¿Necesita estado? |
|---|---|---|
| `ToString()` fijo que llama a los hooks `Name()` y `Area()` | **Sí** | No |
| `color_`, `position_`, `Color()`, `Position()`, `MoveTo()` | **No**: es *Pull Up Field / Pull Up Method* (reutilizar heredando) | Sí |

El patrón no pide estado; el estado entró por la segunda parte, y esa parte
tiene costes:

- **Fija cómo se representa una figura.** Da por hecho que toda figura guarda un
  único `Point`. Una línea (dos extremos) o un polígono (N vértices) no encajan:
  su `MoveTo()` tendría que trasladar varios puntos y el `final` lo impide.
  Tendrían que saltarse `ShapeBase` e implementar `IShape` directamente. Se
  puede, precisamente porque `IShape` sigue pura, pero demuestra que la base es
  menos general de lo que parece.
- **Base frágil.** Cualquier cambio en los campos de `ShapeBase` recompila y
  puede romper todas las figuras.
- **No compensa aquí.** Con dos figuras, lo repetido son unas doce líneas de
  getters triviales. La ganancia no paga el acoplamiento.

**Conclusión.** Funciona, pero como Template Method está mal planteada: mezcla
el patrón con herencia de estado, y esa mezcla es lo que no se justifica. La
versión que sí tiene sentido es **Template Method sin estado**: la base solo
fija el formato y lo monta a partir de métodos virtuales; los datos y los
getters se quedan en cada figura.

```cpp
// ShapeBase.h — sin campos
class ShapeBase : public IShape {
public:
    // Template Method: el formato se escribe una sola vez.
    std::string ToString() const final {
        const Point p = Position();                       // virtual
        return std::string(Name()) + " | area: " + std::to_string(Area()) +
               " | color: " + std::to_string(Color()) +   // virtual
               " | pos: (" + std::to_string(p.x) + ", " +
               std::to_string(p.y) + ")";
    }

private:
    virtual const char* Name() const = 0;   // único hook nuevo
};

// Circle.h — conserva sus datos y sus getters
class Circle final : public ShapeBase {
public:
    Circle(double radius, int color, Point position)
        : radius_(radius), color_(color), position_(position) {}

    double Area() const override { return 3.14159265358979 * radius_ * radius_; }
    int    Color() const override { return color_; }
    Point  Position() const override { return position_; }
    void   MoveTo(Point position) override { position_ = position; }
    std::unique_ptr<IShape> Clone() const override {
        return std::make_unique<Circle>(*this);
    }

private:
    const char* Name() const override { return "Circle"; }
    double radius_;
    int    color_;
    Point  position_;
};
```

Compilado con `-Wall -Wextra -Wpedantic`, sin avisos:

```
Circle | area: 3.141593 | color: 3 | pos: (4, 5)
Circle | area: 3.141593 | color: 3 | pos: (9, 9)
```

Una alternativa sin herencia nueva: como el formato solo necesita la interfaz
pública de `IShape`, puede ser una función libre
`std::string Describe(const IShape&, const char* name)`. Y si se quiere
agrupar color y posición, mejor una pequeña struct de valor que cada figura
contenga (composición) que un campo heredado.

| | Con estado (propuesta inicial) | Sin estado (recomendada) |
|---|---|---|
| Repetición eliminada | Formato + getters | Solo el formato |
| Supone cómo se representa una figura | Sí (un `Point`) | No |
| Una línea o un polígono pueden usar la base | No | Sí |
| Riesgo de base frágil | Sí | Mínimo |

**Mantra:** *Template Method reutiliza el algoritmo, no los datos.*

---

### 2.2 Prototype con CRTP: `Clone()` escrito una sola vez

**El problema en una frase:** cada figura escribe a mano un `Clone()` que es
siempre la misma línea.

**Definición:** CRTP (*Curiously Recurring Template Pattern*) es una clase
plantilla que recibe como parámetro a su propia clase derivada:
`class Circle : public CloneableShape<Circle>`. Así la base conoce el tipo
concreto **en tiempo de compilación** y puede hacer la copia por él.

#### Código actual

```cpp
// Circle.cpp
std::unique_ptr<IShape> Circle::Clone() const {
    return std::make_unique<Circle>(*this);   // ← la misma línea...
}

// Square.cpp
std::unique_ptr<IShape> Square::Clone() const {
    return std::make_unique<Square>(*this);   // ← ...con otro nombre
}
```

El riesgo real no es equivocarse de tipo, porque `make_unique<Circle>(square)`
no compila. El riesgo es **heredar de una figura y olvidar sobrescribir
`Clone()`**:

```cpp
class BorderedCircle : public Circle { int border_; /* ... */ };
// No sobrescribe Clone() → hereda Circle::Clone()
// "duplicate 0" devuelve un Circle: se pierde border_ y nadie avisa (slicing).
```

#### Con la propuesta

```cpp
// CloneableShape.h
template <typename Derived>
class CloneableShape : public ShapeBase {
public:
    std::unique_ptr<IShape> Clone() const final {
        // La base sabe cuál es el tipo concreto: es Derived.
        return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }

protected:
    using ShapeBase::ShapeBase;   // hereda el constructor (color, posición)
};

// Circle.h
class Circle final : public CloneableShape<Circle> {
public:
    Circle(double radius, int color, Point position)
        : CloneableShape(color, position), radius_(radius) {}

    double Area() const override { return 3.14159265358979 * radius_ * radius_; }

private:
    const char* Name() const override { return "Circle"; }
    double radius_;
};

// Square.h
class Square final : public CloneableShape<Square> {
public:
    Square(double side, int color, Point position)
        : CloneableShape(color, position), side_(side) {}

    double Area() const override { return side_ * side_; }

private:
    const char* Name() const override { return "Square"; }
    double side_;
};
```

Con 2.1 y 2.2 juntos, una figura solo tiene su constructor, su `Area()`,
su `Name()` y sus datos.

> **Ojo, a raíz de la revisión de 2.1.** `CloneableShape` no necesita el estado
> de `ShapeBase`: solo necesita estar entre `IShape` y la figura. Con la
> `ShapeBase` sin estado, `CloneableShape<Derived>` hereda de ella (o
> directamente de `IShape`) sin constructor heredado, y cada figura conserva
> sus campos y getters. Probado: compila y clona igual.

| Qué ganamos | Qué pagamos |
|---|---|
| `Clone()` se escribe una vez para todas las figuras. | Las plantillas cuestan más de leer a quien no conoce CRTP. |
| El `final` de `Circle`/`Square` impide el `BorderedCircle` de arriba: el slicing pasa a ser error de compilación. | Una figura nueva que herede de otra tendría que derivar de `CloneableShape<LaNueva>`. |
| Sigue siendo Prototype: el cliente (`DuplicateShapeCommand`) no cambia. | `CloneableShape<Circle>` y `CloneableShape<Square>` son tipos distintos (no importa aquí: todos se manejan como `IShape`). |

**Mantra:** *si la línea solo cambia en el nombre del tipo, el tipo es un
parámetro de plantilla.*

---

### 2.3 Registry genérico: un solo `map` para figuras y verbos

**El problema en una frase:** `ShapeFactory` y `CommandRegistry` son la misma
estructura escrita dos veces; solo cambia el tipo de lo que guardan.

**Definición:** un *Registry* es un objeto que asocia un nombre con algo que se
busca después. Hacerlo plantilla (`Registry<T>`) permite reutilizarlo para
cualquier `T`.

#### Código actual

```cpp
// ShapeFactory.h                              // CommandRegistry.h
struct Entry {                                 struct Entry {
    std::string args;                              std::string args;
    std::string description;                       std::string description;
    Builder builder;    // ← solo cambia esto      Action action;     // ← y esto
};                                             };
std::map<std::string, Entry> entries_;         std::map<std::string, Entry> entries_;
```

```cpp
// ShapeFactory.cpp
void ShapeFactory::Register(const std::string& name, const std::string& args,
                            const std::string& description, Builder builder) {
    entries_[name] = Entry{args, description, std::move(builder)};
}
bool ShapeFactory::Knows(const std::string& name) const {
    return entries_.find(name) != entries_.end();
}
void ShapeFactory::ForEachListed(/* visit */) const {
    for (const auto& entry : entries_) {
        const std::string& args = entry.second.args;
        visit(args.empty() ? entry.first : entry.first + " " + args,
              entry.second.description);
    }
}

// CommandRegistry.cpp  ← los mismos tres métodos, otra vez
void CommandRegistry::Register(/* ... */ Action action) {
    entries_[verb] = Entry{args, description, std::move(action)};
}
bool CommandRegistry::Knows(const std::string& verb) const { /* idéntico */ }
void CommandRegistry::ForEachListed(/* visit */) const {
    for (const auto& entry : entries_) {
        if (entry.second.description.empty()) continue;  // ← única diferencia
        /* idéntico */
    }
}
```

El propio comentario de `ShapeFactory.h` lo dice: *"Mismo contrato que
CommandRegistry::ForEachListed"*.

#### Con la propuesta

```cpp
// Registry.h
template <typename Callable>
class Registry {
public:
    using Visitor = std::function<void(const std::string& usage,
                                       const std::string& description)>;

    void Register(const std::string& name, const std::string& args,
                  const std::string& description, Callable callable) {
        entries_[name] = Entry{args, description, std::move(callable)};
    }

    bool Knows(const std::string& name) const {
        return entries_.find(name) != entries_.end();
    }

    // Busca o lanza. Cada cliente pone su mensaje de error.
    const Callable& Get(const std::string& name, const std::string& error) const {
        auto it = entries_.find(name);
        if (it == entries_.end()) throw std::runtime_error(error + name);
        return it->second.callable;
    }

    void ForEachListed(const Visitor& visit) const {
        for (const auto& [name, entry] : entries_) {
            if (entry.description.empty()) continue;   // alias oculto
            visit(entry.args.empty() ? name : name + " " + entry.args,
                  entry.description);
        }
    }

private:
    struct Entry {
        std::string args;
        std::string description;
        Callable callable;
    };
    std::map<std::string, Entry> entries_;
};
```

`ShapeFactory` y `CommandRegistry` **mantienen su interfaz pública**: por fuera
nada cambia. Por dentro solo delegan:

```cpp
// ShapeFactory.h
class ShapeFactory {
public:
    using Builder = std::function<std::unique_ptr<IShape>(std::istream&)>;
    using Visitor = Registry<Builder>::Visitor;

    void Register(const std::string& name, const std::string& args,
                  const std::string& description, Builder builder) {
        builders_.Register(name, args, description, std::move(builder));
    }
    bool Knows(const std::string& name) const { return builders_.Knows(name); }
    void ForEachListed(const Visitor& visit) const { builders_.ForEachListed(visit); }

    std::unique_ptr<IShape> Create(const std::string& line) const {
        std::istringstream in(line);
        std::string name;
        in >> name;
        return builders_.Get(name, "figura desconocida: ")(in);
    }

private:
    Registry<Builder> builders_;
};

// CommandRegistry::Run queda igual de corto:
Status CommandRegistry::Run(const std::string& verb, AppContext& context,
                            std::istream& args) const {
    return actions_.Get(verb, "comando desconocido: ")(context, args);
}
```

> **Sobre la única diferencia.** Ahora la fábrica lista todas sus figuras y el
> registro oculta las que tienen la descripción vacía. Con la plantilla las dos
> ocultan las vacías. El comportamiento es el mismo porque todas las figuras se
> registran con descripción (`RegisterBuiltinShapes`).

| Qué ganamos | Qué pagamos |
|---|---|
| Búsqueda, alta y listado se escriben una vez. | Las dos clases siguen existiendo como envoltorios que delegan. |
| Un tercer registro futuro (p. ej. de colores) sale gratis. | La plantilla vive entera en el `.h`. |
| `App`, `RegisterBuiltin*` y los tests no se tocan. | |

**Mantra:** *dos clases con la misma forma piden una plantilla, no un
copiar y pegar.*

---

## 3. Patrones descartados

| Patrón | Por qué no |
|---|---|
| **Composite** | Solo tiene sentido si se pueden agrupar figuras, y eso sería funcionalidad nueva. |
| **Memento** | `MoveShapeCommand` ya guarda `previous_`. Para un solo campo, un Memento completo sería ingeniería de más. |
| **Chain of Responsibility** | `App::Run` tiene tres ramas fijas (verbo, figura o desconocido) y el comentario garantiza que no crecen. Una cadena no aporta nada. |
| **Singleton** (quitarlo) | El propio `Canvas.h` reconoce que, una vez existe `ICanvas` y se inyecta desde `main()`, el Singleton sobra. Se queda porque es material didáctico. |

---

## 4. Chuleta

| Propuesta | En una línea |
|---|---|
| Template Method | `ShapeBase` **sin estado** escribe el formato de `ToString()` una vez; la figura pone `Name()` y conserva sus datos. |
| CRTP | `CloneableShape<Derived>` escribe `Clone()` una vez; `final` convierte el slicing en error de compilación. |
| Registry\<T\> | Un solo `map` nombre → `{args, description, callable}` para figuras y verbos. |

---

## Referencias

- 📖 [*Design Patterns*, Gamma, Helm, Johnson y Vlissides (Addison-Wesley)](https://www.informit.com/store/design-patterns-elements-of-reusable-object-oriented-9780201633610): la fuente original de Template Method y Prototype. Merece la pena leer los apartados de *Consecuencias*.
- [Refactoring.Guru: Template Method](https://refactoring.guru/design-patterns/template-method): diagramas claros y ejemplo en C++. **Gratuito.**
- [Refactoring.Guru: Prototype](https://refactoring.guru/design-patterns/prototype): explica por qué `Clone()` evita depender de los tipos concretos. **Gratuito.**
- [cppreference: CRTP](https://en.cppreference.com/w/cpp/language/crtp): la definición exacta del idiom con ejemplos mínimos. **Gratuito.**
- [Fluent C++: The Curiously Recurring Template Pattern](https://www.fluentcpp.com/2017/05/12/curiously-recurring-template-pattern/): serie corta que explica para qué sirve CRTP y sus trampas. **Gratuito.**
- [Martin Fowler: Registry](https://martinfowler.com/eaaCatalog/registry.html): la ficha del patrón en el catálogo de *Patterns of Enterprise Application Architecture*. **Gratuito.**
- 📖 [*Refactoring*, Martin Fowler (2.ª ed.)](https://martinfowler.com/books/refactoring.html): *Pull Up Method* y *Form Template Method* son las técnicas de los apartados 2.1 y 2.2.
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines): C.67 y C.130 explican por qué conviene `Clone()` en lugar de copiar a través de la base. **Gratuito.**
