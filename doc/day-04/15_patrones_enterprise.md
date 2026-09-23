# Patrones Enterprise

## Por qué este bloque es distinto

Los patrones que hemos visto hasta ahora son los **GoF**: Gang of Four,
1994. Están pensados para **diseño orientado a objetos en general**:
herencia, composición, polimorfismo. Encajan en cualquier lenguaje OO,
y desde luego en C++.

Los **patrones enterprise** que veremos aquí son otra cosa. Vienen
mayoritariamente de Martin Fowler (*Patterns of Enterprise Application
Architecture*, 2002) y de la cultura Java EE / .NET. Resuelven
problemas que aparecen cuando construyes **aplicaciones de tres
capas**: presentación, lógica de negocio, persistencia.

**En C++ raramente los aplicamos**, por dos razones:

1. **C++ se usa poco para aplicaciones empresariales de 3 capas.**
   Si estamos en C++, normalmente estamos en sistemas embebidos,
   videojuegos, simulación, drivers, motores gráficos. No CRUD sobre
   una base de datos relacional con interfaz web.
2. **Lenguajes que sí los usan** (Java, C#, Python) tienen
   herramientas que C++ no trae de serie: reflexión, ORM maduro,
   inyección de dependencias declarativa, anotaciones. Sin eso,
   varios de estos patrones son antinaturales en C++.

Aun así los vemos. **Por qué**: porque si trabajas en una empresa donde
parte del stack es C++ y parte es Java/.NET, vas a oír estos nombres en
reuniones, en documentación, en revisiones de arquitectura. Y porque
algunos (Repository, DTO, MVC) tienen analogías legítimas en C++
cuando construyes aplicaciones suficientemente grandes.

Este bloque es un **glosario razonado**, no código.

## MVC (Model–View–Controller)

**Qué es.** Una aplicación se divide en tres roles:

- **Model**: los datos y la lógica de negocio. No sabe de pantalla.
- **View**: la presentación. No sabe de lógica de negocio.
- **Controller**: recibe la entrada del usuario, manipula el Model,
  decide qué View se muestra.

**Para qué sirve.** Separar la lógica de negocio de la presentación.
Probar el modelo sin GUI. Cambiar la GUI sin tocar el modelo.

**En C++.** Tiene sentido, sí. Lo hemos hecho **implícitamente**
durante el curso: `Lienzo` es el modelo, las vistas (BarraHerramientas,
PanelCapas) son las views, y `GestorComandos` es algo parecido a un
controller. La diferencia con MVC formal es que MVC está pensado para
ciclos request/response en aplicaciones web, no para GUI nativas
event-driven.

## Service

**Qué es.** Una clase que expone operaciones de negocio sin estado.
Punto de entrada para el "qué hace la aplicación" desde fuera: HTTP,
controladores, otros servicios.

**Para qué sirve.** Centralizar reglas de negocio en un sitio
reutilizable. Que los controladores sean delgados.

**En C++.** Raro como tal, pero la idea de "objeto sin estado que
agrupa operaciones de un dominio" sí aparece. No le solemos llamar
Service, le llamamos Manager, Engine, o simplemente "facade del
módulo".

## Repository

**Qué es.** Una abstracción que **finge ser una colección en memoria**
de entidades, ocultando cómo se persisten:

```
repo.guardar(usuario);
auto u = repo.buscarPorId(42);
auto activos = repo.buscarActivos();
```

Detrás puede haber una base de datos, un fichero, un servicio remoto.
El cliente no se entera.

**Para qué sirve.** Desacoplar la lógica de negocio de la tecnología
de persistencia. Cambiar Postgres por Mongo sin tocar el dominio.
Testear con un repositorio en memoria.

**En C++.** Tiene sentido si la aplicación habla con almacenamiento
externo. En sistemas embebidos, normalmente no. Se ve en backends en
C++ (servidores de videojuegos, sistemas financieros), pero con menos
ceremonia que en Java porque no tenemos ORM declarativo.

## Entity

**Qué es.** Un objeto que tiene **identidad** (un id que lo distingue
incluso si dos instancias tienen los mismos campos). Persiste en algún
sitio.

```
class Usuario {
    Id id;
    Nombre nombre;
    Email email;
};
```

**Para qué sirve.** Modelar las cosas que el negocio realmente tiene:
clientes, pedidos, facturas. Como concepto contrapuesto a **Value
Objects** (cosas sin identidad: un Color, un Dinero, un Punto).

**En C++.** Existe el concepto, pero sin librería que lo formalice. En
Java tienes anotaciones JPA `@Entity`; en C++ es una clase normal con
un miembro `id` y la decisión de qué la persiste.

## Aggregate

**Qué es.** Un grupo de entidades y value objects que se tratan como
una unidad transaccional. Tiene una **raíz** (aggregate root) que es
el único punto de entrada desde fuera.

Ejemplo: un Pedido (raíz) tiene LineasDePedido. Nadie de fuera modifica
una LineaDePedido directamente; lo hace siempre a través del Pedido.

**Para qué sirve.** Garantizar invariantes complejas. Ejemplo: la suma
de las líneas no puede pasar del límite de crédito del cliente. Si
todo pasa por la raíz, esa raíz puede chequear la invariante.

**En C++.** Existe el concepto, lo aplicamos sin nombrarlo. Si tu
clase tiene composiciones que solo se modifican vía métodos públicos
y validan invariantes, ya tienes un aggregate.

## Generic Entity / Generic Repository

**Qué es.** Un repositorio o entidad **parametrizado por tipo**:

```
class GenericRepository<T, Id> {
    void guardar(T);
    T buscarPorId(Id);
    list<T> buscarTodos();
};
```

**Para qué sirve.** No repetir el mismo código para cada entidad
distinta.

**En C++.** Es lo más natural del mundo: `template<typename T>`. C++
fue uno de los primeros lenguajes en tenerlo. La pega: en Java y C#
hubo una época en la que esto era novedad y se le puso nombre. En C++,
es martes.

## Mapper

**Qué es.** Un objeto que **traduce** entre dos representaciones del
mismo dato. Típicamente entre la entidad de dominio y la fila de la
base de datos, o entre la entidad y un DTO.

```
class UsuarioMapper {
    UsuarioDTO   aDTO(const Usuario&);
    Usuario      deDTO(const UsuarioDTO&);
    Usuario      deFila(const FilaSQL&);
};
```

**Para qué sirve.** Que el dominio no sepa de SQL, ni de JSON, ni de
APIs externas. El mapper hace la traducción.

**En C++.** Concepto válido. Lo aplicamos cuando hay capas (red,
disco, dominio) y queremos mantenerlas independientes. Sin ORM, hay
que escribirlo a mano; eso lo hace algo tedioso.

## DTO (Data Transfer Object)

**Qué es.** Un objeto sin lógica, solo datos, que se envía entre
capas: red, servicio, GUI. Casi siempre **serializable**.

```
struct UsuarioDTO {
    int    id;
    std::string nombre;
    std::string email;
};
```

**Para qué sirve.** No exponer las entidades del dominio al mundo
exterior. Versionar APIs sin romper el dominio. Reducir el tráfico
(solo lo que el cliente necesita).

**En C++.** Sí se usa, sobre todo en backends. Es un `struct` plano
con `to_json` / `from_json`. La diferencia con la entidad: la entidad
puede tener métodos, validaciones, invariantes; el DTO es bag of data.

## Business Delegate

**Qué es.** Una pieza que se interpone entre la capa de presentación y
la capa de servicios, escondiendo los detalles de la comunicación
(remota, autenticación, reintentos, manejo de fallos):

```
class PedidoBusinessDelegate {
    PedidoServiceRemoto* remoto;
public:
    Pedido cargarPedido(Id id) {
        // autenticación, reintentos, cache, manejo de timeouts...
        return remoto->buscar(id);
    }
};
```

**Para qué sirve.** Que la capa de presentación no sepa nada de RMI,
HTTP, gRPC, sesiones. Los Business Delegates le ofrecen una API local
limpia.

**En C++.** Concepto válido pero raro con ese nombre. Si el frontend
es C++ y consume servicios remotos, el equivalente suele llamarse
"client wrapper", "service stub", "API client". La intención es la
misma.

## Reactive Services

**Qué es.** Servicios construidos sobre **streams asíncronos** y
backpressure. Reactive Streams, Project Reactor, RxJava, RxJS. La
idea: el servicio no devuelve `T`, devuelve `Observable<T>` (o
`Flux<T>`, `Stream<T>`), y el consumidor decide cuándo y cómo
procesarlo.

**Para qué sirve.** Manejar grandes volúmenes de datos sin cargarlos
en memoria. Componer transformaciones sobre el flujo. Reaccionar a
eventos.

**En C++.** Existen librerías (RxCpp, libcaf), pero el ecosistema es
menos maduro. C++20 trae **coroutines** y `std::generator` (C++23),
que cubren parte del territorio. La filosofía reactiva no es
*natural* en C++; es importable pero con coste.

## Observables

**Qué es.** El bloque base de los Reactive Services. Un Observable es
un productor de cero, uno o muchos valores en el tiempo. Tres roles:
**Subject** (el que produce), **Observer** (el que consume),
**Subscription** (la conexión). Es lo que vimos en el GoF Observer,
pero con un álgebra encima (map, filter, merge, debounce, ...).

**En C++.** Igual que arriba: existe RxCpp. Pero en la mayoría de los
proyectos C++ lo replicas con `std::function` + cola de eventos, sin
la maquinaria completa de Rx.

## Mapa visual

```
                        ┌─────────────────────┐
                        │   Presentación      │  (vista, controlador)
                        │   ─ MVC, View       │
                        └──────────┬──────────┘
                                   │
                                   ▼ (Business Delegate)
                        ┌─────────────────────┐
                        │   Servicios         │
                        │   ─ Service         │
                        │   ─ Reactive S.     │
                        └──────────┬──────────┘
                                   │
                                   ▼ (DTO / Mapper)
                        ┌─────────────────────┐
                        │   Dominio           │
                        │   ─ Entity          │
                        │   ─ Aggregate       │
                        └──────────┬──────────┘
                                   │
                                   ▼ (Repository)
                        ┌─────────────────────┐
                        │   Persistencia      │
                        │   ─ SQL / Mongo /   │
                        │     ficheros        │
                        └─────────────────────┘
```

## Lo que se llevan ustedes de este bloque

- Si trabajan en C++ "puro" (sistemas, embebidos, videojuegos),
  probablemente no apliquen estos patrones con estos nombres.
- Si C++ es parte de un stack que incluye Java o .NET, **van a oír
  estos términos** en reuniones. Saben qué significan: ya está.
- Algunos (Repository, DTO, MVC) tienen aplicaciones legítimas en C++
  cuando la aplicación es suficientemente grande. Saber cuándo aplican
  es lo que distingue al ingeniero del programador.
- Estos patrones se parecen entre sí porque resuelven problemas
  parecidos en el mismo entorno: aplicaciones de 3 capas. No son
  reglas universales del diseño OO. Son reglas universales del
  diseño OO **en aplicaciones de tres capas**.

## Mantra

> *"GoF describe lenguaje OO. Enterprise describe arquitectura de
> 3 capas. Que en una empresa usen los términos no quiere decir que
> aplique a tu C++."*
