# Día 1 — Bloque 1: Punteros inteligentes y RAII

> **Objetivo del bloque:** que el alumno deje de pensar en `new`/`delete` y empiece
> a pensar en **propiedad** (ownership). Esto es prerrequisito para todos los
> patrones que vienen después.

---

## 1. El problema: ¿por qué punteros inteligentes?

En C clásico (y C++98) la gestión de memoria es manual:

```cpp
Shape* s = new Circle(5.0);
// ... uso ...
delete s;   // ¿seguro que pasamos por aquí siempre?
```

Problemas reales que aparecen en proyectos:

- **Memory leaks**: olvidar `delete`.
- **Double free**: borrar dos veces el mismo puntero.
- **Dangling pointer**: usar un puntero después de borrarlo.
- **Excepciones**: si entre `new` y `delete` salta una excepción, el `delete`
  nunca se ejecuta.

Ejemplo del último caso:

```cpp
void procesa() {
    Shape* s = new Circle(5.0);
    funcionQuePuedeLanzar();   // si lanza, leak garantizado
    delete s;
}
```

## 2. RAII: la idea de fondo

**Resource Acquisition Is Initialization.** El recurso (memoria, fichero,
mutex, conexión) se adquiere en el constructor y se libera en el destructor.
Como el destructor se llama **siempre** que el objeto sale de su ámbito
(incluso ante excepciones), la liberación está garantizada.

Los punteros inteligentes son la aplicación de RAII a la memoria dinámica.

```cpp
void procesa() {
    auto s = std::make_unique<Circle>(5.0);
    funcionQuePuedeLanzar();   // si lanza, el destructor de unique_ptr libera
}   // aquí se libera sí o sí
```

RAII no es solo para memoria: `std::lock_guard`, `std::ifstream`,
`std::scoped_lock`… todos siguen el mismo patrón.

## 3. `std::unique_ptr<T>` — propiedad exclusiva

Es el **caso por defecto**. Si dudas qué puntero usar, usa este.

- Un solo dueño.
- No es copiable (copia → error de compilación).
- Es movible (`std::move`): transfiere la propiedad.
- Coste cero: ocupa lo mismo que un puntero crudo.

```cpp
#include <memory>
#include <iostream>

struct Shape {
    virtual ~Shape() = default;
    virtual void draw() const = 0;
};

struct Circle : Shape {
    double r;
    explicit Circle(double radio) : r(radio) {}
    void draw() const override { std::cout << "Circulo r=" << r << "\n"; }
};

int main() {
    std::unique_ptr<Shape> s = std::make_unique<Circle>(3.0);
    s->draw();

    // std::unique_ptr<Shape> s2 = s;       // ERROR: no copiable
    std::unique_ptr<Shape> s2 = std::move(s); // OK: ahora s2 es dueño
    // s ya no apunta a nada: s == nullptr

    s2->draw();
}   // s2 sale de ámbito y libera el Circle
```

**Regla práctica:** si una función recibe `std::unique_ptr<T>` por valor, está
diciendo *"dame la propiedad"*. Si recibe `T&` o `T*`, está diciendo *"déjame
usarlo, pero no soy el dueño"*.

```cpp
void consume(std::unique_ptr<Shape> s) { s->draw(); } // toma la propiedad
void usa(const Shape& s)               { s.draw();  } // solo observa

int main() {
    auto c = std::make_unique<Circle>(1.0);
    usa(*c);             // ok, sigue siendo nuestro
    consume(std::move(c)); // transferimos
    // c ya no vale
}
```

## 4. `std::shared_ptr<T>` — propiedad compartida

Mantiene un **contador de referencias**. Cuando el contador llega a 0, libera.

- Copiable: cada copia incrementa el contador.
- Más caro que `unique_ptr` (el bloque de control, atomicidad del contador).
- **Solo cuando la propiedad es genuinamente compartida.** Por defecto, NO.

```cpp
auto a = std::make_shared<Circle>(2.0);
{
    auto b = a;                  // mismo objeto, contador = 2
    std::cout << a.use_count();  // 2
}                                // b sale de ámbito, contador = 1
std::cout << a.use_count();      // 1
```

**Antipatrón común:** usar `shared_ptr` "por si acaso". Si nadie comparte
realmente la propiedad, estás pagando un coste sin razón.

## 5. `std::weak_ptr<T>` — observador no propietario

Apunta a un objeto gestionado por `shared_ptr` **sin** participar en el
contador. Sirve para:

- Romper ciclos de referencias.
- Observar un objeto que puede haber sido destruido.

Para usarlo hay que convertirlo a `shared_ptr` con `.lock()`:

```cpp
std::shared_ptr<Shape> fuerte = std::make_shared<Circle>(1.0);
std::weak_ptr<Shape>   debil  = fuerte;

if (auto sp = debil.lock()) {   // ¿sigue vivo?
    sp->draw();
} else {
    std::cout << "ya no existe\n";
}
```

## 6. El problema del ciclo (muy importante)

Si dos objetos se referencian mutuamente con `shared_ptr`, **nunca se liberan**.

```cpp
struct Nodo {
    std::shared_ptr<Nodo> otro;   // ciclo
    ~Nodo() { std::cout << "destruido\n"; }
};

int main() {
    auto a = std::make_shared<Nodo>();
    auto b = std::make_shared<Nodo>();
    a->otro = b;
    b->otro = a;   // ciclo: ninguno bajará a 0
}   // no imprime "destruido": LEAK
```

**Solución:** uno de los dos enlaces debe ser `weak_ptr`. Patrón típico:
padre → hijo con `shared_ptr`, hijo → padre con `weak_ptr`.

Esto reaparecerá en **Observer** y en **Composite**.

## 7. Constructores: copia y move

Aquí conectamos con propiedad y con punteros inteligentes.

### Constructor de copia

Crea un objeto **nuevo** duplicando los datos de otro.

```cpp
struct Buffer {
    std::size_t n;
    int* datos;

    explicit Buffer(std::size_t tam) : n(tam), datos(new int[tam]{}) {}
    ~Buffer() { delete[] datos; }

    // Constructor de copia: copia profunda
    Buffer(const Buffer& otro) : n(otro.n), datos(new int[otro.n]) {
        std::copy(otro.datos, otro.datos + n, datos);
    }

    // Operador de asignación por copia
    Buffer& operator=(const Buffer& otro) {
        if (this == &otro) return *this;
        delete[] datos;
        n = otro.n;
        datos = new int[n];
        std::copy(otro.datos, otro.datos + n, datos);
        return *this;
    }
};
```

### Constructor de move (C++11+)

Crea un objeto nuevo **robando** los recursos del otro, que queda en estado
válido pero vacío. Mucho más barato que copiar.

```cpp
struct Buffer {
    std::size_t n;
    int* datos;

    explicit Buffer(std::size_t tam) : n(tam), datos(new int[tam]{}) {}
    ~Buffer() { delete[] datos; }

    // Constructor de move
    Buffer(Buffer&& otro) noexcept : n(otro.n), datos(otro.datos) {
        otro.n = 0;
        otro.datos = nullptr;   // dejarlo en estado válido
    }

    // Asignación por move
    Buffer& operator=(Buffer&& otro) noexcept {
        if (this == &otro) return *this;
        delete[] datos;
        n = otro.n;
        datos = otro.datos;
        otro.n = 0;
        otro.datos = nullptr;
        return *this;
    }
};
```

### La regla de los 5 (y la del 0)

Si declaras uno de estos, normalmente tienes que pensar en los cinco:

1. Destructor
2. Constructor de copia
3. Operador de asignación por copia
4. Constructor de move
5. Operador de asignación por move

**La regla del 0**, que es la que queremos en C++ moderno: si usas
`unique_ptr`, `vector`, `string`… para gestionar tus recursos, **no
declares ninguno** y el compilador genera los cinco correctos.

```cpp
struct BufferModerno {
    std::vector<int> datos;        // RAII y move-aware
    explicit BufferModerno(std::size_t n) : datos(n) {}
    // No hace falta destructor, copia, move ni asignaciones.
};
```

### `std::move` no mueve nada

`std::move` solo es un **cast** a referencia rvalue (`T&&`). Le dice al
compilador *"trata esto como movible"*. El movimiento real lo hace el
constructor de move.

```cpp
auto a = std::make_unique<Circle>(1.0);
auto b = std::move(a);   // a queda en nullptr; b es el nuevo dueño
```

## 8. Anti-patrones que ver hoy

```cpp
// ❌ No mezclar new con punteros inteligentes
std::unique_ptr<Circle> a(new Circle(1.0));   // funciona, pero usar make_unique

// ❌ No pasar el mismo puntero crudo a dos shared_ptr
Circle* raw = new Circle(1.0);
std::shared_ptr<Circle> s1(raw);
std::shared_ptr<Circle> s2(raw);   // dos contadores independientes → double free

// ❌ shared_ptr por defecto
std::shared_ptr<Logger> log;       // ¿de verdad alguien más es propietario?

// ✅ Versión correcta
auto a = std::make_unique<Circle>(1.0);
auto s = std::make_shared<Circle>(1.0);
```

## 9. Ejercicio guiado

Antes de pasar a templates, plantear este ejercicio en vivo:

> *"Tenemos un lienzo (`Canvas`) que contiene figuras (`Shape`). ¿Quién es el
> propietario de las figuras? ¿`Canvas` las posee o solo las observa?"*

Dos diseños posibles:

```cpp
// Diseño A: el Canvas es dueño de las figuras
class Canvas {
    std::vector<std::unique_ptr<Shape>> figuras;
public:
    void add(std::unique_ptr<Shape> s) { figuras.push_back(std::move(s)); }
};

// Diseño B: las figuras viven en otra parte; el Canvas solo las muestra
class Canvas {
    std::vector<Shape*> figuras;   // no propietario; o weak_ptr si shared
public:
    void add(Shape& s) { figuras.push_back(&s); }
};
```

Discutir: cuál tiene más sentido para el Paint y por qué. Esta decisión
arrastra a todos los patrones siguientes.

---

## Resumen del bloque

| Concepto       | Cuándo usarlo                                              |
|----------------|------------------------------------------------------------|
| `unique_ptr`   | Por defecto. Un solo dueño.                                |
| `shared_ptr`   | Cuando varios componentes comparten propiedad real.        |
| `weak_ptr`     | Observar sin poseer. Romper ciclos.                        |
| Puntero crudo  | Solo como "préstamo" no propietario, parámetro de función. |
| `make_unique`  | Siempre preferible a `new`.                                |
| `make_shared`  | Siempre preferible a `new` + constructor `shared_ptr`.     |

**Mantras del bloque:**

- *"Si dudas, `unique_ptr`."*
- *"`shared_ptr` solo cuando la propiedad es compartida de verdad."*
- *"Regla del 0: deja que el compilador haga el trabajo."*
