# Decorator

## 1. El problema

El cliente del Paint pide características nuevas:

- Algunas figuras deben tener **borde grueso**.
- Algunas deben tener **sombra**.
- Algunas deben tener **transparencia**.
- Algunas deben tener **dos cosas a la vez** (borde + sombra).
- O **las tres**: borde + sombra + transparencia.

Versión torpe — herencia para todo:

```
Circulo
├── CirculoConBorde
├── CirculoConSombra
├── CirculoConTransparencia
├── CirculoConBordeYSombra
├── CirculoConBordeYTransparencia
├── CirculoConSombraYTransparencia
└── CirculoConBordeSombraYTransparencia
```

Siete clases para una sola figura, y eso solo combinando 3 capacidades.
Si añadimos una cuarta (rotación), se duplica a 16. Si hay 5, son 32.
**Explosión combinatoria.** Justo lo que ayer prometimos resolver con
Decorator en el día 2.

## 2. Intención (GoF)

> *Añadir responsabilidades a un objeto dinámicamente. Decorator
> proporciona una alternativa flexible a la herencia para extender
> funcionalidad.*

La clave: **dinámicamente** y **alternativa a la herencia**.

## 3. Bad — la explosión de subclases

La que ya hemos visto arriba. Otra variante igual de mala: meter todos
los flags como campos:

```cpp
class Circulo : public Shape {
    double radio;
    bool tieneBorde   = false;
    bool tieneSombra  = false;
    double opacidad   = 1.0;
public:
    void dibujar() const override {
        std::cout << "Circulo " << radio;
        if (tieneSombra)  std::cout << " [sombra]";
        if (tieneBorde)   std::cout << " [borde]";
        if (opacidad < 1) std::cout << " [opac=" << opacidad << "]";
        std::cout << "\n";
    }
    // setters...
};
```

¿Qué tiene de malo? Hoy son tres flags, mañana siete, pasado quince. La
clase `Circulo` se llena de **responsabilidades ajenas** (dibujar
bordes, dibujar sombras, gestionar opacidad). **SRP llorando**.

## 4. Good — Decorator

La idea: una jerarquía paralela de "decoradores" que **envuelven** a un
`Shape` y le añaden comportamiento. Cada decorador **es un `Shape`** y
**contiene un `Shape`**.

```cpp
// Decorador base: es-un Shape y tiene-un Shape.
class ShapeDecorator : public Shape {
protected:
    std::unique_ptr<Shape> envuelto;
public:
    explicit ShapeDecorator(std::unique_ptr<Shape> s) : envuelto(std::move(s)) {}

    double area() const override { return envuelto->area(); }   // delega por defecto
    // dibujar() es abstracto: cada decorador concreto decide qué añade.
};

class ConBorde : public ShapeDecorator {
    double grosor;
public:
    ConBorde(std::unique_ptr<Shape> s, double g)
        : ShapeDecorator(std::move(s)), grosor(g) {}

    void dibujar() const override {
        envuelto->dibujar();
        std::cout << "  + borde de grosor " << grosor << "\n";
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<ConBorde>(envuelto->clone(), grosor);
    }
};

class ConSombra : public ShapeDecorator {
public:
    using ShapeDecorator::ShapeDecorator;

    void dibujar() const override {
        envuelto->dibujar();
        std::cout << "  + sombra\n";
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<ConSombra>(envuelto->clone());
    }
};

class ConTransparencia : public ShapeDecorator {
    double opacidad;
public:
    ConTransparencia(std::unique_ptr<Shape> s, double o)
        : ShapeDecorator(std::move(s)), opacidad(o) {}

    void dibujar() const override {
        envuelto->dibujar();
        std::cout << "  + opacidad " << opacidad << "\n";
    }

    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<ConTransparencia>(envuelto->clone(), opacidad);
    }
};
```

Uso:

```cpp
// Un círculo simple
auto c1 = std::make_unique<Circulo>(3.0);

// Un círculo con borde
auto c2 = std::make_unique<ConBorde>(std::make_unique<Circulo>(3.0), 2.0);

// Un círculo con borde Y sombra Y transparencia, apilados
auto c3 = std::make_unique<ConTransparencia>(
              std::make_unique<ConSombra>(
                  std::make_unique<ConBorde>(
                      std::make_unique<Circulo>(3.0), 2.0
                  )
              ), 0.8);

c3->dibujar();
// Salida:
// Circulo 3
//   + borde de grosor 2
//   + sombra
//   + opacidad 0.8
```

**Tres capacidades combinables = 8 combinaciones**. ¿Cuántas clases hemos
escrito? **3.** Una por capacidad. La combinación se hace en tiempo de
ejecución, anidando decoradores.

Si añadimos una cuarta capacidad (rotación), es **una clase más**, no 8
más. La explosión está domada.

## 5. UML rápido

```
            ┌─────────────────┐
            │      Shape      │
            ├─────────────────┤
            │ +dibujar()      │
            │ +area()         │
            └─────────────────┘
                    ▲
        ┌───────────┴────────────┐
        │                        │
┌──────────────┐         ┌────────────────────┐
│   Circulo    │         │ ShapeDecorator     │
│   Cuadrado   │         ├────────────────────┤
│   ...        │         │ -envuelto: Shape   │ ◀── tiene-un Shape
└──────────────┘         └────────────────────┘
                                  ▲
                ┌─────────────────┼──────────────────┐
                │                 │                  │
        ┌──────────────┐  ┌──────────────┐  ┌───────────────────┐
        │  ConBorde    │  │  ConSombra   │  │ ConTransparencia  │
        └──────────────┘  └──────────────┘  └───────────────────┘
```

La doble relación con `Shape` (hereda Y contiene) es **la firma de
Decorator**. Si ves ese rombo en un diagrama, es Decorator.

## 6. Variantes y trampas

### Trampa 1: clone() en decoradores

Cada decorador debe clonar **el envuelto también**, llamando a su
`clone()`. Si solo clonas el decorador exterior, el `envuelto` se
compartiría (y `unique_ptr` no lo permite, error de compilación), o si
fuera `shared_ptr` sería un bug silencioso.

### Trampa 2: el orden importa

`ConBorde(ConSombra(circulo))` y `ConSombra(ConBorde(circulo))` **no son
lo mismo**. El orden de dibujado cambia. El cliente debe ser consciente.

### Trampa 3: Decorator vs herencia

¿Cuándo herencia y cuándo Decorator?

- Si la variación es **una característica fija del tipo** (`CirculoRelleno`
  es siempre relleno por definición) → herencia.
- Si la variación es **una capacidad opcional combinable** (cualquier
  figura puede o no tener borde) → Decorator.

### Variante: Decorator + Builder

Para combinaciones largas, el `ConBorde(ConSombra(Con...(...)))` se hace
ilegible. Un mini-builder ayuda:

```cpp
auto figura = decorar(std::make_unique<Circulo>(3.0))
                .con_borde(2.0)
                .con_sombra()
                .con_opacidad(0.8)
                .build();
```

Implementación trivial: cada `con_X` envuelve el `unique_ptr` actual en
el decorador correspondiente. Es Decorator + Builder fluido, conviven
bien.

## 7. Cuándo usar Decorator

- Cuando hay **capacidades opcionales combinables**.
- Cuando la herencia te llevaría a explosión combinatoria.
- Cuando quieres **añadir/quitar funcionalidad en runtime** (esto la
  herencia no lo permite).

En el Paint nos sirve para todo lo que ayer prometimos: bordes, sombras,
transparencia, y todo lo que venga (rotación, marco, etiqueta, capa…).

## 8. SOLID en acción

| Principio | Cómo lo aplica Decorator                                          |
|-----------|-------------------------------------------------------------------|
| SRP       | Cada decorador una responsabilidad concreta.                      |
| OCP       | **Cumplido al máximo.** Nueva capacidad = nuevo decorador.        |
| LSP       | Cumplido: cualquier decorador es un `Shape`.                      |
| ISP       | Cumplido: la interfaz `Shape` no crece.                           |
| DIP       | Cumplido: todo el código sigue dependiendo de `Shape`.            |

El patrón **más SOLID-friendly** de los estructurales. Por eso aparece
en cualquier sistema serio de UI.

## 9. Mantra

> *"Si las capacidades se combinan, no las heredes. Envuélvelas."*
