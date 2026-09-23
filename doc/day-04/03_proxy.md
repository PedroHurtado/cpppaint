# Proxy

## 1. El problema

En el Paint hemos añadido un tipo de figura nuevo: `ImagenBitmap`. Es
una figura que se carga desde un fichero PNG de varios megabytes, los
decodifica, y los pinta. La carga es lenta (cientos de milisegundos por
fichero) y consume mucha memoria.

Problema 1: si el usuario abre un proyecto con 200 bitmaps, **arrancar
el Paint tarda 30 segundos** porque se carga todo de golpe, aunque solo
se vean diez en pantalla.

Problema 2: hay bitmaps que el usuario sin permisos no debería ver, pero
están en el mismo lienzo.

Problema 3: queremos llevar un contador de cuántas veces se ha pintado
cada bitmap (para una herramienta de telemetría).

Tres problemas distintos, una misma solución estructural: **interponer
algo entre el cliente y el objeto real**.

## 2. Intención (GoF)

> *Proporcionar un sustituto o representante de otro objeto para
> controlar el acceso a él.*

El Proxy implementa la misma interfaz que el objeto real, así que el
cliente no se entera de que está hablando con un intermediario.

## 3. Bad: la lógica de carga, permisos y telemetría dentro de la figura

```cpp
class ImagenBitmap : public Shape {
    std::string ruta;
    std::vector<uint8_t> datos;            // los megabytes
    bool cargada = false;
    int  vecesPintada = 0;
    std::string usuarioActual;

public:
    ImagenBitmap(std::string r, std::string usuario)
        : ruta(std::move(r)), usuarioActual(std::move(usuario)) {
        cargarDeFichero();   // ← se carga aunque nunca se pinte
    }

    void dibujar() const override {
        if (!tienePermiso(usuarioActual, ruta))   // ← lógica de seguridad
            return;
        ++const_cast<int&>(vecesPintada);          // ← telemetría
        // ... pintar los megabytes ...
    }

    double area() const override { /* ... */ return 0.0; }

private:
    void cargarDeFichero() { /* lectura del PNG */ }
    bool tienePermiso(const std::string&, const std::string&) const { return true; }
};
```

`ImagenBitmap` ya no es una figura: es una figura, un cargador, un
guardia de seguridad y un contador. SRP llorando.

## 4. Good: tres proxies, una figura

Primero, una `ImagenBitmap` honesta:

```cpp
class ImagenBitmap : public Shape {
    std::string ruta;
    std::vector<uint8_t> datos;
public:
    explicit ImagenBitmap(std::string r) : ruta(std::move(r)) {
        // carga en construcción, eager, una vez.
        cargarDeFichero();
    }
    void dibujar() const override { /* pinta los datos */ }
    double area() const override   { /* ... */ return 0.0; }
private:
    void cargarDeFichero() { /* lectura del PNG */ }
};
```

### Proxy 1: carga perezosa (virtual proxy)

```cpp
class ImagenBitmapLazy : public Shape {
    std::string ruta;
    mutable std::unique_ptr<ImagenBitmap> real;   // mutable: la creamos en const

    ImagenBitmap& asegurarCargada() const {
        if (!real) real = std::make_unique<ImagenBitmap>(ruta);
        return *real;
    }
public:
    explicit ImagenBitmapLazy(std::string r) : ruta(std::move(r)) {}
    void dibujar() const override { asegurarCargada().dibujar(); }
    double area() const override   { return asegurarCargada().area(); }
};
```

Arranque del Paint: 200 `ImagenBitmapLazy` instanciados → casi cero
coste. Solo se cargan al pintarse.

### Proxy 2: control de acceso (protection proxy)

```cpp
class ImagenBitmapProtegida : public Shape {
    std::unique_ptr<Shape> real;
    std::string usuario;
public:
    ImagenBitmapProtegida(std::unique_ptr<Shape> r, std::string u)
        : real(std::move(r)), usuario(std::move(u)) {}

    void dibujar() const override {
        if (!tienePermiso(usuario)) return;
        real->dibujar();
    }
    double area() const override { return real->area(); }

private:
    bool tienePermiso(const std::string&) const { return true; }
};
```

### Proxy 3: telemetría (logging proxy)

```cpp
class ImagenBitmapMonitorizada : public Shape {
    std::unique_ptr<Shape> real;
    mutable int contador = 0;
public:
    explicit ImagenBitmapMonitorizada(std::unique_ptr<Shape> r)
        : real(std::move(r)) {}

    void dibujar() const override {
        ++contador;
        real->dibujar();
    }
    double area() const override { return real->area(); }
    int veces() const { return contador; }
};
```

Cliente: los tres proxies son `Shape`, encajan como cualquier otra:

```cpp
auto lazy  = std::make_unique<ImagenBitmapLazy>("foto.png");
auto prot  = std::make_unique<ImagenBitmapProtegida>(std::move(lazy), "alice");
auto mon   = std::make_unique<ImagenBitmapMonitorizada>(std::move(prot));
Lienzo::instancia().anadir(std::move(mon));
```

Tres proxies anidados, cada uno una responsabilidad.

## 5. UML rápido

```
              Shape (interfaz)
                ▲       ▲
                │       │
        ImagenBitmap   ImagenBitmapLazy ◇──► ImagenBitmap
        (real)         (proxy)
```

Mismo padre, el proxy delega al real.

## 6. Variantes (GoF)

- **Virtual proxy**: carga perezosa. La del PNG.
- **Protection proxy**: control de acceso. La del usuario.
- **Remote proxy**: el objeto real está en otra máquina. El proxy
  serializa, envía, deserializa. Es lo que hace CORBA, RMI, gRPC.
- **Smart reference / smart pointer**: `shared_ptr` es técnicamente un
  proxy de cuenta de referencias. Cuando llamas `p->x`, hay maquinaria
  detrás.

## 7. ¿Proxy o Decorator?

Mismo esquema (clase que implementa la interfaz y compone una
instancia), distinta intención:

| | Proxy | Decorator |
|--|--|--|
| Para qué | **Controlar acceso** al objeto real | **Añadir comportamiento** |
| ¿Se conoce el real? | A veces el proxy lo crea | El decorador siempre recibe el real |
| Apilamiento | Raro (un proxy basta) | Habitual (varios decoradores) |
| El cliente nota la diferencia | Idealmente no | Idealmente no |

Si te encuentras "decorando" con seguridad o con carga perezosa, en
realidad es un Proxy. Y al revés: si tu Proxy añade un borde o un color,
es un Decorator.

## 8. Variantes y trampas

- **El proxy debe ser invisible**: el cliente no debe saber si tiene
  proxy o real. El día que el cliente hace `dynamic_cast<ImagenBitmap*>`,
  la abstracción se rompe.
- **mutable y const**: si el método público es `const` pero el proxy
  cambia estado (cargar, contar), necesitas `mutable` en el miembro.
  No es trampa: es C++ permitiendo cambios internos que no afectan al
  estado observable.
- **Thread-safety**: si dos hilos llaman `dibujar()` simultáneamente y
  el proxy carga perezoso, dos cargas en paralelo. Usa `std::call_once`
  o un mutex.

## 9. SOLID

| Principio | Cómo lo cumple Proxy |
|-----------|---------------------|
| SRP       | Cada proxy una responsabilidad (cargar, proteger, contar). |
| OCP       | Añadir un nuevo proxy no toca al objeto real ni al cliente. |
| LSP       | El proxy cumple la misma interfaz que el real. |
| ISP       | El proxy expone solo lo que la interfaz exige. |
| DIP       | Cliente depende de `Shape`, no de quién hay detrás. |

## 10. Mantra

> *"Si tu objeto pesa demasiado, es peligroso o necesita ser vigilado,
> ponle un portero con su misma cara."*
