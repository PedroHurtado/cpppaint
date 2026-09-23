# C++20: Modules

## Compilación

El soporte de modules varía por compilador. Directivas necesarias:

```bash
# GCC (11+): hay que activar modules y compilar en orden (primero el módulo)
g++ -std=c++20 -fmodules-ts -c matematicas.cppm
g++ -std=c++20 -fmodules-ts main.cpp matematicas.o -o app

# Clang (16+)
clang++ -std=c++20 --precompile matematicas.cppm -o matematicas.pcm
clang++ -std=c++20 -fmodule-file=matematicas=matematicas.pcm main.cpp matematicas.pcm -o app

# MSVC
cl /std:c++20 /c matematicas.ixx
cl /std:c++20 main.cpp matematicas.obj
```

> Nota: la extensión del fichero de módulo cambia según compilador: `.cppm` (GCC/Clang), `.ixx` (MSVC).

## El problema (antes de C++20)

`#include` es **copiar y pegar texto**. Consecuencias:

```cpp
// matematicas.h
#ifndef MATEMATICAS_H        // include guards a mano
#define MATEMATICAS_H

#define PI 3.14159           // esta macro se filtra a TODO el que incluya el header

inline int cuadrado(int x) { // inline para no violar ODR
    return x * x;
}

#endif
```

- Cada `.cpp` que incluye `<iostream>` recompila ~50.000 líneas. En proyectos grandes, minutos de compilación perdidos.
- Las macros y los detalles internos del header **se filtran** al consumidor.
- Hay que mantener guards, separar declaración/definición, cuidar el ODR.

## La solución (C++20)

Un módulo se compila **una vez** y exporta **solo lo que tú decides**:

```cpp
// matematicas.cppm
export module matematicas;     // declaro el módulo

const double PI = 3.14159;    // NO exportado: invisible fuera

export int cuadrado(int x) {   // exportado: visible fuera
    return x * x;
}

export double area_circulo(double radio) {
    return PI * radio * radio; // uso interno de PI, sin problema
}
```

```cpp
// main.cpp
import matematicas;            // import, no #include
#include <iostream>

int main() {
    std::cout << cuadrado(5) << '\n';        // 25
    std::cout << area_circulo(2.0) << '\n';  // 12.566
    // std::cout << PI;  // error: PI no está exportado
}
```

## La analogía correcta: Python y JavaScript

Si venís de otros lenguajes, el modelo mental es exactamente ese:

| C++20 | Python | JavaScript (ES Modules) |
|---|---|---|
| `export module matematicas;` | el fichero `matematicas.py` | el fichero `matematicas.js` |
| `export int cuadrado(...)` | todo lo público del módulo | `export function cuadrado(...)` |
| sin `export` → privado | convención `_privado` | no exportado → privado |
| `import matematicas;` | `import matematicas` | `import { ... } from './matematicas.js'` |

Igual que en Python/JS: el módulo es una **unidad cerrada** que decide qué expone, se carga una vez, y el consumidor no ve sus tripas. La diferencia es que en C++ la frontera la marca `export` explícitamente, no el fichero entero.

## Variantes de `export`

```cpp
export module matematicas;

// 1. Exportar declaración a declaración
export int cuadrado(int x) { return x * x; }
export const double E = 2.71828;

// 2. Bloque de export: todo lo de dentro queda exportado
export {
    int cubo(int x) { return x * x * x; }
    double raiz(double x);
    struct Punto { double x, y; };
}

// 3. Exportar un namespace completo
export namespace geometria {
    double area_circulo(double r);
    double perimetro_circulo(double r);
}

// 4. Re-exportar otro módulo (export import):
// quien importe matematicas recibe TAMBIÉN lo de matematicas.basicas
export import matematicas.basicas;
```

La variante 4 es la que permite construir un módulo "paraguas": un `fudie` que hace `export import` de `fudie.dominio`, `fudie.validacion`, etc., igual que un `__init__.py` que re-exporta submódulos o un `index.js` con `export * from './...'`.

## Variantes de `import`

```cpp
// 1. Importar un módulo con nombre
import matematicas;

// 2. Importar una partición (solo DENTRO del propio módulo, ver abajo)
import :operaciones;

// 3. Header units: importar un header clásico como si fuera un módulo.
//    Puente de migración: gana velocidad de compilación sin reescribir nada,
//    pero a diferencia de un módulo real, SÍ expone sus macros.
import <iostream>;
import "milibreria_legacy.h";

// 4. C++23: toda la librería estándar
import std;
```

> Ojo a la diferencia conceptual: `import matematicas;` importa un **nombre de módulo** (el compilador lo resuelve a su forma binaria compilada); `#include` pegaba un **fichero de texto**. El nombre del módulo no tiene por qué coincidir con el nombre del fichero.

## Ya no tiene sentido separar .h y .cpp

¿Por qué existía la pareja `.h`/`.cpp`? Porque `#include` copia texto: si pones la implementación en el header y lo incluyen 20 ficheros, tienes 20 copias de la función (violación del ODR) o tienes que marcar todo `inline`. La separación declaración/definición era una **servidumbre del preprocesador**, no un principio de diseño.

Con módulos esa servidumbre desaparece: la implementación puede vivir en el propio módulo (como en los ejemplos de arriba) porque el módulo se compila **una vez** y los demás importan el resultado, no el texto. Un fichero, como en Python o JS.

¿Y si un módulo se hace grande? El estándar ofrece dos mecanismos de organización, ahora **opcionales y por diseño**, no por obligación:

```cpp
// (a) Unidad de implementación: separar interfaz e implementación
// matematicas.cppm — interfaz
export module matematicas;
export double raiz(double x);

// matematicas.cpp — implementación ("module" sin "export")
module matematicas;
double raiz(double x) { /* ... */ }   // sin repetir 'export'
```

```cpp
// (b) Particiones: trocear un módulo grande en ficheros
// matematicas-operaciones.cppm
export module matematicas:operaciones;   // partición
export int cuadrado(int x) { return x * x; }

// matematicas.cppm — interfaz principal
export module matematicas;
export import :operaciones;              // ensambla la partición
```

La diferencia con el mundo `.h`/`.cpp`: ya no hay **duplicación** (no mantienes la firma en dos sitios sincronizados a mano) y la división responde a cómo quieres organizar el código, no a cómo funciona el preprocesador.

## Ventajas

1. **Compilación más rápida**: el módulo se compila una vez a formato binario; importarlo es casi gratis. Con `#include` se reparsea el texto en cada unidad de traducción.
2. **Encapsulación real**: solo sale lo que lleva `export`. Las macros no se filtran nunca.
3. **Sin include guards** ni `#pragma once`: importar dos veces no hace nada.
4. **El orden de import no importa** (con `#include` el orden podía romper la compilación).

## El futuro inmediato: `import std;`

En C++23 (ya soportado por MSVC y GCC 15 / Clang 18 con libc++):

```cpp
import std;   // TODA la librería estándar de golpe, y compila más rápido
              // que un solo #include <iostream>

int main() {
    std::println("Hola modules");
}
```
