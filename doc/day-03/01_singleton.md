# Singleton

## 1. El problema

En el Paint hay **un único lienzo** activo. La barra de herramientas
necesita acceder a él. El menú de archivo también. El gestor de undo
también. Todos quieren la **misma instancia**, no copias distintas.

Opción ingenua: pasarlo por parámetro a todo el mundo. Funciona, pero el
constructor de cualquier clase auxiliar acaba pidiendo un `Lienzo&` que
solo retransmite. Acoplamiento por transporte.

Opción tentadora: una variable global `Lienzo g_lienzo;`. Funciona,
pero perdemos el control de cuándo se crea, cómo se inicializa, y
podemos crear otra `Lienzo` sin querer.

Singleton es la respuesta canónica.

## 2. Intención (GoF)

> *Garantizar que una clase tenga una única instancia y proporcionar un
> punto de acceso global a ella.*

Dos promesas, no una:

1. **Unicidad**: no se puede crear una segunda.
2. **Acceso global**: se puede pedir desde cualquier sitio.

## 3. Bad — la clase global suelta

```cpp
class Lienzo {
public:
    std::vector<std::unique_ptr<Shape>> figuras;
    void anadir(std::unique_ptr<Shape> s) { figuras.push_back(std::move(s)); }
    void dibujar() const { for (const auto& f : figuras) f->dibujar(); }
};

// En algún .cpp:
Lienzo g_lienzo;   // global

// Y desde cualquier sitio:
g_lienzo.anadir(std::make_unique<Circulo>(3.0));
```

Problemas:

- Nada impide escribir `Lienzo otro;` en otra parte del código y trabajar
  con el equivocado.
- El orden de inicialización de globales entre unidades de traducción es
  **indefinido** en C++ (*static initialization order fiasco*). Si otra
  global usa `g_lienzo` durante su inicialización, puede explotar.
- No hay un punto único donde mirar para saber "quién es el lienzo".

## 4. Good — Singleton de Meyers

La versión moderna y limpia. Aprovecha que **las estáticas locales en C++11
son thread-safe en su inicialización** (lo garantiza el estándar):

```cpp
class Lienzo {
public:
    static Lienzo& instancia() {
        static Lienzo unica;     // se crea en la primera llamada, una sola vez
        return unica;
    }

    void anadir(std::unique_ptr<Shape> s) { figuras.push_back(std::move(s)); }
    void dibujar() const { for (const auto& f : figuras) f->dibujar(); }

    // Bloquear copia y movimiento: nadie debe duplicar el singleton.
    Lienzo(const Lienzo&)            = delete;
    Lienzo& operator=(const Lienzo&) = delete;
    Lienzo(Lienzo&&)                 = delete;
    Lienzo& operator=(Lienzo&&)      = delete;

private:
    Lienzo() = default;              // constructor privado
    ~Lienzo() = default;              // destructor privado: solo el propio singleton se destruye
    std::vector<std::unique_ptr<Shape>> figuras;
};
```

Uso:

```cpp
int main() {
    Lienzo::instancia().anadir(std::make_unique<Circulo>(3.0));
    Lienzo::instancia().anadir(std::make_unique<Cuadrado>(2.0));
    Lienzo::instancia().dibujar();
}
```

Las cuatro defensas que casi todo el mundo olvida:

| Defensa                  | Para qué                                            |
|--------------------------|-----------------------------------------------------|
| Constructor privado      | Que nadie haga `Lienzo l;`                          |
| Copia/asignación borrada | Que nadie haga `Lienzo l = Lienzo::instancia();`    |
| Movimiento borrado       | Que nadie robe el estado del singleton              |
| Destructor privado       | Que nadie haga `delete &Lienzo::instancia();`       |

## 5. UML rápido

```
┌────────────────────────────┐
│         Lienzo             │
├────────────────────────────┤
│ - figuras                  │
│ - Lienzo()      (privado)  │
├────────────────────────────┤
│ + instancia() : Lienzo&    │   ← punto de acceso global
│ + anadir(...)              │
│ + dibujar()                │
└────────────────────────────┘
        ▲
        │ se devuelve a sí misma
        └─── instancia única (static local)
```

## 6. Variantes y trampas en C++

### Variante: Singleton perezoso clásico (NO recomendado)

```cpp
class Lienzo {
    static Lienzo* unica;
public:
    static Lienzo* instancia() {
        if (!unica) unica = new Lienzo();   // no thread-safe sin lock
        return unica;
    }
};
Lienzo* Lienzo::unica = nullptr;
```

Problemas:

- `new` sin `delete`: la instancia nunca se libera (memory leak técnico,
  aunque "aceptable" porque dura toda la ejecución).
- No es thread-safe sin un `std::mutex` adicional.
- Devolver puntero invita a `delete` accidental.

**Regla**: en C++11 en adelante, usar siempre el Singleton de Meyers.

### Trampa: el orden de destrucción entre singletons

Si tienes dos singletons `A` y `B`, y el destructor de `A` usa `B`, ¿quién
se destruye primero? El estándar dice "en orden inverso al de construcción
**dentro de la misma unidad de traducción**". Entre unidades distintas,
ya hay un orden definido (LIFO de inicialización), pero conviene **no
depender de otros singletons en los destructores**.

### Trampa: Singleton + herencia

¿Puede `Lienzo` ser singleton **y** abstracto? Técnicamente sí (lo haría
un Factory que devuelve el `Lienzo` concreto), pero es un olor: si lo
necesitas, probablemente no querías Singleton, querías **inyección de
una instancia compartida**.

## 7. La cara oscura del Singleton

Aviso obligatorio. Singleton es el patrón **más criticado** del GoF:

- Es un **estado global disfrazado**. Acopla todo el código que lo usa.
- **Rompe testing**: ¿cómo sustituyes el lienzo por un mock en un test?
  Si todo el código llama `Lienzo::instancia()`, no puedes.
- **Viola DIP**: el código que lo usa depende del detalle (la clase
  concreta), no de una abstracción.
- Es **contagioso**: en cuanto tienes uno, aparecen tres más.

Heurística honesta:

- ¿Hay literalmente una sola instancia por restricción física (hardware,
  fichero, recurso del sistema)? → Singleton tiene sentido.
- ¿Hay una sola por **conveniencia**? → Probablemente debería ser una
  instancia normal **inyectada** donde haga falta. Pasar `Lienzo&` por
  constructor es más feo de escribir pero infinitamente más testeable.

**En el Paint** lo usamos porque es didáctico y porque "solo hay un
lienzo" es una restricción del dominio. En sistemas reales, piénsalo dos
veces.

## 8. SOLID en acción

| Principio | Cómo lo aplica Singleton                                            |
|-----------|---------------------------------------------------------------------|
| SRP       | El singleton gestiona su unicidad **y** su trabajo. Ojo: doble responsabilidad. |
| OCP       | Neutro.                                                             |
| LSP       | No aplica (no hay subclases típicamente).                           |
| ISP       | Neutro.                                                             |
| DIP       | **Lo viola.** Por eso es polémico.                                  |

Esta tabla es la razón por la que Singleton va el primero del día: nos
deja servida la conversación de qué cuesta cada patrón. **Ningún patrón
es gratis.**

## 9. Mantra

> *"Singleton: úsalo cuando el dominio diga que solo hay uno. Si lo usas
> por pereza, lo pagarás en los tests."*
