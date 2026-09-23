# Día 2 — Cierre: refactor en vivo del Paint aplicando SOLID

> **Objetivo:** retomar la semilla del Paint del día 1 y dejarla, en vivo,
> aplicando los cinco principios. El resultado es el `Shape` definitivo del
> curso, sobre el que construiremos los patrones de los próximos dos días.

---

## 1. Punto de partida (recordatorio del día 1)

Cerramos ayer con esto:

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void dibujar() const = 0;
};

class Circulo : public Shape {
    double radio;
public:
    explicit Circulo(double r) : radio(r) {}
    void dibujar() const override { std::cout << "Circulo " << radio << "\n"; }
};

class Cuadrado : public Shape {
    double lado;
public:
    explicit Cuadrado(double l) : lado(l) {}
    void dibujar() const override { std::cout << "Cuadrado " << lado << "\n"; }
};

int main() {
    std::vector<std::unique_ptr<Shape>> lienzo;
    lienzo.push_back(std::make_unique<Circulo>(3.0));
    lienzo.push_back(std::make_unique<Cuadrado>(2.0));
    for (const auto& s : lienzo) s->dibujar();
}
```

Esto ya cumple **OCP** (añadir `Triangulo` no toca nada) y **LSP** (no hay
`dynamic_cast`, todas las shapes son intercambiables). Lo veíamos ayer.

Pero **no es suficiente** para el Paint real. Le faltan responsabilidades:
calcular área, serializar, persistir el lienzo, deshacer cambios… Si las
metemos todas a lo bruto en `Shape`, en una semana tenemos un monstruo.

Vamos a evolucionarlo con SOLID en mente.

---

## 2. Iteración 1: aplicar SRP

**Petición del "cliente":** ahora el Paint tiene que:

- Dibujar figuras (eso ya está).
- Calcular el área total del lienzo.
- Guardar el lienzo en un fichero.

Tentación: meter todo en `Shape` y en `Lienzo`. **Que no.**

### Mal camino (no lo hagas)

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
    virtual std::string serializar() const = 0;   // ❌ ¿por qué Shape sabe serializar?
};
```

Eso mete dos responsabilidades en `Shape`: ser figura **y** saber cómo se
representa en disco. El día que cambiemos JSON por XML, tocamos todas las
figuras.

### Buen camino

`Shape` solo conoce lo intrínseco a una figura: cómo se dibuja, cuál es su
área. La serialización **vive fuera**:

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual void dibujar() const = 0;
    virtual double area() const = 0;
};
```

`Lienzo` orquesta. **No dibuja, no serializa, no calcula áreas él mismo**;
delega:

```cpp
class Lienzo {
    std::vector<std::unique_ptr<Shape>> shapes;
public:
    void añadir(std::unique_ptr<Shape> s) { shapes.push_back(std::move(s)); }
    const auto& todas() const { return shapes; }
};
```

Y la serialización va aparte:

```cpp
class SerializadorLienzo {
public:
    virtual ~SerializadorLienzo() = default;
    virtual std::string serializar(const Lienzo& l) const = 0;
};
```

(Aún no implementado: lo aprovecharemos para DIP.)

> **Principio aplicado:** SRP. `Shape` es figura, `Lienzo` colección,
> `SerializadorLienzo` persistencia. Tres motivos de cambio, tres clases.

---

## 3. Iteración 2: aplicar OCP

Las figuras concretas las añadimos sin tocar `Shape`. Ya funcionaba el día 1,
pero ahora con `area()`:

```cpp
class Circulo : public Shape {
    double radio;
public:
    explicit Circulo(double r) : radio(r) {}
    void dibujar() const override { std::cout << "Circulo r=" << radio << "\n"; }
    double area() const override { return 3.14159 * radio * radio; }
};

class Cuadrado : public Shape {
    double lado;
public:
    explicit Cuadrado(double l) : lado(l) {}
    void dibujar() const override { std::cout << "Cuadrado l=" << lado << "\n"; }
    double area() const override { return lado * lado; }
};

class Triangulo : public Shape {
    double base, altura;
public:
    Triangulo(double b, double h) : base(b), altura(h) {}
    void dibujar() const override { std::cout << "Triangulo b=" << base << " h=" << altura << "\n"; }
    double area() const override { return base * altura / 2.0; }
};
```

Y un cálculo de área total que **no toca cuando aparezca un pentágono**:

```cpp
double areaTotal(const Lienzo& l) {
    double total = 0;
    for (const auto& s : l.todas()) total += s->area();
    return total;
}
```

> **Principio aplicado:** OCP. Añadir `Pentagono` solo añade un fichero.
> `areaTotal` y `Lienzo` no se tocan.

---

## 4. Iteración 3: cuidar LSP

**Tentación peligrosa.** Aparece la petición: *"queremos figuras con
relleno y figuras sin relleno"*. La primera idea:

### Bad

```cpp
class FiguraSinRelleno : public Shape {
public:
    // ...
    void setRelleno(Color) {
        throw std::logic_error("Esta figura no tiene relleno");   // ❌
    }
};
```

Eso es **el pingüino** otra vez. Si `setRelleno` está en la base, el
cliente espera que funcione.

### Good

Si no todas las figuras tienen relleno, **el relleno no pertenece a
`Shape`**. Lo sacamos a una capacidad aparte:

```cpp
class Rellenable {
public:
    virtual ~Rellenable() = default;
    virtual void setRelleno(int color) = 0;
};

class CirculoRelleno : public Shape, public Rellenable {
    double radio;
    int color = 0;
public:
    explicit CirculoRelleno(double r) : radio(r) {}
    void dibujar() const override { std::cout << "Circulo relleno color=" << color << "\n"; }
    double area() const override { return 3.14159 * radio * radio; }
    void setRelleno(int c) override { color = c; }
};
```

Quien quiera rellenar pide `Rellenable&`, no `Shape&`. **El compilador
detecta el error en compilación**, no en runtime.

> **Principios aplicados:** LSP (no rompemos contrato de `Shape`) e ISP
> (interfaces pequeñas: `Shape` y `Rellenable` son cosas distintas).

---

## 5. Iteración 4: aplicar ISP

Mismo razonamiento un paso más lejos. Acaba de llegar otra petición:
*"queremos que algunas figuras puedan moverse y otras estén fijas"*.

```cpp
class Movible {
public:
    virtual ~Movible() = default;
    virtual void mover(double dx, double dy) = 0;
};

class CirculoMovible : public Shape, public Rellenable, public Movible {
    // ...
};
```

Cada figura **declara qué capacidades ofrece**. El cliente pide la mínima
necesaria.

```cpp
void moverTodo(const std::vector<Movible*>& movibles, double dx, double dy) {
    for (auto* m : movibles) m->mover(dx, dy);
}
```

Una `EstrellaFija` que solo herede de `Shape` no puede pasarse a
`moverTodo`. **Esto es ISP en estado puro.**

> Recordar: herencia múltiple en C++ aquí no asusta porque son **interfaces
> puras** (todos métodos virtuales puros, cero estado). No hay diamante.

---

## 6. Iteración 5: aplicar DIP

El último golpe. Ahora sí queremos serializar el lienzo. Y queremos poder
elegir JSON, XML o texto plano sin tocar nada del dominio.

```cpp
class SerializadorLienzo {
public:
    virtual ~SerializadorLienzo() = default;
    virtual std::string serializar(const Lienzo& l) const = 0;
};

class SerializadorTexto : public SerializadorLienzo {
public:
    std::string serializar(const Lienzo& l) const override {
        std::ostringstream out;
        for (const auto& s : l.todas()) {
            // muy simplificado: en real haríamos visitor o dynamic info
            out << "shape(area=" << s->area() << ")\n";
        }
        return out.str();
    }
};

class SerializadorJSON : public SerializadorLienzo {
public:
    std::string serializar(const Lienzo& l) const override {
        std::ostringstream out;
        out << "[";
        bool primero = true;
        for (const auto& s : l.todas()) {
            if (!primero) out << ",";
            out << "{\"area\":" << s->area() << "}";
            primero = false;
        }
        out << "]";
        return out.str();
    }
};
```

Y `GuardarLienzo` (alto nivel) **no sabe** qué serializador concreto le
toca. Recibe la abstracción:

```cpp
class GuardarLienzo {
    SerializadorLienzo* serializador;
public:
    explicit GuardarLienzo(SerializadorLienzo* s) : serializador(s) {}
    void guardar(const Lienzo& l, const std::string& ruta) const {
        std::ofstream f(ruta);
        f << serializador->serializar(l);
    }
};
```

Composition root:

```cpp
int main() {
    Lienzo lienzo;
    lienzo.añadir(std::make_unique<Circulo>(3.0));
    lienzo.añadir(std::make_unique<Cuadrado>(2.0));

    SerializadorJSON json;
    GuardarLienzo guardador(&json);
    guardador.guardar(lienzo, "salida.json");

    // Cambiar a texto: cambias 2 líneas, no tocas Lienzo ni Shape ni nada del dominio
    SerializadorTexto texto;
    GuardarLienzo guardadorTxt(&texto);
    guardadorTxt.guardar(lienzo, "salida.txt");
}
```

> **Principio aplicado:** DIP. `GuardarLienzo` (alto nivel) depende de
> `SerializadorLienzo` (abstracción), no de `SerializadorJSON` (detalle).
> El día que llegue XML, añadimos `SerializadorXML` y listo.

---

## 7. La foto final

Estructura del Paint tras los cinco principios:

```
Shape                  ← interfaz de figura (dibujar, área)
├── Circulo
├── Cuadrado
└── Triangulo

Rellenable             ← capacidad opcional
Movible                ← capacidad opcional

CirculoRelleno : Shape, Rellenable
CirculoMovible : Shape, Movible
EstrellaFija  : Shape

Lienzo                 ← colección de Shapes (SRP: solo gestiona la lista)

SerializadorLienzo     ← interfaz de serialización (ISP/DIP)
├── SerializadorTexto
└── SerializadorJSON

GuardarLienzo          ← alto nivel; recibe SerializadorLienzo por constructor
```

Diagnóstico:

| Principio | Cómo se cumple                                                  |
|-----------|-----------------------------------------------------------------|
| SRP       | `Shape` figura, `Lienzo` colección, `Serializador*` persistencia, `GuardarLienzo` orquestación. |
| OCP       | Añadir figuras o serializadores no toca código existente.       |
| LSP       | Ninguna subclase lanza "no soportado" ni rompe contrato.        |
| ISP       | `Shape`, `Rellenable`, `Movible` son interfaces pequeñas y específicas. |
| DIP       | `GuardarLienzo` depende de `SerializadorLienzo`, no de detalles.|

---

## 8. Por qué este Paint va a aguantar todo el curso

Lo que vamos a hacer en los próximos dos días, **sin tocar este diseño**:

- **Singleton**: un `Lienzo` único accesible globalmente (con cuidado: ya
  veremos por qué Singleton es un patrón "polémico").
- **Prototype**: clonar figuras (`virtual Shape* clone()`).
- **Factory** y **Abstract Factory**: para crear figuras desde texto del
  usuario o desde fichero.
- **Builder**: para construir figuras complejas paso a paso.
- **Adapter**: integrar una librería externa de gráficos que tiene su
  propia interfaz.
- **Decorator**: añadir borde, sombra, transparencia… **sin explosión de
  subclases**.
- **Composite**: agrupar figuras en grupos que se comportan como una
  figura más (`Grupo : Shape`).
- **Strategy**: distintos algoritmos de dibujado (alámbrico, relleno,
  texturizado).
- **Command**: cada acción del usuario (añadir, mover, eliminar) es un
  `Command` → así implementamos **deshacer/rehacer**.
- **Observer**: que la barra de herramientas se entere de los cambios en
  el `Lienzo`.
- **Memento**: snapshots del lienzo para implementar undo a otro nivel.
- **Visitor**: aplicar operaciones nuevas (exportar, contar, transformar)
  sin tocar las figuras.

**Si este diseño aguanta todos esos patrones sin reescribirse, SOLID está
bien aplicado.** Si no lo aguantara, tendríamos que rediseñar. (Spoiler:
aguanta.)

---

## 9. Cierre del día

Frase para llevarse a casa:

> *"SOLID son cinco preguntas que te haces antes de empezar a teclear:*
> - *¿Qué hace esta clase, y solo esta clase?*
> - *¿Qué pasa cuando llegue el siguiente caso?*
> - *¿Las subclases cumplen lo que la base promete?*
> - *¿Le estoy obligando a alguien a implementar lo que no necesita?*
> - *¿De qué decisión concreta estoy dependiendo, y debería invertirla?"*

Las cinco preguntas, en este orden, son **la chuleta mental** que el
alumno se debería llevar del día.

---

## 10. Conexión con el día 3

Mañana arrancamos con **patrones creacionales**. Veremos cómo Singleton,
Prototype, Factory, Builder y Abstract Factory **encajan en este Paint**
sin destrozarlo. Y por qué cada patrón es, en el fondo, una aplicación
mecánica de los principios que hemos visto hoy.

**Mantra del día:**

> *"Los patrones no son cosas que se aprenden de memoria. Son lo que pasa
> cuando aplicas SOLID con honestidad."*
