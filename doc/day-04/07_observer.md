# Observer ⭐ (prometido el día 2)

## 1. El problema

Volvemos al `TODO` que llevamos arrastrando desde el día 3:

```cpp
void Lienzo::anadir(std::unique_ptr<Shape> s) {
    figuras.push_back(std::move(s));
    // TODO día 5 (Observer): notificar a los suscriptores
}
```

El Paint tiene varias vistas que deben enterarse cuando el lienzo
cambia:

- La **barra de herramientas** que muestra el contador de figuras.
- El **panel de capas** que lista las figuras una a una.
- El **mini-mapa** que renderiza el lienzo en pequeño.
- El **botón de guardar** que se enciende cuando hay cambios sin
  guardar.

Cuatro componentes que dependen del estado del lienzo. Y mañana habrá
un quinto.

Opción ingenua: el `Lienzo` llama a cada uno por su nombre:

```cpp
void Lienzo::anadir(std::unique_ptr<Shape> s) {
    figuras.push_back(std::move(s));
    barraHerramientas.actualizar();
    panelCapas.actualizar();
    miniMapa.actualizar();
    botonGuardar.marcarSucio();
}
```

DIP roto: el Lienzo (modelo) conoce a sus vistas. OCP roto: añadir una
vista quinta toca el código del Lienzo.

## 2. Intención (GoF)

> *Definir una dependencia uno-a-muchos entre objetos, de manera que
> cuando uno cambia de estado, todos los que dependen de él son
> notificados y actualizados automáticamente.*

El sujeto no conoce a las vistas; conoce una **lista de observadores
abstractos**. Las vistas se registran y se desregistran.

## 3. Bad: el sujeto sabe quién mira

(El código del párrafo anterior, en cualquiera de sus variantes.)

Si la barra cambia de nombre, hay que tocar el lienzo. Si llega una
vista nueva, hay que tocar el lienzo. Si una vista se quita en runtime,
no se puede.

## 4. Good: Observer

Interfaces:

```cpp
class IObservadorLienzo {
public:
    virtual ~IObservadorLienzo() = default;
    virtual void onLienzoCambio() = 0;
};

class ISujetoLienzo {
public:
    virtual ~ISujetoLienzo() = default;
    virtual void suscribir(IObservadorLienzo* obs)   = 0;
    virtual void desuscribir(IObservadorLienzo* obs) = 0;
};
```

El `Lienzo` ahora notifica a una lista abstracta:

```cpp
class Lienzo : public ISujetoLienzo {
public:
    static Lienzo& instancia() {
        static Lienzo unica;
        return unica;
    }

    void anadir(std::unique_ptr<Shape> s) {
        figuras.push_back(std::move(s));
        notificar();
    }
    void limpiar() {
        figuras.clear();
        notificar();
    }

    void suscribir(IObservadorLienzo* obs) override {
        observadores.push_back(obs);
    }
    void desuscribir(IObservadorLienzo* obs) override {
        observadores.erase(
            std::remove(observadores.begin(), observadores.end(), obs),
            observadores.end());
    }

private:
    void notificar() {
        for (auto* obs : observadores) obs->onLienzoCambio();
    }

    std::vector<std::unique_ptr<Shape>>  figuras;
    std::vector<IObservadorLienzo*>      observadores;

    Lienzo() = default;
    // ... rule of five borrada como el día 3 ...
};
```

Vistas concretas:

```cpp
class BarraHerramientas : public IObservadorLienzo {
public:
    BarraHerramientas() { Lienzo::instancia().suscribir(this); }
    ~BarraHerramientas() { Lienzo::instancia().desuscribir(this); }
    void onLienzoCambio() override {
        std::cout << "[barra] hay " << Lienzo::instancia().numFiguras() << " figuras\n";
    }
};

class PanelCapas : public IObservadorLienzo {
public:
    PanelCapas() { Lienzo::instancia().suscribir(this); }
    ~PanelCapas() { Lienzo::instancia().desuscribir(this); }
    void onLienzoCambio() override {
        std::cout << "[panel] capas actualizadas\n";
    }
};
```

Uso:

```cpp
BarraHerramientas barra;
PanelCapas        panel;

Lienzo::instancia().anadir(std::make_unique<Circulo>(3.0));
// [barra] hay 1 figuras
// [panel] capas actualizadas
```

Añadir una vista nueva: una clase nueva. Cero cambios en `Lienzo`.

## 5. UML rápido

```
ISujetoLienzo  ◇──────► IObservadorLienzo
     ▲                          ▲
     │                          │
   Lienzo               BarraHerramientas, PanelCapas, MiniMapa
```

## 6. Variantes y trampas

- **Push vs pull**: el sujeto puede mandar **datos** del cambio
  (`onCambio(Shape* nueva)`) o solo **avisar** y que el observador
  pregunte. Push es más eficiente cuando hay un dato concreto; pull
  es más flexible. En el Paint hacemos pull: el observador consulta
  `Lienzo::numFiguras()`.
- **Múltiples eventos**: si el lienzo tiene varios tipos de cambio
  (figura añadida, figura movida, fondo cambiado), un solo
  `onCambio()` se queda corto. Dos opciones: o varios métodos
  (`onAnadida`, `onMovida`...) en la interfaz, o un parámetro de
  evento. La de varios métodos respeta ISP; la del parámetro evita
  explotar la interfaz. Decisión de dominio.
- **Re-entrada**: si el observador, en su `onLienzoCambio()`, modifica
  el lienzo, se reentra. Si modificas el vector de observadores
  durante la iteración, **boom**. Soluciones: copia el vector antes
  de iterar; o usa una cola de notificaciones pendientes.
- **Lifetime**: el observador se desuscribe en su destructor. Si no
  lo hace, el lienzo guarda un puntero colgante y al siguiente
  `notificar()` revienta. Usa RAII o `weak_ptr`.
- **C++ moderno con std::function**: el observer puede ser una
  función registrada, no una clase. Suele ser más conveniente para
  casos simples. Pero pierdes la disciplina del `desuscribir` por
  destructor, así que ojo con el lifetime.

## 7. Observer y los `TODO` del día 3

Aquí se cierra el otro hilo del Singleton. El `TODO` de `Lienzo::anadir`
y `Lienzo::limpiar` queda **resuelto**: las dos llamadas notifican a
sus observadores, sin saber quiénes son. Singleton + Observer encajan
sin pisarse: el Singleton garantiza un único punto de notificación; el
Observer evita que ese único punto conozca a sus suscriptores.

## 8. Observer y Command

Las dos piezas se combinan: la GUI no llama al `Lienzo`, llama al
`GestorComandos`, que ejecuta el `Command`, que llama al `Lienzo`, que
notifica a los observadores. Cada eslabón hace una cosa.

```
GUI ──► GestorComandos ──► Command ──► Lienzo ──► notifica ──► Observers
```

Cinco eslabones, sí, pero cada uno tiene una responsabilidad clara, y
cada uno se puede testar por separado.

## 9. SOLID

| Principio | Cómo lo cumple Observer |
|-----------|------------------------|
| SRP       | El sujeto no sabe pintar barras; las barras se pintan solas. |
| OCP       | Nueva vista = nueva clase. Cero ediciones en el sujeto. |
| LSP       | Todos los observadores cumplen la misma interfaz. |
| ISP       | La interfaz observador tiene los métodos justos. |
| DIP       | El sujeto depende de la interfaz abstracta de observador. |

## 10. Mantra

> *"El que cambia no conoce a los que miran. Solo grita al vacío. Y los
> que miran se apuntan al vacío."*
