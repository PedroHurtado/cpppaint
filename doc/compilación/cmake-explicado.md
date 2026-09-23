# Los dos `CMakeLists.txt` del proyecto Paint

> Guía para alumnos que vienen de **Java** (Maven/Gradle). Explica cómo está
> montado el sistema de compilación del proyecto y **por qué** cada línea está
> donde está.

---

## La idea de fondo

En Java tienes un `pom.xml` (Maven) o un `build.gradle` (Gradle): un archivo que
describe **cómo se construye el proyecto**.

En C++ no existe un estándar oficial de build, y **CMake** es el equivalente de
facto. Hay un detalle importante:

> **CMake no compila.** CMake *genera* los ficheros del compilador real
> (Makefiles, proyectos de Visual Studio, Ninja…) y luego esa herramienta nativa
> compila. Es como un Gradle que, en vez de compilar directamente, te prepara la
> configuración para la herramienta de tu sistema.

Hay **dos** archivos `CMakeLists.txt` porque seguimos el mismo patrón que en
Java: se separa el módulo de la **aplicación** del módulo de **tests**.

```
paint/
├── CMakeLists.txt          ← la aplicación
├── include/                ← cabeceras (.h)
├── src/                    ← implementación (.cpp) + main.cpp
└── tests/
    ├── CMakeLists.txt      ← los tests
    └── test_paint.cpp
```

---

## 1) `paint/CMakeLists.txt` — la aplicación

### Cabecera del proyecto

```cmake
cmake_minimum_required(VERSION 3.16)
project(paint
    VERSION 1.0.0
    DESCRIPTION "Paint de consola: Singleton, Prototype, Factory y Command"
    LANGUAGES CXX)
```

- `cmake_minimum_required`: versión mínima de CMake. Como exigir un Maven/JDK
  mínimo.
- `project(...)`: nombre, versión y lenguaje (`CXX` = C++). Equivale al bloque de
  coordenadas (`groupId` / `artifactId` / `version`) del `pom.xml`.

### Estándar del lenguaje

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++17, no -std=gnu++17
```

Es como fijar el `source`/`target` de Java (p. ej. Java 17). Dice "usa C++17".

- `REQUIRED ON`: si el compilador no soporta C++17, **falla** (no degrada en
  silencio).
- `EXTENSIONS OFF`: usa C++17 **estándar puro** (`-std=c++17`), no la variante con
  extensiones de GNU (`-std=gnu++17`). Buscamos portabilidad, no atarnos a un
  compilador concreto.

### Tipo de build por defecto

```cmake
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
endif()
```

C++ distingue entre build de **Debug** (con símbolos, sin optimizar) y **Release**
(optimizado). Es parecido a los perfiles de Maven, pero aquí es algo nativo del
compilador. Esta línea dice: "si nadie eligió, usa Release".

La condición doble es porque algunos generadores (Visual Studio) eligen el tipo
**al compilar** y no al configurar; en esos casos no se toca.

### El punto clave: librería vs ejecutable

```cmake
add_library(paint_core
    src/ConsoleWriter.cpp
    src/Circle.cpp
    src/Square.cpp
    src/ShapeFactory.cpp
    src/ShapeRegistration.cpp
    src/Canvas.cpp
    src/AddShapeCommand.cpp
    src/MoveShapeCommand.cpp
    src/DuplicateShapeCommand.cpp
    src/CommandManager.cpp
    src/App.cpp)
```

**Esta es la decisión de diseño más importante**, y la razón de que los dos
archivos queden tan limpios:

> Toda la lógica se mete en una librería (`paint_core`), y el `main` se deja
> **fuera**.

En Java esto te sale sin pensarlo: compilas todas tus clases a un conjunto de
`.class`, y tanto el `main` como los tests de JUnit las usan. En C++ hay que
decirlo explícitamente.

Si metieras `main.cpp` dentro de la librería, los tests **también** tendrían un
`main`, y chocaría con el `main` propio del test → **error de enlazado** (dos
`main`). Por eso:

- `paint_core` = toda la lógica (las clases de los patrones).
- El `main` real vive aparte (más abajo).

Así **app y tests comparten exactamente el mismo código compilado**, sin
recompilarlo dos veces. Es justo lo que JUnit hace con tus clases de
`src/main/java`.

### Cabeceras públicas

```cmake
target_include_directories(paint_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

En C++ una clase se parte en `.h` (declaración, como una firma/interfaz) y `.cpp`
(implementación). Para que otro módulo "vea" esas clases necesita saber dónde
están los `.h`.

La palabra `PUBLIC` es clave y muy CMake-moderno: significa **"todo el que enlace
con `paint_core` hereda automáticamente esta carpeta de cabeceras"**. Así los
tests no tienen que volver a declarar dónde están los includes: lo heredan.
Conceptualmente es como una dependencia `api` transitiva en Gradle: lo `PUBLIC` /
`api` se propaga a quien te use; lo `PRIVATE` no.

### Features y avisos del compilador

```cmake
target_compile_features(paint_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(paint_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(paint_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

- `compile_features ... PUBLIC cxx_std_17`: reafirma el requisito de C++17 y lo
  propaga a quien use la librería.
- Los `compile_options` son los **warnings estrictos**, como activar lint o
  `-Werror`-style en Java. El `if(MSVC)` distingue compilador de Microsoft
  (`/W4`) de GCC/Clang (`-Wall -Wextra`), porque las banderas tienen nombres
  distintos. Van como `PRIVATE`: son para compilar `paint_core`, **no** se
  imponen a quien la use.

> **Regla mental de visibilidad:**
> `PRIVATE` = "para construirme yo" · `PUBLIC` = "para mí y para quien me use" ·
> `INTERFACE` = "solo para quien me use".

### El detalle de MinGW (Windows)

```cmake
if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()
```

Específico de entornos Windows con **MinGW**. El `.exe`, al arrancar, busca la DLL
del runtime de C++ (`libstdc++-6.dll`). Si encuentra en el `PATH` una versión
ajena (por ejemplo la que trae **Git**), el programa puede **colgarse**.

Enlazando el runtime de forma **estática**, ese código se mete dentro del propio
`.exe` y queda autónomo, sin depender de DLLs externas. En MSVC o Linux no aplica,
por eso va dentro del `if`.

### El ejecutable

```cmake
add_executable(paint src/main.cpp)
target_link_libraries(paint PRIVATE paint_core)
```

Ahora sí: el ejecutable `paint` es **solo** `main.cpp` enlazado contra la librería
con toda la lógica. `PRIVATE` aquí significa que esa dependencia es un detalle
interno del ejecutable (nadie enlaza "contra" un ejecutable, así que no necesita
propagar nada).

### Activar tests e incluir la subcarpeta

```cmake
enable_testing()
add_subdirectory(tests)
```

- `enable_testing()`: habilita **CTest**, el corredor de tests de CMake
  (equivalente a `mvn test` / Surefire).
- `add_subdirectory(tests)`: entra a procesar el otro `CMakeLists.txt`. Como un
  módulo hijo en un proyecto Maven multi-módulo.

---

## 2) `paint/tests/CMakeLists.txt` — los tests

```cmake
# Tests sin framework externo: solo <cassert>. Suficiente para lo didáctico y
# se integra con CTest (ejecútalos con `ctest` tras compilar).
add_executable(paint_tests test_paint.cpp)
target_link_libraries(paint_tests PRIVATE paint_core)

add_test(NAME paint_tests COMMAND paint_tests)
```

Es deliberadamente corto. Tres líneas:

1. **`add_executable(paint_tests test_paint.cpp)`** — los tests son **otro
   ejecutable independiente**. En C++ los tests no son "métodos anotados" que un
   runner descubre por reflexión como en JUnit; son un programa que se compila y
   se ejecuta. Aquí no usan ningún framework externo (ni Google Test ni Catch2):
   solo `<cassert>`, la macro `assert()` de la librería estándar. Para algo
   didáctico es suficiente y evita añadir dependencias.

2. **`target_link_libraries(paint_tests PRIVATE paint_core)`** — aquí se **cierra
   el círculo**. Los tests enlazan contra la **misma** `paint_core` que usa la
   app. Y como la carpeta `include` era `PUBLIC`, los tests ven las cabeceras
   automáticamente sin más configuración. Esta es la recompensa de haber separado
   lógica y `main` arriba.

3. **`add_test(NAME paint_tests COMMAND paint_tests)`** — **registra** ese
   ejecutable en CTest. A partir de aquí, tras compilar, puedes lanzar `ctest` y
   CTest ejecutará el binario; si termina con código 0 → test pasa, si no →
   falla. Es el descubrimiento de tests que JUnit hace por ti, pero declarado a
   mano.

---

## Resumen para el alumno Java

| Concepto Java / Maven | Equivalente aquí |
|---|---|
| `pom.xml` | `CMakeLists.txt` |
| `source` / `target = 17` | `CMAKE_CXX_STANDARD 17` |
| Compilar `src/main/java` a `.class` | `add_library(paint_core ...)` |
| Clase `Main` con `main()` | `add_executable(paint src/main.cpp)` |
| JUnit usa tus clases compiladas | tests enlazan `paint_core` |
| `mvn test` / Surefire | `enable_testing()` + `ctest` |
| Módulo hijo (multi-módulo) | `add_subdirectory(tests)` |
| Dependencia transitiva (`api`) | `PUBLIC` |
| Dependencia interna (`implementation`) | `PRIVATE` |

> **La gran lección de diseño:** se separa la lógica (`paint_core`) del punto de
> entrada (`main.cpp`) para que la app y los tests compartan exactamente el mismo
> código sin recompilarlo ni duplicar `main`. Todo lo demás (estándar, warnings,
> enlace estático en MinGW) son ajustes de calidad y portabilidad alrededor de
> esa decisión.

---

## Cómo compilar y ejecutar (referencia rápida)

```bash
# Configurar (genera el sistema de build en la carpeta build/)
cmake -S . -B build

# Compilar
cmake --build build

# Ejecutar la aplicación
./build/paint            # en Windows: build\paint.exe

# Ejecutar los tests
ctest --test-dir build
```
