# Día 1 — Bloque 3: POO en C++ moderno

> **Objetivo del bloque:** asentar herencia, polimorfismo, composición y
> delegación con las particularidades de C++. Salir de aquí con el mantra
> *"composición sobre herencia"* interiorizado, porque es la base filosófica
> de la mitad del GoF.

---

## 1. Clases en C++: lo que ya saben, recordado

```cpp
class Cuenta {
private:
    double saldo;

public:
    explicit Cuenta(double inicial) : saldo(inicial) {}

    void ingresar(double cantidad) { saldo += cantidad; }
    void retirar(double cantidad)  { saldo -= cantidad; }
    double getSaldo() const        { return saldo; }
};
```

Cosas a remarcar:

- `explicit` evita conversiones implícitas en constructores de un solo
  argumento. En general, ponlo siempre.
- `const` al final del método: *"este método no modifica el objeto"*. Es
  obligatorio si el objeto puede ser `const` en algún momento.
- **Lista de inicialización** (`: saldo(inicial)`) en lugar de asignación en
  el cuerpo. Es más eficiente y necesario para miembros `const` o referencias.

## 2. Herencia

### Sintaxis básica

```cpp
class Animal {
public:
    virtual ~Animal() = default;        // ¡destructor virtual!
    virtual void hablar() const = 0;    // método virtual puro = clase abstracta
};

class Perro : public Animal {
public:
    void hablar() const override { std::cout << "Guau\n"; }
};

class Gato : public Animal {
public:
    void hablar() const override { std::cout << "Miau\n"; }
};
```

### El destructor virtual (bug clásico)

Si vas a borrar un objeto derivado a través de un puntero a la base, el
destructor de la base **debe** ser virtual. Si no, el destructor de la
derivada nunca se llama → leak.

```cpp
class Base { public: ~Base() {} };           // ❌ no virtual
class Derivada : public Base { /* recursos */ };

Base* p = new Derivada;
delete p;   // solo llama ~Base. Los recursos de Derivada NO se liberan.
```

Regla simple: **toda clase pensada para ser heredada tiene destructor
virtual**. Si no quieres permitir herencia, márcala `final`.

### `override` y `final`

`override` no es decorativo: hace que el compilador verifique que de verdad
estás sobrescribiendo un método virtual de la base. Si te equivocas en la
firma, te avisa.

```cpp
class Base {
    virtual void hacer(int) const;
};

class Derivada : public Base {
    void hacer(int) override;        // ok
    void hacer(double) override;     // ERROR: no sobrescribe nada
};
```

`final` marca una clase o un método como no-extensible:

```cpp
class Hoja final : public Base { };  // nadie puede heredar de Hoja
```

### Herencia pública vs privada vs protegida

Esto sorprende a los que vienen de Java/C#. En C++ la herencia tiene
**visibilidad**:

| Herencia    | Significado                                          |
|-------------|------------------------------------------------------|
| `public`    | "Es-un" (lo normal). Convierte de derivada a base.   |
| `protected` | Herencia oculta para el exterior, visible a hijos.   |
| `private`   | "Está implementado en términos de". Casi composición.|

```cpp
class Pila : private std::vector<int> {       // implementada con vector
public:
    using std::vector<int>::push_back;        // pero solo expongo push/pop
    using std::vector<int>::pop_back;
    using std::vector<int>::back;
    using std::vector<int>::empty;
};
```

Para casi todo lo que viene en el curso, **herencia pública**. Solo era
para que sepan que existen las otras.

## 3. Polimorfismo dinámico

```cpp
void hacerHablar(const Animal& a) {
    a.hablar();    // llama al método correcto según el tipo real
}

int main() {
    Perro p;
    Gato g;
    hacerHablar(p);   // Guau
    hacerHablar(g);   // Miau

    std::vector<std::unique_ptr<Animal>> zoo;
    zoo.push_back(std::make_unique<Perro>());
    zoo.push_back(std::make_unique<Gato>());
    for (const auto& a : zoo) a->hablar();
}
```

Bajo el capó: cada clase con métodos virtuales tiene una **vtable**, y cada
objeto un puntero a ella. La llamada virtual es una indirección extra.

## 4. El problema del slicing (object slicing)

**Esto es un bug clásico y muy doloroso en C++.** Si pasas un objeto derivado
**por valor** a un parámetro de la clase base, lo que se copia es **solo la
parte base**. Pierdes todo lo añadido por la derivada.

```cpp
void usar(Animal a) { a.hablar(); }   // ❌ pasa por valor

int main() {
    Perro p;
    usar(p);   // ¡slicing! Copia solo la parte Animal.
               // De hecho ni compila, porque Animal es abstracta.
}
```

La forma correcta es **por referencia** (o por puntero):

```cpp
void usar(const Animal& a) { a.hablar(); }   // ✅
```

Lo mismo en contenedores: un `std::vector<Animal>` no puede contener
`Perro` y `Gato`. Hay que usar `std::vector<std::unique_ptr<Animal>>`.

Esto lo veremos otra vez en **Prototype**: el `clone()` virtual existe
precisamente para clonar el objeto **completo** sin sufrir slicing.

## 5. Composición

*"Tiene-un"* en lugar de *"es-un"*. Un objeto contiene a otros como miembros.

```cpp
class Motor {
public:
    void arrancar() { std::cout << "Brum\n"; }
};

class Coche {
    Motor motor;     // composición: el Coche TIENE un Motor
public:
    void arrancar() { motor.arrancar(); }
};
```

Ventajas frente a la herencia:

- **Menos acoplamiento**: el Coche no depende de la jerarquía del Motor.
- **Más flexible**: puedes cambiar de motor (eléctrico, diésel) sin tocar la
  jerarquía. Si el Motor fuera una interfaz, podrías cambiarlo en runtime.
- **No hay slicing.**
- **No hay problema del destructor virtual.**

## 6. Delegación

Composición + reenviar trabajo. La diferencia con la "composición pura" es
sutil: aquí el objeto **delega responsabilidades** a otro objeto miembro,
normalmente intercambiable.

```cpp
class Logger {
public:
    virtual ~Logger() = default;
    virtual void log(const std::string& msg) = 0;
};

class LoggerConsola : public Logger {
public:
    void log(const std::string& msg) override { std::cout << msg << "\n"; }
};

class LoggerFichero : public Logger {
public:
    void log(const std::string& msg) override { /* a fichero */ }
};

class Servicio {
    std::unique_ptr<Logger> logger;   // delegamos el log a otro objeto
public:
    explicit Servicio(std::unique_ptr<Logger> l) : logger(std::move(l)) {}

    void hacerAlgo() {
        logger->log("hice algo");   // delegación
    }
};
```

Decisivo: el `Servicio` no sabe **cómo** se loguea. Le da igual. Si mañana
queremos loguear a syslog, escribimos `LoggerSyslog` y lo inyectamos.

Esto **es** ya media inyección de dependencias y media **Strategy**.

## 7. Composición sobre herencia: el mantra

Heurística: ¿"X es un Y"? → herencia. ¿"X usa un Y"? → composición.

Pero en la práctica, **prefiere composición**. Razones:

- La herencia ata la jerarquía en tiempo de compilación. La composición
  permite cambiar el comportamiento en runtime.
- Las jerarquías profundas son frágiles: un cambio en la base se propaga a
  todo lo que cuelgue.
- "Composición + interfaces" da el polimorfismo que necesitas sin pagar el
  precio de la herencia.

Ejemplo del Paint que veremos:

```cpp
// ❌ Tentación: jerarquía profunda
class Shape { };
class ShapeRellenable : public Shape { };
class ShapeRellenableConBorde : public ShapeRellenable { };
class CirculoRellenableConBorde : public ShapeRellenableConBorde { };

// ✅ Composición: comportamiento como objetos miembro
class Shape {
    std::unique_ptr<EstrategiaRelleno> relleno;
    std::unique_ptr<EstrategiaBorde>   borde;
public:
    void dibujar() const {
        relleno->aplicar();
        borde->aplicar();
    }
};
```

El segundo permite cualquier combinación sin explotar la jerarquía. **Esto
es Strategy, y nace casi solo de aplicar este principio.**

## 8. Constructores especiales: regla de los 5 (recordatorio)

Ya vistos en el bloque 1, pero los recordamos en contexto de POO:

```cpp
class Recurso {
public:
    Recurso();                                  // por defecto
    ~Recurso();                                 // destructor
    Recurso(const Recurso&);                    // copia
    Recurso& operator=(const Recurso&);         // asignación por copia
    Recurso(Recurso&&) noexcept;                // move
    Recurso& operator=(Recurso&&) noexcept;     // asignación por move
};
```

Marcadores útiles:

```cpp
Recurso(const Recurso&) = delete;       // prohibir copia (ej. Singleton, unique_ptr)
Recurso() = default;                    // pedir el generado por el compilador
```

`= delete` lo veremos en Singleton para bloquear copia y asignación.

## 9. Conexión con los patrones que vienen

Hacer este puente explícito al final del bloque:

| Concepto visto hoy           | Patrón donde reaparece                            |
|------------------------------|---------------------------------------------------|
| Herencia + virtual           | Casi todos los patrones de comportamiento clásicos|
| Destructor virtual           | Factory, Prototype (devuelven `Base*`)            |
| Composición                  | Strategy, Bridge, State, Decorator                |
| Delegación                   | Proxy, Adapter, Command                           |
| Slicing                      | Prototype (`clone()` virtual)                     |
| Composición sobre herencia   | Decorador frente a explosión de subclases         |
| `unique_ptr<Base>`           | Cualquier contenedor polimórfico                  |
| Regla de los 5 / 0           | Implementación correcta de cualquier patrón       |

## 10. Ejercicio de cierre del día

Pequeño rediseño en vivo. Partimos de:

```cpp
// Diseño feo: herencia y casuística por tipo
class Shape {
public:
    enum Tipo { CIRCULO, CUADRADO };
    Tipo tipo;
    double param;
    void dibujar() const {
        if (tipo == CIRCULO)  std::cout << "Circulo " << param << "\n";
        if (tipo == CUADRADO) std::cout << "Cuadrado " << param << "\n";
    }
};
```

Refactor objetivo:

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

Discutir qué hemos ganado:

- Añadir `Triangulo` no requiere tocar `Shape` ni los métodos existentes
  (Open/Closed, que vemos mañana).
- No hay `switch`/`if` por tipo (Liskov / sustitución).
- El `vector` polimórfico funciona porque guardamos punteros, no objetos
  por valor (sin slicing).

**Cierre del día:** este `Shape` es la semilla del Paint que vamos a
construir durante todo el curso.

---

## Resumen del bloque

- Herencia pública: *"es-un"*. Destructor virtual siempre.
- Polimorfismo dinámico: `virtual` + `override` + punteros/referencias a la
  base.
- Slicing: pasar por valor mata el polimorfismo.
- Composición: *"tiene-un"*. Más flexible, menos acoplada.
- Delegación: composición + reenviar trabajo a un miembro intercambiable.
- **Composición sobre herencia** como regla por defecto.

**Mantra del bloque:**

> *"Heredas cuando no te queda más remedio. Compones siempre que puedes."*
