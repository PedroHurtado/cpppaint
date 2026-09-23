# Cierre del curso

## Las 13 promesas del día 2

Recordemos. En el día 2 prometimos 13 patrones específicos aplicados al
Paint. Tres días después, esta es la cuenta final:

| Prometido día 2  | Cumplido en | Estado |
|------------------|-------------|--------|
| Singleton        | día 3       | ✅     |
| Prototype        | día 3       | ✅     |
| Factory          | día 3       | ✅     |
| Abstract Factory | día 3       | ✅     |
| Builder          | día 3       | ✅     |
| Adapter          | día 3       | ✅     |
| Decorator        | día 3       | ✅     |
| Composite        | día 3       | ✅     |
| Strategy         | hoy         | ✅     |
| Command          | hoy         | ✅     |
| Observer         | hoy         | ✅     |
| Memento          | hoy         | ✅     |
| Visitor          | hoy         | ✅     |

**Trece de trece**. Cada uno con código sobre el mismo `Shape` que
arrancamos el día 1.

## Lo que se llevan

### Día 1 — Fundamentos C++ orientado a objeto

Herencia, composición, delegación. Constructores, destructores
virtuales. `unique_ptr`, `shared_ptr`. La diferencia entre **ser** y
**tener**. CRTP como avanzadilla.

### Día 2 — SOLID

Cinco principios. Aplicados al Paint paso a paso. La conclusión: **un
diseño SOLID no es uno que aplica los cinco principios, es uno que
respira en su dirección**. Esto es lo que hizo que los patrones
encajaran sin reescrituras.

### Día 3 — Creacionales + tres estructurales

Cómo crear objetos es una decisión de diseño. Singleton, Prototype,
Factory (con sus tres variantes), Builder, Abstract Factory. Adapter,
Decorator y Composite cerraron las promesas pendientes.

### Día 4 — Estructurales + comportamiento + enterprise

Bridge, Facade, Proxy, Flyweight cerraron la familia estructural.
Strategy, Command, Observer, Memento, Visitor cumplieron las cinco
promesas restantes. Chain of Responsibility, Mediator, State y
Template Method completaron el GoF. Y los patrones enterprise quedaron
mapeados como referencia conceptual.

## Tres ideas para llevarse a casa

### 1. SOLID es el suelo; los patrones son la casa.

No al revés. Si construyes la casa sin suelo, los patrones se hunden
unos sobre otros. Si tienes el suelo, los patrones encajan casi solos.
La prueba la vivieron ustedes: el día 3 metimos cinco patrones nuevos
sobre el código del día 2 **sin reescribir nada**.

### 2. Cada patrón tiene una **intención**, no una estructura.

Decorator y Proxy se parecen como dos huevos. Bridge y Strategy
también. Template Method y Strategy también. La diferencia no está en
las clases que escribes: está en **qué problema resuelves**. La
estructura es consecuencia.

Cuando alguien te diga "esto es Decorator", pregúntale: ¿qué añades?
Si no sabe responder, no es Decorator.

### 3. Los patrones no son obligatorios.

Si un `std::function` y un `if` resuelven el problema, no metas
Visitor. Cada patrón tiene un coste de mantenimiento, de
comprensión, de líneas. **Patrón prematuro es sobrecoste sin
ventaja**. Y código que no entiende quien viene detrás es código mal
escrito, lleve patrones o no.

## Pregunta de cierre

Si dentro de un año encuentran el código del Paint y tienen que añadir
un nuevo tipo de figura, ¿qué tocan?

> Una clase. Una llamada a `FabricaFiguras::registrar`. Cero ediciones
> en el `Lienzo`. Cero ediciones en las vistas. Cero ediciones en los
> comandos existentes. Cero ediciones en los visitantes existentes
> (excepto añadir el `visitar(NuevaFigura&)` en cada uno).

Eso es OCP cumplido, y es la mejor métrica de "está bien diseñado":
**cambio pequeño = ediciones pequeñas, localizadas, predecibles**.

## Mantra final del curso

> *"Cuando escribas la próxima línea de C++, pregúntate qué cambiará
> mañana. Si tienes la respuesta, ya sabes qué patrón usar — y a veces,
> ya sabes que no usar ninguno."*

---

Gracias por estos cuatro días. Cualquier duda que les surja al volver
a sus proyectos, ya saben dónde encontrarme.
