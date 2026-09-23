# Facade

## 1. El problema

El Paint, a estas alturas, tiene un montón de piezas:

- `Lienzo` (Singleton).
- `FabricaFiguras` con su registro.
- `FabricaTema` con sus variantes claro/oscuro.
- `Renderer` (Bridge) con consola/SVG/OpenGL.
- `Poligono::Builder`.
- Y mañana llegarán Command, Observer, Memento…

Si una GUI nueva quiere pintar un círculo rojo en consola, tiene que:

```cpp
auto renderer = std::make_shared<RendererConsola>();
auto fabrica  = std::make_unique<FabricaTemaClaro>();
auto figura   = fabrica->circulo(3.0);
// ¿y dónde meto el renderer? ¿y el color rojo? ¿y el lienzo?
Lienzo::instancia().anadir(std::move(figura));
Lienzo::instancia().dibujar();
```

Demasiada maquinaria para algo conceptualmente simple. Cada cliente
nuevo (GUI de escritorio, CLI, tests, scripting) tiene que aprenderse
todo el subsistema.

## 2. Intención (GoF)

> *Proporcionar una interfaz unificada a un conjunto de interfaces de
> un subsistema. Facade define una interfaz de alto nivel que hace más
> fácil de usar el subsistema.*

Una clase, pocos métodos, esconde toda la maquinaria.

## 3. Bad: cada cliente reaprende el subsistema

Tres clientes distintos (GUI, CLI, tests). Cada uno repite el cableado:

```cpp
// GUI
auto rendererGUI = std::make_shared<RendererSVG>();
auto fabricaGUI  = std::make_unique<FabricaTemaClaro>();
auto cGUI = fabricaGUI->circulo(3.0);
Lienzo::instancia().anadir(std::move(cGUI));

// CLI
auto rendererCLI = std::make_shared<RendererConsola>();
auto fabricaCLI  = std::make_unique<FabricaTemaOscuro>();
auto cCLI = fabricaCLI->circulo(5.0);
Lienzo::instancia().anadir(std::move(cCLI));

// Tests
// ... mismo baile ...
```

Cuando el subsistema cambia (mañana llega Command), **los tres
clientes** tienen que actualizarse.

## 4. Good: una Facade para el Paint

```cpp
class Paint {
public:
    enum class Tema   { Claro, Oscuro };
    enum class Salida { Consola, SVG };

    Paint(Tema tema, Salida salida) {
        renderer = (salida == Salida::SVG)
            ? std::shared_ptr<Renderer>(std::make_shared<RendererSVG>())
            : std::shared_ptr<Renderer>(std::make_shared<RendererConsola>());
        fabricaTema = (tema == Tema::Claro)
            ? std::unique_ptr<FabricaTema>(std::make_unique<FabricaTemaClaro>())
            : std::unique_ptr<FabricaTema>(std::make_unique<FabricaTemaOscuro>());
    }

    void anadirCirculo(double radio) {
        Lienzo::instancia().anadir(fabricaTema->circulo(radio));
    }
    void anadirCuadrado(double lado) {
        Lienzo::instancia().anadir(fabricaTema->cuadrado(lado));
    }
    void anadirDesdeTexto(const std::string& linea) {
        Lienzo::instancia().anadir(FabricaFiguras::crear(linea));
    }
    void dibujarTodo() { Lienzo::instancia().dibujar(); }
    void limpiar()     { Lienzo::instancia().limpiar(); }
    size_t numFiguras() const { return Lienzo::instancia().numFiguras(); }

private:
    std::shared_ptr<Renderer>     renderer;
    std::unique_ptr<FabricaTema>  fabricaTema;
};
```

Cliente:

```cpp
Paint p{Paint::Tema::Claro, Paint::Salida::SVG};
p.anadirCirculo(3.0);
p.anadirDesdeTexto("cuadrado 2.0");
p.dibujarTodo();
```

Una línea de configuración, tres operaciones. La GUI no sabe nada de
fábricas, renderers, ni de cómo se registran las figuras.

## 5. UML rápido

```
Cliente ─►  Paint (Facade)
                │
                ├──► Lienzo
                ├──► FabricaTema
                ├──► FabricaFiguras
                └──► Renderer
```

El Cliente solo conoce a `Paint`. Lo que hay detrás puede cambiar sin
que el cliente se entere.

## 6. Variantes y trampas

- **Facade no oculta, simplifica**. El cliente que necesite acceso
  directo a `Lienzo` o a una fábrica puede tenerlo. Facade es la puerta
  fácil, no la única puerta. Si la haces única, has hecho otra cosa
  (un Mediador, o peor, un God Object).
- **Una facade por caso de uso, no una facade total**. Si tu Facade
  acaba teniendo 50 métodos, hazte dos: `PaintEditor` y `PaintViewer`,
  por ejemplo. Es ISP otra vez.
- **Facade no es Adapter**. Adapter cambia una interfaz por otra
  (1 a 1). Facade unifica varias interfaces detrás de una nueva
  (N a 1).
- **No metas lógica de negocio en la Facade**. Si la Facade decide qué
  hacer, es Mediador. La Facade solo cablea: recibe llamadas y las
  redistribuye.

## 7. Facade y los días anteriores

La Facade es donde **se cobra** todo el trabajo SOLID y de patrones de
los días anteriores:

- Sin SOLID, la Facade sería un God Object con todo dentro.
- Sin Singleton, tendría que pasar `Lienzo` a todos los métodos.
- Sin Factories, tendría que hacer `new` directo.
- Sin Bridge, tendría que conocer cada combinación figura×backend.

La Facade es **la cara presentable** del subsistema. Lo demás es la
cocina.

## 8. SOLID

| Principio | Cómo lo cumple Facade |
|-----------|----------------------|
| SRP       | Su responsabilidad es cablear, no hacer. |
| OCP       | Si lo de dentro cambia, la Facade puede quedarse igual. |
| LSP       | N/A — no es una jerarquía. |
| ISP       | Una facade por caso de uso, no una facade total. |
| DIP       | El cliente depende de la Facade, no del subsistema. |

## 9. Mantra

> *"Si tu API tiene treinta clases, dale al cliente una sola puerta."*
