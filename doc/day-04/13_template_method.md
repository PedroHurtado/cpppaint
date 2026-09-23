# Template Method

## 1. El problema

El Paint exporta el lienzo a varios formatos: PNG, SVG y PDF. Los tres
hacen lo mismo en abstracto:

1. Abrir un fichero.
2. Escribir una cabecera específica del formato.
3. Para cada figura, escribir su representación en ese formato.
4. Escribir un pie específico del formato.
5. Cerrar el fichero.

Lo único que cambia son los **pasos concretos** (qué cabecera, qué pie,
cómo se serializa una figura en ese formato). Pero la **estructura
general** es siempre la misma.

Solución ingenua: tres clases (`ExportadorPNG`, `ExportadorSVG`,
`ExportadorPDF`) que repiten los cinco pasos cada una:

```cpp
class ExportadorPNG {
public:
    void exportar(const Lienzo& l, const std::string& ruta) {
        std::ofstream f(ruta, std::ios::binary);
        if (!f) throw std::runtime_error("no se pudo abrir");

        f << "PNG-HEADER";
        for (const auto& fig : l.figurasReadOnly()) {
            // ... serializar a PNG ...
        }
        f << "PNG-FOOTER";

        f.close();
    }
};

class ExportadorSVG {
public:
    void exportar(const Lienzo& l, const std::string& ruta) {
        std::ofstream f(ruta);
        if (!f) throw std::runtime_error("no se pudo abrir");

        f << "<svg ...>";
        for (const auto& fig : l.figurasReadOnly()) {
            // ... serializar a SVG ...
        }
        f << "</svg>";

        f.close();
    }
};
```

Tres veces el mismo esqueleto. Mañana añadimos manejo de errores
robusto: tres ediciones idénticas. Pasado mañana, logging: tres más.

## 2. Intención (GoF)

> *Definir el esqueleto de un algoritmo en una operación, dejando
> algunos pasos a las subclases. Template Method permite a las
> subclases redefinir ciertos pasos sin cambiar la estructura del
> algoritmo.*

El padre tiene el método "plantilla" que llama a métodos abstractos.
Los hijos rellenan los huecos.

## 3. Bad: el esqueleto duplicado

(El código del párrafo anterior.)

## 4. Good: Template Method

```cpp
class Exportador {
public:
    virtual ~Exportador() = default;

    // Método plantilla — NO virtual, NO se sobrescribe.
    void exportar(const Lienzo& l, const std::string& ruta) {
        auto f = abrirFichero(ruta);
        if (!f) throw std::runtime_error("no se pudo abrir " + ruta);

        escribirCabecera(*f);
        for (const auto& fig : l.figurasReadOnly())
            escribirFigura(*f, *fig);
        escribirPie(*f);
    }

protected:
    // Pasos que las subclases rellenan
    virtual std::unique_ptr<std::ostream> abrirFichero(const std::string& ruta) = 0;
    virtual void escribirCabecera(std::ostream& out)                     = 0;
    virtual void escribirFigura(std::ostream& out, const Shape& f)       = 0;
    virtual void escribirPie(std::ostream& out)                          = 0;
};

class ExportadorSVG : public Exportador {
protected:
    std::unique_ptr<std::ostream> abrirFichero(const std::string& r) override {
        return std::make_unique<std::ofstream>(r);
    }
    void escribirCabecera(std::ostream& o) override {
        o << "<svg xmlns=\"http://www.w3.org/2000/svg\">\n";
    }
    void escribirFigura(std::ostream& o, const Shape& f) override {
        // ... según el tipo de figura (aquí encajaría un Visitor) ...
        o << "  <!-- figura -->\n";
    }
    void escribirPie(std::ostream& o) override {
        o << "</svg>\n";
    }
};

class ExportadorPNG : public Exportador {
protected:
    std::unique_ptr<std::ostream> abrirFichero(const std::string& r) override {
        return std::make_unique<std::ofstream>(r, std::ios::binary);
    }
    void escribirCabecera(std::ostream& o) override { o << "PNG-HEADER"; }
    void escribirFigura(std::ostream& o, const Shape&) override { /* ... */ }
    void escribirPie(std::ostream& o) override { o << "PNG-FOOTER"; }
};
```

Uso:

```cpp
ExportadorSVG svg;
svg.exportar(Lienzo::instancia(), "salida.svg");

ExportadorPNG png;
png.exportar(Lienzo::instancia(), "salida.png");
```

Añadir manejo de errores, logging o validación: **una sola edición en
`Exportador::exportar`** y vale para los tres formatos.

Añadir un formato nuevo (JPG, GIF): una clase nueva con cuatro métodos.

## 5. UML rápido

```
Exportador
   exportar()  ←── método plantilla (no virtual)
   abrirFichero()*       ←── pasos abstractos
   escribirCabecera()*
   escribirFigura()*
   escribirPie()*
       ▲
       │
ExportadorSVG, ExportadorPNG, ExportadorPDF
```

## 6. Variantes y trampas

- **Hook methods**: pasos que tienen una implementación por defecto
  en el padre que las subclases **pueden** sobrescribir (no tienen
  que). Útil para extender, no obligar:

  ```cpp
  protected:
      virtual void preProcesar(std::ostream&) {}   // hook: por defecto no-op
      virtual void postProcesar(std::ostream&) {}  // hook: por defecto no-op
  ```

- **El método plantilla NO debe ser virtual.** Si las subclases pueden
  reescribirlo, ya no es plantilla, es libre albedrío. Mantén el
  esqueleto fijo y solo los huecos virtuales.
- **CRTP como variante en C++**: en vez de polimorfismo runtime, el
  padre conoce al hijo en tiempo de compilación.

  ```cpp
  template <typename Derived>
  class Exportador {
  public:
      void exportar(const Lienzo& l, const std::string& r) {
          static_cast<Derived*>(this)->escribirCabecera(/* ... */);
          // ...
      }
  };

  class ExportadorSVG : public Exportador<ExportadorSVG> {
  public:
      void escribirCabecera(std::ostream& o) { /* ... */ }
      // ...
  };
  ```

  Sin coste de vtable, todo inline. La pega: cada `Exportador<X>` es
  un tipo distinto, no hay polimorfismo común. Útil cuando rendimiento
  es crítico y no necesitas una colección heterogénea.

- **Template Method vs Strategy**: ya volvemos a comparar.
  - **Template Method**: el esqueleto es **herencia**; los pasos son
    métodos virtuales en subclases. Acoplamiento estructural.
  - **Strategy**: el esqueleto es **un cliente**; los pasos son
    objetos inyectados. Acoplamiento por composición.
  - Strategy es más flexible: puedes cambiar el algoritmo en runtime,
    intercambiar piezas, combinar. Template Method es más simple si la
    variación es estable y los pasos están ligados al "tipo" de la
    cosa.

## 7. Template Method y los días anteriores

- **Día 1 (CRTP)**: la variante CRTP de Template Method.
- **Día 2 (LSP)**: cumplir LSP es lo que permite que el método
  plantilla del padre funcione cuando los hijos rellenan los huecos.

## 8. SOLID

| Principio | Cómo lo cumple Template Method |
|-----------|-------------------------------|
| SRP       | El esqueleto vs los pasos: separados. |
| OCP       | Nuevo formato = nueva subclase. Cero ediciones en el padre. |
| LSP       | Crítico: los hijos deben cumplir el contrato del padre. |
| ISP       | El hijo expone los pasos que el padre necesita. |
| DIP       | Hay quien lo discute: la subclase depende del padre concreto. |

## 9. Mantra

> *"Si el cómo cambia y el cuándo no, escribe el cuándo una vez y deja
> el cómo a quien hereda."*
