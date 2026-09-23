# Día 1 — Bloque 2: Templates (lo justo y necesario)

> **Objetivo del bloque:** que el alumno entienda templates lo suficiente para
> no asustarse cuando aparezcan en los patrones. **No** vamos a entrar en
> SFINAE, concepts profundos ni metaprogramación.

---

## 1. ¿Qué problema resuelven?

Sin templates, si queremos un "contenedor de enteros" y un "contenedor de
strings" hay que escribir dos clases casi idénticas. O peor, recurrir a
`void*` y perder el chequeo de tipos.

```cpp
class CajaInt    { int x;    public: int    get() const { return x; } };
class CajaString { std::string x; public: std::string get() const { return x; } };
// ... y CajaDouble, CajaShape, ...
```

Con templates, escribimos **una sola vez** y el compilador genera la versión
para cada tipo que necesitemos:

```cpp
template <typename T>
class Caja {
    T x;
public:
    explicit Caja(T v) : x(std::move(v)) {}
    T get() const { return x; }
};

Caja<int>         a(42);
Caja<std::string> b("hola");
Caja<Circle>      c(Circle{1.0});
```

## 2. Function templates

Igual de fácil para funciones:

```cpp
template <typename T>
T maximo(T a, T b) {
    return (a > b) ? a : b;
}

int    main() {
    auto x = maximo(3, 7);        // T deducido como int
    auto y = maximo(1.5, 2.5);    // T deducido como double
    auto z = maximo<double>(1, 2.5); // T forzado a double
}
```

El compilador **deduce** el tipo a partir de los argumentos. Si la deducción
es ambigua o falla, hay que especificarlo entre `<>`.

## 3. Class templates

Lo más común que veremos. Ejemplo más cercano al curso: una pila genérica.

```cpp
template <typename T>
class Pila {
    std::vector<T> datos;
public:
    void push(T v)    { datos.push_back(std::move(v)); }
    void pop()        { datos.pop_back(); }
    T&   top()        { return datos.back(); }
    bool vacia() const { return datos.empty(); }
};

Pila<int>                 pi;
Pila<std::unique_ptr<Shape>> ps;   // pila de figuras
```

Cuando lleguemos a **Command** y a **Memento**, una pila de comandos / estados
es exactamente esto.

## 4. Por qué los templates van en cabeceras

Esto desconcierta a la gente que viene de Java/C#. En C++ el compilador
necesita ver la **definición completa** de la template para instanciarla
con un tipo concreto. Por eso:

- Las templates se definen en archivos `.hpp` / `.h`, no en `.cpp`.
- Si separas declaración e implementación en archivos distintos, tendrás
  errores de linkado.

Hay técnicas para sortearlo (instanciación explícita), pero para este curso
basta con la regla: **template = todo en la cabecera**.

## 5. Múltiples parámetros y valores por defecto

```cpp
template <typename Clave, typename Valor>
class Diccionario { /* ... */ };

template <typename T, std::size_t N = 16>
class BufferFijo {
    T datos[N];
};

BufferFijo<int>      a;     // N = 16 por defecto
BufferFijo<int, 64>  b;     // N = 64
```

Los templates también admiten **valores** (no solo tipos) como parámetros.
`std::array<T, N>` es el ejemplo canónico.

## 6. Deducción de tipos: una pincelada

Reglas que casi siempre quieres recordar:

- `template <typename T> void f(T x)` → `T` se deduce **sin** referencias ni
  `const` del argumento. `int&` llega como `int`.
- `template <typename T> void f(const T& x)` → `T` se deduce sin la
  referencia, manteniendo el resto.
- `template <typename T> void f(T&& x)` → caso especial: **reenvío perfecto**
  (forwarding reference). Útil pero peligroso, lo veremos solo si surge.

```cpp
template <typename T>
void inspecciona(const T& x) {
    // T se deduce sin referencia, conservando const en el parámetro
}
```

## 7. STL: templates que ya estás usando

Para desmitificar el tema, recordar que todo esto **ya son templates**:

```cpp
std::vector<int>                 v;
std::map<std::string, int>       m;
std::unique_ptr<Shape>           p;
std::function<void(int)>         f;
std::pair<int, std::string>      par;
std::optional<double>            opt;
```

No tienes que escribir templates para usarlos, pero entender que detrás hay
una template explica por qué `std::vector<int>` y `std::vector<std::string>`
son **tipos distintos** que comparten interfaz.

## 8. Templates y herencia: CRTP

**Curiously Recurring Template Pattern.** Lo introducimos brevemente porque
aparecerá en el bloque de patrones, sobre todo como alternativa estática a
**Template Method** y a algunos casos de **Strategy**.

La idea: una clase hereda de una plantilla parametrizada por **ella misma**.

```cpp
template <typename Derivada>
class Imprimible {
public:
    void imprimir() const {
        // llama al método de la clase derivada sin virtual
        static_cast<const Derivada*>(this)->imprimirImpl();
    }
};

class Circulo : public Imprimible<Circulo> {
public:
    void imprimirImpl() const { std::cout << "Circulo\n"; }
};

int main() {
    Circulo c;
    c.imprimir();   // resolución en tiempo de compilación, sin v-table
}
```

**Ventaja:** polimorfismo sin coste de llamada virtual.
**Desventaja:** no hay polimorfismo en tiempo de ejecución; no puedes meter
`Imprimible*` heterogéneos en un `vector`.

No hace falta dominarlo hoy. Solo que sepan que existe y que más adelante
veremos cuándo elegir entre el "sabor dinámico" (`virtual`) y el "sabor
estático" (CRTP) del mismo patrón.

## 9. Lo que NO entra en este curso

Mencionar que existen, pero no entrar:

- **SFINAE** (Substitution Failure Is Not An Error): mecanismo histórico para
  activar/desactivar templates según propiedades del tipo.
- **Concepts** (C++20): la forma moderna y legible de hacer lo anterior.
- **Variadic templates**: templates con número variable de parámetros
  (`template <typename... Args>`). La base de `std::tuple`, `std::variant`,
  emplace, etc.
- **Template metaprogramming**: cómputo en tiempo de compilación con templates.
- **Especialización de templates**: definir comportamiento distinto para
  tipos concretos.

Para un curso de patrones, lo que ya hemos visto es suficiente.

## 10. Ejercicio guiado

Crear una pequeña plantilla `Coleccion<T>` con `add`, `size`, y un `each` que
acepte una función. Conectaremos esto luego con **Iterator** y **Visitor**.

```cpp
#include <functional>
#include <vector>

template <typename T>
class Coleccion {
    std::vector<T> datos;
public:
    void add(T v)                                 { datos.push_back(std::move(v)); }
    std::size_t size() const                      { return datos.size(); }
    void each(std::function<void(const T&)> fn) const {
        for (const auto& x : datos) fn(x);
    }
};
```

Uso:

```cpp
Coleccion<int> c;
c.add(1); c.add(2); c.add(3);
c.each([](const int& x) { std::cout << x << " "; });
```

Discutir: ¿qué pasa si `T` es `std::unique_ptr<Shape>`? ¿Por qué el `each`
recibe `const T&` y no `T`?

---

## Resumen del bloque

- Templates = código genérico parametrizado por tipos (y a veces valores).
- Se instancian en tiempo de compilación: una versión por cada tipo usado.
- Van en cabeceras.
- La STL es templates, ya los estás usando.
- CRTP existe; lo veremos cuando toque elegir entre polimorfismo
  dinámico/estático.

**Mantra del bloque:**

> *"Templates resuelven la duplicación cuando la única diferencia es el tipo.
> Si te ves copiando una clase para cambiar un `int` por un `double`, eso es
> una template."*
