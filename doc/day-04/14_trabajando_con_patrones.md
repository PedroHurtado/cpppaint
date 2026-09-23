# Trabajando con patrones

Recap final del Paint. No introducimos patrones nuevos: miramos atrás
y vemos cómo encajan los que llevamos.

## Extensibilidad

Repaso: cada eje de cambio se trata con su patrón.

| Eje de cambio                              | Patrón que lo cubre |
|--------------------------------------------|---------------------|
| Nuevos tipos de figura                     | **Factory con registro** |
| Familias visuales (claro/oscuro)           | **Abstract Factory** |
| Múltiples backends gráficos                | **Bridge**           |
| Algoritmos de pintado intercambiables      | **Strategy**         |
| Decoraciones acumulables (borde, sombra)   | **Decorator**        |
| Composición jerárquica de figuras          | **Composite**        |
| Operaciones nuevas sobre figuras           | **Visitor**          |
| Acciones reversibles del usuario           | **Command**          |
| Snapshots completos del lienzo             | **Memento**          |
| Notificación a vistas                      | **Observer**         |
| Modos del editor (selección, dibujo...)    | **State**            |
| Filtrado de eventos por etapas             | **Chain of R.**      |
| Coordinación entre componentes GUI         | **Mediator**         |
| Esqueleto fijo, pasos variables            | **Template Method**  |
| Una API simple sobre todo lo anterior      | **Facade**           |
| Adaptar librerías externas                 | **Adapter**          |
| Acceso controlado / carga perezosa         | **Proxy**            |
| Compartir representación entre instancias  | **Flyweight**        |
| Garantizar instancia única                 | **Singleton**        |
| Clonar objetos polimórficos                | **Prototype**        |
| Construcción paso a paso                   | **Builder**          |

Veintiún ejes, veintiún patrones. **Cada eje un patrón, y un patrón un
eje.** Cuando un mismo patrón cubre dos ejes, hay que sospechar que
los ejes son uno solo; cuando un eje necesita dos patrones, hay que
sospechar que el eje son dos.

## Encapsulación: qué oculta cada patrón

Otra mirada: los patrones se distinguen por **lo que esconden**.

| Patrón            | Oculta...                                    |
|-------------------|----------------------------------------------|
| Factory           | qué clase concreta se instancia              |
| Abstract Factory  | qué **familia** de clases se instancia        |
| Builder           | el orden y validación de construcción         |
| Singleton         | el control de instanciación                  |
| Prototype         | cómo se copia (lo decide el propio objeto)   |
| Adapter           | una interfaz ajena bajo una propia           |
| Bridge            | la implementación detrás de la abstracción   |
| Composite         | si trato con uno o con muchos                |
| Decorator         | qué responsabilidades se han añadido         |
| Facade            | la complejidad interna del subsistema        |
| Proxy             | si trato con el real o con un intermediario  |
| Flyweight         | que muchos objetos comparten representación  |
| Strategy          | qué algoritmo se está usando                 |
| State             | en qué modo está el objeto                   |
| Command           | qué acción hay detrás del verbo              |
| Memento           | el formato interno del estado guardado       |
| Observer          | quién está mirando                           |
| Mediator          | cómo se coordinan las piezas                 |
| Chain of R.       | quién va a manejar la petición               |
| Visitor           | qué operación se aplicará                    |
| Template Method   | qué partes son fijas y cuáles variables      |

> Esto no es un examen. Es **cómo elegir el patrón**: ¿qué quieres
> ocultar al cliente? Esa pregunta tiene una respuesta canónica.

## Combinando patrones: el Paint completo

Las combinaciones más interesantes que han salido durante el curso:

### Singleton + Observer
El `Lienzo` es único (Singleton) y notifica a sus suscriptores
(Observer). Una pieza central a la que todos hablan y de la que todos
se enteran. Es la arquitectura de un modelo MVC sin escribir "MVC".

### Command + Memento
Command para undo paso a paso (rápido, granular). Memento para
snapshots manuales y save/load (completo, opaco). Conviven sin
pisarse: el `GestorComandos` apila Commands, el cuidador apila
Mementos.

### Composite + Visitor
El `Grupo` (Composite) acepta visitantes y propaga la visita a sus
hijos. Operaciones nuevas (exportar, contar, calcular bbox) recorren
el árbol sin que el árbol cambie. Es uno de los acoplamientos más
clásicos del libro GoF.

### Factory + Prototype
La fábrica con registro no crea cada figura desde cero: tiene un
**prototipo** de cada tipo y lo clona. Variante "fábrica de
prototipos" que ahorra el constructor variable.

### Strategy + State
Strategy elige un algoritmo desde fuera; State cambia el algoritmo
desde dentro. Cuando el editor entra en modo "borrar", **es** un
estado nuevo, pero **usa** una estrategia de pintado distinta. Los
dos a la vez sin conflicto.

### Bridge + Abstract Factory
Bridge separa figura de renderer; Abstract Factory separa familia
clara de oscura. Juntos: una **fábrica de pares** (figura+renderer
coherentes) que cumple las dos invariantes.

### Facade sobre todo lo demás
La `Paint` (Facade) reúne `Lienzo` (Singleton), `FabricaFiguras`
(Factory), `FabricaTema` (Abstract Factory), `Renderer` (Bridge),
`GestorComandos` (Command). Once piezas detrás de cuatro métodos
públicos.

## La regla de los tres niveles

Cuando termines un diseño con patrones, mira si cumple los tres niveles:

1. **Nivel objeto**: cada clase tiene una responsabilidad clara
   (SRP). Cualquier estudiante sabría describirla en una frase.
2. **Nivel patrón**: cada patrón cumple su intención (no su
   estructura), está donde tiene que estar, y es el más simple que
   resuelve el problema.
3. **Nivel sistema**: los patrones se combinan en colaboraciones
   estables (Singleton+Observer, Command+Memento, etc.) que cualquier
   alumno de patrones reconocería.

Si los tres niveles funcionan, no se trata de "haber metido patrones":
se trata de **una arquitectura**. Esa es la diferencia.

## Lo que no son los patrones

Tres avisos antes de cerrar:

- **No son piezas para coleccionar**: si tu código tiene los 23 GoF,
  algo está mal. Cada patrón debe tener un porqué.
- **No son sustitutos del pensamiento**: aplicar Strategy a una
  función que recibe dos enteros y devuelve uno es ridículo. Patrón
  prematuro = sobrecoste sin ventaja.
- **No son la única solución**: muchos problemas se resuelven con un
  `std::function` y un `if`. Si lo que tienes funciona, no metas
  Visitor para sentirte arquitecto.

## Mantra del recap

> *"Los patrones no son piezas que se ensamblan. Son nombres con los
> que reconocemos decisiones de diseño que otros tomaron antes que
> nosotros."*
