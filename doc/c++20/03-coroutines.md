# C++20: Coroutines

## Compilación

```bash
g++ -std=c++20 coroutines.cpp -o app          # GCC 11+
g++ -std=c++20 -fcoroutines coroutines.cpp    # GCC 10 necesita el flag extra
clang++ -std=c++20 coroutines.cpp -o app      # Clang 14+
# MSVC: cl /std:c++20 coroutines.cpp
```

## ¿Qué es una corrutina?

Una función normal empieza, ejecuta y termina. Una **corrutina** puede **pausarse** a mitad, devolver el control al llamador, y **reanudarse** después justo donde se quedó, conservando sus variables locales.

Una función es corrutina si usa cualquiera de estas tres palabras:

- `co_yield` → pausa y entrega un valor (generadores)
- `co_await` → pausa hasta que algo termine (asincronía)
- `co_return` → termina la corrutina

## El problema (antes de C++20)

Quiero una secuencia "perezosa" de números (no quiero generar el millón de valores de golpe). Antes había que escribir una clase con estado a mano:

```cpp
// Antes: máquina de estados manual
class Contador {
    int actual, fin;
public:
    Contador(int desde, int hasta) : actual(desde), fin(hasta) {}
    bool tiene_siguiente() const { return actual < fin; }
    int siguiente() { return actual++; }   // el "estado" lo gestiono yo
};

int main() {
    Contador c(1, 5);
    while (c.tiene_siguiente())
        std::cout << c.siguiente() << ' ';
}
```

Para algo trivial vale, pero en cuanto la lógica se complica (bucles anidados, condiciones), convertirla en máquina de estados es un infierno.

## La solución (C++20)

La corrutina **escribe la lógica de forma natural** y el compilador genera la máquina de estados por ti:

```cpp
#include <iostream>
#include <coroutine>

// --- Boilerplate: el tipo Generator (en C++23 ya viene: std::generator) ---
template <typename T>
struct Generator {
    struct promise_type {
        T valor;
        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(T v) { valor = v; return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> h;
    explicit Generator(std::coroutine_handle<promise_type> h) : h(h) {}
    ~Generator() { if (h) h.destroy(); }

    bool siguiente() { h.resume(); return !h.done(); }
    T valor() const { return h.promise().valor; }
};
// --------------------------------------------------------------------------

// La corrutina: se lee como código normal
Generator<int> contador(int desde, int hasta) {
    for (int i = desde; i < hasta; ++i)
        co_yield i;          // pausa aquí y entrega i; al reanudar, sigue el for
}

Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {           // ¡secuencia infinita! No pasa nada: es perezosa
        co_yield a;
        auto tmp = a;
        a = b;
        b = tmp + b;
    }
}

int main() {
    auto g = contador(1, 5);
    while (g.siguiente())
        std::cout << g.valor() << ' ';      // 1 2 3 4
    std::cout << '\n';

    auto fib = fibonacci();
    for (int i = 0; i < 8 && fib.siguiente(); ++i)
        std::cout << fib.valor() << ' ';    // 0 1 1 2 3 5 8 13
}
```

Fijaos en `fibonacci()`: el `while (true)` no cuelga el programa porque **solo se ejecuta una iteración cada vez que alguien pide el siguiente valor**.

## Ventajas

1. **Lógica natural**: escribes bucles y condiciones normales; el compilador fabrica la máquina de estados.
2. **Evaluación perezosa**: secuencias infinitas o muy grandes sin gastar memoria.
3. **Base de la asincronía**: `co_await` permite escribir código asíncrono que se lee como síncrono (sin el callback hell de antes).

## Nota honesta para el curso

C++20 trae la **infraestructura** (las tres palabras clave y `<coroutine>`), pero no tipos de alto nivel listos para usar: el `Generator` de arriba hay que escribirlo (una vez) o usar una librería. En **C++23** ya existe `std::generator` y el boilerplate desaparece:

```cpp
// C++23
#include <generator>
std::generator<int> contador(int desde, int hasta) {
    for (int i = desde; i < hasta; ++i)
        co_yield i;
}
```
