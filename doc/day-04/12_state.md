# State

## 1. El problema

El editor del Paint tiene **modos**:

- **Selección**: el click selecciona figuras; arrastrar las mueve.
- **Dibujo**: el click empieza una figura nueva; arrastrar la dimensiona.
- **Borrado**: el click elimina la figura clicada.
- **Zoom**: el click amplía; arrastrar hace pan.

El comportamiento del **mismo evento** (un click, un arrastre, una
tecla) **depende del modo activo**. Lo ingenuo:

```cpp
class EditorPaint {
    enum class Modo { Seleccion, Dibujo, Borrado, Zoom } modo;
public:
    void onClick(int x, int y) {
        switch (modo) {
            case Modo::Seleccion: /* ... */ break;
            case Modo::Dibujo:    /* ... */ break;
            case Modo::Borrado:   /* ... */ break;
            case Modo::Zoom:      /* ... */ break;
        }
    }
    void onArrastrar(int x, int y) {
        switch (modo) {
            case Modo::Seleccion: /* ... */ break;
            case Modo::Dibujo:    /* ... */ break;
            // ...
        }
    }
    void onTecla(char k) { /* otro switch */ }
};
```

Tres `switch` que **siempre** mencionan los mismos modos. Cada modo
nuevo: tres ediciones. Cada evento nuevo: un switch nuevo en todas las
clases que tengan modos. Es lo mismo que en Strategy con `switch`, pero
agravado porque aquí el "algoritmo" depende del **estado interno**, no
de una elección externa.

Cuando lo vimos en Strategy resolvimos con una clase por algoritmo. State
hace lo mismo pero para los estados internos de un objeto.

## 2. Intención (GoF)

> *Permitir que un objeto altere su comportamiento cuando cambia su
> estado interno. El objeto parecerá cambiar de clase.*

Un estado, una clase. El editor delega cada evento al estado actual.

## 3. Bad: el switch que crece

(El código del párrafo anterior, repetido por cada evento.)

## 4. Good: State

```cpp
class EditorPaint;   // forward

class Estado {
public:
    virtual ~Estado() = default;
    virtual void onClick(EditorPaint& e, int x, int y) = 0;
    virtual void onArrastrar(EditorPaint& e, int x, int y) = 0;
    virtual void onTecla(EditorPaint& e, char k) = 0;
};

class EditorPaint {
    std::unique_ptr<Estado> estado;
public:
    explicit EditorPaint(std::unique_ptr<Estado> inicial)
        : estado(std::move(inicial)) {}

    void cambiarEstado(std::unique_ptr<Estado> nuevo) { estado = std::move(nuevo); }

    void onClick(int x, int y)     { estado->onClick(*this, x, y); }
    void onArrastrar(int x, int y) { estado->onArrastrar(*this, x, y); }
    void onTecla(char k)           { estado->onTecla(*this, k); }
};

class EstadoSeleccion : public Estado {
public:
    void onClick(EditorPaint&, int x, int y) override {
        std::cout << "[sel] click en (" << x << "," << y << ")\n";
    }
    void onArrastrar(EditorPaint&, int x, int y) override {
        std::cout << "[sel] arrastra a (" << x << "," << y << ")\n";
    }
    void onTecla(EditorPaint& e, char k) override {
        if (k == 'd') e.cambiarEstado(std::make_unique<class EstadoDibujo>());
    }
};

class EstadoDibujo : public Estado {
public:
    void onClick(EditorPaint&, int x, int y) override {
        std::cout << "[dibujo] nueva figura en (" << x << "," << y << ")\n";
    }
    void onArrastrar(EditorPaint&, int x, int y) override {
        std::cout << "[dibujo] redimensiona a (" << x << "," << y << ")\n";
    }
    void onTecla(EditorPaint& e, char k) override {
        if (k == 's') e.cambiarEstado(std::make_unique<EstadoSeleccion>());
    }
};
```

Uso:

```cpp
EditorPaint editor(std::make_unique<EstadoSeleccion>());

editor.onClick(10, 20);   // [sel] click...
editor.onTecla('d');      // pasa a EstadoDibujo
editor.onClick(30, 40);   // [dibujo] nueva figura...
```

Añadir un modo nuevo: una clase nueva, con tres métodos (cuatro si
añades un evento más). Cero ediciones en `EditorPaint` ni en los
demás estados.

## 5. UML rápido

```
EditorPaint ◇──► Estado
                  ▲
                  │
        EstadoSeleccion, EstadoDibujo, EstadoBorrado, EstadoZoom
```

## 6. Variantes y trampas

- **Quién decide la transición**: ¿el estado actual decide a qué
  pasar (como en el ejemplo), o decide el contexto (`EditorPaint`)?
  Si lo decide el estado, los estados se conocen entre sí (acoplados).
  Si lo decide el contexto, el contexto vuelve a tener un switch. La
  decisión es: **¿conoces el grafo de transiciones en un sitio o
  repartido?**. Para grafos simples, repartido (lo que hace el
  ejemplo). Para grafos complejos, una tabla de transiciones en el
  contexto.
- **Estados sin estado**: si un estado no tiene datos propios, puedes
  hacerlo singleton (un único `EstadoSeleccion` en toda la app).
  Ahorra memoria si hay muchos editores.
- **Estados con datos**: si el estado guarda información (puntos
  marcados, figura en construcción), conviene que cada editor tenga
  sus propios estados.
- **State vs Strategy**: estructuralmente idénticos, distintos en
  intención:
  - **Strategy**: el **cliente** elige el algoritmo y se lo pone.
  - **State**: el **objeto mismo** cambia su estado en función de los
    eventos que recibe.
  - Strategy es estable a lo largo del tiempo; State cambia
    activamente.
- **No abuses**: si solo tienes dos estados con dos eventos, un `bool`
  con un `if` es perfectamente legítimo. State paga renta cuando hay
  un grafo real con varios estados.

## 7. State y los `TODO` del día 2

En el día 2 dejamos caer que el editor del Paint sería un buen
ejemplo de máquina de estados. State es el patrón que lo formaliza.

## 8. SOLID

| Principio | Cómo lo cumple State |
|-----------|---------------------|
| SRP       | Cada estado, una manera de comportarse. |
| OCP       | Nuevo estado = clase nueva, sin tocar `EditorPaint`. |
| LSP       | Cualquier estado intercambiable como `Estado`. |
| ISP       | La interfaz de estado lleva solo los eventos relevantes. |
| DIP       | `EditorPaint` depende de la interfaz `Estado`. |

## 9. Mantra

> *"Si tu objeto se comporta distinto según el modo, no es un objeto
> con switch. Son varios objetos con la misma cara y el mismo nombre."*
