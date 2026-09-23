# Herencia múltiple de interfaces en C++: qué pasa por debajo y cuándo importa

Cuatro demostraciones ejecutables (g++ 13, Linux x64, Itanium C++ ABI) que prueban los efectos físicos de implementar varias interfaces abstractas, y un análisis de los escenarios concretos donde esos efectos dejan de ser inofensivos.

---

## Demo 1 — Cada interfaz añade un vptr al objeto

### Código

```cpp
#include <cstdio>

struct IDraw  { virtual void draw()  = 0; virtual ~IDraw()  = default; };
struct ISave  { virtual void save()  = 0; virtual ~ISave()  = default; };
struct IClick { virtual void click() = 0; virtual ~IClick() = default; };

struct W0 { int data; };

struct W1 : IDraw {
    int data;
    void draw() override {}
};

struct W2 : IDraw, ISave {
    int data;
    void draw() override {}
    void save() override {}
};

struct W3 : IDraw, ISave, IClick {
    int data;
    void draw()  override {}
    void save()  override {}
    void click() override {}
};

int main() {
    printf("sizeof(int)            = %zu\n", sizeof(int));
    printf("W0 (sin interfaces)    = %zu\n", sizeof(W0));
    printf("W1 (1 interfaz)        = %zu\n", sizeof(W1));
    printf("W2 (2 interfaces)      = %zu\n", sizeof(W2));
    printf("W3 (3 interfaces)      = %zu\n", sizeof(W3));
}
```

### Salida

```
sizeof(int)            = 4
W0 (sin interfaces)    = 4
W1 (1 interfaz)        = 16
W2 (2 interfaces)      = 24
W3 (3 interfaces)      = 32
```

### Qué está pasando

Una clase con funciones virtuales necesita un puntero oculto (vptr) que apunta a su tabla de funciones virtuales. Con herencia simple basta un vptr porque la vtable de la derivada *extiende* la de la base: ambas vistas del objeto comparten el mismo origen.

Con herencia múltiple eso es imposible. Un `ISave*` tiene que poder despachar `save()` sin saber que detrás hay un `Widget`: necesita encontrar un vptr en el offset 0 de *su* vista del objeto. Como `IDraw` ya ocupa el offset 0 del objeto completo, `ISave` se coloca a continuación **con su propio vptr**. El layout de `W2` es:

```
offset 0:   vptr → sub-vtable de IDraw   (base primaria)
offset 8:   vptr → sub-vtable de ISave
offset 16:  int data
offset 20:  padding (alineación a 8)
```

La progresión 4 → 16 → 24 → 32 se descompone así: la primera interfaz cuesta un vptr (8 bytes) **más** el salto de alineación del objeto de 4 a 8 (de ahí que parezca costar 12). Cada interfaz adicional cuesta exactamente 8 bytes más.

Detalle didáctico: solo la **primera** base (la "primaria") es gratis en el sentido de que comparte vptr con la clase derivada. Por eso el orden de la lista de herencia tiene consecuencias físicas, algo que sorprende a cualquiera que venga de C# o Java donde `class A : I1, I2` e `class A : I2, I1` son idénticos.

### Cuándo da problemas

**Objetos pequeños en grandes cantidades.** El caso de los 4 bytes útiles que ocupan 32 es real en:

- Nodos de árboles/grafos (AST de un compilador, scene graph de un motor, DOM). Un AST de un fichero grande puede tener millones de nodos; si cada nodo implementa `IVisitable`, `IPrintable`, `ISerializable`, el 75–87% de la memoria son vptrs y padding.
- Entidades de un game engine o partículas. Es uno de los motivos históricos por los que los motores abandonaron jerarquías de interfaces y migraron a ECS (Entity-Component-System): no solo por los vptrs, sino porque destruyen la densidad de caché.
- Mensajes/eventos en sistemas de alta frecuencia (trading, telemetría). Triplicar el tamaño del mensaje triplica el ancho de banda de memoria consumido.

**Efecto caché, que es el coste real.** Una línea de caché son 64 bytes. Con `W0` caben 16 objetos por línea; con `W3` caben 2. En un recorrido secuencial eso es hasta 8× más fallos de caché — y un fallo de caché a memoria principal cuesta ~100-300 ciclos, frente a los 1-4 ciclos de cualquier instrucción de las demos siguientes. **De los cuatro efectos de este documento, este es el único que puede degradar un sistema un orden de magnitud.**

**Cuándo NO importa:** objetos de cientos de bytes o más, objetos de vida larga en cantidades moderadas (servicios, repositorios, controladores). Un `ReservationService` con 3 interfaces y 200 bytes de estado paga un 12% de overhead en un objeto que existe una vez. Irrelevante.

---

## Demo 2 — Los casts entre bases hacen aritmética de punteros

### Código

```cpp
#include <cstdio>

struct IDraw { virtual void draw() = 0; virtual ~IDraw() = default; };
struct ISave { virtual void save() = 0; virtual ~ISave() = default; };

struct Widget : IDraw, ISave {
    int data = 42;
    void draw() override {}
    void save() override {}
};

int main() {
    Widget w;
    Widget* pw = &w;
    IDraw*  pd = &w;
    ISave*  ps = &w;

    printf("Widget* = %p\n", (void*)pw);
    printf("IDraw*  = %p\n", (void*)pd);
    printf("ISave*  = %p  (+%td bytes)\n", (void*)ps, (char*)ps - (char*)pw);
}
```

### Salida

```
Widget* = 0x7ffe92fd6df0
IDraw*  = 0x7ffe92fd6df0
ISave*  = 0x7ffe92fd6df8  (+8 bytes)
```

### Qué está pasando

`ISave* ps = &w;` no copia el puntero: emite `ps = (char*)&w + 8` (con comprobación de nulo si el origen puede ser nulo). El subobjeto `ISave` *vive* 8 bytes dentro de `Widget`, y un `ISave*` debe apuntar a su vptr para que el despacho virtual funcione.

Consecuencia conceptual importante: **la identidad del objeto ya no es un valor de puntero único**. `pw == ps` compila y da `true` porque el compilador ajusta antes de comparar (conoce ambos tipos), pero los bits son distintos. En cuanto los punteros pasan por `void*`, la identidad se rompe.

### Cuándo da problemas

Este es el efecto que más **bugs reales** produce, porque rompe el modelo mental "un cast solo cambia el tipo, no el valor". Escenarios concretos:

**1. `reinterpret_cast` o paso por `void*`.**

```cpp
void registrar(void* ctx);                 // API de callback estilo C
registrar(static_cast<ISave*>(&w));        // guarda &w + 8
// ... más tarde:
Widget* w2 = static_cast<Widget*>(ctx);    // ¡puntero corrupto: apunta 8 bytes dentro!
w2->data;                                  // lee basura o corrompe memoria
```

La regla segura es hacer el viaje de ida y vuelta por **el mismo tipo**: si metes un `Widget*` en el `void*`, saca un `Widget*`. Cualquier API de callbacks con `void* user_data` (la mayoría de APIs de sistema y bibliotecas C++ con frontera estilo C) es terreno minado si el objeto usa herencia múltiple.

**2. Comparar identidad a través de interfaces distintas.**

```cpp
std::set<void*> registrados;
registrados.insert(static_cast<IDraw*>(&w));
// en otro módulo:
registrados.count(static_cast<ISave*>(&w));   // 0: "no está", pero sí está
```

El mismo objeto registrado dos veces, deduplicación que no deduplica, un observer que se desuscribe y sigue recibiendo eventos. El idioma correcto para identidad es normalizar siempre con `dynamic_cast<void*>(p)`, que devuelve la dirección del objeto **completo** independientemente de por qué interfaz se mire.

**3. `static_cast` descendente a la base equivocada.**

```cpp
ISave* ps = obtener();
Widget* w = static_cast<Widget*>(ps);   // correcto: resta 8
// pero si 'ps' en realidad apuntaba a OtroWidget con layout distinto → UB silencioso
```

`static_cast` hacia abajo aplica el offset del tipo *declarado*, sin comprobar nada. Con herencia simple un downcast equivocado suele "funcionar por accidente"; con herencia múltiple el offset incorrecto produce corrupción inmediata. Es la razón por la que el downcast comprobado (`dynamic_cast`) deja de ser opcional en jerarquías múltiples.

**4. Serialización, hashing o copia por bytes.** Cualquier `memcpy`, `std::bit_cast`, hash del contenido del objeto o volcado a disco incluye los dos vptrs, que son direcciones válidas solo en ese proceso y esa ejecución. Deserializar restaura vptrs colgantes: el primer `->` virtual salta a una dirección arbitraria. (Esto ya pasa con un solo vptr; con varios solo hay más balas en el tambor.) Los tipos con interfaces **no son trivially copyable** y el compilador lo dice: `static_assert(std::is_trivially_copyable_v<Widget>)` falla — buen guard-rail para enseñar.

---

## Demo 3 — El compilador genera thunks: funciones que no escribiste

### Código

```cpp
struct IDraw { virtual void draw() = 0; virtual ~IDraw() = default; };
struct ISave { virtual void save() = 0; virtual ~ISave() = default; };

struct Widget : IDraw, ISave {
    int data = 42;
    void draw() override { data++; }
    void save() override { data--; }
};

Widget w;
IDraw* pd = &w;
ISave* ps = &w;
```

```bash
g++ -std=c++20 -O1 -c demo3.cpp -o demo3.o
nm -C demo3.o | grep Widget
```

### Salida

```
W Widget::draw()
W Widget::save()
W Widget::~Widget()
W non-virtual thunk to Widget::save()
W non-virtual thunk to Widget::~Widget()
W non-virtual thunk to Widget::~Widget()
```

### Qué está pasando

Cuando se llama `ps->save()`, el `this` que recibe la función vale `&w + 8` (Demo 2). Pero el cuerpo de `Widget::save` accede a `data` asumiendo el `this` del objeto completo. Alguien tiene que restar esos 8 bytes.

La solución del ABI es el **thunk**: una microfunción generada por el compilador que ajusta `this` y salta a la implementación real:

```
non-virtual thunk to Widget::save():
    sub  rdi, 8                ; this -= 8
    jmp  Widget::save()
```

La sub-vtable de `ISave` no apunta a `Widget::save` sino al thunk. Observaciones que la salida demuestra:

- Hay thunk para `save` y **no** para `draw`: la base primaria comparte `this` con el objeto completo y llama directo. Otra vez: el orden de herencia tiene efectos físicos.
- El **destructor** tiene dos thunks (en Itanium ABI existen el destructor "completo" D1 y el "deleting" D0, y cada uno necesita el suyo). Esto es lo que hace que `delete ps;` funcione: el thunk reajusta `this` antes de destruir, y la entrada D0 libera la memoria desde la dirección del objeto completo, no desde la del subobjeto. Sin destructor virtual en la interfaz, `delete ps;` sería UB doble: destrucción parcial **y** `free()` de un puntero desplazado 8 bytes — que típicamente revienta el heap. Con herencia simple ese mismo error a veces "funciona"; con múltiple, no.

### Cuándo da problemas

El coste directo es minúsculo: 1-2 instrucciones, salto incondicional bien predicho. Los problemas reales son de segundo orden:

**1. Hot loops con despacho virtual masivo por la base no-primaria.** Si un bucle procesa millones de elementos por segundo vía `ISave*`, se paga thunk + la llamada virtual. Pero hay que ser honesto en clase: el coste dominante es la **llamada virtual en sí** (impide inlining, que es de donde el optimizador saca todo lo demás: vectorización, hoisting, eliminación de código). El thunk añade un porcentaje pequeño sobre un coste que ya existía. Si ese bucle importa, la solución no es reordenar bases: es eliminar el despacho dinámico del bucle (templates, `std::variant` + `visit`, ordenar por tipo).

**2. Devirtualización frustrada.** Cuando el optimizador puede demostrar el tipo dinámico, convierte la llamada virtual en directa (y la inlinea). Los thunks y las entradas duplicadas en la vtable añaden casos al análisis; GCC y Clang devirtualizan peor a través de bases no-primarias en algunos patrones. Es la diferencia entre "el bucle se vectoriza" y "el bucle hace una call por iteración".

**3. Tamaño de binario y presión de icache.** Cada clase × cada interfaz no-primaria × cada función (incluidos dos destructores) genera un thunk. En una base de código estilo COM con cientos de clases implementando 5-10 interfaces, son miles de símbolos. Afecta a tiempos de link, tamaño del binario (relevante en embedded/WASM) y dispersión del código en caché de instrucciones.

**4. Frontera de depuración y profiling.** En un profiler o un stack trace aparecen `non-virtual thunk to...` como frames propios. No es un bug, pero quien no sabe qué es un thunk pierde tiempo persiguiéndolo.

---

## Demo 4 — La vtable real: dos tablas en una y el offset-to-top

### Código

```cpp
struct IDraw { virtual void draw() = 0; virtual ~IDraw() = default; };
struct ISave { virtual void save() = 0; virtual ~ISave() = default; };

struct Widget : IDraw, ISave {
    int data = 42;
    void draw() override {}
    void save() override {}
};

int main() { Widget w; }
```

```bash
g++ -std=c++20 -fdump-lang-class demo4.cpp -o demo4
ls *.class          # genera demo4.cpp.001l.class (el nombre varía)
```

### Salida (sección relevante del fichero .class)

```
Vtable for Widget: 11 entries
 0   0                         ← offset-to-top de la vista IDraw
 8   typeinfo for Widget
16   Widget::draw              ┐
24   Widget::~Widget           │ sub-vtable de IDraw (primaria)
32   Widget::~Widget           ┘
40   Widget::save
48   -8                        ← offset-to-top de la vista ISave
56   typeinfo for Widget
64   thunk to Widget::save     ┐
72   thunk to ~Widget          │ sub-vtable de ISave
80   thunk to ~Widget          ┘

Class Widget   size=24
  IDraw  offset 0   vptr = vtable+16   (primary)
  ISave  offset 8   vptr = vtable+64
```

### Qué está pasando

Esta demo une las tres anteriores. "La vtable de Widget" son en realidad **dos sub-vtables consecutivas**, y cada vptr del objeto (Demo 1) apunta al arranque de la suya. Encima de cada grupo de punteros a función hay dos campos ocultos:

- **offset-to-top**: la distancia desde esa vista hasta el objeto completo. `0` para la vista primaria, `-8` para `ISave`. Es el dato que usa `dynamic_cast<void*>` para recuperar la identidad real (Demo 2), y la versión "en tabla" del ajuste que los thunks hacen "en código" (Demo 3).
- **typeinfo**: el RTTI, idéntico en ambas vistas — las dos saben que el objeto completo es un `Widget`.

Con esto se puede explicar mecánicamente qué hace cada operación:

| Operación | Mecanismo | Coste |
|---|---|---|
| `pd->draw()` | vptr → tabla+16, call directa | llamada virtual normal |
| `ps->save()` | vptr → tabla+64, call al thunk, ajuste de `this` | virtual + 2 instr. |
| `ISave* s = w` (upcast) | suma constante en compilación | 1 instr. |
| `static_cast<Widget*>(ps)` | resta constante, sin comprobación | 1 instr. |
| `dynamic_cast<Widget*>(ps)` | lee offset-to-top, comprueba RTTI | decenas de instr. |
| `dynamic_cast<IDraw*>(ps)` (cross-cast) | recorre el grafo RTTI en runtime | **el más caro: ~50-200 ciclos** |

### Cuándo da problemas

**1. Cross-casts entre interfaces hermanas en rutas calientes.** `dynamic_cast<IDraw*>(ps)` no es aritmética: ejecuta `__dynamic_cast` en la biblioteca de runtime, recorriendo la estructura de typeinfo (con comparaciones de cadenas de nombres mangleados en algunos casos entre bibliotecas dinámicas). Patrón de riesgo típico: un sistema de plugins/componentes donde cada frame o cada petición se hace `queryInterface`-style con `dynamic_cast` para descubrir capacidades. Si el cross-cast está en el bucle, está mal puesto: se resuelve una vez y se cachea el puntero ya convertido.

Señal de diseño que conviene enseñar junto al coste: **cross-casts frecuentes indican que la segregación está mal trazada**. Si el consumidor casi siempre necesita `IDraw` *y* `ISave`, su dependencia real es la pareja; dale una referencia que exponga ambas en lugar de hacerle pescar la segunda con RTTI.

**2. `-fno-rtti`.** Proyectos de juegos y embedded compilan sin RTTI por tamaño de binario. Sin RTTI no hay `dynamic_cast`, y con herencia múltiple eso elimina la única forma *comprobada* de navegar la jerarquía: quedan los `static_cast` a ciegas del problema 3 de la Demo 2. Las bases de código en esa situación acaban reimplementando RTTI a mano (un `virtual int typeId()` + tablas de offsets), que es exactamente el mecanismo de la vtable hecho artesanalmente y con más bugs.

**3. Fronteras de ABI / dlopen.** El layout de la vtable (orden de entradas, thunks, offsets) lo fija el ABI, y en Linux/macOS el Itanium ABI es estable — por eso los plugins via interfaces puras funcionan. Pero: añadir una función virtual a una interfaz, reordenar las bases o insertar una interfaz nueva en medio de la lista de herencia **recoloca offsets y sub-vtables enteras**, rompiendo silenciosamente todos los binarios compilados contra la versión anterior. Con herencia simple, añadir métodos *al final* de la única vtable es (frágilmente) compatible; con múltiple, la segunda sub-vtable se desplaza y no hay añadido seguro. Es el motivo por el que COM congeló sus interfaces como inmutables: con herencia múltiple, la única política de versionado viable es "una interfaz publicada no se toca jamás; se crea `IFoo2`".

**4. `offsetof` y código que asume el layout.** `offsetof` sobre tipos no-standard-layout (cualquiera con virtuales) es condicionalmente soportado en el mejor de los casos; los miembros no empiezan en 0 (empiezan en 16 en `W2`); y el padding final puede ser reutilizado por una clase derivada. Cualquier interoperabilidad con código que espere structs planos (drivers, formatos binarios, FFI) debe usar tipos separados sin virtuales.

---

## Síntesis: el mapa de riesgo completo

Los cuatro efectos, ordenados por impacto real en sistemas en producción:

1. **Memoria/caché (Demo 1)** — el único efecto capaz de degradar el rendimiento un orden de magnitud. Aparece con objetos pequeños × cantidades masivas × recorridos. Solución: no usar interfaces en los *datos*; usarlas en los *servicios*. O polimorfismo estático (concepts/templates), que cumple ISP con cero vptrs.

2. **Identidad y casts (Demo 2)** — el efecto que más bugs causa, no por coste sino por corrupción. Aparece en fronteras `void*`, comparaciones de identidad y serialización. Soluciones: ida y vuelta por el mismo tipo, `dynamic_cast<void*>` para identidad, `static_assert(is_trivially_copyable_v<T>)` como centinela.

3. **Cross-casts y RTTI (Demo 4)** — coste real solo si está en rutas calientes o si se compila sin RTTI. Y su frecuencia es un *smell* de segregación mal trazada, independientemente del coste.

4. **Thunks (Demo 3)** — el más citado y el menos importante. Dos instrucciones sobre una llamada que ya era virtual. Su relevancia práctica es indirecta: binario más grande, devirtualización más difícil, frames raros en el profiler.

## Conclusión para ISP: tres mecanismos, mismo principio

ISP dice que los **clientes** no dependan de métodos que no usan. No dice que cada capacidad deba ser una clase base abstracta. C++ tiene tres formas de cumplir el principio, y la elección se decide con los datos de las demos. Para verlo, el mismo problema resuelto de las tres maneras: una función `render` que solo necesita poder dibujar.

### Mecanismo 1 — Interfaz abstracta (lo que mide este documento)

```cpp
struct IDraw { virtual void draw() = 0; virtual ~IDraw() = default; };

struct Circle : IDraw {
    int radius;
    void draw() override { /*...*/ }
};

void render(IDraw& d) { d.draw(); }
```

Coste: `sizeof(Circle)` = 16 (Demo 1), llamada virtual sin inlining, y `Circle` queda **obligado a heredar**. A cambio: `render` puede compilarse hoy y recibir tipos que se escribirán mañana — plugins, bibliotecas, inyección de dependencias.

### Mecanismo 2 — Concept (ISP sin herencia)

```cpp
template<typename T>
concept Drawable = requires(T t) { t.draw(); };

struct Circle {            // no hereda de NADA
    int radius;
    void draw() { /*...*/ }
};

void render(Drawable auto& d) { d.draw(); }
```

`render` sigue sin poder tocar nada que no sea `draw()` — si intenta `d.save()`, error de compilación. Eso **es** ISP: el cliente no depende de lo que no usa. Y los costes del documento desaparecen, comprobable con las mismas demos: `sizeof(Circle)` = 4, cero vptrs, cero thunks, la llamada se inlinea. El precio es otro: `render` debe ser template (vive en un header) y solo acepta tipos conocidos en compilación — no puede recibir un tipo cargado de un plugin.

### Mecanismo 3 — Type erasure ("borrar el tipo")

```cpp
struct Circle {            // tampoco hereda de nada
    int radius;
    void draw() { /*...*/ }
};

void render(std::function<void()> draw) { draw(); }

Circle c;
render([&c] { c.draw(); });   // el tipo Circle se "borra" dentro del function
```

`std::function` acepta cualquier cosa invocable sin que esa cosa herede de nada: por dentro construye él la pequeña vtable que en el mecanismo 1 construía el compilador sobre `Circle`. Coste: una indirección al llamar (comparable a virtual) y posible reserva en heap. A cambio, `Circle` queda limpio — sigue siendo trivially copyable, sigue midiendo 4 bytes — y `render` no es template.

### La tabla de decisión

Cada celda está demostrada arriba, no enunciada:

| | ¿`Circle` paga vptr? | ¿Inlining? | ¿Acepta tipos desconocidos en compilación? |
|---|---|---|---|
| Interfaz abstracta | Sí (Demo 1) | No | Sí |
| Concept | No | Sí | No |
| Type erasure | No (paga el wrapper, no el tipo) | No | Sí |

De la tabla sale el criterio sin necesidad de dogma: si el tipo se instancia por millones, la columna 1 te echa de las interfaces (Demo 1, efecto caché); si necesitas plugins o inyección de dependencias, la columna 3 te echa de los concepts; si no quieres ninguna de las dos servidumbres, type erasure.

---

*Entorno de las mediciones: g++ 13.x, `-std=c++20`, Linux x86-64, Itanium C++ ABI. En MSVC x64 los tamaños y offsets de las Demos 1 y 2 son idénticos; el equivalente de `-fdump-lang-class` es `cl /d1reportSingleClassLayoutWidget`, y en Clang, `-Xclang -fdump-vtable-layouts`.*
