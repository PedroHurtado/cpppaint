# Paint de consola — patrones de diseño en C++

Ejercicio del curso: un "Paint" **sin interfaz gráfica** que aplica, sobre el
`Shape` que venimos construyendo desde el día 2, **solo los patrones
estrictamente necesarios** para que el ejemplo sea didáctico:

| Patrón        | Día | Dónde se ve en el código                                  |
|---------------|-----|-----------------------------------------------------------|
| **Singleton** | 3   | [`Canvas`](include/paint/Canvas.h) — un único lienzo      |
| **Prototype** | 3   | [`IShape::Clone`](include/paint/IShape.h) — clonar figuras |
| **Factory**   | 3   | [`ShapeFactory`](include/paint/ShapeFactory.h) — con registro |
| **Command**   | 4   | [`ICommand`](include/paint/ICommand.h) + comandos + [`CommandManager`](include/paint/CommandManager.h) |
| **Strategy**  | 4   | [`CommandRegistry`](include/paint/CommandRegistry.h) — un verbo del REPL = una acción registrada |

No están todos los patrones del curso a propósito: el objetivo es ver estos
encajando limpiamente, no acumular código.

Y los principios SOLID que sostienen el diseño:

| Principio | Dónde                                                              |
|-----------|--------------------------------------------------------------------|
| **DIP**   | `ICanvas`, `IReader`, `IWriter`, `IShape`: nadie depende de concreciones |
| **OCP**   | Registrar figuras y verbos, en vez de tocar `ShapeFactory` o `App::Run` |
| **ISP**   | Interfaces mínimas: `IReader` es "dame una línea", no un `std::istream` entero |
| **SRP**   | La figura describe, el `IWriter` imprime, el comando deshace, el manager historia |

## Estructura del proyecto

```
paint/
├── CMakeLists.txt          # build principal: biblioteca + ejecutable + tests
├── include/paint/          # cabeceras (.h): la interfaz pública
│   ├── Point.h             #   tipo de valor (posición)
│   ├── IReader.h           #   puerto de entrada  (DIP)
│   ├── IWriter.h           #   puerto de salida   (DIP)
│   ├── ConsoleReader.h / ConsoleWriter.h
│   ├── IShape.h            #   abstracción de figura (+ Clone = Prototype)
│   ├── Circle.h / Square.h
│   ├── ShapeFactory.h      #   Factory con registro
│   ├── ShapeRegistration.h #   registro de figuras de serie (OCP)
│   ├── ICanvas.h           #   abstracción del lienzo (DIP)
│   ├── Canvas.h            #   Singleton, implementa ICanvas
│   ├── ICommand.h          #   Command
│   ├── AddShapeCommand.h
│   ├── MoveShapeCommand.h
│   ├── DuplicateShapeCommand.h
│   ├── CommandManager.h    #   historial undo/redo
│   ├── CommandRegistry.h   #   verbo -> acción (Strategy + registro) + AppContext
│   ├── CommandRegistration.h #  registro de verbos de serie (OCP)
│   └── App.h               #   bucle REPL
├── src/                    # implementaciones (.cpp)
│   ├── ...                 #   un .cpp por cada cabecera con lógica
│   └── main.cpp            #   composition root
└── tests/                  # pruebas con <cassert> + CTest
    ├── CMakeLists.txt
    └── test_paint.cpp      #   incluye los dobles: FakeCanvas, StringReader...
```

**Por qué `.h` + `.cpp` separados:** la cabecera declara *qué* ofrece cada
clase; el `.cpp` dice *cómo*. Quien usa la clase solo necesita leer el `.h`.
Además, así CMake compila cada `.cpp` por separado y solo recompila lo que
cambia.

**Por qué biblioteca (`paint_core`) + ejecutable (`paint`):** toda la lógica
vive en una biblioteca; `main.cpp` solo la arranca. Los **tests** enlazan con
esa misma biblioteca sin tocar `main`.

## Compilar y ejecutar

Requisitos: CMake ≥ 3.16 y un compilador con C++17 (MSVC, g++ o clang).

> **⚠️ Importante: ejecuta todos los comandos desde dentro de la carpeta
> `paint/`** (la que contiene este `README.md` y el `CMakeLists.txt` raíz).
> El `.` y las rutas relativas como `build` se interpretan respecto a ahí. Si los
> lanzas desde la carpeta padre verás un error tipo
> `Failed to change working directory to ".../build": No such file or directory`,
> porque el `build/` se crea en `paint/build`, no en la raíz del repositorio.
>
> ```bash
> cd paint     # asegúrate de estar aquí antes de nada
> ```
>
> Si prefieres no entrar en la carpeta, prefija las rutas: `cmake -S paint -B paint/build`
> y `ctest --test-dir paint/build`.

```bash
cmake -S . -B build
cmake --build build
```

Ejecutar el programa (lee comandos de la entrada estándar):

```bash
./build/paint            # Linux/macOS
build\Debug\paint.exe    # Windows / MSVC
```

Pasar tests:

```bash
ctest --test-dir build --output-on-failure
```

## Comandos del programa

La entrada se teclea por `cin`, una acción por línea. `help` los lista, y esa
lista **se genera del registro**, no está escrita a mano:

```
circle <radio> <color> <x> <y>   añade un círculo
square <lado> <color> <x> <y>    añade un cuadrado
move <indice> <x> <y>            mueve la figura del índice a (x, y)
duplicate <indice>               duplica una figura (usa Prototype)
undo                             deshace la última acción
redo                             rehace la última acción deshecha
print                            lista el lienzo con índices
help                             muestra la ayuda
exit                             termina
```

### Ejemplo de sesión

```
circle 5 2 0 0
square 3 1 10 10
print
move 0 4 4
undo
duplicate 1
```

## Decisiones de diseño (las "buenas prácticas")

- **El cliente nunca hace `new`.** Las figuras nacen en `ShapeFactory`
  (Factory) o se clonan (Prototype). El resto del código depende de `IShape`,
  no de `Circle`/`Square` (DIP).
- **La GUI/REPL no toca el `Canvas` directamente.** Crea `ICommand`s y los
  entrega al `CommandManager`. Por eso `undo`/`redo` funcionan sin strings ni
  casos especiales: cada comando sabe deshacerse a sí mismo.
- **Factory con registro, instanciable.** Añadir una figura nueva es una línea
  en [`ShapeRegistration.cpp`](src/ShapeRegistration.cpp); no se toca
  `ShapeFactory` (OCP). La hacemos **inyectable** (la posee `App`) en lugar de
  100 % estática, que sería un Singleton oculto y difícil de testear.
- **Los verbos del REPL también se registran.** `App::Run` tenía un `if/else`
  que crecía con cada comando nuevo: OCP roto en el mismo proyecto que enseña
  a no romperlo. Ahora un verbo es una entrada en
  [`CommandRegistry`](include/paint/CommandRegistry.h) y `App::Run` tiene
  **tres ramas fijas**: verbo registrado, figura conocida, o desconocido.
- **La ayuda no tiene texto escrito a mano.** `help` recorre el registro y la
  fábrica. Registrar un `triangle` con parámetros distintos a los del resto lo
  hace aparecer en la ayuda con *su* sintaxis, sin tocar nada.
- **Entrada y salida son puertos simétricos.** `IReader` / `IWriter`. Ojo al
  matiz: `std::istream` *ya* era una abstracción, así que pasarlo no violaba
  DIP en sentido estricto. Lo que violaba era **ISP** (el REPL solo necesita
  "dame una línea" y recibía formateo, locales y `seekg`) y, en la práctica,
  no se puede sustituir con un doble: lo que se sustituye en un `istream` es
  su `streambuf`. Con `IReader`, el REPL entero se prueba con un
  `StringReader` de diez líneas.
- **Singleton con honestidad.** `Canvas` mantiene las cuatro defensas, pero
  implementa `ICanvas` y `Instance()` devuelve `ICanvas&`. Los comandos no
  conocen el tipo concreto ni llaman a `Instance()` por dentro, así que se
  prueban contra un `FakeCanvas`. Dicho todo: **en cuanto existe `ICanvas`, el
  Singleton no aporta nada** — el composition root podría crear el único
  lienzo y repartir la referencia, que es justo lo que ya hace `main()`. Está
  aquí porque el patrón hay que enseñarlo, no porque haga falta.
- **La figura no conoce la salida.** `IShape::ToString()` devuelve texto;
  *quién* lo imprime es el `IWriter` (consola, fichero, test). La figura no
  depende del puerto de salida.
- **Propiedad explícita con `unique_ptr` y `move`.** Quién es dueño de cada
  figura está claro en cada momento: el lienzo, o el comando que la tiene
  "guardada" mientras está deshecha.

## Dos trampas que este código documenta

Las dos se descubrieron compilando, no razonando, y las dos son buen material
de clase.

### 1. Una interfaz puede desarmarte un Singleton sin un solo warning

Si [`ICanvas`](include/paint/ICanvas.h) tuviera el destructor `public virtual`
—lo habitual en una interfaz— esto compilaría **limpio con `-Wall -Wextra`**:

```cpp
paint::ICanvas& c = paint::Canvas::Instance();
delete &c;                    // destruye el Singleton; crash
```

El `~Canvas()` privado no protege de nada ahí: el acceso al destructor se
comprueba sobre el **tipo estático** (`ICanvas`, donde es público) y la llamada
es virtual, así que en tiempo de ejecución ya no hay control de acceso. El
objeto es estático, de modo que se destruye una vez con el `delete`, se
libera memoria que no vino del heap, y el runtime lo destruiría **otra vez** al
salir de `main`.

Heredar `protected` **no lo arregla**: restringe la conversión `Canvas*` →
`ICanvas*` fuera de la clase, pero `Instance()` es miembro de `Canvas`, hace la
conversión dentro y te entrega la referencia ya convertida.

La solución es el destructor **`protected` y no virtual** en `ICanvas`
(la regla de Sutter: *public virtual, o protected non-virtual*). Entonces
`delete &c` es error de compilación. El precio: nadie puede poseer un `ICanvas`
por `unique_ptr`. Aquí es deliberado — el lienzo siempre se maneja por
referencia.

### 2. Unos tests que pasaban sin comprobar nada

`CMAKE_BUILD_TYPE` por defecto es `Release`, que define `NDEBUG`, y con
`NDEBUG` **`<cassert>` convierte todos los `assert` en no-ops**. Los tests
compilaban, enlazaban, devolvían 0... y no verificaban nada: hasta un
`assert(false)` pasaba.

Está arreglado en [`tests/CMakeLists.txt`](tests/CMakeLists.txt) con
`-UNDEBUG` (`/UNDEBUG` en MSVC) solo para el target de tests. Truco para
comprobarlo en cualquier proyecto: mete un `assert(false)` y asegúrate de que
falla. Si pasa, tu suite es decorativa.

## Relación con la teoría

- Singleton → `doc/day-03/01_singleton.md`
- Prototype → `doc/day-03/02_prototype.md`
- Factory   → `doc/day-03/03_factory.md`
- Command   → `doc/day-04/06_command.md`
