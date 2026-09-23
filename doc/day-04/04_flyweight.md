# Flyweight

## 1. El problema

El usuario del Paint pinta una nube de puntos: un mapa con cien mil
píxeles, cada uno representado como un `Circulo` minúsculo. O un campo
de estrellas. O las hojas de un árbol fractal.

```cpp
for (int i = 0; i < 100'000; ++i) {
    auto c = std::make_unique<Circulo>(/* radio */ 1.0);
    c->setPosicion(rand() % 1000, rand() % 1000);
    c->setColor(Color::Negro);
    Lienzo::instancia().anadir(std::move(c));
}
```

Cien mil `Circulo`. Si cada uno son 40 bytes (radio, color, posición,
vptr, etc.), son **4 MB de objetos** para representar algo que en
esencia es: "un círculo de radio 1, color negro, repetido en 100.000
posiciones distintas".

La mayoría de los atributos son **idénticos** entre objetos. Solo la
posición varía. Estamos duplicando lo que no varía.

## 2. Intención (GoF)

> *Usar compartición para soportar grandes cantidades de objetos de
> grano fino de manera eficiente.*

La clave: separar lo que es **intrínseco** (igual en todos los
objetos, compartible) de lo **extrínseco** (varía por instancia, no
compartible). Los Flyweights guardan solo lo intrínseco; lo extrínseco
se pasa como parámetro al usarlos.

## 3. Bad: cada círculo carga con todo

```cpp
class Circulo {
    double radio;
    Color  color;
    double x, y;        // posición
    Textura textura;    // pesada
    Estilo  estilo;     // borde, grosor, opacidad
public:
    Circulo(double r, Color c, double px, double py, Textura t, Estilo e)
        : radio(r), color(c), x(px), y(py), textura(t), estilo(e) {}
    void dibujar() const { /* ... */ }
};
```

Cien mil instancias, cada una con su copia de `Textura` y `Estilo`.
Si la textura ocupa 1KB, son **100 MB solo de texturas duplicadas**.

## 4. Good: separar intrínseco y extrínseco

Primero, qué es qué:

- **Intrínseco** (compartible): radio, color, textura, estilo. No
  cambian entre los 100.000 círculos.
- **Extrínseco** (por instancia): posición x, y. Estas sí cambian.

El Flyweight guarda solo lo intrínseco:

```cpp
class CirculoFlyweight {
    double  radio;
    Color   color;
    Textura textura;
    Estilo  estilo;
public:
    CirculoFlyweight(double r, Color c, Textura t, Estilo e)
        : radio(r), color(c), textura(std::move(t)), estilo(std::move(e)) {}

    void dibujarEn(double x, double y) const {
        // pinta usando x, y (extrínseco) + sus atributos (intrínseco)
    }
};
```

Una fábrica con caché garantiza que **dos peticiones con los mismos
parámetros devuelven el mismo objeto**:

```cpp
class FabricaCirculos {
    struct Clave {
        double radio; Color color; /* ... */
        bool operator<(const Clave& o) const { /* ... */ }
    };
    std::map<Clave, std::shared_ptr<CirculoFlyweight>> cache;
public:
    std::shared_ptr<CirculoFlyweight> obtener(double r, Color c, Textura t, Estilo e) {
        Clave k{r, c};
        auto it = cache.find(k);
        if (it != cache.end()) return it->second;
        auto fw = std::make_shared<CirculoFlyweight>(r, c, std::move(t), std::move(e));
        cache[k] = fw;
        return fw;
    }
};
```

Y el cliente tiene "instancias ligeras" que solo guardan lo extrínseco
y una referencia al flyweight:

```cpp
class CirculoEnEscena : public Shape {
    std::shared_ptr<CirculoFlyweight> fw;   // intrínseco compartido
    double x, y;                            // extrínseco
public:
    CirculoEnEscena(std::shared_ptr<CirculoFlyweight> f, double px, double py)
        : fw(std::move(f)), x(px), y(py) {}

    void dibujar() const override { fw->dibujarEn(x, y); }
    double area() const override   { /* ... */ return 0.0; }
};
```

Uso:

```cpp
FabricaCirculos fab;
auto fw = fab.obtener(1.0, Color::Negro, /* textura */ {}, /* estilo */ {});

for (int i = 0; i < 100'000; ++i) {
    Lienzo::instancia().anadir(
        std::make_unique<CirculoEnEscena>(fw, rand() % 1000, rand() % 1000));
}
```

Cien mil `CirculoEnEscena` (16-24 bytes cada uno) + **un único**
`CirculoFlyweight` (con su textura, su estilo, su color).
Antes: 100 MB. Ahora: ~2 MB.

## 5. UML rápido

```
       Shape
        ▲
        │
  CirculoEnEscena ──► CirculoFlyweight   ◄──── FabricaCirculos
  (extrínseco: x,y)   (intrínseco)              (caché)
```

## 6. Variantes y trampas

- **¿Qué es intrínseco?** Lo que no cambia entre instancias y se puede
  compartir sin riesgo. Si dudas, mira: ¿dos clientes que reciben el
  mismo flyweight podrían pisarse? Si sí, no es intrínseco.
- **El flyweight debe ser inmutable.** Si lo dejas mutable y dos
  clientes lo comparten, un cliente cambia algo y al otro le explota.
  Esto es la regla más importante del patrón.
- **Caché vs reuso eterno.** La fábrica con `shared_ptr` reutiliza,
  pero también podrías usar `weak_ptr` para que la caché libere los
  flyweights que ya nadie usa. Decisión de dominio.
- **No flyweight prematuro.** Si el ahorro de memoria es despreciable,
  no metas la complejidad. Flyweight tiene sentido cuando hay
  **miles** de instancias casi idénticas.

## 7. Flyweight y los días anteriores

- **Prototype** clona para crear muchos objetos parecidos. Flyweight
  va más allá: **no clona, comparte**. Si los clones serían idénticos,
  flyweight es la versión "ahorradora".
- **Singleton** tiene una instancia única. Flyweight tiene **pocas
  instancias compartidas**. Es Singleton en multiplicidad controlada.
- **Factory** crea bajo demanda. Flyweight crea bajo demanda **y
  recuerda**. La fábrica de flyweights tiene caché siempre.

## 8. SOLID

| Principio | Cómo lo cumple Flyweight |
|-----------|--------------------------|
| SRP       | Flyweight guarda intrínseco; cliente guarda extrínseco. |
| OCP       | Añadir tipos nuevos de flyweight no afecta a los existentes. |
| LSP       | Cumplido: el flyweight implementa la interfaz esperada. |
| ISP       | Cumplido: la interfaz del flyweight es la mínima. |
| DIP       | El cliente depende del flyweight abstracto. |

## 9. Mantra

> *"Si dos objetos son iguales en todo menos en su posición, no son
> dos objetos. Son uno repetido."*
