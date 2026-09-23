# Abstract Factory

## 1. El problema

El Paint funciona. Pero ahora el cliente nos pide dos modos visuales:

- **Modo claro**: figuras con bordes finos, colores pastel, fondo blanco.
- **Modo oscuro**: figuras con bordes gruesos, colores neón, fondo negro.

Cada modo tiene su propio **conjunto** de figuras: `CirculoClaro`,
`CuadradoClaro`, `TrianguloClaro` por un lado; `CirculoOscuro`,
`CuadradoOscuro`, `TrianguloOscuro` por otro.

La regla del dominio: **no mezclar**. Un `CirculoOscuro` con un
`CuadradoClaro` en el mismo lienzo se ve fatal.

Si usamos Factory Method (que vimos antes), el cliente sigue eligiendo
figura a figura:

```cpp
fabrica.crear("circulo");    // ¿clara u oscura?
fabrica.crear("cuadrado");   // ¿clara u oscura?
```

Nadie evita que el cliente mezcle. **Necesitamos una fábrica que
garantice coherencia de familia.**

## 2. Intención (GoF)

> *Proporcionar una interfaz para crear familias de objetos relacionados
> o dependientes sin especificar sus clases concretas.*

Las palabras clave: **familia**, **coherencia**, **interfaz**. No
fabricamos un objeto; fabricamos un **kit**.

## 3. Bad — el flag en cada llamada

Versión torpe que parece resolver y no resuelve:

```cpp
enum class Tema { Claro, Oscuro };

class FabricaFiguras {
public:
    static std::unique_ptr<Shape> crearCirculo(Tema t, double r) {
        if (t == Tema::Claro) return std::make_unique<CirculoClaro>(r);
        else                  return std::make_unique<CirculoOscuro>(r);
    }
    static std::unique_ptr<Shape> crearCuadrado(Tema t, double l) {
        if (t == Tema::Claro) return std::make_unique<CuadradoClaro>(l);
        else                  return std::make_unique<CuadradoOscuro>(l);
    }
    // ... y así para cada figura
};
```

Síntomas:

- El cliente **tiene que recordar** pasar el mismo `Tema` en cada
  llamada. Si se despista una vez, mezcla.
- Añadir un tercer tema (alto contraste) obliga a tocar **todos** los
  métodos. OCP roto.
- El `if (tema)` se repite N veces.

## 4. Good — Abstract Factory

Definimos una **interfaz de fábrica** que tiene un método por cada tipo
de la familia, y dos implementaciones concretas (una por tema):

```cpp
// La fábrica abstracta: una operación por tipo de figura.
class FabricaFiguras {
public:
    virtual ~FabricaFiguras() = default;
    virtual std::unique_ptr<Shape> crearCirculo(double radio)        const = 0;
    virtual std::unique_ptr<Shape> crearCuadrado(double lado)        const = 0;
    virtual std::unique_ptr<Shape> crearTriangulo(double a, double b, double c) const = 0;
};

// Familia "claro"
class FabricaClara : public FabricaFiguras {
public:
    std::unique_ptr<Shape> crearCirculo(double r) const override {
        return std::make_unique<CirculoClaro>(r);
    }
    std::unique_ptr<Shape> crearCuadrado(double l) const override {
        return std::make_unique<CuadradoClaro>(l);
    }
    std::unique_ptr<Shape> crearTriangulo(double a, double b, double c) const override {
        return std::make_unique<TrianguloClaro>(a, b, c);
    }
};

// Familia "oscuro"
class FabricaOscura : public FabricaFiguras {
public:
    std::unique_ptr<Shape> crearCirculo(double r) const override {
        return std::make_unique<CirculoOscuro>(r);
    }
    std::unique_ptr<Shape> crearCuadrado(double l) const override {
        return std::make_unique<CuadradoOscuro>(l);
    }
    std::unique_ptr<Shape> crearTriangulo(double a, double b, double c) const override {
        return std::make_unique<TrianguloOscuro>(a, b, c);
    }
};
```

Uso, vía inyección:

```cpp
class Editor {
    std::unique_ptr<FabricaFiguras> fabrica;
public:
    explicit Editor(std::unique_ptr<FabricaFiguras> f) : fabrica(std::move(f)) {}

    void dibujarEscenaDePrueba() {
        Lienzo::instancia().anadir(fabrica->crearCirculo(3.0));
        Lienzo::instancia().anadir(fabrica->crearCuadrado(2.0));
        Lienzo::instancia().anadir(fabrica->crearTriangulo(3, 4, 5));
    }
};

int main() {
    Editor editor(std::make_unique<FabricaOscura>());
    editor.dibujarEscenaDePrueba();   // todas las figuras coherentemente oscuras
}
```

El `Editor` **no puede mezclar** aunque quisiera. La coherencia de
familia está garantizada por construcción.

## 5. UML rápido

```
┌─────────────────────┐          ┌─────────┐
│  FabricaFiguras     │ ───crea──▶│  Shape  │
├─────────────────────┤          └─────────┘
│ + crearCirculo()    │              ▲
│ + crearCuadrado()   │              │
│ + crearTriangulo()  │      ┌───────┼────────┐
└─────────────────────┘      │       │        │
        ▲              ┌──────────┐  ...  ┌──────────┐
        │              │CirculoClaro│      │CirculoOscuro│
        │              └──────────┘       └──────────┘
   ┌────┴────┐
   │         │
┌──────────────┐ ┌──────────────┐
│FabricaClara  │ │FabricaOscura │
└──────────────┘ └──────────────┘
   │ crean             │ crean
   ▼                   ▼
   CirculoClaro,...    CirculoOscuro,...
```

Dos jerarquías paralelas que se mueven juntas.

## 6. Abstract Factory vs Factory Method

La pregunta de examen. Mismo prefijo, distinto problema:

| Factory Method                        | Abstract Factory                       |
|---------------------------------------|----------------------------------------|
| Crea **un** objeto                    | Crea **una familia** de objetos        |
| Una decisión: "qué tipo de X"         | Una decisión: "qué variante de todo"   |
| Suele resolverse con función o map    | Suele requerir jerarquía de fábricas   |
| Cliente trata con tipos sueltos       | Cliente trata con un kit completo      |
| Útil contra "switch por tipo"         | Útil contra "mezclar familias por error"|

Regla práctica: si tu fábrica devuelve **un solo tipo de cosa**, es
Factory (Method). Si devuelve **varios tipos relacionados que tienen
que ir juntos**, es Abstract Factory.

## 7. Variantes y trampas

### Trampa 1: cuando la familia es de un solo miembro

Si tu Abstract Factory solo tiene `crearCirculo()`, no es Abstract
Factory: es un Factory Method con sombrero. La gracia del patrón es la
**familia**. Si no hay familia, simplifica.

### Trampa 2: la explosión combinatoria

Si tienes 3 temas × 10 figuras, tienes 30 clases concretas. Es **caro**.
Antes de aplicar el patrón, pregúntate si la variación visual no se podría
resolver con:

- **Strategy** (un único `Circulo` con una `EstrategiaDeRenderizado`
  intercambiable).
- **Decorator** (un `Circulo` decorado con `TemaClaro` o `TemaOscuro`).
- **Composición de datos** (un único `Circulo` con un struct `Estilo`).

Abstract Factory tiene sentido cuando las **diferencias son
estructurales**, no solo de pintura. Si solo cambia el color, hay
herramientas más baratas.

### Variante moderna: Abstract Factory con templates

Para evitar la jerarquía paralela, se puede parametrizar:

```cpp
template<typename Familia>
class FabricaGenerica {
public:
    std::unique_ptr<Shape> crearCirculo(double r) {
        return std::make_unique<typename Familia::Circulo>(r);
    }
    std::unique_ptr<Shape> crearCuadrado(double l) {
        return std::make_unique<typename Familia::Cuadrado>(l);
    }
};

struct FamiliaClara {
    using Circulo  = CirculoClaro;
    using Cuadrado = CuadradoClaro;
};

struct FamiliaOscura {
    using Circulo  = CirculoOscuro;
    using Cuadrado = CuadradoOscuro;
};

FabricaGenerica<FamiliaClara> fabrica;
```

Sin herencia, todo en tiempo de compilación. Más rápido, pero más
opaco y pierde polimorfismo dinámico (no puedes cambiar de tema en
runtime). Útil cuando la familia se elige una vez al inicio.

### Trampa 3: convertirlo en Singleton

Casi inevitable que aparezca como "tengo una fábrica global, la hago
Singleton". Hereda todos los problemas del Singleton. **Inyéctala** por
constructor donde haga falta.

## 8. SOLID en acción

| Principio | Cómo lo aplica Abstract Factory                                   |
|-----------|-------------------------------------------------------------------|
| SRP       | Cada fábrica concreta se ocupa solo de su familia.                |
| OCP       | Cumplido para nuevas **familias** (añadir tema). Roto para nuevos **miembros** (añadir tipo de figura obliga a tocar la interfaz y todas las fábricas). |
| LSP       | Cumplido: cualquier fábrica concreta vale donde se espera la abstracta. |
| ISP       | Si la familia es grande, la interfaz se hincha. Vigila.           |
| DIP       | **Cumplido al máximo.** El editor depende solo de la fábrica abstracta. |

Observación importante de OCP: Abstract Factory está optimizado para
**nuevas familias**, no para **nuevos miembros**. Si tu eje de variación
real son los miembros (añades tipos de figura a menudo pero rara vez
temas), el patrón te penaliza. Es una decisión consciente.

## 9. Mantra

> *"Si los objetos tienen que ir juntos, que los cree la misma fábrica.
> Si no, déjalos sueltos."*
