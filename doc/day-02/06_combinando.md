# Día 2 — Bloque 6: Combinando los principios

> *"SOLID no son cinco principios aislados, son cinco lentes para mirar
> el mismo problema."*

---

## 1. Por qué este bloque

En los cinco bloques anteriores hemos visto cada principio **por separado**.
La realidad es que en cualquier refactor serio aparecen **varios a la vez**.
Y al revés: una violación normalmente **rompe más de uno**.

El objetivo de este bloque es:

1. Mostrar cómo se **encadenan** los principios en un refactor real.
2. Dar **heurísticas** para detectar violaciones múltiples.
3. Dejar claro que **los patrones GoF son SOLID en acción**.

## 2. Ejemplo Bad: el código que rompe los cinco

Vamos con un caso que viola **todos los principios a la vez**. Lo vamos a
arreglar paso a paso.

```cpp
class GestorNotificaciones {
public:
    void notificar(const std::string& usuario,
                   const std::string& mensaje,
                   const std::string& tipo) {
        // EMAIL
        if (tipo == "email") {
            std::cout << "Conectando a SMTP smtp.gmail.com:587...\n";
            std::cout << "Enviando email a " << usuario << ": " << mensaje << "\n";
        }
        // SMS
        else if (tipo == "sms") {
            std::cout << "Conectando a la API de Twilio...\n";
            std::cout << "Enviando SMS a " << usuario << ": " << mensaje << "\n";
        }
        // PUSH
        else if (tipo == "push") {
            std::cout << "Conectando a Firebase Cloud Messaging...\n";
            std::cout << "Enviando push a " << usuario << ": " << mensaje << "\n";
        }

        // Y de paso, logueamos
        std::ofstream f("notif.log", std::ios::app);
        f << "Notificado " << usuario << " por " << tipo << "\n";
    }
};
```

### Diagnóstico: ¿qué viola, y dónde?

| Violación | Dónde se ve                                                    |
|-----------|----------------------------------------------------------------|
| **SRP**   | Una clase decide canal, formatea, envía y loguea. Cuatro responsabilidades. |
| **OCP**   | Añadir WhatsApp = tocar el `if/else if`. Cerrado a extensión está, abierto a modificación. |
| **LSP**   | Aquí no hay jerarquía, así que no aplica en el sentido literal — pero **el día que hagamos la jerarquía mal, aparecerá**. |
| **ISP**   | No hay interfaz aún, pero si la hubiera con `enviarEmail/enviarSMS/enviarPush`, sería un caso de libro: fat interface. |
| **DIP**   | Depende **directamente** de `cout` y de `ofstream` (representando "infra"). Acoplado a SMTP/Twilio/Firebase concretos. |

## 3. Refactor paso a paso

### Paso 1: aplicar SRP — separar canales

Cada canal es una responsabilidad. Una clase por canal:

```cpp
class EnviadorEmail {
public:
    void enviar(const std::string& u, const std::string& m) {
        std::cout << "SMTP → " << u << ": " << m << "\n";
    }
};

class EnviadorSMS {
public:
    void enviar(const std::string& u, const std::string& m) {
        std::cout << "Twilio → " << u << ": " << m << "\n";
    }
};

class EnviadorPush {
public:
    void enviar(const std::string& u, const std::string& m) {
        std::cout << "Firebase → " << u << ": " << m << "\n";
    }
};
```

Ya hemos roto SRP en la dirección correcta. El log lo dejamos para el
paso 4.

### Paso 2: aplicar OCP — interfaz polimórfica

Ahora `GestorNotificaciones` sigue con `if/else if` porque no hay
abstracción común. Extraemos:

```cpp
class INotificador {
public:
    virtual ~INotificador() = default;
    virtual void enviar(const std::string& usuario,
                        const std::string& mensaje) = 0;
};

class NotificadorEmail : public INotificador {
public:
    void enviar(const std::string& u, const std::string& m) override {
        std::cout << "SMTP → " << u << ": " << m << "\n";
    }
};

class NotificadorSMS : public INotificador {
public:
    void enviar(const std::string& u, const std::string& m) override {
        std::cout << "Twilio → " << u << ": " << m << "\n";
    }
};

class NotificadorPush : public INotificador {
public:
    void enviar(const std::string& u, const std::string& m) override {
        std::cout << "Firebase → " << u << ": " << m << "\n";
    }
};
```

Añadir WhatsApp ahora es:

```cpp
class NotificadorWhatsApp : public INotificador {
public:
    void enviar(const std::string& u, const std::string& m) override {
        std::cout << "WhatsApp → " << u << ": " << m << "\n";
    }
};
```

Sin tocar nada anterior. **OCP cumplido.** Y de paso, los notificadores son
**intercambiables** (LSP cumplido por diseño: contrato pequeño y unívoco).

### Paso 3: aplicar ISP — la interfaz mínima

`INotificador` solo tiene **un método**, `enviar`. Eso es ISP llevado al
extremo. Si mañana alguien quiere meter `verificarConfiguracion()` o
`obtenerCuotaMensual()` en `INotificador`, la respuesta correcta es
**otra interfaz aparte**, no engordar esta.

```cpp
class IConfigurableNotificador {
public:
    virtual ~IConfigurableNotificador() = default;
    virtual bool verificarConfiguracion() = 0;
};

// El que sea configurable, hereda ambas:
class NotificadorEmail : public INotificador, public IConfigurableNotificador {
    // ...
};
```

### Paso 4: aplicar DIP — inyectar dependencias

El `GestorNotificaciones` ahora **recibe** los notificadores. No los crea.
Y de paso saquemos el logger fuera:

```cpp
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string& msg) = 0;
};

class LoggerFichero : public ILogger {
public:
    void log(const std::string& msg) override {
        std::ofstream f("notif.log", std::ios::app);
        f << msg << "\n";
    }
};

class GestorNotificaciones {
    INotificador* notificador;
    ILogger*      logger;

public:
    GestorNotificaciones(INotificador* n, ILogger* l)
        : notificador(n), logger(l) {}

    void notificar(const std::string& usuario, const std::string& mensaje) {
        notificador->enviar(usuario, mensaje);
        logger->log("Notificado " + usuario);
    }
};
```

### Paso 5: composition root

Toda la composición concreta se queda al inicio del programa:

```cpp
int main() {
    LoggerFichero    logger;
    NotificadorEmail email;

    GestorNotificaciones gestor(&email, &logger);
    gestor.notificar("ana@example.com", "Bienvenida");
}
```

Y para testear:

```cpp
class NotificadorFake : public INotificador {
public:
    int veces = 0;
    void enviar(const std::string&, const std::string&) override { veces++; }
};

class LoggerFake : public ILogger {
public:
    void log(const std::string&) override {}
};

NotificadorFake n;
LoggerFake l;
GestorNotificaciones g(&n, &l);
g.notificar("test", "hola");
assert(n.veces == 1);
```

## 4. Diagnóstico final

| Principio | Antes        | Después                                |
|-----------|--------------|----------------------------------------|
| SRP       | ❌ todo en una | ✅ canal, log y orquestación separados |
| OCP       | ❌ if/else if  | ✅ jerarquía `INotificador`            |
| LSP       | (n/a)        | ✅ contrato único, sin sorpresas       |
| ISP       | ❌ riesgo de fat interface | ✅ interfaz de un solo método |
| DIP       | ❌ infra clavada | ✅ inyección por constructor       |

## 5. Patrones que aparecen "gratis"

Mira lo que tenemos sin haberlo planeado:

- **Strategy**: `INotificador` con sus implementaciones. El gestor
  *elige una estrategia de notificación*.
- **Dependency Injection**: inyectamos por constructor. Patrón "no GoF"
  fundamental.
- **Adapter** asomando: si mañana metes una librería externa de SMTP,
  envuelves su API en un `NotificadorEmail` que adapta su interfaz a la
  tuya. Eso es Adapter.

**SOLID bien aplicado produce patrones GoF por inercia.** No al revés.

## 6. Heurísticas rápidas para code review

Cuando revises código (tuyo o ajeno), pasa estos cinco filtros mentales:

1. **SRP**: *"¿cuántos motivos de cambio puede haber para esta clase?"*.
   Si más de uno, partir.
2. **OCP**: *"¿qué pasa cuando llegue el siguiente caso?"*. Si la respuesta
   es *"tocar el `switch`"*, polimorfizar.
3. **LSP**: *"¿hay `dynamic_cast` o `if (es instancia de)` cerca?"*. Si sí,
   la jerarquía probablemente está rota.
4. **ISP**: *"¿hay métodos vacíos o que lanzan 'no soportado'?"*. Si sí,
   segregar la interfaz.
5. **DIP**: *"¿esta clase de negocio menciona libcurl, SQLite o algo
   concreto de infra?"*. Si sí, invertir.

Son cinco preguntas. Te llevan dos minutos. Sacan el 80% de los problemas
de diseño.

## 7. Relación con el GoF

Hagamos explícito el puente que pasaremos los próximos dos días:

| Patrón GoF              | Principios que materializa         |
|-------------------------|------------------------------------|
| **Strategy**            | OCP + DIP + ISP                    |
| **Factory Method**      | OCP + DIP                          |
| **Abstract Factory**    | OCP + DIP + SRP                    |
| **Singleton**           | (a menudo *viola* DIP y SRP — lo discutiremos) |
| **Adapter**             | DIP + ISP                          |
| **Decorator**           | OCP + LSP                          |
| **Observer**            | OCP + DIP                          |
| **Template Method**     | OCP + LSP                          |
| **Visitor**             | OCP (a costa de modificar la jerarquía añadiendo `accept`) |
| **Composite**           | LSP (hoja y compuesto sustituibles)|

Cada vez que mañana presentemos un patrón, esta tabla será la pregunta de
fondo: *"¿qué principios SOLID está aplicando este patrón?"*. Si la
respuesta es clara, entiendes el patrón. Si no, no lo entiendes todavía.

---

## Resumen del bloque

- Los cinco principios SOLID **se aplican juntos** en cualquier refactor real.
- Una sola violación suele arrastrar varias: un `switch` (OCP) por tipo
  suele venir con una clase que hace de todo (SRP) y depende de infra (DIP).
- El refactor mecánico es siempre el mismo: **separar responsabilidades,
  introducir interfaces, invertir dependencias, componer fuera**.
- El resultado son **los patrones GoF**, descubiertos por necesidad y no
  copiados de un catálogo.

**Mantra del bloque:**

> *"Los patrones no se aplican. Se descubren cuando aplicas SOLID bien."*
