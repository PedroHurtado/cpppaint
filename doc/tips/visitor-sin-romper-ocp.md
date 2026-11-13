# Visitor sin romper OCP y sin fallos silenciosos

Con Visitor clásico, añadir una figura obliga a tocar **todos** los
visitantes. Si para evitarlo metes `dynamic_cast` o un caso "por
defecto", el olvido deja de ser un error de compilación y pasa a ser un
bug **silencioso** en runtime.

Este documento responde a una sola pregunta:

> **¿Se puede extender sin romper OCP y sin que nada falle en silencio?**

Respuesta corta: **en un eje sí, en los dos a la vez no.** Con
templates eliges **qué** eje queda abierto, y el compilador vigila el
otro.

---

## 1. La tabla que no puedes evitar

Piensa en una hoja de cálculo. Filas = figuras. Columnas = operaciones.
**Cada celda es código que alguien tiene que escribir.**

```
                   area   nombre   svg (mañana)
  Circulo           ■       ■        □
  Cuadrado          ■       ■        □
  Triangulo         ■       ■        □
  Rombo (mañana)    □       □        □
```

- Mañana añades una **columna** (SVG) → hay que rellenar una celda por figura.
- Mañana añades una **fila** (Rombo) → hay que rellenar una celda por operación.

**Ninguna técnica hace desaparecer celdas.** 100 figuras × SVG = 100
celdas, uses Visitor, `variant`, herencia o lo que sea. Esto tiene
nombre: el **Expression Problem** (Wadler, 1998).

Lo que la técnica decide es **cómo agrupas las celdas**:

| Agrupas por... | Añadir lo de tu grupo | Añadir lo otro |
|---|---|---|
| **columnas** (cada operación junta todas sus figuras) | fichero nuevo → OCP ✔ | tocar cada columna ✘ |
| **filas** (cada figura junta todas sus operaciones) | fichero nuevo → OCP ✔ | tocar cada fila ✘ |

Visitor agrupa por columnas. La herencia clásica agrupa por filas.
No hay una tercera forma de agrupar una tabla.

## 2. Las tres preguntas que sí puedes responder

Como las celdas son inevitables, lo que importa es:

1. **¿Toco ficheros existentes o solo creo nuevos?** (OCP)
2. **Si me dejo una celda vacía, ¿quién me avisa?** El compilador ✔ / nadie ✘
3. **¿Puedo rellenar muchas celdas de golpe?** (código genérico)

Las dos opciones de este documento responden **"el compilador"** a la
pregunta 2. Esa es la condición mínima: **nada en silencio**.

## 3. El escenario (el mismo en todo el documento)

Figuras como structs normales: **sin herencia, sin virtuales, sin vtable.**
Los polígonos saben dar sus vértices (cualquier Paint ya lo necesita
para dibujarlos).

```cpp
// figuras.hpp
#pragma once
#include <utility>
#include <vector>

using Punto = std::pair<double, double>;

struct Circulo {
    double radio;
};

struct Cuadrado {
    double lado;
    std::vector<Punto> vertices() const {
        return {{0, 0}, {lado, 0}, {lado, lado}, {0, lado}};
    }
};

struct Triangulo {
    double base, altura;
    std::vector<Punto> vertices() const {
        return {{0, 0}, {base, 0}, {base / 2, altura}};
    }
};
```

Y la figura que llegará "mañana", en su propio fichero:

```cpp
// rombo.hpp
#pragma once
#include "figuras.hpp"

struct Rombo {
    double ancho, alto;
    std::vector<Punto> vertices() const {
        return {{ancho / 2, 0}, {ancho, alto / 2}, {ancho / 2, alto}, {0, alto / 2}};
    }
};
```

---

## 4. Opción A: `std::variant` → operaciones abiertas

**La idea:** agrupar por columnas, como Visitor, pero sin `aceptar`,
sin clase base y sin herencia múltiple. Cada operación es una función.

### Código

```cpp
// a_variant.cpp
#include "figuras.hpp"
#include <iostream>
#include <string>
#include <variant>

// Helper de 2 líneas (C++17). Junta varias lambdas en un solo objeto.
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// El ÚNICO sitio donde se enumeran las figuras.
using Figura = std::variant<Circulo, Cuadrado, Triangulo>;

// Operación 1
double area(const Figura& f) {
    return std::visit(overloaded{
        [](const Circulo& c)   { return 3.14159 * c.radio * c.radio; },
        [](const Cuadrado& q)  { return q.lado * q.lado; },
        [](const Triangulo& t) { return t.base * t.altura / 2; },
    }, f);
}

// Operación 2
std::string nombre(const Figura& f) {
    return std::visit(overloaded{
        [](const Circulo&)   { return std::string("circulo"); },
        [](const Cuadrado&)  { return std::string("cuadrado"); },
        [](const Triangulo&) { return std::string("triangulo"); },
    }, f);
}

int main() {
    std::vector<Figura> lienzo{Circulo{2}, Cuadrado{3}, Triangulo{4, 5}};
    for (const auto& f : lienzo)
        std::cout << nombre(f) << " -> area " << area(f) << '\n';
}
```

### Salida

```
circulo -> area 12.5664
cuadrado -> area 9
triangulo -> area 10
```

### Añadir una operación (SVG)

Una función nueva `svg(const Figura&)`, en un fichero nuevo.
**No se toca nada existente. OCP ✔**

### Añadir una figura (Rombo)

Una línea en el único punto de registro:

```cpp
using Figura = std::variant<Circulo, Cuadrado, Triangulo, Rombo>;   // ← + Rombo
```

…y **el compilador te da la lista de lo que falta** (salida real, recortada):

```
a_olvido.cpp:15:22:   required from here     ← area()   no sabe de Rombo
a_olvido.cpp:24:22:   required from here     ← nombre() no sabe de Rombo
error: no type named 'type' in 'struct std::invoke_result<overloaded<...>, const Rombo&>'
```

No es OCP (has tocado una línea), pero **no puede fallar en silencio**:
no compila hasta que cada operación sepa qué hacer con un `Rombo`.

### Lo reconoces porque…

- Hay un `using X = std::variant<...>` con la lista de tipos.
- Las operaciones son funciones libres con `std::visit(overloaded{...})`.
- Los tipos no heredan de nada.

---

## 5. Opción B: type erasure → figuras abiertas

**La idea:** agrupar por filas. Cada figura trae sus operaciones como
funciones libres. Un único envoltorio `Shape` sabe llamarlas.

### Código

Las operaciones, como funciones libres:

```cpp
// operaciones.hpp
#pragma once
#include "figuras.hpp"
#include <string>

inline double area_de(const Circulo& c)   { return 3.14159 * c.radio * c.radio; }
inline double area_de(const Cuadrado& q)  { return q.lado * q.lado; }
inline double area_de(const Triangulo& t) { return t.base * t.altura / 2; }

inline std::string nombre_de(const Circulo&)   { return "circulo"; }
inline std::string nombre_de(const Cuadrado&)  { return "cuadrado"; }
inline std::string nombre_de(const Triangulo&) { return "triangulo"; }
```

El envoltorio. Se escribe **una vez** y no conoce ninguna figura:

```cpp
// shape.hpp
#pragma once
#include <memory>
#include <string>

// El ÚNICO sitio donde se enumeran las operaciones.
class Shape {
    struct Concepto {                        // la vtable vive aquí, solo aquí
        virtual ~Concepto() = default;
        virtual double      area()   const = 0;
        virtual std::string nombre() const = 0;
    };

    template<class T>
    struct Modelo : Concepto {               // el compilador escribe uno por figura
        T obj;
        explicit Modelo(T o) : obj(std::move(o)) {}
        double      area()   const override { return area_de(obj); }
        std::string nombre() const override { return nombre_de(obj); }
    };

    std::unique_ptr<Concepto> p;

public:
    template<class T>
    Shape(T obj) : p(std::make_unique<Modelo<T>>(std::move(obj))) {}

    double      area()   const { return p->area(); }
    std::string nombre() const { return p->nombre(); }
};
```

Uso:

```cpp
// b_erasure.cpp
#include "operaciones.hpp"
#include "shape.hpp"
#include <iostream>
#include <vector>

int main() {
    std::vector<Shape> lienzo;
    lienzo.emplace_back(Circulo{2});
    lienzo.emplace_back(Cuadrado{3});
    lienzo.emplace_back(Triangulo{4, 5});

    for (const auto& f : lienzo)
        std::cout << f.nombre() << " -> area " << f.area() << '\n';
}
```

### Salida

```
circulo -> area 12.5664
cuadrado -> area 9
triangulo -> area 10
```

### Añadir una figura (Rombo)

`rombo.hpp` + sus funciones libres en un fichero nuevo + su registro en
la factory. **No se toca nada existente. OCP ✔**

¿Y si se te olvida una? Probamos a dejarnos `nombre_de(const Rombo&)`:

```cpp
// rombo_ops.hpp
inline double area_de(const Rombo& r) { return r.ancho * r.alto / 2; }
// ¡se nos olvida nombre_de!
```

```cpp
lienzo.emplace_back(Rombo{4, 2});
```

Salida real del compilador:

```
shape.hpp: In instantiation of 'std::string Shape::Modelo<T>::nombre() const [with T = Rombo]':
shape.hpp:18:63: error: no matching function for call to 'nombre_de(const Rombo&)'
   18 |         std::string nombre() const override { return nombre_de(obj); }
      |                                                      ~~~~~~~~~^~~~~
```

Dice **qué figura** (`T = Rombo`) y **qué operación** (`nombre_de`). No
compila. **Nada en silencio.**

### Añadir una operación (SVG)

Aquí está el lado cerrado: hay que tocar `Shape`. Son **3 líneas**:

```cpp
virtual std::string svg() const = 0;                         // en Concepto
std::string svg() const override { return svg_de(obj); }     // en Modelo
std::string svg() const { return p->svg(); }                 // en la parte pública
```

Y las funciones `svg_de` van **todas en un fichero nuevo** (`svg.hpp`).
**Ningún struct de figura se modifica.** Que "toque escribir 100 SVG"
no es culpa de type erasure: son las 100 celdas de la tabla, que existen
con cualquier técnica. El apartado siguiente explica cómo no escribirlas
una a una.

### Lo reconoces porque…

- Hay una clase con un `struct Concepto` (virtual) y un `template Modelo<T>` dentro.
- Su constructor es `template<class T>` y acepta cualquier tipo.
- Los tipos no heredan de nada. Las operaciones son funciones libres.

---

## 6. El ahorro de verdad: programar contra lo que las figuras comparten

Las celdas no desaparecen, **pero muchas se pueden rellenar con una
sola plantilla**. Casi todas las figuras de un Paint son polígonos, y
todo polígono tiene vértices. Basta escribir el SVG **una vez** para
"cualquier cosa con vértices":

```cpp
// svg.hpp  (fichero NUEVO)
#pragma once
#include "figuras.hpp"
#include <sstream>
#include <string>

// "Cualquier cosa que sepa darme sus vértices"
template<class T>
concept TieneVertices = requires(const T& t) { t.vertices(); };

// UNA plantilla para TODOS los polígonos
template<TieneVertices T>
std::string svg_de(const T& f) {
    std::ostringstream out;
    out << "<polygon points=\"";
    for (auto [x, y] : f.vertices()) out << x << ',' << y << ' ';
    out << "\"/>";
    return out.str();
}

// Solo las figuras especiales llevan versión propia
inline std::string svg_de(const Circulo& c) {
    std::ostringstream out;
    out << "<circle r=\"" << c.radio << "\"/>";
    return out.str();
}
```

Con las 3 líneas del apartado anterior en `Shape`, y `Rombo` añadido al
lienzo (solo con `area_de` y `nombre_de`; **nadie escribió
`svg_de(Rombo)`**):

### Salida

```
circulo -> <circle r="2"/>
cuadrado -> <polygon points="0,0 3,0 3,3 0,3 "/>
triangulo -> <polygon points="0,0 4,0 2,5 "/>
rombo -> <polygon points="2,0 4,1 2,2 0,1 "/>
```

**El Rombo exporta a SVG gratis.** Con 100 figuras, el SVG de mañana
son 1 plantilla + las 3 o 4 figuras curvas.

### ¿Y si llega una figura curva y me olvido de su SVG?

```cpp
struct Elipse { double a, b; };   // curva: sin vertices()
// area_de y nombre_de sí, svg_de no
lienzo.emplace_back(Elipse{3, 2});
```

Salida real:

```
shape.hpp: In instantiation of '... Shape::Modelo<T>::svg() const [with T = Elipse]':
shape.hpp:19:60: error: no matching function for call to 'svg_de(const Elipse&)'
svg.hpp:12:13: note: candidate: 'template<class T> requires TieneVertices<T> std::string svg_de(const T&)'
svg.hpp:12:13: note: constraints not satisfied
```

El compilador explica incluso **por qué** la plantilla no sirve: la
elipse no cumple `TieneVertices`. **Nada en silencio.**

> Esto funciona igual en la opción A: una lambda
> `[](const TieneVertices auto& f) { ... }` dentro del `overloaded`
> cubre todos los polígonos.

---

## 7. Bad: tres formas de volver al fallo silencioso

Las tres "arreglan" el OCP dándole al compilador permiso para callarse.

### 7.1 El comodín `auto` "por si acaso"

```cpp
using Figura = std::variant<Circulo, Cuadrado, Triangulo, Rombo>;

double area(const Figura& f) {
    return std::visit(overloaded{
        [](const Circulo& c)   { return 3.14159 * c.radio * c.radio; },
        [](const Cuadrado& q)  { return q.lado * q.lado; },
        [](const Triangulo& t) { return t.base * t.altura / 2; },
        [](const auto&)        { return 0.0; },   // ← "por si acaso"
    }, f);
}
```

Compila **sin un solo warning** con `-Wall -Wextra`. Salida real:

```
area rombo = 0
```

Un rombo de 4×2 con área 0, y nadie avisa.

### 7.2 Cascada de `dynamic_cast`

```cpp
if (auto* c = dynamic_cast<const Circulo*>(&s))  { ... return; }
if (auto* q = dynamic_cast<const Cuadrado*>(&s)) { ... return; }
// un Rombo no entra en ningún if → se ignora
```

### 7.3 Acyclic Visitor / visitante con implementación por defecto

```cpp
void aceptar(Visitante& v) const override {
    if (auto* vr = dynamic_cast<VisitaRombo*>(&v)) vr->visitar(*this);
    // si el visitante no implementa VisitaRombo → no pasa nada
}
```

**Las tres tienen el mismo síntoma:** hay un camino del código que
dice *"si no sé qué es, no hago nada"*. Ahí vive el bug.

> Si por diseño **necesitas** uno de estos (plugins cargados en
> runtime), al menos convierte el silencio en un test: recorre el
> registry de figuras × todos los visitantes y comprueba que ninguno
> cae en el caso por defecto.

---

## 8. Cómo elegir: una sola pregunta

> **¿Qué va a crecer más: las figuras o las operaciones?**

```
             ¿Qué crece más?
            /               \
     operaciones           figuras
          |                    |
   Opción A: variant     Opción B: type erasure
   añadir figura =       añadir operación =
   1 línea en el using   3 líneas en Shape
          \                   /
           \                 /
     En las dos: el olvido NO compila.
     En las dos: usa concepts para rellenar
     muchas celdas con una plantilla.
```

| | Opción A: `std::variant` | Opción B: type erasure |
|---|---|---|
| Añadir **operación** | función nueva, OCP ✔ | 3 líneas en `Shape` |
| Añadir **figura** | 1 línea en el `using` | ficheros nuevos, OCP ✔ |
| Si olvidas una celda | no compila ✔ | no compila ✔ |
| Herencia en las figuras | ninguna | ninguna |
| Herencia múltiple | no (la de `overloaded` es de lambdas vacías, coste 0) | no |
| Vtable | ninguna | una sola, dentro de `Shape` |
| Figuras de plugins en runtime | ✘ | ✔ (si el plugin se compila contra `Shape`) |

## 9. ¿Y las vtables con muchísimas figuras?

- Hay **una vtable por clase**, no por objeto. Cada objeto lleva un
  solo puntero (8 bytes). 200 figuras = 200 tablas pequeñas en memoria
  estática: irrelevante.
- Lo que sí pesaba en Visitor clásico/Acyclic era la **herencia
  múltiple** (un vptr por interfaz, thunks, `dynamic_cast`; ver
  [herencia-multiple-interfaces-vtables.md](herencia-multiple-interfaces-vtables.md)).
- **Opción A:** cero vtables. Coste: el `variant` mide lo que la figura
  más grande, y compilar 100 tipos en un `variant` es más lento.
- **Opción B:** una sola vtable (la de `Concepto`), escrita por ti en
  un sitio, sin herencia múltiple ni `dynamic_cast`.

---

## Chuleta

- **Las celdas figura × operación siempre existen.** Ninguna técnica las borra.
- **OCP en los dos ejes a la vez es imposible.** Elige cuál.
- **Operaciones crecen** → `std::variant` + `overloaded`.
- **Figuras crecen** → type erasure (`Shape` con `Concepto`/`Modelo`).
- **Muchas celdas iguales** → un concept (`TieneVertices`) y una plantilla.
- **Nunca** `[](const auto&)` de comodín, cascadas de `dynamic_cast` ni "por defecto no hago nada".
- Si una celda vacía **compila**, el diseño está mal.

## Mantra

> *"No puedes evitar escribir las celdas. Sí puedes evitar que una
> celda vacía compile."*

---

## Referencias

- **Philip Wadler — *The Expression Problem* (1998, gratis).**
  <https://homepages.inf.ed.ac.uk/wadler/papers/expression/expression.txt>
  El correo original que le pone nombre a la tabla del apartado 1. Una página; merece leerse para ver que el problema es de los lenguajes, no de tu diseño.
- **Sean Parent — *Inheritance Is The Base Class of Evil* (GoingNative 2013, gratis).**
  <https://www.youtube.com/watch?v=bIhUE5uUFOA>
  La charla que popularizó el `Concepto`/`Modelo` de la opción B. 25 minutos que cambian cómo ves la herencia.
- **Klaus Iglberger — *Breaking Dependencies: Type Erasure - A Design Analysis* (CppCon 2021, gratis).**
  <https://www.youtube.com/watch?v=4eeESJQk-mw>
  Type erasure explicado paso a paso con figuras, exactamente el escenario de este documento, y comparado con Visitor.
- 📖 **Klaus Iglberger — *C++ Software Design* (O'Reilly, 2022).** Ejemplos de código gratis en
  <https://github.com/igl42/cpp_software_design>
  Capítulos dedicados a Visitor con `std::variant` y a type erasure, con sus costes medidos. El libro de patrones en C++ moderno.
- **Bartłomiej Filipek — *Everything You Need to Know About std::variant from C++17* (gratis).**
  <https://www.cppstories.com/2018/06/variant/>
  Referencia práctica de `std::variant`, `std::visit` y el truco `overloaded`.
- **cppreference — `std::visit` y *Constraints and concepts* (gratis).**
  <https://en.cppreference.com/w/cpp/utility/variant/visit> ·
  <https://en.cppreference.com/w/cpp/language/constraints>
  Para consultar la letra pequeña: el ejemplo de `overloaded` de la primera página es el que se usa aquí.

---

*Todo el código de este documento se ha compilado y ejecutado con
MinGW-w64 GCC 14.2, `g++ -std=c++20 -Wall`. Las salidas, incluidos los
errores de compilación, son reales (recortadas a las líneas que
importan).*
