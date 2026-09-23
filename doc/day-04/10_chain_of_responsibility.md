# Chain of Responsibility

## 1. El problema

El Paint procesa eventos de ratón. Un click en una zona puede
significar muchas cosas según el contexto:

- Si hay una **figura seleccionada** y el click está sobre ella → mover.
- Si hay **dos puntos marcados** previamente → trazar línea.
- Si está activo el **modo borrar** → eliminar la figura clicada.
- Si está activo el **modo zoom** → ampliar en ese punto.
- Si nada de lo anterior → click ignorado.

Una sola función `onClick(x, y)` con cinco `if/else` encadenados
funciona... hasta que:

- Llega un sexto modo.
- Cambia el orden de prioridad.
- Un plugin añade su propio modo en runtime.
- Tests quieren validar cada filtro por separado.

```cpp
void GUI::onClick(int x, int y) {
    if (modoBorrar && hayFiguraEn(x, y))      { borrar(x, y);    return; }
    if (modoZoom)                              { ampliar(x, y);  return; }
    if (haySeleccionada() && enFigura(x, y))  { mover(x, y);    return; }
    if (dosPuntosMarcados())                   { trazarLinea();  return; }
    // ...
}
```

OCP roto y SRP perdido: la GUI sabe **todo** sobre todos los modos.

## 2. Intención (GoF)

> *Evitar acoplar el remitente de una petición con su receptor dando a
> más de un objeto la oportunidad de manejarla. Encadenar los objetos
> receptores y pasar la petición por la cadena hasta que uno la
> maneje.*

Cada manejador decide: **lo manejo yo y la cadena se para** o **paso
al siguiente**.

## 3. Bad: el if encadenado

(El código del párrafo anterior. Si lo reescribimos con métodos
auxiliares es algo más limpio, pero la lógica de orden y prioridad
sigue concentrada en un sitio.)

## 4. Good: la cadena

```cpp
struct EventoClick { int x, y; };

class ManejadorClick {
protected:
    std::unique_ptr<ManejadorClick> siguiente;
public:
    virtual ~ManejadorClick() = default;

    ManejadorClick* encadenar(std::unique_ptr<ManejadorClick> s) {
        if (siguiente) siguiente->encadenar(std::move(s));
        else           siguiente = std::move(s);
        return this;
    }

    void manejar(const EventoClick& e) {
        if (procesar(e)) return;        // me ocupo, la cadena para
        if (siguiente) siguiente->manejar(e);   // si no, paso
    }

protected:
    virtual bool procesar(const EventoClick& e) = 0;
};

class ManejadorBorrar : public ManejadorClick {
    bool activo = false;
public:
    void activar(bool v) { activo = v; }
protected:
    bool procesar(const EventoClick& e) override {
        if (!activo) return false;
        // ... borrar figura en (e.x, e.y) ...
        std::cout << "[borrar] (" << e.x << "," << e.y << ")\n";
        return true;
    }
};

class ManejadorZoom : public ManejadorClick {
    bool activo = false;
public:
    void activar(bool v) { activo = v; }
protected:
    bool procesar(const EventoClick& e) override {
        if (!activo) return false;
        std::cout << "[zoom] (" << e.x << "," << e.y << ")\n";
        return true;
    }
};

class ManejadorMover : public ManejadorClick {
protected:
    bool procesar(const EventoClick& e) override {
        // si hay figura seleccionada y el click está sobre ella ...
        std::cout << "[mover] (" << e.x << "," << e.y << ")\n";
        return true;   // o false, según contexto
    }
};
```

Construir la cadena:

```cpp
auto cadena = std::make_unique<ManejadorBorrar>();
cadena->encadenar(std::make_unique<ManejadorZoom>());
cadena->encadenar(std::make_unique<ManejadorMover>());

cadena->manejar({100, 200});
```

Añadir un modo nuevo: una clase, una llamada a `encadenar`. La GUI ya
no decide nada: solo recibe el click y se lo entrega a la cadena.

## 5. UML rápido

```
ManejadorClick (interfaz)
       ▲          ◇ siguiente
       │
ManejadorBorrar ─► ManejadorZoom ─► ManejadorMover ─► nullptr
```

## 6. Variantes y trampas

- **Cadena estática vs dinámica**: nuestro ejemplo construye la cadena
  una vez. También se puede modificar en runtime (añadir, quitar,
  reordenar). Útil cuando los plugins se cargan/descargan.
- **Todos los manejadores procesan** (variante "broadcast"): cada
  manejador decide si procesa, pero **siempre** pasa al siguiente.
  Útil para logging, validación de varias reglas, etc.
- **Cíclos**: si por error encadenas A→B→A, bucle infinito. Detecta
  o no permitas.
- **Tan similar a Decorator que conviene aclararlo**:
  - Decorator **suma** comportamiento: todos los decoradores
    contribuyen.
  - Chain **filtra**: uno se queda con la petición, los demás no se
    enteran.
  - Misma estructura, intención opuesta.

## 7. CoR y los días anteriores

Hace eco a algo del día 3: **Decorator**. La estructura es igual (uno
envuelve al siguiente), pero el flujo de control es distinto. Donde
Decorator deja pasar siempre el control, Chain lo intercepta. En cierto
sentido, Chain es Decorator con `if (procesar) return;`.

## 8. SOLID

| Principio | Cómo lo cumple Chain |
|-----------|---------------------|
| SRP       | Cada manejador, una responsabilidad. |
| OCP       | Nuevo manejador = nueva clase + `encadenar`. |
| LSP       | Todos los manejadores intercambiables. |
| ISP       | La interfaz manejador es mínima. |
| DIP       | La GUI depende de `ManejadorClick`, no de cada concreto. |

## 9. Mantra

> *"Que el primero que sepa qué hacer, lo haga. Los demás, que no
> tienen nada que aportar, que se aparten."*
