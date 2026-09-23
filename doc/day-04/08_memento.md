# Memento ⭐ (prometido el día 2)

## 1. El problema

Con Command tenemos deshacer/rehacer **por acciones**. Pero hay casos
donde eso no basta:

- El usuario quiere **snapshots manuales**: pulsa "guardar punto",
  hace experimentos, y vuelve a un estado anterior con un clic. No es
  deshacer paso a paso: es "vuelve a como estaba".
- Hay acciones **no encapsuladas como Command** (un script automático,
  un import masivo) que cambian el lienzo. Si solo tenemos Command, no
  podemos volver atrás.
- Queremos guardar el estado completo del lienzo en disco para reabrir
  un proyecto. Necesitamos extraerlo de manera ordenada.

Necesitamos una forma de **capturar el estado del lienzo en un objeto
opaco**, guardarlo, y restaurarlo cuando haga falta.

Lo ingenuo es exponer las tripas:

```cpp
auto figuras = lienzo.figurasInternas();   // ← rompe encapsulación
// ... usar figuras ...
lienzo.setFiguras(std::move(figuras));     // ← y la sigue rompiendo
```

Quien tenga acceso a `figurasInternas()` puede hacer cualquier cosa con
la memoria del lienzo. Mal.

## 2. Intención (GoF)

> *Sin violar la encapsulación, capturar y externalizar el estado
> interno de un objeto, de modo que pueda restaurarse después.*

La clave: el objeto **se encarga de crear su propio memento**, y solo
**él** sabe cómo restaurarse desde un memento. El cliente solo lo
guarda y lo devuelve. No mira dentro.

## 3. Bad: exponer las tripas

```cpp
class Lienzo {
public:
    std::vector<std::unique_ptr<Shape>>& acceso() { return figuras; }  // ←
private:
    std::vector<std::unique_ptr<Shape>> figuras;
};

// cliente:
auto& f = Lienzo::instancia().acceso();
auto copia = /* clonar todas las figuras una por una */;
// ... más tarde ...
Lienzo::instancia().acceso() = std::move(copia);
```

El cliente decide la representación, el cliente clona, el cliente
restaura. Si mañana el `Lienzo` cambia su representación interna (un
`std::list`, un árbol, lo que sea), **todo el código cliente rompe**.

## 4. Good: Memento

El `Lienzo` genera y consume sus propios mementos. El cliente no sabe
qué hay dentro.

```cpp
class LienzoMemento {
    std::vector<std::unique_ptr<Shape>> snapshot;

    // El lienzo es amigo: solo él construye y abre mementos.
    explicit LienzoMemento(std::vector<std::unique_ptr<Shape>> s)
        : snapshot(std::move(s)) {}

    friend class Lienzo;
};

class Lienzo {
public:
    static Lienzo& instancia() {
        static Lienzo unica;
        return unica;
    }

    // Crear memento: clona todas las figuras (usamos el Prototype del día 3).
    std::unique_ptr<LienzoMemento> crearMemento() const {
        std::vector<std::unique_ptr<Shape>> copia;
        copia.reserve(figuras.size());
        for (const auto& f : figuras) copia.push_back(f->clone());
        return std::unique_ptr<LienzoMemento>(new LienzoMemento(std::move(copia)));
    }

    // Restaurar: tomar las figuras del memento y volver a notificar.
    void restaurar(std::unique_ptr<LienzoMemento> m) {
        figuras = std::move(m->snapshot);
        notificar();   // los observadores también se enteran
    }

    void anadir(std::unique_ptr<Shape> s) {
        figuras.push_back(std::move(s));
        notificar();
    }
    // ... resto del Lienzo igual que en día 4 con Observer ...

private:
    std::vector<std::unique_ptr<Shape>>  figuras;
    std::vector<IObservadorLienzo*>      observadores;
    void notificar() { for (auto* o : observadores) o->onLienzoCambio(); }

    Lienzo() = default;
};
```

Uso:

```cpp
// Snapshot manual
auto punto1 = Lienzo::instancia().crearMemento();

Lienzo::instancia().anadir(std::make_unique<Circulo>(3.0));
Lienzo::instancia().anadir(std::make_unique<Cuadrado>(2.0));

// ... el usuario se arrepiente ...
Lienzo::instancia().restaurar(std::move(punto1));
// el lienzo vuelve a como estaba antes del snapshot
```

El cliente nunca toca `figuras`. El memento es **opaco**. Si mañana
`Lienzo` cambia su representación, los clientes siguen funcionando.

## 5. UML rápido

```
        Lienzo  ◇──crea──►  LienzoMemento
          │                       ▲
          │                       │ friend
          └───── restaurar ───────┘

Cuidador (Gestor):  ─► LienzoMemento (solo lo guarda)
```

El cuidador (caretaker) es el que pide mementos y los guarda. El
originador (Lienzo) los crea y los consume. El memento es opaco para
todos menos para el originador.

## 6. Memento vs Command para deshacer

Las dos resuelven undo, con filosofías opuestas:

| | Command | Memento |
|--|---------|---------|
| Qué guarda | La **acción** y cómo revertirla | El **estado** completo |
| Granularidad | Una operación | Una foto del lienzo entero |
| Coste de memoria | Bajo (un objeto pequeño por acción) | Alto (clon completo) |
| Coste de restaurar | Bajo (revertir la operación) | Alto (reemplazar todo) |
| Funciona para acciones externas | No (solo lo que pasa por Command) | Sí (captura todo, no importa quién cambió qué) |

Lo habitual es **combinarlos**:

- Command para deshacer/rehacer en uso normal (rápido, granular).
- Memento para snapshots manuales y save/load (completo, seguro).

## 7. Variantes y trampas

- **Memento profundo vs superficial**: el `crearMemento()` de arriba
  clona cada figura (profundo). Si solo guardara punteros, sería
  superficial y se invalidaría al cambiar una figura. Para snapshots
  fiables, profundo. Otra vez nos salva el `clone()` del día 3.
- **Mementos serializables**: si el memento debe persistir a disco,
  no basta con clonar punteros: hay que serializar (a JSON, a binario).
  Eso introduce dependencias y normalmente lleva a un patrón aparte
  (visitor, por ejemplo, para recorrer y serializar). Decisión.
- **Memento como amigo**: usamos `friend class Lienzo` para que solo
  el Lienzo construya y abra el memento. Hay quien lo evita por
  filosofía. Alternativas: clase anidada privada, interfaz "estrecha"
  pública + interfaz "ancha" interna. El `friend` es la más simple y,
  cuando es controlado, no es pecado.
- **Mementos en vectores**: si guardas muchos (para una pila de
  snapshots), cuidado con la memoria. Cada snapshot clona TODAS las
  figuras. Lienzos con miles de figuras y cientos de snapshots se
  vuelven prohibitivos. Estrategias: poda (mantén los últimos N),
  delta-compression (guarda solo lo que cambió), o usa Command para
  granularidad fina y Memento solo para hitos.

## 8. Memento y los días anteriores

- **Prototype (día 3)** es la pieza que hace que Memento sea posible:
  sin `clone()` no podríamos copiar `Shape*` por su valor real.
- **Observer (este día)** se acopla bien: tras `restaurar()`, las
  vistas se enteran y se redibujan.
- **Command (este día)** se complementa con Memento, como ya vimos.

## 9. SOLID

| Principio | Cómo lo cumple Memento |
|-----------|-----------------------|
| SRP       | El Lienzo guarda su estado; el cuidador guarda mementos. |
| OCP       | Cambiar la representación interna del Lienzo no rompe clientes. |
| LSP       | N/A — no es una jerarquía. |
| ISP       | El memento expone una interfaz mínima (al originador). |
| DIP       | El cuidador depende del memento como caja negra. |

## 10. Mantra

> *"Cuando guardes un estado, guárdalo como una caja cerrada. Solo el
> dueño tiene la llave."*
