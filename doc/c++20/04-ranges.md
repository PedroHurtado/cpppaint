# C++20: Ranges

## Compilación

```bash
g++ -std=c++20 ranges.cpp -o app       # GCC 10+
clang++ -std=c++20 ranges.cpp -o app   # Clang 15+ (libstdc++) / 16+ (libc++)
# MSVC: cl /std:c++20 ranges.cpp
```

## El problema (antes de C++20)

Los algoritmos STL trabajaban con **pares de iteradores**, y componer operaciones obligaba a crear contenedores intermedios:

```cpp
// Antes: "los cuadrados de los pares"
std::vector<int> nums{1, 2, 3, 4, 5, 6};

std::vector<int> pares;
std::copy_if(nums.begin(), nums.end(),          // begin/end por todas partes
             std::back_inserter(pares),
             [](int n) { return n % 2 == 0; });

std::vector<int> cuadrados;                     // OTRO vector intermedio
std::transform(pares.begin(), pares.end(),
               std::back_inserter(cuadrados),
               [](int n) { return n * n; });
// 2 vectores temporales, 2 recorridos, y el código se lee fatal
```

## La solución (C++20)

```cpp
#include <iostream>
#include <vector>
#include <ranges>

int main() {
    std::vector<int> nums{1, 2, 3, 4, 5, 6};

    auto resultado = nums
        | std::views::filter([](int n) { return n % 2 == 0; })
        | std::views::transform([](int n) { return n * n; });

    for (int n : resultado)
        std::cout << n << ' ';   // 4 16 36
}
```

Sin vectores intermedios, sin `begin()/end()`, y se lee de arriba abajo como una tubería (igual que `|` en Linux o LINQ en C#).

## LA CLAVE: una view es una INTENCIÓN, no una ejecución

Cuando escribís esto:

```cpp
auto resultado = nums | std::views::filter(...) | std::views::transform(...);
```

**no se ha ejecutado nada todavía**. `resultado` no es un vector con datos: es un objeto ligero que *describe* qué hacer. El trabajo ocurre solo cuando alguien **itera**. Demostración:

```cpp
#include <iostream>
#include <vector>
#include <ranges>

int main() {
    std::vector<int> nums{1, 2, 3, 4, 5, 6};

    auto vista = nums | std::views::transform([](int n) {
        std::cout << "[procesando " << n << "] ";   // espía
        return n * n;
    });

    std::cout << "Vista creada. Aún no se ha impreso nada del espía.\n";

    std::cout << "Ahora iteramos:\n";
    for (int n : vista)
        std::cout << n << '\n';
}
```

Salida:

```
Vista creada. Aún no se ha impreso nada del espía.
Ahora iteramos:
[procesando 1] 1
[procesando 2] 4
[procesando 3] 9
...
```

El lambda se ejecuta **elemento a elemento, bajo demanda**.

## Secuencias infinitas: `iota` + `take`

Primero, los dos protagonistas:

- **`std::views::iota(1)`** genera los enteros consecutivos `1, 2, 3, 4, ...` **sin fin**. No hay contenedor detrás: es una view que, cada vez que le pides "el siguiente", suma 1 y te lo da. Con dos argumentos sí tiene fin: `iota(1, 10)` es `1..9`.
- **`std::views::take(5)`** corta una secuencia: entrega los 5 primeros elementos y se acaba.

¿Cómo puede existir en C++ una secuencia infinita sin reventar la memoria ni colgar el programa? Porque las views funcionan por **arrastre (pull)**: nadie produce nada hasta que el consumidor pide el siguiente elemento. La pipeline no es una cadena de fábricas trabajando, es una cadena de teléfonos:

```cpp
// Los 5 primeros cuadrados de números pares
auto v = std::views::iota(1)                                    // 1,2,3,... infinito
       | std::views::filter([](int n){ return n % 2 == 0; })
       | std::views::transform([](int n){ return n * n; })
       | std::views::take(5);

for (int n : v) std::cout << n << ' ';   // 4 16 36 64 100
```

Seguid la llamada para el primer elemento del `for`:

1. El `for` le pide un elemento a `take` → "me quedan 5 por entregar, voy a buscar uno".
2. `take` se lo pide a `transform` → `transform` se lo pide a `filter` → `filter` se lo pide a `iota`.
3. `iota` produce `1`. `filter` lo rechaza (impar) y pide otro. `iota` produce `2`. `filter` lo acepta.
4. `transform` lo eleva al cuadrado: `4`. `take` lo entrega y descuenta: le quedan 4.

Y así 5 veces. Cuando `take` ha entregado sus 5, dice "se acabó" y el `for` termina. **`iota` solo llegó a producir hasta el 10**: el resto de la secuencia infinita nunca existió, porque nadie lo pidió.

Esto es imposible con el enfoque clásico: un `std::vector` con "todos los enteros" no cabe en ninguna memoria. La infinitud no es el objetivo en sí: es la prueba definitiva de que **lo que declaras es una intención y solo se ejecuta lo que se consume**.

¿Para qué sirve en la práctica? Para separar *qué* secuencia quieres de *cuántos* elementos necesitas: numerar elementos sin bucle de índice, generar candidatos hasta encontrar N válidos, paginar resultados...

```cpp
// Numerar líneas sin contador manual: zip de un infinito con un finito (C++23)
for (auto [num, linea] : std::views::zip(std::views::iota(1), lineas))
    std::cout << num << ": " << linea << '\n';
```


## Consecuencia 1: una view se puede DEVOLVER desde una función

Como la view es solo la receta (no los datos), devolverla es baratísimo y la ejecución la decide quien la consuma:

```cpp
#include <iostream>
#include <vector>
#include <ranges>

// Devuelve la INTENCIÓN "pares al cuadrado". No procesa nada.
auto pares_al_cuadrado(const std::vector<int>& v) {
    return v | std::views::filter([](int n) { return n % 2 == 0; })
             | std::views::transform([](int n) { return n * n; });
}

int main() {
    std::vector<int> nums{1, 2, 3, 4, 5, 6};

    auto receta = pares_al_cuadrado(nums);  // todavía no ha pasado nada

    for (int n : receta)                    // AQUÍ se ejecuta
        std::cout << n << ' ';              // 4 16 36
}
```

> ⚠️ Cuidado: la view **referencia** al contenedor original, no lo copia. El vector debe seguir vivo mientras se use la view. Nunca devolváis una view sobre un vector local de la función.

## Consecuencia 2: una view se puede PASAR como parámetro

Una función puede recibir "cualquier cosa iterable", sea un vector, una view filtrada, o una pipeline entera. Aquí conectan ranges con **concepts**:

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <ranges>

// Acepta cualquier rango cuyos elementos sean enteros
void imprimir(std::ranges::input_range auto&& rango) {
    for (auto n : rango)
        std::cout << n << ' ';
    std::cout << '\n';
}

int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6};
    std::list<int>   l{10, 20, 30};

    imprimir(v);                                                  // un vector
    imprimir(l);                                                  // una lista
    imprimir(v | std::views::filter([](int n){ return n > 3; })); // ¡una view!
    imprimir(std::views::iota(1) | std::views::take(4));          // pipeline entera
}
```

La pipeline se le pasa a `imprimir` **sin haberse ejecutado**; se materializa dentro, en el `for`.

## Materializar: cuando sí quieres el vector

Si al final necesitas los datos en un contenedor:

```cpp
// C++23: directo
auto vec = receta | std::ranges::to<std::vector>();

// C++20: con el constructor de rango de iteradores
std::vector<int> vec(receta.begin(), receta.end());
```

## Bonus: algoritmos sobre rangos completos

```cpp
std::ranges::sort(nums);                  // antes: std::sort(nums.begin(), nums.end())
auto it = std::ranges::find(nums, 4);

// Con proyecciones: ordenar personas por edad sin lambda de comparación
std::ranges::sort(personas, {}, &Persona::edad);
```

## Ventajas (resumen)

1. **Composición legible**: pipelines con `|` que se leen de arriba abajo.
2. **Lazy**: lo declarado es una intención; se ejecuta solo al iterar, elemento a elemento. Cero contenedores intermedios.
3. **Secuencias infinitas** posibles: como nadie produce nada hasta que el consumidor lo pide, `iota(1)` puede ser infinito y `take(5)` decide cuánto se materializa (ver sección dedicada).
4. **Views como valores**: se pasan y se devuelven entre funciones, separando la *definición* de la transformación de su *ejecución*.
5. **Adiós a los pares begin/end** en el 95% de los casos.
