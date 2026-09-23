# Día 2 — Bloque 5: DIP (Dependency Inversion Principle)

> *"Los módulos de alto nivel no deben depender de los de bajo nivel.
> Ambos deben depender de abstracciones. Las abstracciones no deben
> depender de los detalles. Los detalles deben depender de las
> abstracciones."* — Robert C. Martin

---

## 1. El problema en una frase

Cuando un módulo importante de tu programa **crea él mismo sus
dependencias concretas**, queda **clavado a esas elecciones**: no puedes
cambiar la base de datos, ni testear sin BBDD real, ni reemplazar el
logger, ni meter un mock.

## 2. Definición formal

Dos partes que conviene leer juntas:

> a) Los **módulos de alto nivel** (lógica de negocio) **no deben depender
> de módulos de bajo nivel** (infraestructura). Ambos deben depender de
> **abstracciones**.
>
> b) Las **abstracciones no deben depender de los detalles**. Los detalles
> deben depender de las abstracciones.

Idea: en lugar de que el alto nivel use directamente al bajo, **el alto
nivel define la interfaz** que necesita, y el bajo nivel **la implementa**.
La dirección de la dependencia se invierte (de ahí el nombre).

DIP suele venir acompañado de **DI** (Dependency Injection), que es el
*cómo* mecánico: en vez de instanciar la dependencia dentro, **te la pasan
desde fuera** (por constructor, normalmente).

> Ojo: DIP ≠ DI. DIP es el **principio**, DI es la **técnica** para
> implementarlo. Pero en la práctica casi siempre van juntos.

## 3. Bad: dependencias concretas clavadas

Un servicio de pedidos que crea su propio logger y su propio cliente HTTP
dentro. No puedes tocar nada sin tocar `PedidoService`.

```cpp
class LoggerFichero {
public:
    void log(const std::string& msg) {
        std::ofstream f("app.log", std::ios::app);
        f << msg << "\n";
    }
};

class ClienteHTTPCurl {
public:
    void post(const std::string& url, const std::string& body) {
        // llama a libcurl
    }
};

class PedidoService {
    LoggerFichero       logger;        // ❌ dependencia concreta
    ClienteHTTPCurl     http;          // ❌ dependencia concreta

public:
    void confirmarPedido(int id) {
        logger.log("Confirmando pedido " + std::to_string(id));
        http.post("https://api.tienda.com/pedido/" + std::to_string(id), "{}");
    }
};
```

### ¿Qué duele?

- **No puedo testear** `PedidoService` sin un fichero `app.log` ni sin
  hacer llamadas HTTP reales. Cada test mete ruido en producción.
- Si decidimos pasar de fichero a syslog → tocar `PedidoService`.
- Si decidimos cambiar libcurl por Boost.Beast → tocar `PedidoService`.
- `PedidoService` (alto nivel, lógica de negocio) está **acoplada a
  detalles de infraestructura** (bajo nivel). Justo lo que DIP prohíbe.

## 4. Good: depender de abstracciones

Primero **el alto nivel define lo que necesita** mediante interfaces:

```cpp
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string& msg) = 0;
};

class IClienteHTTP {
public:
    virtual ~IClienteHTTP() = default;
    virtual void post(const std::string& url, const std::string& body) = 0;
};
```

Después `PedidoService` **recibe** las dependencias en el constructor.
**No las crea, no las elige.** Solo las usa:

```cpp
class PedidoService {
    ILogger*       logger;
    IClienteHTTP*  http;

public:
    PedidoService(ILogger* l, IClienteHTTP* h)
        : logger(l), http(h) {}

    void confirmarPedido(int id) {
        logger->log("Confirmando pedido " + std::to_string(id));
        http->post("https://api.tienda.com/pedido/" + std::to_string(id), "{}");
    }
};
```

Las implementaciones concretas viven en sus propios ficheros:

```cpp
class LoggerFichero : public ILogger {
public:
    void log(const std::string& msg) override {
        std::ofstream f("app.log", std::ios::app);
        f << msg << "\n";
    }
};

class ClienteHTTPCurl : public IClienteHTTP {
public:
    void post(const std::string& url, const std::string& body) override {
        // libcurl
    }
};
```

Y alguien **al inicio del programa** (la *composition root*) decide qué
implementaciones se inyectan:

```cpp
int main() {
    LoggerFichero    logger;
    ClienteHTTPCurl  http;

    PedidoService servicio(&logger, &http);
    servicio.confirmarPedido(42);
}
```

### Para testear: mock como cualquier otra implementación

```cpp
class LoggerMock : public ILogger {
public:
    std::vector<std::string> mensajes;
    void log(const std::string& msg) override { mensajes.push_back(msg); }
};

class ClienteHTTPMock : public IClienteHTTP {
public:
    std::string ultimaUrl;
    void post(const std::string& url, const std::string&) override {
        ultimaUrl = url;
    }
};

// Test
LoggerMock       loggerFake;
ClienteHTTPMock  httpFake;
PedidoService    servicio(&loggerFake, &httpFake);

servicio.confirmarPedido(42);
assert(loggerFake.mensajes.size() == 1);
assert(httpFake.ultimaUrl.find("/42") != std::string::npos);
```

**Sin tocar `PedidoService`**, podemos verificar su comportamiento.

## 5. Variante moderna con `unique_ptr`

Pasar punteros crudos en C++ moderno chirría. Con propiedad explícita:

```cpp
class PedidoService {
    std::unique_ptr<ILogger>      logger;
    std::unique_ptr<IClienteHTTP> http;

public:
    PedidoService(std::unique_ptr<ILogger> l,
                  std::unique_ptr<IClienteHTTP> h)
        : logger(std::move(l)), http(std::move(h)) {}

    void confirmarPedido(int id) {
        logger->log("Confirmando pedido " + std::to_string(id));
        http->post("https://api.tienda.com/pedido/" + std::to_string(id), "{}");
    }
};

// Composition root
int main() {
    auto servicio = PedidoService(
        std::make_unique<LoggerFichero>(),
        std::make_unique<ClienteHTTPCurl>()
    );
    servicio.confirmarPedido(42);
}
```

Si la dependencia es **compartida** con otros servicios, `shared_ptr`. Si
**no es propiedad** del servicio (vive más que él, alguien externo la
gestiona), referencia o puntero crudo no-propietario.

## 6. La inversión, vista en un dibujo mental

**Antes (sin DIP)**, la dependencia apunta del alto al bajo nivel:

```
[PedidoService]  ──depende de──▶  [LoggerFichero]
```

**Después (con DIP)**, las dos cosas dependen de la abstracción:

```
[PedidoService]  ──▶  [ILogger]  ◀──  [LoggerFichero]
```

Eso es la *inversión*: la flecha que iba de alto a bajo, ahora va **del
bajo a la abstracción**. El alto nivel ya no depende del bajo.

> La abstracción **pertenece al módulo de alto nivel**, no al de bajo.
> `ILogger` lo define el dominio, no la infraestructura. Esto importa:
> el alto nivel **dicta** lo que necesita, el bajo nivel **se adapta**.

## 7. Qué hemos ganado y qué hemos pagado

**Ganamos:**

- **Testeo aislado** con mocks/fakes triviales.
- **Sustituir** implementaciones (fichero → syslog → Elastic) sin tocar
  lógica de negocio.
- Lógica de negocio **independiente de detalles** de infra.
- Arquitectura limpia: el dominio en el centro, la infra orbita alrededor.

**Pagamos:**

- Más interfaces, más clases.
- Hay que decidir **quién compone** (composition root). En aplicaciones
  grandes esto se delega en un contenedor IoC (en C++ es menos común que
  en Java/.NET; lo habitual es componer a mano en `main`).
- El que lee código tiene que saltar más: ya no ve "qué logger se usa" en
  `PedidoService`, lo ve en el `main`.

### Cuándo NO aplicarlo

- Si una dependencia **nunca va a cambiar** y **nunca necesitas mockearla**
  (p. ej. `std::string`), no inviertas nada. Sería ruido.
- En clases muy pequeñas y autocontenidas (utilidades sin estado), DIP
  sobra.

## 8. Conexión con patrones

DIP es lo que permite que muchos patrones existan limpiamente. Casi todos
los patrones que **inyectan algo** son DIP en acción:

| Patrón                | Cómo se relaciona con DIP                         |
|-----------------------|---------------------------------------------------|
| **Strategy**          | Inyectamos la estrategia (interfaz) en el contexto |
| **Bridge**            | Inyectamos la implementación (interfaz) en la abstracción |
| **Factory**           | El cliente recibe el producto por su interfaz, no por su clase concreta |
| **Service / Repository** (enterprise) | El service depende de `IRepository`, no de SQLite o de Mongo |
| **Observer**          | El sujeto depende de `IObserver`, no de observadores concretos |

Hay un patrón "no GoF" muy directo que es **Inyección de Dependencias** a
secas. Es literalmente DIP aplicado de forma mecánica.

## 9. Mini-ejercicio mental (3 min)

```cpp
class GeneradorInformes {
    BaseDeDatosOracle bd;     // ❌
    ImpresoraEpson    impresora;   // ❌
public:
    void generarYImprimir() {
        auto datos = bd.consultar("SELECT * FROM ventas");
        impresora.imprimir(datos);
    }
};
```

Refactor con DIP. ¿Qué interfaces extraes? ¿Quién las define? ¿Quién las
implementa? ¿Quién compone?

(Respuesta breve: `IBaseDatos` e `IImpresora` (definidas por el dominio),
`OracleAdapter` y `EpsonAdapter` (implementaciones, infra), `main` o un
factory las compone y se las pasa a `GeneradorInformes` por constructor.
Para test: `BaseDatosFake` e `ImpresoraFake`.)

---

## Resumen del bloque

- DIP = **alto y bajo nivel dependen de abstracciones**, no al revés.
- Las **interfaces las define el alto nivel**, las implementan los detalles.
- En la práctica se materializa con **inyección por constructor**.
- La **composition root** (el `main`, normalmente) es quien decide qué
  implementación concreta entra.
- Hace los tests con mocks triviales y permite sustituir infra sin tocar
  dominio.
- Es el principio que más impacto tiene en **arquitectura**, no solo en
  diseño de clases sueltas.

**Mantra del bloque:**

> *"Tu lógica de negocio no debería saber que existe libcurl, Oracle o
> Windows. Si lo sabe, es porque la tienes acoplada a la decisión de
> ayer."*
