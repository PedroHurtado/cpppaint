# Día 2 — Bloque 1: SRP (Single Responsibility Principle)

> *"Una clase debe tener una sola razón para cambiar."* — Robert C. Martin

---

## 1. El problema en una frase

Cuando una clase hace demasiadas cosas, **cualquier cambio en cualquiera de
ellas la obliga a tocarse**. Y tocar una clase es siempre un riesgo: rompes
tests, rompes a quien la usa, generas merges conflictivos.

## 2. Definición formal

> Una clase debe tener **una única responsabilidad**, entendida como una
> **única razón para cambiar**.

Ojo con la palabra *responsabilidad*: no es *"un solo método"*, ni *"hace
una sola cosa"*. Es **un solo motivo de cambio**.

Un motivo de cambio típico es un **actor** distinto: el equipo de negocio,
el de persistencia, el de presentación, el de auditoría… Si dos actores
distintos pueden pedirte cambios sobre la misma clase, esa clase viola SRP.

## 3. Bad: una clase que hace de todo

Una clase `Empleado` que calcula la nómina, persiste en BBDD y formatea un
informe. Tres responsabilidades, tres actores distintos (RRHH, DBA, gerencia).

```cpp
class Empleado {
    std::string nombre;
    double horasMes;
    double tarifaHora;

public:
    Empleado(std::string n, double h, double t)
        : nombre(std::move(n)), horasMes(h), tarifaHora(t) {}

    // Responsabilidad 1: lógica de negocio
    double calcularNomina() const {
        double base = horasMes * tarifaHora;
        if (horasMes > 160) base += (horasMes - 160) * tarifaHora * 0.25;
        return base;
    }

    // Responsabilidad 2: persistencia
    void guardarEnBD() const {
        std::ofstream f("empleados.txt", std::ios::app);
        f << nombre << ";" << horasMes << ";" << tarifaHora << "\n";
    }

    // Responsabilidad 3: presentación
    std::string informeHTML() const {
        std::ostringstream out;
        out << "<h1>" << nombre << "</h1>"
            << "<p>Nómina: " << calcularNomina() << " €</p>";
        return out.str();
    }
};
```

### ¿Qué duele?

- Si **Hacienda cambia el cálculo de horas extra** → toco `Empleado`.
- Si **migramos de fichero a SQLite** → toco `Empleado`.
- Si **el informe pasa de HTML a PDF** → toco `Empleado`.

Tres razones de cambio. Tres actores. Una sola clase. **Eso es SRP roto.**

Síntomas adicionales:

- El fichero crece sin parar.
- Los tests unitarios se complican: ¿cómo testeo `calcularNomina` sin tocar
  el sistema de ficheros?
- Acoplamiento accidental: cambiar el formato del informe puede romper la
  persistencia si comparten utilidades internas.

## 4. Good: una responsabilidad por clase

Separamos en tres clases, cada una con su motivo de cambio:

```cpp
// Solo datos + lógica de negocio
class Empleado {
    std::string nombre;
    double horasMes;
    double tarifaHora;

public:
    Empleado(std::string n, double h, double t)
        : nombre(std::move(n)), horasMes(h), tarifaHora(t) {}

    const std::string& getNombre() const { return nombre; }

    double calcularNomina() const {
        double base = horasMes * tarifaHora;
        if (horasMes > 160) base += (horasMes - 160) * tarifaHora * 0.25;
        return base;
    }
};

// Solo persistencia
class EmpleadoRepository {
public:
    void guardar(const Empleado& e) {
        std::ofstream f("empleados.txt", std::ios::app);
        f << e.getNombre() << ";" << e.calcularNomina() << "\n";
    }
};

// Solo presentación
class EmpleadoReportHTML {
public:
    std::string generar(const Empleado& e) const {
        std::ostringstream out;
        out << "<h1>" << e.getNombre() << "</h1>"
            << "<p>Nómina: " << e.calcularNomina() << " €</p>";
        return out.str();
    }
};
```

Uso:

```cpp
int main() {
    Empleado emp("Ana", 180, 20.0);

    EmpleadoRepository repo;
    repo.guardar(emp);

    EmpleadoReportHTML reporter;
    std::cout << reporter.generar(emp) << "\n";
}
```

## 5. Qué hemos ganado y qué hemos pagado

**Ganamos:**

- Tres razones de cambio → tres clases independientes.
- Testeo aislado: `calcularNomina()` se prueba sin tocar disco.
- Sustituir el `Repository` por uno SQLite no obliga a tocar `Empleado`.
- El informe HTML puede convivir con uno PDF (`EmpleadoReportPDF`) sin
  duplicar lógica.

**Pagamos:**

- Más clases → más ficheros, más navegación.
- Hay que decidir **quién orquesta** (en el `main` del ejemplo, en un
  `Service` en aplicaciones reales).

### El equilibrio

SRP llevado al absurdo da clases de una sola línea. La pregunta práctica es:

> *"¿Hay dos personas/equipos que podrían pedirme cambios sobre esto por
> motivos distintos?"*

Si la respuesta es sí, separar. Si no, dejar tranquilo.

## 6. Pista falsa: "una clase, un método"

Confusión común. SRP **no** es:

- "Mis clases solo tienen un método público" → eso es absurdo.
- "Mis clases tienen menos de 50 líneas" → métrica accidental.

Una clase `EmpleadoRepository` con `guardar`, `buscar`, `eliminar`, `listar`
sigue cumpliendo SRP: **todos esos métodos cambian por el mismo motivo**
(cambia el almacenamiento). Si el día de mañana añadimos `exportarAExcel`,
*eso* sí sería una responsabilidad nueva (otro actor: el de reporting).

## 7. Conexión con patrones

SRP es el principio que justifica casi todos los patrones que **separan
responsabilidades**:

| Patrón                | Qué responsabilidad extrae                       |
|-----------------------|--------------------------------------------------|
| **Strategy**          | El algoritmo se saca a su propia jerarquía       |
| **Command**           | "Ejecutar acción" se convierte en objeto         |
| **Observer**          | Notificación se separa de la lógica de negocio   |
| **Repository** (enterprise) | Persistencia fuera de la entidad           |
| **Mapper / DTO**      | Conversión de formatos fuera del dominio         |
| **MVC**               | Modelo, vista y controlador con responsabilidades distintas |

Cada vez que veas un patrón que "saca X a otra clase", está aplicando SRP.

## 8. Mini-ejercicio mental (3 min)

Tienes esto en un proyecto:

```cpp
class Pedido {
public:
    void añadirLinea(Producto p, int cantidad);
    double calcularTotal() const;
    void aplicarDescuento(double pct);
    void enviarEmailConfirmacion();   // ❌
    void guardarEnBD();               // ❌
    std::string serializarJSON();     // ❌
};
```

¿Cuántas responsabilidades hay? ¿Cómo lo partirías?

(Respuesta breve: el cuerpo del pedido y los descuentos son lógica de
negocio → `Pedido`. Email → `EmailService`. BBDD → `PedidoRepository`.
JSON → `PedidoSerializer`. Cuatro motivos de cambio, cuatro clases.)

---

## Resumen del bloque

- SRP = **una sola razón para cambiar**.
- Razón ≈ actor (negocio, persistencia, presentación, infraestructura…).
- No es *"un método por clase"*, es *"un motivo de cambio por clase"*.
- Conduce de forma natural a separar lógica, persistencia y presentación.
- Es la justificación filosófica de Strategy, Command, Repository, MVC…

**Mantra del bloque:**

> *"Si dos personas distintas pueden pedirte cambios sobre la misma clase,
> esa clase tiene dos responsabilidades."*
