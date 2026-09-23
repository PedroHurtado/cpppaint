# C++20: Concepts

## Compilación

```bash
g++ -std=c++20 concepts.cpp -o concepts
clang++ -std=c++20 concepts.cpp -o concepts
# MSVC: cl /std:c++20 concepts.cpp
```

## El problema (antes de C++20)

Si un template recibía un tipo que no cumplía los requisitos, el error aparecía **dentro** del template, con mensajes kilométricos e ilegibles:

```cpp
template <typename T>
T suma(T a, T b) {
    return a + b;   // si T no soporta +, el error explota aquí
}

struct Persona {};

int main() {
    suma(Persona{}, Persona{});   // error de 40 líneas hablando de operator+
}
```

Para restringir tipos había que usar **SFINAE** (*Substitution Failure Is Not An Error*): una regla del lenguaje que dice que si al sustituir T en un template el resultado no compila, el compilador **no da error**, simplemente descarta ese template de la lista de candidatos y sigue buscando otro.

La regla existía para resolver sobrecargas, pero la comunidad la convirtió en un truco para restringir tipos: se mete a propósito algo que "falla la sustitución" cuando T no cumple lo que quieres. La pieza habitual era `std::enable_if`:

- Si la condición es `true` → `enable_if_t` produce un tipo válido → el template existe.
- Si la condición es `false` → `enable_if_t` no produce nada → la sustitución falla → el template desaparece silenciosamente (eso es SFINAE).

```cpp
// Antes: "solo acepta enteros" con SFINAE
template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
//        ^^^^^^^^ parámetro de template fantasma cuyo único
//        propósito es "romperse" cuando T no es entero
T suma(T a, T b) {
    return a + b;
}

suma(2, 3);     // is_integral_v<int> es true  → compila
suma(2.5, 3.1); // is_integral_v<double> es false → enable_if no produce tipo
                // → sustitución fallida → el template se descarta
                // → error: "no matching function for call to suma"
```

Funciona, pero fijaos en el coste: la restricción está codificada como un **parámetro de template falso** que existe solo para fallar, y el error resultante ("no matching function") no te dice **por qué** no hay candidata. ¿Quién entiende eso a la primera? Nadie.

## La solución (C++20)

Un **concept** es una restricción con nombre. Se lee como castellano:

```cpp
#include <concepts>

// Forma 1: en el template
template <std::integral T>
T suma(T a, T b) {
    return a + b;
}

// Forma 2: con requires
template <typename T>
    requires std::integral<T>
T suma2(T a, T b) {
    return a + b;
}

// Forma 3: abreviada con auto
std::integral auto suma3(std::integral auto a, std::integral auto b) {
    return a + b;
}

int main() {
    suma(2, 3);       // OK
    // suma(2.5, 3.1); // error claro: "double no satisface std::integral"
}
```

## Crear tu propio concept

```cpp
#include <concepts>
#include <string>

// "T es Sumable si a + b compila y devuelve algo convertible a T"
template <typename T>
concept Sumable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};

template <Sumable T>
T suma(T a, T b) {
    return a + b;
}

int main() {
    suma(1, 2);                              // OK: int
    suma(std::string("ho"), std::string("la")); // OK: string tiene +
    // suma(Persona{}, Persona{});           // error: Persona no satisface Sumable
}
```

## Ventajas

1. **Errores legibles**: el compilador dice "el tipo X no satisface el concept Y", no 40 líneas de template internals.
2. **El código documenta la intención**: `std::integral T` se lee solo; `enable_if` no.
3. **Sobrecarga por capacidades**: puedes tener varias versiones de una función y el compilador elige la más restrictiva que cumpla el tipo.
4. **Adiós SFINAE** para el 95% de los casos.

## Concepts estándar útiles (`<concepts>`)

| Concept | Significado |
|---|---|
| `std::integral` | tipos enteros |
| `std::floating_point` | float, double |
| `std::convertible_to<U>` | convertible a U |
| `std::same_as<U>` | exactamente U |
| `std::invocable<Args...>` | se puede llamar con esos argumentos |
| `std::equality_comparable` | soporta == y != |
