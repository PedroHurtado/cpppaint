# `explicit`, `constexpr`, `noexcept` y move semantics

Resumen para clase. Tres palabras clave de C++ moderno, y cómo `noexcept` se conecta con la decisión de **copiar** o **mover** en `std::vector`.

---

## 0. `explicit`

**Versión:** C++98 (constructores de un argumento). C++11 lo amplía a operadores de conversión y a constructores con lista de inicialización. C++20 añade `explicit(expr)` condicional.

**Problema que resuelve:** un constructor de un solo argumento actúa como conversión implícita automática, lo que provoca bugs silenciosos.

```cpp
struct Buffer {
    explicit Buffer(int size);
};
Buffer b = 10;   // error: la conversión implícita está prohibida
Buffer b(10);    // ok: conversión intencionada
```

`explicit` obliga a que la conversión sea deliberada.

---

## 1. `constexpr`

**Versión:** C++11 (funciones muy limitadas: un solo `return`). C++14 lo relaja mucho (bucles, variables, ramas). C++17 añade `if constexpr`. C++20 permite memoria dinámica, `virtual`, etc.

**Problema que resuelve:** el tamaño de un array y los argumentos de plantilla deben conocerse en **tiempo de compilación**. Una función normal se ejecuta en runtime, así que no sirve:

```cpp
int doble(int x) { return x * 2; }
int arr[doble(5)];   // ERROR: doble() se ejecuta al correr, no al compilar
```

`constexpr` le dice al compilador *"esta función es lo bastante simple para ejecutarla tú durante la compilación si los argumentos son constantes"*:

```cpp
constexpr int doble(int x) { return x * 2; }
int arr[doble(5)];   // ok: el compilador calcula 10 al compilar
```

Idea de fondo: mover trabajo de ejecución a compilación → coste cero en runtime.

---

## 2. lvalue vs rvalue

- **lvalue**: tiene nombre, persiste, se le puede "señalar". Va a seguir existiendo.
- **rvalue**: temporal, sin nombre, a punto de desaparecer (un literal, el resultado de una operación, lo que devuelve una función sin guardar).

Regla mnemotécnica: lo que puede ir a la **izquierda** de un `=` suele ser lvalue; lo que solo puede ir a la **derecha**, rvalue.

```cpp
int x = 5;
x;          // lvalue
5;          // rvalue
x + 1;      // rvalue (temporal)
```

---

## 3. Copiar vs mover

Trasladar un objeto a otro sitio se puede hacer de dos formas:

- **Copiar**: duplica el contenido. El original queda intacto. Seguro pero lento.
- **Mover**: le "roba las tripas" al original (copia punteros internos, no los datos). Rápido, pero deja el original vacío.

**Mover es seguro solo cuando el origen es desechable** (un rvalue): si se va a destruir igualmente, da igual dejarlo vacío.

---

## 4. El constructor de move y `&&`

C++ distingue lvalue de rvalue con el tipo del parámetro:

```cpp
class MiClase {
    MiClase(const MiClase& otro);   // &  → recibe lvalue → CONSTRUCTOR DE COPIA
    MiClase(MiClase&& otro) noexcept;// && → recibe rvalue → CONSTRUCTOR DE MOVE
};
```

El compilador elige automáticamente:

```cpp
MiClase a;
MiClase b = a;             // a tiene nombre (lvalue) → COPIA
MiClase c = MiClase();     // temporal (rvalue)       → MOVE
```

Implementación típica del move:

```cpp
MiClase(MiClase&& otro) noexcept {
    datos = otro.datos;     // roba el puntero
    otro.datos = nullptr;   // deja al otro vacío para que no libere lo que ya es nuestro
}
```

---

## 5. `std::move`

**`std::move` no mueve nada.** Solo reetiqueta un lvalue como rvalue (es un `static_cast`), para *forzar* que se elija el constructor de move.

```cpp
MiClase a;
MiClase b = std::move(a);   // ahora se mueve: 'a' queda vacía
// a partir de aquí NO usar el contenido de 'a' (solo destruirla o reasignarla)
```

Es una promesa del programador: *"ya no voy a usar el contenido de esto"*.

Implementación (esencialmente una línea):

```cpp
template <typename T>
constexpr std::remove_reference_t<T>&& move(T&& arg) noexcept {
    return static_cast<std::remove_reference_t<T>&&>(arg);
}
```

Caso de uso típico:

```cpp
std::vector<std::string> lista;
std::string nombre = "Pedro";
lista.push_back(nombre);             // COPIA: 'nombre' sigue usable
lista.push_back(std::move(nombre));  // MUEVE: roba el texto, 'nombre' queda vacía
```

---

## 6. `noexcept`: la promesa de no lanzar

Dos usos con la misma palabra:

```cpp
void f() noexcept;        // ESPECIFICADOR: marca f como "no lanza"
bool b = noexcept(f());   // OPERADOR: pregunta "¿f() no lanza?" → true/false (en compilación)
```

El **operador** no ejecuta el código: solo inspecciona los tipos y las marcas `noexcept`. Por eso la respuesta se conoce en tiempo de compilación, con coste cero en ejecución.

---

## 7. Cómo lo usa `std::vector` (el punto clave)

Cuando un `vector` se llena y necesita crecer:

1. Pide un bloque de memoria más grande.
2. Traslada todos los elementos al bloque nuevo.
3. Destruye el bloque viejo.

En el paso 2 decide entre **mover** o **copiar**:

> Mueve **solo si** el constructor de move del tipo está marcado `noexcept`. Si no, copia.

**¿Por qué?** Por la *strong exception guarantee*. Si moviera y una excepción saltara a mitad del traslado, el bloque viejo ya estaría destrozado (elementos vaciados) y no habría forma de recuperar el estado original. Copiar deja el bloque viejo intacto, así que si algo falla, se descarta el nuevo y no pasa nada.

La comprobación, en código:

```cpp
if (std::is_nothrow_move_constructible<T>::value) {
    // mover cada elemento  (rápido)
} else {
    // copiar cada elemento (seguro)
}
```

`std::is_nothrow_move_constructible<T>` es un atajo que por dentro usa el operador `noexcept`. Devuelve un `bool` constante de compilación, así que el `if` se resuelve al compilar y la rama no usada desaparece del binario.

En la práctica el vector usa `std::move_if_noexcept`, que empaqueta justo esta lógica: entrega el elemento como rvalue (para mover) si el move es `noexcept`, o como lvalue (para copiar) si no.

---

## 8. Conclusión práctica para el programador

- **Marca tus constructores de move con `noexcept`** siempre que de verdad no lancen. Si no lo haces, los contenedores copiarán en vez de mover y perderás rendimiento sin darte cuenta.
- Tú **no** necesitas comprobar `noexcept` normalmente: eso lo hace el contenedor. Tu trabajo es **poner la marca**.
- `std::is_nothrow_*` y el operador `noexcept(...)` solo los usarás si escribes código genérico que tenga que decidir según si algo lanza.

```cpp
class MiClase {
    MiClase(MiClase&&) noexcept;   // ← tu responsabilidad: poner la marca
};
```

---

## Resumen en una frase

`std::move` reetiqueta un lvalue como rvalue para activar el constructor de move (`&&`), que roba recursos en vez de copiarlos; y los contenedores como `vector` solo se atreven a mover si ese constructor está marcado `noexcept`, porque es la única forma de garantizar que un fallo a mitad del traslado no deje la estructura irrecuperable.
