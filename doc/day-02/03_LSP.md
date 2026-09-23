# Día 2 — Bloque 3: LSP (Principio de sustitución de Liskov)

> *"Los objetos de un programa deben poder ser sustituidos por instancias
> de sus subtipos sin alterar el correcto funcionamiento del programa."*
> — Barbara Liskov

---

## 1. El problema en una frase

Heredas para reutilizar y, sin darte cuenta, **cambias el contrato** de la
clase base. Quien usa la base esperando un comportamiento, se encuentra con
otro distinto. **La herencia rota es peor que no heredar.**

## 2. Definición formal

> Si `S` es un subtipo de `T`, entonces los objetos de tipo `T` pueden ser
> sustituidos por objetos de tipo `S` sin que el programa deje de funcionar
> correctamente.

Traducido: **una subclase debe poder usarse en cualquier sitio donde se use
la base, sin sorpresas**. Si para usar `Cuadrado` tengo que saber que en
realidad es un `Cuadrado` y no un `Rectangulo`, LSP está roto.

LSP es el principio **más sutil de SOLID**. No habla de sintaxis (eso lo
fuerza el compilador) sino de **semántica del contrato**.

## 3. El ejemplo clásico: cuadrado/rectángulo

Tentación matemática: *"un cuadrado es un rectángulo, ¿no?"* Aritméticamente
sí. Pero en programación, **no necesariamente**.

### Bad

```cpp
class Rectangulo {
protected:
    double ancho, alto;
public:
    Rectangulo(double a, double h) : ancho(a), alto(h) {}
    virtual void setAncho(double a) { ancho = a; }
    virtual void setAlto(double h)  { alto = h; }
    double getAncho() const { return ancho; }
    double getAlto()  const { return alto;  }
    double area() const { return ancho * alto; }
};

class Cuadrado : public Rectangulo {
public:
    explicit Cuadrado(double lado) : Rectangulo(lado, lado) {}

    // Para mantener la invariante "ancho == alto", sobrescribimos los setters:
    void setAncho(double a) override { ancho = a; alto = a; }
    void setAlto(double h)  override { ancho = h; alto = h; }
};
```

A primera vista parece razonable. Pero ahora alguien escribe **una función
que usa `Rectangulo`** asumiendo el contrato natural de un rectángulo:

```cpp
void duplicarAncho(Rectangulo& r) {
    double altoOriginal = r.getAlto();
    r.setAncho(r.getAncho() * 2);
    assert(r.getAlto() == altoOriginal);   // ❌ falla si r es Cuadrado
}
```

Si le pasas un `Cuadrado`, el `assert` falla. El subtipo ha **roto el
contrato** que la base prometía implícitamente: *"cambiar el ancho no
cambia el alto"*.

### ¿Qué duele?

- El polimorfismo se vuelve una **trampa**: quien recibe `Rectangulo&` no
  sabe si en realidad es un cuadrado con setters acoplados.
- El código cliente tiene que **chequear el tipo real**, lo cual destroza
  el polimorfismo.
- Los tests del `Rectangulo` no detectan el problema, porque solo se
  rompen *al sustituir*.

### Good

La solución no es *"arreglar la herencia"*, es **no heredar así**. Si
`Cuadrado` y `Rectangulo` tienen contratos distintos, son tipos distintos:

```cpp
class Figura {
public:
    virtual ~Figura() = default;
    virtual double area() const = 0;
};

class Rectangulo : public Figura {
    double ancho, alto;
public:
    Rectangulo(double a, double h) : ancho(a), alto(h) {}
    void setAncho(double a) { ancho = a; }
    void setAlto(double h)  { alto = h; }
    double area() const override { return ancho * alto; }
};

class Cuadrado : public Figura {
    double lado;
public:
    explicit Cuadrado(double l) : lado(l) {}
    void setLado(double l) { lado = l; }
    double area() const override { return lado * lado; }
};
```

Ahora la jerarquía es plana: ambos heredan de `Figura`, y cada uno tiene
**su propio contrato**, coherente con el dominio. Nadie sustituye a nadie
de forma engañosa.

## 4. Bad/Good número 2: el pájaro que no vuela

Otro clásico. *"Un pingüino es un pájaro, pero no vuela."*

### Bad

```cpp
class Pajaro {
public:
    virtual ~Pajaro() = default;
    virtual void volar() { std::cout << "Volando\n"; }
};

class Aguila : public Pajaro {
public:
    void volar() override { std::cout << "Surcando los cielos\n"; }
};

class Pinguino : public Pajaro {
public:
    void volar() override {
        throw std::logic_error("Los pingüinos no vuelan");   // ❌
    }
};
```

Función que confía en la base:

```cpp
void hacerVolarBandada(const std::vector<std::unique_ptr<Pajaro>>& bandada) {
    for (auto& p : bandada) p->volar();   // explota si hay un pingüino
}
```

El subtipo **lanza una excepción inesperada** que la clase base no anunciaba.
Eso es violar el contrato. Es exactamente el tipo de cosa que Liskov prohíbe.

### Good

Si no todos los pájaros vuelan, **volar no pertenece a `Pajaro`**. Pertenece
a un subtipo intermedio, o mejor todavía, a una interfaz aparte:

```cpp
class Pajaro {
public:
    virtual ~Pajaro() = default;
    virtual void comer() = 0;
};

class Volador {
public:
    virtual ~Volador() = default;
    virtual void volar() = 0;
};

class Aguila : public Pajaro, public Volador {
public:
    void comer() override { std::cout << "Aguila come\n"; }
    void volar() override { std::cout << "Aguila vuela\n"; }
};

class Pinguino : public Pajaro {
public:
    void comer() override { std::cout << "Pinguino come\n"; }
    // No es Volador, no implementa volar. No puede ser llamado a volar.
};
```

Ahora `hacerVolarBandada` recibe `std::vector<Volador*>`, no `Pajaro*`. El
compilador te impide pasar un pingüino. **El error se detecta en compilación,
no en runtime con una excepción rara.**

Esto, además, es **ISP** (lo veremos en el siguiente bloque).

## 5. Reglas concretas para no violar LSP

Listado práctico para detectar violaciones en code review:

1. **Una subclase no puede lanzar excepciones nuevas** que la base no anuncie.
2. **Una subclase no puede pedir precondiciones más estrictas**. Si la base
   acepta cualquier `int`, la derivada no puede exigir solo positivos.
3. **Una subclase no puede debilitar postcondiciones**. Si la base promete
   devolver una lista no vacía, la derivada tampoco puede devolverla vacía.
4. **Una subclase no puede romper invariantes** de la base (caso cuadrado/rectángulo).
5. **Una subclase no debería devolver `nullptr`** donde la base devuelve objeto.
6. **Si tienes que hacer `dynamic_cast` para usar correctamente la jerarquía,
   probablemente has roto LSP.**

## 6. Señal de alarma muy típica

Cuando aparece esto en el código:

```cpp
if (auto p = dynamic_cast<Pinguino*>(pajaro.get())) {
    // tratamiento especial
} else {
    pajaro->volar();
}
```

Eso es el síntoma. La base prometía que `volar()` funcionaba para todos
los `Pajaro`, pero **el cliente está obligado a distinguir subtipos**. La
herencia ahí no está aportando: está estorbando.

## 7. Conexión con patrones

LSP no genera tantos patrones nuevos como OCP, pero sí condiciona **cómo
diseñas las jerarquías** de la mayoría:

| Patrón                | Dónde LSP es crítico                              |
|-----------------------|---------------------------------------------------|
| **Strategy**          | Todas las estrategias deben cumplir el mismo contrato |
| **Template Method**   | Los `hook` de las subclases no pueden romper la lógica de la base |
| **Decorator**         | El decorador debe ser intercambiable con el objeto decorado |
| **Composite**         | Hoja y compuesto comparten interfaz, deben ser sustituibles |
| **Adapter**           | El adaptador debe respetar el contrato del adaptado |

Cuando veas una jerarquía bien diseñada, **siempre cumple Liskov**. Cuando
veas una clase con `if (instanceof X)` o `dynamic_cast` para casos
especiales, **siempre lo viola**.

## 8. Mini-ejercicio mental (3 min)

```cpp
class Coleccion {
public:
    virtual ~Coleccion() = default;
    virtual void añadir(int x) = 0;
    virtual int  tamaño() const = 0;
};

class ColeccionInmutable : public Coleccion {
public:
    void añadir(int) override {
        throw std::logic_error("Inmutable");   // ❌
    }
    int  tamaño() const override { return 0; }
};
```

¿Qué viola? ¿Cómo lo arreglas?

(Respuesta breve: viola LSP, igual que el pingüino. Solución: separar
`Coleccion` (solo lectura) de `ColeccionMutable` (con `añadir`). La inmutable
hereda de la primera, la mutable de la segunda. ISP otra vez asomando.)

---

## Resumen del bloque

- LSP = **una subclase debe poder sustituir a la base sin romper el programa**.
- Es **el principio más semántico** de SOLID: no lo fuerza el compilador.
- Síntomas de violación: excepciones nuevas, `dynamic_cast` para casos
  especiales, precondiciones más estrictas, invariantes rotas.
- *"Cuadrado es un rectángulo"* y *"Pingüino es un pájaro"* son las dos
  trampas clásicas: matemáticamente sí, en código no.
- Cuando LSP falla, normalmente la respuesta es **no heredar** y elegir
  composición o jerarquía plana.

**Mantra del bloque:**

> *"Si para usar bien una clase derivada necesitas saber que es derivada,
> la herencia está rota."*
