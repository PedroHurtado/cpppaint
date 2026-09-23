# Visitor ⭐ (prometido el día 2)

## 1. El problema

A lo largo del curso, el Paint ha ido sumando operaciones sobre las
figuras:

- `dibujar()`: cómo se pinta.
- `area()`: el cálculo geométrico.
- `clone()`: copia profunda (Prototype).
- `dibujarContorno()`: la versión sin estrategia.

Cada vez que el cliente pide una operación nueva — exportar a JSON,
contar vértices, calcular bounding box, traducir a SVG — **tenemos que
tocar todas las figuras**:

```cpp
class Shape {
public:
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual std::string aJson() const = 0;          // operación nueva
    virtual int numVertices() const = 0;             // otra más
    virtual std::pair<double,double> boundingBox() const = 0;  // y otra
    // ...
};
```

Y reimplementar cada método en `Circulo`, `Cuadrado`, `Triangulo`,
`Poligono`. Eso es OCP roto **al revés**: añadir una **operación
nueva** obliga a tocar todas las clases. (OCP de manual nos protege
de añadir **tipos** nuevos; aquí estamos en el otro eje.)

Además, mezclamos responsabilidades: `aJson()` no es geometría, es
serialización. No tiene por qué vivir en `Shape`.

## 2. Intención (GoF)

> *Representar una operación que se realiza sobre los elementos de una
> estructura de objetos. Visitor permite definir una nueva operación
> sin cambiar las clases de los elementos sobre los que opera.*

La operación es **un objeto externo** (el visitante). La figura solo
sabe **aceptar visitantes** y dejarse procesar.

## 3. Bad: una operación más en la jerarquía

```cpp
class Shape {
public:
    virtual std::string aJson() const = 0;   // ← operación nueva
    // ... y todo lo demás ...
};

class Circulo : public Shape {
    double radio;
public:
    std::string aJson() const override {
        return "{\"tipo\":\"circulo\",\"r\":" + std::to_string(radio) + "}";
    }
};

class Cuadrado : public Shape {
    double lado;
public:
    std::string aJson() const override {
        return "{\"tipo\":\"cuadrado\",\"l\":" + std::to_string(lado) + "}";
    }
};
```

Funciona, pero:

1. Cada figura nueva tiene que recordar implementar `aJson()`.
2. Cada operación nueva nos hace tocar **todas** las figuras.
3. `Shape` empieza a saber de JSON, SVG, bounding boxes, exportación,
   estadísticas... cada vez menos figura, cada vez más Frankenstein.

## 4. Good: Visitor (double dispatch)

Dos jerarquías. Una para las figuras (que ya tenemos), otra para los
visitantes.

```cpp
// Adelantamos las figuras concretas
class Circulo;
class Cuadrado;
class Triangulo;
class Poligono;

class VisitanteShape {
public:
    virtual ~VisitanteShape() = default;
    virtual void visitar(const Circulo&)   = 0;
    virtual void visitar(const Cuadrado&)  = 0;
    virtual void visitar(const Triangulo&) = 0;
    virtual void visitar(const Poligono&)  = 0;
};
```

Las figuras tienen un **único método nuevo**:

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void aceptar(VisitanteShape& v) const = 0;
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
};

class Circulo : public Shape {
    double radio;
public:
    explicit Circulo(double r) : radio(r) {}
    double getRadio() const { return radio; }

    void aceptar(VisitanteShape& v) const override { v.visitar(*this); }
    void dibujar() const override { /* ... */ }
    double area() const override   { return 3.14159 * radio * radio; }
};

class Cuadrado : public Shape {
    double lado;
public:
    explicit Cuadrado(double l) : lado(l) {}
    double getLado() const { return lado; }

    void aceptar(VisitanteShape& v) const override { v.visitar(*this); }
    void dibujar() const override { /* ... */ }
    double area() const override   { return lado * lado; }
};
```

Visitantes concretos: **una clase por operación**.

```cpp
class VisitanteJson : public VisitanteShape {
    std::ostringstream out;
public:
    void visitar(const Circulo& c) override {
        out << "{\"tipo\":\"circulo\",\"r\":" << c.getRadio() << "}";
    }
    void visitar(const Cuadrado& c) override {
        out << "{\"tipo\":\"cuadrado\",\"l\":" << c.getLado() << "}";
    }
    void visitar(const Triangulo&)  override { /* ... */ }
    void visitar(const Poligono&)   override { /* ... */ }

    std::string resultado() const { return out.str(); }
};

class VisitanteAreaTotal : public VisitanteShape {
    double total = 0;
public:
    void visitar(const Circulo& c)   override { total += c.area(); }
    void visitar(const Cuadrado& c)  override { total += c.area(); }
    void visitar(const Triangulo& t) override { total += t.area(); }
    void visitar(const Poligono& p)  override { total += p.area(); }
    double resultado() const { return total; }
};
```

Uso:

```cpp
VisitanteJson json;
for (const auto& f : Lienzo::instancia().figurasReadOnly())
    f->aceptar(json);
std::cout << json.resultado();

VisitanteAreaTotal sumador;
for (const auto& f : Lienzo::instancia().figurasReadOnly())
    f->aceptar(sumador);
std::cout << "Area total = " << sumador.resultado();
```

Añadir un visitante (exportar a SVG, contar vértices, calcular bbox):
**una clase nueva**, cero ediciones en las figuras.

## 5. UML rápido

```
   Shape                          VisitanteShape
     ▲                                  ▲
     │                                  │
Circulo, Cuadrado, ...      VisitanteJson, VisitanteArea, ...

  aceptar(v) ──── v.visitar(*this) ────► (resolución por sobrecarga)
```

Eso de "doble despacho": la primera llamada (`aceptar`) se resuelve
por polimorfismo según el tipo de `Shape`; la segunda (`visitar`) se
resuelve por sobrecarga según el tipo del parámetro (`Circulo`,
`Cuadrado`, ...). Es cómo C++ rodea la falta de despacho doble nativo.

## 6. "¿Y por qué no `visitante.visit(figura)`?"

Pregunta natural mirando el código de uso:

```cpp
for (const auto& f : Lienzo::instancia().figurasReadOnly())
    f->aceptar(json);    // ← raro: el visitado acepta al visitante
```

Estéticamente, uno espera lo contrario: **el visitante visita, no el
visitado acepta**. La sintaxis bonita sería:

```cpp
for (const auto& f : Lienzo::instancia().figurasReadOnly())
    json.visit(*f);      // ← el visitante toma la iniciativa
```

Esta sección explica **por qué no se puede directamente** y **qué
opciones tienes** si esa sintaxis te importa.

### Por qué la inversión no es trivial

Si escribes `json.visit(*f)`, el compilador necesita elegir **en tiempo
de compilación** qué sobrecarga llamar: `visit(const Circulo&)`,
`visit(const Cuadrado&)`, etc. El tipo estático de `*f` es `Shape&` —
el compilador no tiene forma de saber que detrás hay un `Circulo`. El
polimorfismo virtual de C++ resuelve el tipo dinámico **del objeto
receptor** (`this`), no de los parámetros.

```cpp
json.visit(*f);
// f es Shape*. *f es de tipo estático Shape&.
// El compilador busca: visit(const Shape&). Si no existe, error.
// Si existe, se llama siempre la genérica, aunque el objeto real
// sea Circulo. Polimorfismo perdido.
```

El `aceptar()` virtual rodeaba este problema: dentro de
`Circulo::aceptar`, `*this` **ya está tipado como `Circulo&`**, y ahí
sí elige `visitar(const Circulo&)`. El virtual hace el primer salto
(resuelve el tipo del visitado); la sobrecarga hace el segundo
(resuelve qué operación del visitante). De ahí el nombre **double
dispatch**.

Sin ese primer salto, te falta la información del tipo concreto **en
el sitio donde la necesitas**. Toca recuperarla de algún modo. Hay
tres caminos.

### Camino A — `std::variant` (C++17): la sintaxis bonita gratis

Si el conjunto de tipos de figura es **cerrado** (lo conoces todo en
compile-time), tiras la jerarquía y usas variantes:

```cpp
using Figura = std::variant<Circulo, Cuadrado, Triangulo, Poligono>;

class VisitanteJson {
    std::ostringstream out;
public:
    void visit(const Figura& f) {
        std::visit([this](const auto& x) { visitar(x); }, f);
    }

    void visitar(const Circulo& c)   { out << "{\"r\":"  << c.getRadio() << "}"; }
    void visitar(const Cuadrado& c)  { out << "{\"l\":"  << c.getLado()  << "}"; }
    void visitar(const Triangulo&)   { /* ... */ }
    void visitar(const Poligono&)    { /* ... */ }

    std::string resultado() const { return out.str(); }
};
```

Cliente:

```cpp
VisitanteJson json;
for (const auto& f : lienzo.figuras())   // vector<Figura>, no vector<unique_ptr<Shape>>
    json.visit(f);                       // ← la sintaxis que querías
std::cout << json.resultado();
```

`std::visit` hace el dispatch interno por ti. **No hace falta `aceptar`
ni hay clase base `Shape`.** Y si añades `Estrella` al `variant` pero
olvidas un `visitar(const Estrella&)`, **el compilador chilla**: la
red de seguridad del visitor clásico se mantiene.

**Coste:** la jerarquía es cerrada. Un plugin no puede añadir
`Estrella` en runtime sin tocar el `using Figura = ...`. Para el Paint
del curso encaja bien — los tipos son estables. Para sistemas con
extensibilidad por plugin, no sirve.

### Camino B — `dynamic_cast` cadena: sin tocar `Shape`, pero con trampa

Mantienes `Shape` exactamente como está (sin `aceptar`). Toda la
maquinaria va al visitante:

```cpp
class VisitanteJson {
    std::ostringstream out;
public:
    void visit(const Shape& s) {
        if (auto* c = dynamic_cast<const Circulo*>(&s))   { visitar(*c); return; }
        if (auto* q = dynamic_cast<const Cuadrado*>(&s))  { visitar(*q); return; }
        if (auto* t = dynamic_cast<const Triangulo*>(&s)) { visitar(*t); return; }
        if (auto* p = dynamic_cast<const Poligono*>(&s))  { visitar(*p); return; }
        // ¿y si llega otra cosa? Silencio.
    }

    void visitar(const Circulo&   c) { /* ... */ }
    void visitar(const Cuadrado&  c) { /* ... */ }
    void visitar(const Triangulo& t) { /* ... */ }
    void visitar(const Poligono&  p) { /* ... */ }
};
```

Cliente: tu sintaxis preferida funciona y `Shape` queda virgen.

```cpp
VisitanteJson json;
for (const auto& f : lienzo.figurasReadOnly())
    json.visit(*f);
```

**Tres costes a mencionar al alumno:**

1. **Coste de runtime:** cada `visit` hace una cascada de
   `dynamic_cast` hasta acertar. Pequeño pero medible. RTTI activo
   obligatorio.
2. **Olvido silencioso:** si añades `Estrella` y olvidas su rama en
   `VisitanteJson`, **el compilador no avisa**. La figura entra,
   ningún `if` la captura, y se ignora sin más. Con el `aceptar`
   clásico, si olvidas un `visitar(Estrella&)` en algún visitante,
   error de compilación. Esa red de seguridad se pierde.
3. **Duplicación:** la cascada de `dynamic_cast` se repite en cada
   visitante. Se puede meter en una base común con un `dispatch`
   virtual… pero entonces estás reinventando `aceptar` desde el otro
   lado, con más ceremonia.

### Camino C — el doble despacho clásico (lo que vimos arriba)

`aceptar()` virtual en `Shape`. La sintaxis del cliente es
`f->aceptar(visitante)`, no `visitante.visit(*f)`. Pierdes la estética
pero ganas: red de seguridad de compilación, sin RTTI, jerarquía
abierta (un plugin puede añadir `Estrella` sin tocar nada existente
salvo los visitantes que quieran procesarla).

### Cómo elegir

| Quieres `visitante.visit(figura)`        | Jerarquía cerrada | Camino |
|------------------------------------------|-------------------|--------|
| Sí, y los tipos son estables             | OK                | **A** (`std::variant`) |
| Sí, y los tipos crecen por plugin        | NO                | B (`dynamic_cast`), asumiendo el coste |
| Me da igual la sintaxis                  | NO                | **C** (clásico GoF) |

**Recomendación para el Paint:** camino A. Los cuatro tipos son
estables, `std::variant` te da exactamente la sintaxis que quieres, y
el compilador te avisa de olvidos. Es C++ moderno haciendo lo que
tiene que hacer.

**Recomendación para código que verán fuera:** reconocer el camino C
(es lo que aparece en el GoF y en código heredado), y evitar el camino
B salvo emergencia.

## 7. El precio: el OCP "al revés"

Visitor invierte el OCP:

- Si tu eje de variación es **añadir tipos** (figuras nuevas):
  Visitor es **mala** elección. Cada nueva figura te obliga a tocar
  **todos los visitantes**.
- Si tu eje de variación es **añadir operaciones**: Visitor es
  **excelente**. Cada nueva operación es un visitante nuevo.

Pregúntate: ¿qué cambia más en mi dominio, los tipos o las
operaciones? Las jerarquías estables y de pocos tipos (compiladores,
ASTs, formatos de fichero) son **ideales** para Visitor. Las jerarquías
abiertas (figuras en un Paint donde el usuario añade tipos
constantemente) son **malos** candidatos.

En el Paint, los tipos básicos (`Circulo`, `Cuadrado`, `Triangulo`,
`Poligono`) son estables — el usuario no inventa figuras nuevas en
runtime —, así que Visitor encaja bien.

## 8. Variantes y trampas

- **Visitante con resultado**: si todos los `visitar` devuelven el
  mismo tipo, conviene una clase base templatizada `VisitanteShape<T>`
  con métodos `T visitar(...)`. Más limpio que el "guardo el resultado
  en un miembro" del ejemplo.
- **Visitor en estructuras anidadas (Composite)**: el `Grupo` del día
  3 también acepta visitantes, y en su `visitar` propaga a sus hijos.
  Composite + Visitor es una combinación clásica.
- **Forward declarations** son críticas (en el camino C): el
  `VisitanteShape` necesita conocer los tipos concretos, y los tipos
  concretos necesitan conocer al visitante. Cabeceras separadas con
  forward declarations resuelven la dependencia circular.

## 9. SOLID

| Principio | Cómo lo cumple Visitor |
|-----------|-----------------------|
| SRP       | Cada visitante una operación; las figuras no saben de operaciones. |
| OCP       | Cumplido **en operaciones**, no en tipos. (Inversión consciente.) |
| LSP       | Todos los visitantes intercambiables ante una figura. |
| ISP       | El visitante expone una operación; el cliente la usa. |
| DIP       | La figura depende del visitante abstracto. |

## 10. Mantra

> *"Si tus operaciones cambian más que tus tipos, no las metas en los
> tipos. Pásalas en visita."*