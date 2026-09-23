# Día 2 — Bloque 2: OCP (Open/Closed Principle)

> *"Las entidades software deben estar abiertas a extensión, pero cerradas
> a modificación."* — Bertrand Meyer

---

## 1. El problema en una frase

Cuando para añadir un caso nuevo tienes que **tocar código que ya funciona**,
estás abriendo la puerta a romperlo. OCP busca que **añadir** no implique
**modificar**.

## 2. Definición formal

> Una clase está **abierta a extensión** si podemos añadirle comportamiento
> nuevo, y **cerrada a modificación** si lo logramos **sin tocar su código
> fuente**.

¿Cómo logramos esa magia? Con **abstracciones**: la clase trabaja contra una
interfaz, y el comportamiento nuevo entra por una **nueva implementación** de
esa interfaz, no por un `if` más.

## 3. Bad: el `switch` que crece

Un calculador de áreas con `switch` por tipo. Cada figura nueva → otro `case`.

```cpp
enum class TipoFigura { CIRCULO, RECTANGULO };

struct Figura {
    TipoFigura tipo;
    double a;   // radio o ancho
    double b;   // alto (solo rectángulo)
};

class Calculadora {
public:
    double area(const Figura& f) const {
        switch (f.tipo) {
            case TipoFigura::CIRCULO:
                return 3.14159 * f.a * f.a;
            case TipoFigura::RECTANGULO:
                return f.a * f.b;
        }
        return 0;
    }
};
```

Ahora viene el cliente y pide **triángulos**. Tienes que:

1. Añadir `TRIANGULO` al `enum`.
2. Añadir un `case` al `switch` en `area()`.
3. Buscar todos los demás `switch` que probablemente existan repartidos por
   el código (`dibujar`, `serializar`, `perimetro`…) y añadir el `case` en
   cada uno.
4. Recompilar y rezar para que no se te haya olvidado ninguno.

### ¿Qué duele?

- **Cada `case` nuevo te obliga a tocar todos los `switch`** del programa.
- El compilador **no te avisa** si olvidas un `case` (a no ser que actives
  warnings específicos).
- Los tests de regresión que pasaban antes pueden romperse, porque has
  tocado código existente.
- Cuanto más vivo está el programa, más caro es cada cambio.

## 4. Good: jerarquía polimórfica

Damos la vuelta a la dependencia. La `Calculadora` no decide qué tipo es la
figura: **se lo pregunta a la figura**.

```cpp
class Figura {
public:
    virtual ~Figura() = default;
    virtual double area() const = 0;
};

class Circulo : public Figura {
    double radio;
public:
    explicit Circulo(double r) : radio(r) {}
    double area() const override { return 3.14159 * radio * radio; }
};

class Rectangulo : public Figura {
    double ancho, alto;
public:
    Rectangulo(double w, double h) : ancho(w), alto(h) {}
    double area() const override { return ancho * alto; }
};
```

Y ahora cuando llega el triángulo, **no tocamos ninguna clase existente**.
Solo añadimos una nueva:

```cpp
class Triangulo : public Figura {
    double base, altura;
public:
    Triangulo(double b, double h) : base(b), altura(h) {}
    double area() const override { return base * altura / 2.0; }
};
```

El código cliente sigue igual:

```cpp
double areaTotal(const std::vector<std::unique_ptr<Figura>>& figuras) {
    double total = 0;
    for (const auto& f : figuras) total += f->area();
    return total;
}
```

`areaTotal` ya está **cerrada a modificación** pero **abierta a extensión**:
acepta cualquier `Figura`, presente o futura.

## 5. Qué hemos ganado y qué hemos pagado

**Ganamos:**

- Añadir tipos nuevos **no requiere tocar el código existente**.
- Sin `switch`, sin `enum` que mantener, sin "olvidé un case".
- Cada figura concreta vive en su propio fichero.
- Tests existentes intactos: solo se añaden tests para la figura nueva.

**Pagamos:**

- Más clases, más ficheros.
- Polimorfismo dinámico tiene un coste (indirección virtual, vtable). En
  99% de los casos es irrelevante.
- Necesitas tener **clara la abstracción correcta**: si el día de mañana
  aparece un `area()` que necesita parámetros distintos según el tipo, la
  abstracción se rompe.

### Cuándo NO aplicarlo

OCP no significa *"todo con jerarquías polimórficas siempre"*. Si tienes:

```cpp
double cuadradoDe(double x) { return x * x; }
```

eso no necesita interfaz, jerarquía ni nada. OCP es una herramienta para
**puntos del programa donde el conjunto de variantes va a crecer**.

Heurística: aplica OCP cuando **anticipes** o **veas crecer** un `switch`
por tipo. Antes de eso, vivir con un `if` simple es perfectamente legítimo.

## 6. Variante moderna: OCP por templates (estático)

En C++ hay dos sabores. Polimorfismo dinámico (lo de arriba) y polimorfismo
estático con templates. Esto último vale la pena mencionarlo:

```cpp
template <typename F>
double areaDe(const F& figura) {
    return figura.area();   // no hay virtual, se resuelve en compilación
}
```

Cualquier tipo `F` con un método `area()` encaja. Es OCP también: para
añadir un nuevo tipo basta con que lo implemente, sin tocar `areaDe`.

> Cuándo elegir uno u otro: si vas a meter las figuras en un contenedor
> heterogéneo, **dinámico**. Si lo conoces todo en tiempo de compilación
> y quieres el rendimiento máximo, **estático**. En este curso usamos
> sobre todo el dinámico porque conecta con el GoF clásico.

## 7. Conexión con patrones

OCP es probablemente el principio que **más patrones genera**. Casi todos
los patrones GoF existen para sustituir un `switch` o un `if` por una
abstracción extensible:

| Patrón                | El `switch` que sustituye                        |
|-----------------------|--------------------------------------------------|
| **Strategy**          | `switch` sobre algoritmo                         |
| **State**             | `switch` sobre estado actual                     |
| **Factory Method**    | `switch` sobre tipo a crear                      |
| **Abstract Factory**  | `switch` sobre familia de productos              |
| **Chain of Responsibility** | `if/else if` de quién maneja la petición   |
| **Visitor**           | `switch` sobre tipo, cuando el `switch` está fuera de la jerarquía |
| **Decorator**         | `if` de qué responsabilidades aplicar            |
| **Template Method**   | `switch` sobre variantes de un algoritmo         |

Cuando mañana presentemos cada patrón, la pregunta clave será siempre la
misma: **¿qué `switch` me está ahorrando este patrón?**

## 8. Mini-ejercicio mental (3 min)

```cpp
double calcularDescuento(const Cliente& c) {
    if (c.tipo() == "VIP")        return 0.20;
    if (c.tipo() == "FRECUENTE")  return 0.10;
    if (c.tipo() == "OCASIONAL")  return 0.05;
    return 0.0;
}
```

Si en seis meses van a aparecer "EMPLEADO", "JUBILADO", "ESTUDIANTE"…
¿cómo lo refactorizas con OCP?

(Respuesta breve: jerarquía `Cliente` con método virtual `descuento()`, o
una interfaz `PoliticaDescuento` inyectada en `Cliente`. La segunda opción
permite cambiar la política sin tocar el tipo de cliente, y es Strategy.)

---

## Resumen del bloque

- OCP = **abierto a extensión, cerrado a modificación**.
- El mecanismo en C++ es **polimorfismo** (dinámico o estático).
- Sustituir `switch` por jerarquía es la traducción mecánica del principio.
- Añadir una variante nueva no debe forzar tocar código probado.
- Es el principio que mejor explica por qué existe medio GoF.

**Mantra del bloque:**

> *"Cuando veas un `switch` que crece, pregúntate qué jerarquía está
> intentando salir de ahí."*
