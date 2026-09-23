# Mediator

## 1. El problema

La GUI del Paint tiene varios componentes que se hablan entre sí:

- La **paleta de colores** cambia el color activo.
- La **barra de herramientas** activa modos (lápiz, borrador, zoom).
- El **panel de capas** selecciona una capa.
- El **selector de figuras** elige `Circulo`/`Cuadrado`/etc.

Cuando el usuario selecciona "borrador" en la barra, varias cosas
deben pasar a la vez: la paleta se pone en gris (no aplica color),
el cursor cambia, el selector de figuras se deshabilita, el panel de
capas resalta la capa activa.

Si cada componente conoce a los demás, sale **el grafo completo**:

```
paleta ──► barra
paleta ──► panel
paleta ──► selector
barra  ──► paleta
barra  ──► panel
barra  ──► selector
panel  ──► paleta
panel  ──► barra
panel  ──► selector
selector ──► paleta
selector ──► barra
selector ──► panel
```

N(N-1) conexiones. Y mañana llega el "panel de filtros": **N(N-1) + 2N**
nuevas líneas. Es lo que llamábamos en el día 1 *acoplamiento
geométrico*: crece más rápido que las piezas.

## 2. Intención (GoF)

> *Definir un objeto que encapsula cómo se comunica un conjunto de
> objetos. Mediator promueve el bajo acoplamiento evitando que los
> objetos se refieran unos a otros explícitamente, y permite variar
> su interacción de manera independiente.*

Una pieza nueva en el medio: el mediador. Los componentes no se
hablan entre sí; **le hablan al mediador**. El mediador decide a
quién avisar.

## 3. Bad: todos contra todos

```cpp
class Paleta;
class Barra;
class Panel;
class Selector;

class Barra {
    Paleta*  paleta;
    Panel*   panel;
    Selector* selector;
public:
    void activarBorrador() {
        paleta->desactivar();
        selector->desactivar();
        panel->resaltarCapaActiva();
        // ... y si añado un componente nuevo, lo añado aquí también ...
    }
};
```

Cada componente conoce a todos. Cualquier cambio en uno reverbera.

## 4. Good: Mediator

```cpp
class Mediador {
public:
    virtual ~Mediador() = default;
    virtual void notificar(const std::string& origen, const std::string& evento) = 0;
};

class Paleta {
    Mediador* med;
public:
    explicit Paleta(Mediador* m) : med(m) {}
    void clicarColor() { med->notificar("paleta", "color elegido"); }
    void desactivar() { /* ... */ }
};

class Barra {
    Mediador* med;
public:
    explicit Barra(Mediador* m) : med(m) {}
    void activarBorrador() { med->notificar("barra", "borrador activo"); }
    void activarLapiz()    { med->notificar("barra", "lapiz activo"); }
};

class Panel {
    Mediador* med;
public:
    explicit Panel(Mediador* m) : med(m) {}
    void resaltarCapaActiva() { /* ... */ }
};

class Selector {
    Mediador* med;
public:
    explicit Selector(Mediador* m) : med(m) {}
    void desactivar() { /* ... */ }
};

class MediadorPaint : public Mediador {
public:
    Paleta*  paleta   = nullptr;
    Barra*   barra    = nullptr;
    Panel*   panel    = nullptr;
    Selector* selector = nullptr;

    void notificar(const std::string& origen, const std::string& evento) override {
        if (origen == "barra" && evento == "borrador activo") {
            paleta->desactivar();
            selector->desactivar();
            panel->resaltarCapaActiva();
        } else if (origen == "barra" && evento == "lapiz activo") {
            paleta->desactivar();   // ejemplo
            // ...
        }
        // ... otras reglas ...
    }
};
```

Uso:

```cpp
MediadorPaint med;
Paleta   p(&med);
Barra    b(&med);
Panel    pa(&med);
Selector s(&med);

med.paleta = &p; med.barra = &b; med.panel = &pa; med.selector = &s;

b.activarBorrador();   // dispara las reacciones a través del mediador
```

Añadir un componente nuevo: **dos líneas**. Una en el mediador
(reglas que le afectan) y otra para construirlo y registrarlo.

## 5. UML rápido

```
              Mediador
               ▲   ▲
               │   │
   ┌─────┬─────┼───┴─┬─────┐
   │     │     │     │     │
Paleta Barra Panel Selector ...
```

Los componentes no se conocen entre sí. Solo conocen al mediador.

## 6. Variantes y trampas

- **El mediador como God Object**: el riesgo más obvio. El mediador
  centraliza tantas reglas que se vuelve enorme e ilegible. Si pasa,
  hay dos opciones: o trocear (varios mediadores especializados), o
  reconocer que ese trozo de lógica es realmente un **flujo de
  trabajo** y modelarlo como una máquina de estados (ver State).
- **Notificar con strings es frágil**: el ejemplo usa strings por
  claridad. En producción, mejor un `enum` por tipo de evento, o
  eventos como clases. Strings invitan a typos silenciosos.
- **Mediator vs Observer**: ambos descomponen comunicación
  todos-contra-todos. Diferencia:
  - **Observer**: el sujeto **grita al vacío**; cualquier observador
    se apunta. Comunicación 1→N.
  - **Mediator**: el mediador **decide a quién avisar**. Comunicación
    N↔N centralizada.
  - Si tu lógica es "el A cambió, los X y Z reaccionan", Observer.
  - Si tu lógica es "si A hace x y además B hace y, entonces hacer z
    en C", Mediator.
- **Observer + Mediator** combinados: el mediador puede ser observador
  de los componentes y a su vez sujeto de otros. Es válido y se ve.

## 7. Mediator y los días anteriores

- **Facade (estructural)** unifica acceso. Mediator (comportamental)
  unifica interacción. Facade es de fuera hacia dentro; Mediator es
  de dentro hacia dentro.
- **Observer** es Mediator descentralizado: si el sujeto solo notifica
  y los observadores deciden por sí mismos, es Observer; si hay una
  pieza central que coordina, es Mediator.

## 8. SOLID

| Principio | Cómo lo cumple Mediator |
|-----------|------------------------|
| SRP       | Cada componente lo suyo; el mediador, coordinar. |
| OCP       | Nuevo componente = registrarlo + nuevas reglas en el mediador. |
| LSP       | Cualquier mediador concreto cumple la interfaz. |
| ISP       | Los componentes exponen solo lo que el mediador llama. |
| DIP       | Los componentes dependen del mediador abstracto. |

## 9. Mantra

> *"Si N piezas se hablan todas con todas, mete una pieza más en el
> centro. Una conexión por pieza, no N-1."*
