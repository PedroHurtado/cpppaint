# Composite

## 1. El problema

El cliente quiere **agrupar figuras**. Selecciona varias con el ratón,
las agrupa con `Ctrl+G`, y a partir de ahí ese grupo se mueve, se
copia, se borra **como si fuera una figura más**. Y puede haber grupos
de grupos (un mecano completo es un grupo de subgrupos).

Versión torpe — tratamiento especial para grupos:

```cpp
class Lienzo {
    std::vector<std::unique_ptr<Shape>> figuras;
    std::vector<Grupo> grupos;   // ¡tipo distinto!

    void dibujar() {
        for (auto& f : figuras) f->dibujar();
        for (auto& g : grupos)  g.dibujar();   // dos bucles
    }
};
```

Síntomas:

- El cliente tiene que **distinguir** entre figuras y grupos en todas
  partes. Cada operación duplica la lógica.
- ¿Y un grupo de grupos? Otro `std::vector<Grupo>` dentro de `Grupo`,
  otro nivel de tratamiento especial.
- Cuando lleguen 5 operaciones nuevas (mover, rotar, exportar…), cada
  una tiene que pensar en figuras **y** en grupos. Bug factory.

La solución: que un **grupo sea también una figura**. Eso es Composite,
y es exactamente lo que prometimos ayer con `Grupo : Shape`.

## 2. Intención (GoF)

> *Componer objetos en estructuras de árbol para representar jerarquías
> parte-todo. Composite permite que los clientes traten de manera
> uniforme a objetos individuales y composiciones de objetos.*

Las palabras clave: **árbol** y **tratamiento uniforme**.

## 3. Bad — el switch entre hoja y rama

Ya visto. Otra variante igual de mala: un campo `bool esGrupo` en
`Shape` con un `if` en cada método. Otra vez `switch` por tipo, otra
vez SOLID llorando.

## 4. Good — Composite

Un **`Grupo` es un `Shape`** que internamente contiene `Shape`s. Para
dibujar, dibuja a todos sus hijos. Para calcular área, suma. Para
moverse, mueve a todos.

```cpp
class Grupo : public Shape {
    std::vector<std::unique_ptr<Shape>> hijos;
public:
    void anadir(std::unique_ptr<Shape> s) {
        hijos.push_back(std::move(s));
    }

    void dibujar() const override {
        for (const auto& h : hijos) h->dibujar();
    }

    double area() const override {
        double total = 0.0;
        for (const auto& h : hijos) total += h->area();
        return total;
    }

    std::unique_ptr<Shape> clone() const override {
        auto copia = std::make_unique<Grupo>();
        for (const auto& h : hijos) copia->anadir(h->clone());
        return copia;
    }
};
```

Uso:

```cpp
auto grupo = std::make_unique<Grupo>();
grupo->anadir(std::make_unique<Circulo>(3.0));
grupo->anadir(std::make_unique<Cuadrado>(2.0));

// Un grupo dentro de otro grupo
auto super = std::make_unique<Grupo>();
super->anadir(std::move(grupo));
super->anadir(std::make_unique<Triangulo>(3, 4, 5));

Lienzo::instancia().anadir(std::move(super));
Lienzo::instancia().dibujar();   // dibuja recursivamente todo el árbol
```

Mira el `Lienzo`: **no tiene la menor idea** de que está dibujando un
grupo. Solo ve `Shape`s. La recursividad la resuelve el propio `Grupo`
delegando en sus hijos.

## 5. UML rápido

```
            ┌─────────────────┐
            │      Shape      │
            ├─────────────────┤
            │ +dibujar()      │
            │ +area()         │
            └─────────────────┘
                    ▲
        ┌───────────┼────────────┐
        │           │            │
   ┌──────────┐ ┌──────────┐ ┌───────────────┐
   │ Circulo  │ │ Cuadrado │ │    Grupo      │
   │  (hoja)  │ │  (hoja)  │ │  (compuesto)  │
   └──────────┘ └──────────┘ ├───────────────┤
                             │ -hijos: vec<S>│ ◀── tiene-un Shape*
                             │ +anadir()     │     (recursivo)
                             └───────────────┘
                                     │
                                     └──◊ contiene Shape (incluye otros Grupos)
```

Como en Decorator, la doble relación con `Shape` (hereda Y contiene).
**La diferencia**: Decorator envuelve **uno**, Composite contiene
**muchos**.

## 6. Variantes y trampas

### Trampa 1: el `anadir` que no encaja en la base

Pregunta clásica: ¿debería `Shape` tener `anadir()` para que el cliente
trate todo uniformemente? Dos posiciones:

- **GoF original**: sí, `anadir` y `quitar` en la base (con
  implementación vacía o que lanza en las hojas). Tratamiento más
  uniforme.
- **Práctica moderna**: no, `anadir` solo en `Grupo`. El cliente que
  quiera añadir hace un `dynamic_cast`. **Más seguro de tipos**, menos
  uniforme.

En C++ moderno se prefiere la segunda: que la interfaz refleje lo que
realmente se puede hacer. Si llamas `anadir` en un círculo y lanza
excepción, es **violación de LSP** (visto el día 2).

### Trampa 2: ciclos en el árbol

Si por error añades un grupo a sí mismo (directa o indirectamente),
`dibujar()` recurre infinitamente. Si necesitas seguridad, valida en
`anadir`. En el Paint no nos preocupamos (un usuario no puede crear
ciclos), en sistemas críticos sí.

### Trampa 3: propiedad de los hijos

Con `unique_ptr` el grupo es **dueño** de sus hijos. Si quieres compartir
un mismo `Shape` entre dos grupos, necesitas `shared_ptr`. Pero ojo:
ahí entra Flyweight (mañana). Decide qué quieres.

### Variante moderna: visitor sobre composite

La operación "dibuja todo el árbol" es trivial. Pero "exporta todo el
árbol a JSON" añade responsabilidad a `Shape`. Si las operaciones
crecen, **Visitor** (mañana) las saca de la jerarquía. Composite +
Visitor es una pareja muy frecuente.

## 7. Cuándo usar Composite

- Estructuras **jerárquicas parte-todo**: figuras agrupadas, ficheros y
  directorios, expresiones aritméticas, menús con submenús, escenas
  3D, árboles DOM…
- Cuando el cliente debe tratar **uniformemente** elementos simples y
  compuestos.
- Cuando la operación se aplica **recursivamente** al árbol.

En el Paint nos sirve para grupos seleccionables que se comportan como
una figura.

## 8. SOLID en acción

| Principio | Cómo lo aplica Composite                                          |
|-----------|-------------------------------------------------------------------|
| SRP       | El `Grupo` solo orquesta a sus hijos.                             |
| OCP       | Cumplido: nuevas figuras encajan en grupos sin tocar `Grupo`.     |
| LSP       | **Cuidado**: si `Shape` tiene `anadir`, hojas que lanzan rompen LSP. |
| ISP       | Cumplido si dejas `anadir` solo en `Grupo`.                       |
| DIP       | Cumplido: el cliente depende de `Shape`.                          |

## 9. Mantra

> *"Si el todo se comporta como la parte, deja que el todo sea una parte
> más."*
