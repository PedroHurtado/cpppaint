# C++20: El resto de novedades destacables

Compilación: todo lo de este documento funciona con `g++ -std=c++20` / `clang++ -std=c++20` / `cl /std:c++20`.

## Lenguaje

### Operador nave espacial `<=>`
Una sola línea genera **todas** las comparaciones (`<`, `>`, `<=`, `>=`, `==`, `!=`). Antes había que escribir 6 operadores a mano.
```cpp
struct Punto { int x, y; auto operator<=>(const Punto&) const = default; };
```

### Designated initializers
Inicialización por nombre de campo, como en C (y como los objetos de JS). Más legible y a prueba de reordenaciones.
```cpp
struct Config { int puerto; bool debug; };
Config c{ .puerto = 8080, .debug = true };
```

### Templates de función abreviados
`auto` en parámetros crea un template sin escribir `template<...>`.
```cpp
void imprimir(const auto& x) { std::cout << x; }  // = template<typename T>
```

### `consteval` y `constinit`
`consteval`: la función **debe** ejecutarse en compilación (si no, error). `constinit`: la variable global **debe** inicializarse en compilación (evita el fiasco del orden de inicialización estática), pero sigue siendo mutable.
```cpp
consteval int cuadrado(int x) { return x * x; }
constinit int contador = cuadrado(4);
```

### `constexpr` por todas partes
`std::vector`, `std::string`, funciones virtuales, `try/catch`, `new/delete`... casi todo puede ejecutarse ahora en tiempo de compilación.

### `[[likely]]` / `[[unlikely]]`
Pistas al optimizador sobre qué rama es la habitual.
```cpp
if (error) [[unlikely]] { /* ... */ }
```

### `[[no_unique_address]]`
Un miembro vacío (p.ej. un allocator o comparador sin estado) puede ocupar 0 bytes en lugar de 1 + padding.

### `using enum`
Trae los valores de un enum class al ámbito actual; quita verbosidad en los `switch`.
```cpp
switch (color) { using enum Color; case Rojo: ...; case Verde: ...; }
```

### Lambdas mejoradas
Lambdas con `template` explícito (`[]<typename T>(T x){...}`), lambdas en contextos no evaluados, y captura de packs de parámetros.

## Librería

### `std::format`
Formateo tipo Python f-strings. Se acabó el infierno de `<<` encadenados y los printf inseguros. (En C++23, `std::println` lo remata.)
```cpp
std::string s = std::format("Mesa {} para {} personas", 12, 4);
```

### `std::span`
Vista no propietaria sobre memoria contigua (array C, vector, parte de un vector). Sustituye al clásico par `(puntero, tamaño)` en parámetros.
```cpp
void procesa(std::span<const int> datos);  // acepta vector, array, lo que sea
```

### `starts_with` / `ends_with`
Por fin, en `std::string` y `string_view`. Antes: el truco ilegible de `compare(0, n, ...)`.
```cpp
if (url.starts_with("https://")) ...
```

### `std::erase` / `std::erase_if`
Borrar elementos de un contenedor en una llamada. Mata el idiom *erase-remove* que nadie recordaba cómo se escribía.
```cpp
std::erase_if(reservas, [](auto& r) { return r.cancelada; });
```

### `contains` en mapas y sets
`if (mapa.contains(clave))` en lugar del críptico `mapa.find(clave) != mapa.end()`.

### `std::numbers`
Constantes matemáticas estándar: `std::numbers::pi`, `e`, `sqrt2`... Se acabó definir PI a mano (o la macro `M_PI` no estándar).

### `<bit>` y `std::bit_cast`
Operaciones de bits con nombre (`popcount`, `rotl`, `has_single_bit`...) y reinterpretación de bytes segura sin UB (adiós al `memcpy` ritual o al cast con puntero).

### `std::source_location`
Fichero, línea y función actuales sin macros `__FILE__`/`__LINE__`. Ideal para funciones de logging.
```cpp
void log(std::string_view msg, std::source_location loc = std::source_location::current());
```

### Calendario y zonas horarias en `<chrono>`
Fechas de calendario (`2026y/6/5`), zonas horarias IANA y conversiones entre ellas, todo en la librería estándar. Antes: librerías externas o sufrimiento con `tm`.

## Concurrencia

### `std::jthread`
Thread que hace `join()` automático en su destructor (RAII) y soporta cancelación cooperativa vía `std::stop_token`. El `std::thread` que olvida un `join` llama a `std::terminate`; `jthread` arregla ese diseño.

### `std::latch` y `std::barrier`
Sincronización de "esperar a que N hilos lleguen a un punto": `latch` de un solo uso, `barrier` reutilizable por fases. Antes se simulaba con mutex + condition_variable.

### `std::counting_semaphore`
El semáforo clásico, por fin en el estándar. Limitar a N accesos concurrentes en tres líneas.

### `atomic::wait` / `notify_one`
Esperar a que un atomic cambie de valor sin spin-lock ni condition_variable.
