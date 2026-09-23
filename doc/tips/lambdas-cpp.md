# Lambdas en C++: funciones de usar y tirar

## 1. Qué es realmente una lambda

Una lambda **no es una función**. Es azúcar sintáctico para que el compilador genere una **clase anónima con `operator()` sobrecargado** (un *functor*) y cree una instancia de ella en el punto de uso. Por eso decimos que es una "función de usar y tirar": se define inline, se usa, y no contamina el espacio de nombres con un identificador.

```cpp
auto suma = [](int a, int b) { return a + b; };
int r = suma(3, 4); // 7
```

Lo que el compilador genera por debajo es equivalente a:

```cpp
class __lambda_anonima_1 {
public:
    int operator()(int a, int b) const { return a + b; }
};

auto suma = __lambda_anonima_1{};
```

Cada lambda genera un **tipo único e innombrable** (*closure type*). Dos lambdas con el mismo cuerpo tienen tipos distintos. Por eso solo se pueden guardar con `auto`, plantillas o `std::function`.

## 2. Anatomía de una lambda

```cpp
[captura](parámetros) especificadores -> tipo_retorno { cuerpo }
```

| Parte | Obligatoria | Ejemplo |
|---|---|---|
| `[captura]` | Sí | `[x]`, `[&x]`, `[=]`, `[&]`, `[this]` |
| `(parámetros)` | No (si está vacía y no hay especificadores) | `(int a, int b)` |
| Especificadores | No | `mutable`, `constexpr`, `noexcept` |
| `-> tipo` | No (se deduce) | `-> double` |
| `{ cuerpo }` | Sí | `{ return a + b; }` |

## 3. Capturas: el estado del closure

Las capturas son **variables miembro** de la clase generada. Es lo que diferencia un closure de una función normal: lleva estado consigo.

```cpp
int factor = 3;

auto porCopia      = [factor](int x)  { return x * factor; };   // miembro int
auto porReferencia = [&factor](int x) { return x * factor; };   // miembro int&
auto todoCopia     = [=](int x)       { return x * factor; };   // captura implícita por valor
auto todoRef       = [&](int x)       { return x * factor; };   // captura implícita por referencia
auto init          = [f = factor * 2](int x) { return x * f; }; // init-capture (C++14)
```

Equivalencia generada para `porCopia`:

```cpp
class __lambda_anonima_2 {
    int factor; // copia capturada
public:
    __lambda_anonima_2(int f) : factor(f) {}
    int operator()(int x) const { return x * factor; }
};
```

Puntos clave:

- `operator()` es `const` por defecto: no puedes modificar las capturas por valor salvo que declares la lambda `mutable`.
- Capturar por referencia (`&`) crea *dangling references* si el closure sobrevive a la variable. Error clásico al devolver lambdas o lanzarlas a hilos.
- `[this]` captura el puntero; `[*this]` (C++17) copia el objeto entero.

```cpp
int contador = 0;
auto inc = [contador]() mutable { return ++contador; }; // modifica SU copia
inc(); inc();          // copia interna = 2
// contador sigue siendo 0
```

## 4. Punteros a función: el mecanismo de C

Un puntero a función es solo eso: una dirección de código. **No puede llevar estado.**

```cpp
int duplica(int x) { return x * 2; }

int (*pf)(int) = duplica;   // sintaxis clásica
auto pf2 = &duplica;        // equivalente
int r = pf(5);              // 10
```

### Conversión lambda → puntero a función

Una lambda **sin capturas** es convertible implícitamente a puntero a función. El compilador genera un `operator` de conversión:

```cpp
int (*pf)(int) = [](int x) { return x * 2; };  // OK: sin capturas

int factor = 3;
int (*pf2)(int) = [factor](int x) { return x * factor; }; // ERROR: tiene estado
```

Esto demuestra el punto central: **en cuanto hay captura, la lambda deja de ser "solo código" y pasa a ser "código + datos"**. Un puntero a función no tiene dónde guardar los datos.

Esta conversión es útil para APIs de C:

```cpp
std::qsort(datos, n, sizeof(int),
    [](const void* a, const void* b) {
        return *static_cast<const int*>(a) - *static_cast<const int*>(b);
    });
```

## 5. `std::function`: el contenedor universal

`std::function<R(Args...)>` es un wrapper con **type erasure**: almacena cualquier *callable* con esa firma — lambda (con o sin capturas), puntero a función, functor, resultado de `std::bind`, puntero a miembro.

```cpp
#include <functional>

std::function<int(int)> f;

f = [](int x) { return x * 2; };          // lambda sin captura
int factor = 3;
f = [factor](int x) { return x * factor; }; // lambda con captura
f = duplica;                               // puntero a función
```

### El coste del type erasure

`std::function` no es gratis:

- **Indirección**: la llamada pasa por un puntero virtual o equivalente → el compilador normalmente **no puede inlinear**.
- **Posible asignación en heap** si el closure no cabe en el *small buffer* interno (típicamente 16–32 bytes según implementación).
- **Tamaño**: un `std::function` ocupa ~32–64 bytes frente a los 8 de un puntero a función o los 0–N bytes exactos de un closure.

Por eso, en código genérico se prefiere recibir el callable como **parámetro de plantilla** (o `auto` en C++20), que conserva el tipo concreto y permite inlining:

```cpp
// Bien: cero overhead, inlineable
template <typename F>
void aplicar(F&& f) { f(42); }

// C++20, equivalente
void aplicar(auto&& f) { f(42); }

// Solo si necesitas almacenarlo o cruzar una frontera ABI
void aplicar(std::function<void(int)> f) { f(42); }
```

### Qué es la frontera ABI

La ABI (*Application Binary Interface*) es el contrato a nivel binario entre módulos compilados por separado: layout de structs, convenciones de llamada, name mangling, layout de vtables. Una **frontera ABI** es el punto donde tu código se comunica con un binario que no compilas tú: una DLL/.so, un plugin, una librería precompilada, o código compilado con otro compilador u otros flags.

A través de esa frontera no puedes pasar tipos cuya representación dependa del compilador o de la instanciación. El closure de una lambda tiene un tipo anónimo único que solo existe en tu unidad de traducción: no puede aparecer en la firma de una función exportada. Y las plantillas directamente no cruzan la frontera, porque se instancian en compilación:

```cpp
// Exportable: firma fija, el otro lado sabe exactamente qué recibe
extern "C" void registrar_callback(void (*cb)(int, void* ctx), void* ctx);

// NO exportable: ¿qué tipo es F al otro lado de la DLL?
template <typename F>
void registrar_callback(F&& f);
```

Opciones para pasar callables a través de la frontera:

1. **Puntero a función + `void* ctx`**: el patrón C clásico, máxima compatibilidad.
2. **`std::function`**: solo si ambos lados usan el mismo compilador y la misma implementación de la librería estándar (su representación interna no está estandarizada; libstdc++ y libc++ son incompatibles entre sí).
3. **Interfaz abstracta** (clase con virtuales puras): estable dentro del mismo vendor de compilador.

## 6. Comparativa

| | Puntero a función | Lambda (tipo closure) | `std::function` |
|---|---|---|---|
| Estado (capturas) | ❌ | ✅ | ✅ |
| Tamaño | 8 bytes | Exacto a sus capturas (0 si no hay) | ~32–64 bytes + posible heap |
| Inlining | Difícil | ✅ (tipo conocido en compilación) | Prácticamente nunca |
| Almacenable en contenedores homogéneos | ✅ | ❌ (cada lambda es un tipo distinto) | ✅ |
| Reasignable a otro callable | ✅ (misma firma) | ❌ | ✅ |
| Interop con APIs de C | ✅ | Solo sin capturas | ❌ |
| Coste de llamada | Indirección | Directa | Indirección + type erasure |

## 7. Lambdas genéricas y plantillas (C++14/20)

```cpp
// C++14: operator() es plantilla
auto imprime = [](const auto& x) { std::cout << x << '\n'; };
imprime(42);
imprime("hola");

// C++20: sintaxis de plantilla explícita
auto primero = []<typename T>(const std::vector<T>& v) { return v.front(); };
```

El compilador genera:

```cpp
class __lambda_anonima_3 {
public:
    template <typename T>
    void operator()(const T& x) const { std::cout << x << '\n'; }
};
```

Una lambda genérica **no es convertible** a `std::function` de forma única (¿qué instanciación?) hasta que se fija la firma en la asignación.

## 8. Demostración final: usar y tirar

El caso de uso canónico — pasar comportamiento a un algoritmo sin definir nada con nombre:

```cpp
std::vector<int> v{5, 2, 8, 1, 9};
int umbral = 4;

// Antes de C++11: functor con nombre, definido lejos del punto de uso
struct MayorQue {
    int umbral;
    bool operator()(int x) const { return x > umbral; }
};
auto n1 = std::count_if(v.begin(), v.end(), MayorQue{umbral});

// Con lambda: se define, se usa, desaparece
auto n2 = std::count_if(v.begin(), v.end(),
                        [umbral](int x) { return x > umbral; });
```

La lambda hace exactamente lo mismo que `MayorQue` — porque **es** `MayorQue`, generada por el compilador, anónima y en el punto exacto donde se necesita. Función de usar y tirar.

## 9. Reglas prácticas

1. **Parámetro de plantilla / `auto`** para recibir callables → cero coste.
2. **`std::function`** solo cuando necesitas almacenar callables heterogéneos o una firma estable (callbacks en miembros, plugins, fronteras de módulo).
3. **Puntero a función** solo para interop con C o cuando garantizas ausencia de estado.
4. Cuidado con `[&]` en lambdas que escapan del scope (hilos, callbacks asíncronos, `return`): captura por valor o con `std::move`.
5. `mutable` modifica la copia interna del closure, no la variable original.
