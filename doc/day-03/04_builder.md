# Builder

## 1. El problema

Hasta ahora las figuras del Paint eran simples: un círculo con un radio,
un cuadrado con un lado. Pero el dominio crece. Imagina una figura
compleja: un **polígono configurable** con N vértices, color de borde,
color de relleno, grosor de línea, sombra, transparencia, rotación,
nombre para el panel de capas…

Constructor opción 1 — todos los parámetros:

```cpp
Poligono p(
    {{0,0}, {3,0}, {3,4}}, // vértices
    Color::Rojo,            // borde
    Color::Amarillo,        // relleno
    2.0,                    // grosor
    true,                   // sombra
    0.8,                    // opacidad
    0.0,                    // rotación
    "triángulo verde"       // nombre
);
```

Problemas:

- **Constructor telescópico**: imposible leer qué es cada parámetro.
- Si quieres dos parámetros y los demás por defecto, **el orden te
  obliga** a poner todos los intermedios.
- Añadir un parámetro nuevo rompe a todos los clientes.

Constructor opción 2 — un struct gigante de configuración:

```cpp
struct ConfigPoligono { /* 8 campos */ };
Poligono p(ConfigPoligono{ /* ... */ });
```

Menos malo, pero no valida que la configuración tenga sentido (¿y si
das opacidad = 5? ¿y si la lista de vértices está vacía?).

**Builder** resuelve los dos: construcción paso a paso, con nombres,
validación al final, e inmutabilidad del objeto resultante.

## 2. Intención (GoF)

> *Separar la construcción de un objeto complejo de su representación,
> de modo que el mismo proceso de construcción pueda crear distintas
> representaciones.*

En cristiano: el objeto final es inmutable y simple; toda la complejidad
de "cómo se ensambla" vive en el Builder.

## 3. Bad — el constructor telescópico

Lo que ya hemos visto arriba. Otro síntoma del mismo dolor:

```cpp
class Poligono {
public:
    Poligono();
    Poligono(std::vector<Punto> v);
    Poligono(std::vector<Punto> v, Color borde);
    Poligono(std::vector<Punto> v, Color borde, Color relleno);
    Poligono(std::vector<Punto> v, Color borde, Color relleno, double grosor);
    // ... y sigue
};
```

Cinco constructores, y cada uno se llama según qué te dé pereza. El
cliente nunca sabe cuál usar.

## 4. Good — Builder con encadenamiento (fluent)

La variante moderna que más se usa en C++:

```cpp
class Poligono : public Shape {
    std::vector<Punto> vertices;
    Color borde       = Color::Negro;
    Color relleno     = Color::Ninguno;
    double grosor     = 1.0;
    bool sombra       = false;
    double opacidad   = 1.0;
    double rotacion   = 0.0;
    std::string nombre = "poligono";

    // El constructor es privado: solo el Builder puede invocarlo.
    Poligono() = default;

public:
    void dibujar() const override { /* ... */ }
    double area() const override   { /* ... */ }

    class Builder;   // declaración adelantada
    friend class Builder;
};

class Poligono::Builder {
    Poligono p;   // se va rellenando
public:
    Builder& con_vertices(std::vector<Punto> v) { p.vertices = std::move(v); return *this; }
    Builder& con_borde(Color c)                 { p.borde = c;               return *this; }
    Builder& con_relleno(Color c)               { p.relleno = c;             return *this; }
    Builder& con_grosor(double g)               { p.grosor = g;              return *this; }
    Builder& con_sombra(bool s)                 { p.sombra = s;              return *this; }
    Builder& con_opacidad(double o)             { p.opacidad = o;            return *this; }
    Builder& con_rotacion(double r)             { p.rotacion = r;            return *this; }
    Builder& con_nombre(std::string n)          { p.nombre = std::move(n);   return *this; }

    Poligono build() {
        validar();
        return std::move(p);
    }

private:
    void validar() {
        if (p.vertices.size() < 3)
            throw std::invalid_argument("un polígono necesita al menos 3 vértices");
        if (p.opacidad < 0.0 || p.opacidad > 1.0)
            throw std::invalid_argument("opacidad fuera de [0, 1]");
        if (p.grosor <= 0.0)
            throw std::invalid_argument("grosor debe ser positivo");
    }
};
```

Uso:

```cpp
Poligono triangulo = Poligono::Builder{}
    .con_vertices({{0,0}, {3,0}, {3,4}})
    .con_borde(Color::Rojo)
    .con_relleno(Color::Amarillo)
    .con_nombre("triángulo verde")
    .build();
```

Tres ganancias:

1. **Legibilidad**: cada parámetro tiene nombre.
2. **Validación**: `build()` comprueba que el objeto tenga sentido antes
   de devolverlo. El objeto resultante es siempre coherente.
3. **Inmutabilidad**: una vez construido, los campos del `Poligono` no
   se pueden cambiar (no hay setters públicos).

## 5. UML rápido

```
┌────────────────────┐                  ┌────────────────┐
│  Poligono::Builder │ ──── build() ───▶│   Poligono     │
├────────────────────┤                  ├────────────────┤
│ + con_vertices(...)│                  │ - vertices     │
│ + con_borde(...)   │                  │ - borde        │
│ + con_relleno(...) │                  │ - relleno      │
│ + ...              │                  │ - ...          │
│ + build() : Poligono                  │ + dibujar()    │
└────────────────────┘                  └────────────────┘
        ▲
        │ retorna *this en cada con_*
        └─────── encadenamiento fluido
```

## 6. Variantes y trampas

### Variante: Director + Builder (GoF puro)

El GoF original separa **Builder** (los pasos) de **Director** (la receta
que decide en qué orden invocarlos). Útil cuando hay **varias recetas**
para construir variantes del mismo objeto:

```cpp
class DirectorFiguras {
public:
    static Poligono triangulo_rectangulo_estandar(Poligono::Builder& b) {
        return b
            .con_vertices({{0,0}, {3,0}, {3,4}})
            .con_borde(Color::Negro)
            .con_nombre("triángulo 3-4-5")
            .build();
    }

    static Poligono pentagono_regular(Poligono::Builder& b, double radio) {
        return b
            .con_vertices(pentagonoRegular(radio))
            .con_nombre("pentágono")
            .build();
    }
};
```

En la práctica moderna, **el director se usa poco**. Suele bastar con
funciones libres que envuelvan al Builder.

### Trampa 1: confundir Builder con Factory

Si la construcción es **una sola llamada** con todos los parámetros, es
Factory. Si es **un proceso por pasos** con validación al final, es
Builder.

| Necesidad                                | Patrón     |
|------------------------------------------|------------|
| "Dame un círculo de radio 3"             | Factory    |
| "Configura el polígono, valida, dámelo"  | Builder    |
| "Decide qué crear según el nombre"       | Factory    |
| "Construye paso a paso con muchos opcs"  | Builder    |

### Trampa 2: el Builder mutable que se cuela

Resiste la tentación de exponer el `Poligono` antes de que esté listo.
Si tu `build()` devuelve algo que aún se puede modificar, has perdido
la mitad del valor del patrón. **El Builder se queda con la mutabilidad,
el objeto final es inmutable**.

### Variante moderna: parámetros con nombre simulados (designated initializers)

C++20 introduce **designated initializers**, que en algunos casos cubren
parte del dominio del Builder sin escribir tanto código:

```cpp
struct PoligonoConfig {
    std::vector<Punto> vertices;
    Color borde     = Color::Negro;
    double opacidad = 1.0;
};

Poligono p({
    .vertices = {{0,0}, {3,0}, {3,4}},
    .borde    = Color::Rojo,
});
```

Pero no validan, no encadenan, y no se llevan bien con la herencia. Para
casos simples están bien; cuando hay lógica de construcción real,
Builder sigue ganando.

### Trampa 3: el Builder que clona vs el que muta

Dos sabores del fluent:

- **Mutable** (el que hemos visto): cada `con_X` devuelve `*this`. Más
  rápido, pero el Builder es de un solo uso.
- **Inmutable**: cada `con_X` devuelve un **nuevo** Builder con el campo
  cambiado. Más caro pero permite ramificar construcciones a partir de
  un Builder base.

El mutable es el estándar en C++. El inmutable se ve más en Java y Scala.

## 7. Cuándo usar Builder

- Constructor con **más de 4-5 parámetros**, sobre todo si varios son
  opcionales.
- Cuando el objeto **no debe existir en estado inválido** y la validación
  es compleja.
- Cuando construyes el mismo tipo con **muchas configuraciones** y
  quieres reutilizar pasos.
- Cuando quieres **inmutabilidad** después de la construcción.

En el Paint nos sirve para `Poligono` (y para cualquier figura compleja
que añadamos más adelante: texto con fuente y alineación, gráfico con
ejes y leyenda, etc.).

## 8. SOLID en acción

| Principio | Cómo lo aplica Builder                                            |
|-----------|-------------------------------------------------------------------|
| SRP       | El Builder construye; el Poligono representa. Dos responsabilidades, dos clases. |
| OCP       | Cumplido: añadir un parámetro nuevo es añadir un `con_X` y un campo. |
| LSP       | Neutro.                                                           |
| ISP       | Cumplido: el cliente solo llama a los `con_X` que necesita.       |
| DIP       | Neutro.                                                           |

Es el patrón que más aplica **SRP** de los creacionales: separar
"cómo se construye" de "qué es".

## 9. Mantra

> *"El constructor con diez parámetros no es C++. Es un grito de socorro."*
