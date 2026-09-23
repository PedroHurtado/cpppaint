# Paso de parámetros en C++: valor, referencia y puntero

## 1. Escenario inicial

```cpp
class Foo {
public:
    int id;
    explicit Foo(int id) : id(id) {}
};

void fn(Foo foo)   { foo.id = 20; }
void fn1(Foo& foo) { foo.id = 20; }
void fn2(Foo* foo) { foo->id = 20; }
```

## 2. Qué ocurre en cada función

### `fn(Foo foo)`: paso por valor
Se crea una **copia** del objeto dentro de la función. `foo.id = 20` modifica la copia, que se destruye al salir. El original **no cambia**.

### `fn1(Foo& foo)`: paso por referencia
No se pasa la dirección de memoria: `foo` es un **alias** del objeto original, el mismo objeto con otro nombre. `foo.id = 20` modifica el original y **sí cambia**.

### `fn2(Foo* foo)`: paso por puntero
Se pasa una **copia de la dirección de memoria** del objeto. `foo->id = 20` accede al original a través de esa dirección y **sí cambia**. Se llama con `fn2(&obj)` y el puntero podría ser `nullptr`.

| Función | Qué recibe | ¿Se crea un `Foo` nuevo? | ¿Cambia el original? |
|---|---|---|---|
| `fn(Foo)` | Una copia del objeto | Sí | No |
| `fn1(Foo&)` | Un alias del objeto | No | Sí |
| `fn2(Foo*)` | Una copia de la dirección | No | Sí |

## 3. Cómo demostrarlo: constructores de copia y move instrumentados

Si los constructores imprimen un mensaje, se ve exactamente qué objetos se crean en cada llamada:

- **`fn(obj)`** imprime `[constructor copia]`: el parámetro se construye a partir de `obj`. Prueba de que dentro se trabaja con otro objeto.
- **`fn(std::move(obj))`** imprime `[constructor move]`: al pasar un rvalue se usa el constructor de movimiento. `obj` queda en estado válido pero no especificado (aquí `id` sigue siendo 10 porque mover un `int` es copiarlo).
- **`fn(Foo(10))`** no imprime ni copia ni move en C++17: el temporal se construye directamente en el parámetro (*copy elision* garantizada).
- **`fn1(obj)`** y **`fn2(&obj)`** no imprimen nada: no se crea ningún objeto `Foo`.

## 4. Código completo

```cpp
#include <iostream>
#include <utility>

class Foo {
public:
    int id;

    explicit Foo(int id) : id(id) {
        std::cout << "  [constructor]\n";
    }

    Foo(const Foo& other) : id(other.id) {
        std::cout << "  [constructor copia]\n";
    }

    Foo(Foo&& other) noexcept : id(other.id) {
        std::cout << "  [constructor move]\n";
    }

    ~Foo() {
        std::cout << "  [destructor id=" << id << "]\n";
    }
};

void fn(Foo foo)   { foo.id = 20; }
void fn1(Foo& foo) { foo.id = 20; }
void fn2(Foo* foo) { foo->id = 20; }

int main() {
    std::cout << "--- Foo obj(10) ---\n";
    Foo obj(10);

    std::cout << "\n--- fn(obj): por valor ---\n";
    fn(obj);
    std::cout << "obj.id = " << obj.id << "\n";

    std::cout << "\n--- fn(std::move(obj)): por valor con rvalue ---\n";
    fn(std::move(obj));
    std::cout << "obj.id = " << obj.id << "\n";

    std::cout << "\n--- fn(Foo(10)): temporal ---\n";
    fn(Foo(10));

    std::cout << "\n--- fn1(obj): por referencia ---\n";
    fn1(obj);
    std::cout << "obj.id = " << obj.id << "\n";

    obj.id = 10;

    std::cout << "\n--- fn2(&obj): por puntero ---\n";
    fn2(&obj);
    std::cout << "obj.id = " << obj.id << "\n";

    std::cout << "\n--- fin de main ---\n";
}
```

## 5. Salida en C++17

```bash
g++ -std=c++17 main.cpp -o main && ./main
```

```
--- Foo obj(10) ---
  [constructor]

--- fn(obj): por valor ---
  [constructor copia]
  [destructor id=20]
obj.id = 10

--- fn(std::move(obj)): por valor con rvalue ---
  [constructor move]
  [destructor id=20]
obj.id = 10

--- fn(Foo(10)): temporal ---
  [constructor]
  [destructor id=20]

--- fn1(obj): por referencia ---
obj.id = 20

--- fn2(&obj): por puntero ---
obj.id = 20

--- fin de main ---
  [destructor id=20]
```

El `[destructor id=20]` tras `fn(obj)` demuestra que se modificó y destruyó la **copia**, mientras `obj` sigue valiendo 10.

## 6. Recibir un parámetro con `&` o con `*`

### Qué pasa

En ambos casos **no se crea ningún `Foo`**: la función trabaja sobre el objeto original.

- **`Foo&`**: la referencia se liga al objeto en la llamada y ya no puede cambiar. Se usa como el propio objeto (`foo.id`).
- **`Foo*`**: se copia una dirección (8 bytes en 64 bits). Se accede con `->` y el puntero puede ser `nullptr` o apuntar a otro objeto.

### Qué ganas frente a pasar por valor

- No hay copia ni move: más eficiente con objetos grandes.
- Puedes modificar el original.

### Qué pierdes frente a pasar por valor

- La función depende de que el original siga vivo: si se destruye, tienes una referencia o un puntero colgante (*dangling*).
- Los cambios dentro afectan fuera, así que hay efectos secundarios.

### `&` frente a `*`

| | `Foo&` | `Foo*` |
|---|---|---|
| Puede ser nulo | No | Sí (`nullptr`) |
| Hay que comprobar nulo | No | Sí |
| Se puede reasignar a otro objeto | No | Sí |
| Sintaxis dentro | `foo.id` | `foo->id` |
| Llamada | `fn1(obj)` | `fn2(&obj)` |
| Se ve en la llamada que puede modificar | No | Sí, por el `&` |
| Acepta temporales `Foo(10)` | Solo `const Foo&` | No |
| Rendimiento | Equivalente | Equivalente |

### Cuándo usar cada uno

- **`const Foo&`**: solo leer un objeto sin copiarlo.
- **`Foo&`**: el objeto es obligatorio y quieres modificarlo.
- **`Foo*`**: el parámetro es opcional (`nullptr` = "no hay") o hay interoperabilidad con C.
- **Punteros inteligentes** (`std::unique_ptr`, `std::shared_ptr`): cuando se transfiere o comparte la propiedad. Un `Foo*` crudo no dice quién debe liberarlo.

## 7. C++14 frente a C++17

La única diferencia está en `fn(Foo(10))`, al pasar un temporal (prvalue) por valor.

| | C++14 | C++17 |
|---|---|---|
| Elisión de copia con temporales | Permitida, no garantizada | Garantizada |
| Constructor move/copia accesible | Obligatorio aunque se elida | No necesario |
| `fn(Foo(10))` con move/copia `= delete` | No compila | Compila |

### C++14 sin elisión

```bash
g++ -std=c++14 -fno-elide-constructors main.cpp -o main && ./main
```

```
--- fn(Foo(10)): temporal ---
  [constructor]
  [constructor move]
  [destructor id=20]
  [destructor id=10]
```

Se construye el temporal, se mueve al parámetro y se destruyen ambos: el parámetro (modificado a 20) y el temporal (que conserva 10).

### C++14 con elisión (por defecto en GCC/Clang)

```bash
g++ -std=c++14 main.cpp -o main && ./main
```

```
--- fn(Foo(10)): temporal ---
  [constructor]
  [destructor id=20]
```

Salida igual que en C++17, pero porque el compilador **decide** optimizar, no porque el estándar lo exija.

> `-fno-elide-constructors` no cambia nada en C++17 para este caso: la elisión es obligatoria.

El resto de secciones (`fn(obj)`, `fn(std::move(obj))`, `fn1`, `fn2`) producen la misma salida en ambos estándares.

## 8. Resumen

- **Por valor:** se crea un objeto nuevo (copia o move); los cambios no afectan al original.
- **Por referencia:** alias del original; no se crea nada y los cambios se ven fuera.
- **Por puntero:** se copia la dirección; no se crea ningún `Foo` y los cambios se ven fuera, pero puede ser `nullptr`.
- **C++14 vs C++17:** con temporales, C++17 garantiza que no hay copia ni move; C++14 solo lo permite.
