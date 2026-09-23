# Command ⭐ (prometido el día 2)

## 1. El problema

El día 3, en el Singleton, dejamos dos `TODO` explícitos:

```cpp
void Lienzo::anadir(std::unique_ptr<Shape> s) {
    figuras.push_back(std::move(s));
    // TODO día 5 (Observer): notificar a los suscriptores...
}
```

Y también dijimos que las acciones del usuario no debían llamar a
`anadir` directamente desde la GUI, sino canalizarse para implementar
**deshacer/rehacer**.

Hoy rellenamos ese hueco. La GUI no toca el `Lienzo`. La GUI **crea
Comandos** y se los entrega a un gestor.

Ejemplo del dolor sin Command:

```cpp
// GUI, evento "click en barra → añadir círculo"
void onAnadirCirculoClick() {
    Lienzo::instancia().anadir(std::make_unique<Circulo>(3.0));
}

// GUI, evento "Ctrl+Z"
void onCtrlZ() {
    // ¿cómo deshacer? Tengo que saber qué se hizo, en qué orden, y revertirlo.
    // ¿guardo el historial yo en la GUI? Pero hay varios eventos que tocan el
    // lienzo... ¿qué pasa si dos paneles distintos modifican el mismo lienzo?
}
```

El problema no es técnico: es **arquitectónico**. La GUI no debe llevar
contabilidad de la historia. Y la historia no es del Lienzo: el Lienzo
solo tiene su estado actual.

## 2. Intención (GoF)

> *Encapsular una petición como un objeto, permitiendo así parametrizar
> clientes con distintas peticiones, encolar o registrar peticiones, y
> soportar operaciones que se pueden deshacer.*

Cada acción es **un objeto**. Tiene un método `ejecutar()` y un método
`deshacer()`. Un gestor las apila.

## 3. Bad: la GUI gestiona el historial

```cpp
class GUI {
    std::vector<std::string> historial;
public:
    void onAnadirCirculo() {
        Lienzo::instancia().anadir(std::make_unique<Circulo>(3.0));
        historial.push_back("anadir circulo");
    }
    void onLimpiar() {
        Lienzo::instancia().limpiar();
        historial.push_back("limpiar");
    }
    void onUndo() {
        if (historial.empty()) return;
        auto ultima = historial.back();
        historial.pop_back();
        // ahora interpreto la string...
        if (ultima == "anadir circulo") {
            // ¿quito el último? ¿y si han metido otros entre medias?
        } else if (ultima == "limpiar") {
            // ¿restauro el lienzo? ¿con qué estado?
        }
    }
};
```

Strings, interpretación, casos especiales. Inviable.

## 4. Good: Command

Una interfaz pequeña:

```cpp
class Command {
public:
    virtual ~Command() = default;
    virtual void ejecutar() = 0;
    virtual void deshacer() = 0;
};
```

Un comando por acción:

```cpp
class AnadirFiguraCommand : public Command {
    std::unique_ptr<Shape> figura;
    Shape* puntero = nullptr;   // para localizarla al deshacer

public:
    explicit AnadirFiguraCommand(std::unique_ptr<Shape> s)
        : figura(std::move(s)) {}

    void ejecutar() override {
        puntero = figura.get();
        Lienzo::instancia().anadir(std::move(figura));
    }
    void deshacer() override {
        figura = Lienzo::instancia().extraer(puntero);  // método nuevo en Lienzo
    }
};

class LimpiarLienzoCommand : public Command {
    std::vector<std::unique_ptr<Shape>> respaldo;

public:
    void ejecutar() override {
        respaldo = Lienzo::instancia().vaciar();  // devuelve y limpia
    }
    void deshacer() override {
        Lienzo::instancia().restaurar(std::move(respaldo));
    }
};
```

El `Lienzo` necesita métodos auxiliares (`extraer`, `vaciar`,
`restaurar`) que antes no tenía. No los inventamos por capricho: los
necesitamos porque la responsabilidad de **deshacer** está en el
comando, y el comando necesita herramientas para devolverle al lienzo
el estado anterior.

El gestor de historial:

```cpp
class GestorComandos {
    std::vector<std::unique_ptr<Command>> hechos;
    std::vector<std::unique_ptr<Command>> deshechos;

public:
    void ejecutar(std::unique_ptr<Command> c) {
        c->ejecutar();
        hechos.push_back(std::move(c));
        deshechos.clear();   // se invalidan al hacer algo nuevo
    }

    void deshacer() {
        if (hechos.empty()) return;
        auto c = std::move(hechos.back());
        hechos.pop_back();
        c->deshacer();
        deshechos.push_back(std::move(c));
    }

    void rehacer() {
        if (deshechos.empty()) return;
        auto c = std::move(deshechos.back());
        deshechos.pop_back();
        c->ejecutar();
        hechos.push_back(std::move(c));
    }
};
```

Y la GUI, ahora, no toca el `Lienzo`:

```cpp
GestorComandos gestor;

void onAnadirCirculo() {
    gestor.ejecutar(std::make_unique<AnadirFiguraCommand>(
        std::make_unique<Circulo>(3.0)));
}
void onLimpiar() { gestor.ejecutar(std::make_unique<LimpiarLienzoCommand>()); }
void onCtrlZ()   { gestor.deshacer(); }
void onCtrlY()   { gestor.rehacer(); }
```

**Deshacer y rehacer funcionan**. Sin strings, sin casos especiales.
Cada comando sabe deshacerse a sí mismo.

## 5. UML rápido

```
Cliente (GUI) ──► GestorComandos ◇──► Command (interfaz)
                                          ▲
                                          │
                              AnadirFiguraCommand
                              LimpiarLienzoCommand
                              MoverFiguraCommand
                              ...
```

## 6. Variantes y trampas

- **Comandos compuestos** (macro-commands): un `Command` que contiene
  una lista de `Command` y los ejecuta en orden. Útil para "duplicar
  selección" cuando la selección son varias figuras.
- **Comandos con parámetros tardíos**: el `Command` recibe la
  configuración en su constructor, no al ejecutar. Eso permite
  encolarlos, serializarlos, programarlos.
- **Comandos no reversibles**: imprimir, exportar a PDF. No tienen
  `deshacer()` razonable. Decisión: o se separan en otra interfaz, o
  `deshacer()` queda como no-op. Los GoF aceptan ambas; pragmáticamente
  el no-op es lo más común.
- **Memory leak silencioso**: si el `Command` guarda la figura por
  puntero crudo y la figura ya se destruyó (porque otro comando lo
  hizo), `puntero` cuelga. Usa `weak_ptr` o haz que el comando se
  invalide cuando esto ocurra.
- **Command + std::function**: para comandos triviales sin estado,
  `std::function<void()>` y otra para deshacer puede ser suficiente.
  No te obsesiones con la jerarquía si no aporta.

## 7. Command y los `TODO` del día 3

Aquí se rellena la otra mitad de lo que dejamos pendiente. Recordemos
el código de ayer:

```cpp
void Lienzo::anadir(std::unique_ptr<Shape> s) {
    figuras.push_back(std::move(s));
    // TODO día 5: notificar
}
```

Ese método ya no se llama desde la GUI: se llama **desde el Command**.
Y la notificación (Observer) la dispara el método. La GUI quedó
desacoplada del Lienzo, y el Lienzo notifica sin saber a quién.

## 8. SOLID

| Principio | Cómo lo cumple Command |
|-----------|-----------------------|
| SRP       | Cada comando una acción concreta. |
| OCP       | Nuevo tipo de acción = nuevo Command, sin tocar el gestor. |
| LSP       | Todos los Command son intercambiables. |
| ISP       | La interfaz Command es mínima (`ejecutar`/`deshacer`). |
| DIP       | El gestor depende de `Command`, no de cada acción concreta. |

## 9. Mantra

> *"Si quieres deshacer una acción, primero convierte la acción en un
> objeto. Un verbo no se puede deshacer; un sustantivo sí."*
