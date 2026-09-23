# Colofón del curso — Programar con IA, si tienes las bases

> **Se puede programar con IA. Sí — *si* tienes unas bases sólidas, agnósticas
> al lenguaje de programación que utilices.**

Este documento no es teoría de patrones (eso está en `doc/`). Es una reflexión
honesta sobre cómo se ha construido el proyecto [`paint/`](paint/) y qué
demuestra. Todo el código de `paint/` lo ha generado una IA en una única
sesión de trabajo. Lo que sigue explica por qué eso **no** contradice el valor
de aprender fundamentos, sino que lo confirma.

---

## 1. El punto de partida: un prompt, no una especificación

La sesión empezó, literalmente, así:

> *"Estoy impartiendo un curso de C++ a alumnos que están empezando. El objetivo
> como ejercicio es hacer el paint pero sin interfaz gráfica, de forma que
> debemos aplicar el ejemplo a la teoría que hay en los días 2, 3, 4."*

Seguido del código base de los alumnos y una lista de objetivos:

- Entrada por consola (`cin`).
- Factory Method con **registro**.
- Crear y mover figuras → patrón **Command**.
- Definición en `.h`, implementación en `.cpp`.
- *"Como si de un proyecto profesional de C++ se tratase."*
- *"Sobre todo, buenas prácticas."*

Fíjate en lo que **no** hay ahí: ni una sola instrucción de cómo teclear el
código. No se dijo "haz el Canvas un Singleton de Meyers con las cuatro
defensas", ni "inyecta el Canvas en los comandos para no romper la
testabilidad", ni "usa `unique_ptr` y punteros raw no propietarios". Se
describió **qué** y **con qué fundamentos**. El **cómo** lo rellenó la
herramienta — guiada por esos fundamentos.

---

## 2. Lo que salió (datos verificables, están en disco)

| Métrica                          | Valor |
|----------------------------------|------:|
| Ficheros del proyecto            | **32** |
| Cabeceras `.h`                   | 15 |
| Fuentes `.cpp`                   | 13 |
| Ficheros CMake                   | 2 |
| Líneas de código (sin `build/`)  | **882** |
| Patrones aplicados               | 4 |
| Compila sin warnings             | Sí (`-Wall -Wextra -Wpedantic`) |
| Tests                            | Pasan (CTest) |
| Ejecuta                          | Verificado por `cin` |

Los cuatro patrones, **solo los estrictamente necesarios** para que sea
didáctico:

- **Singleton** — `Canvas`, el lienzo único.
- **Prototype** — `IShape::Clone()`, clonar figuras.
- **Factory con registro** — `ShapeFactory`, añadir figuras sin tocarla (OCP).
- **Command** — acciones reversibles con undo/redo.

Y estructurado como un proyecto real: separación interfaz/implementación,
biblioteca + ejecutable + tests, *composition root*, propiedad de la memoria
explícita.

### El tiempo y el coste

Estos dos datos **no** se inventan en una diapositiva: se miden en vivo.
En Claude Code, `/cost` muestra los tokens y el coste de la sesión. Ejecútalo
**delante de los alumnos** y lee el número real. Tiene más fuerza que cualquier
cifra escrita, y encaja con el espíritu del curso: **verificar, no creer.**

---

## 3. La prueba de que las bases son lo que importa

El código generado no es lo interesante. Lo interesante son las **decisiones**
detrás, y las **preguntas** que se hicieron al revisarlo. Durante esta misma
sesión, revisando el código línea a línea, surgieron estas cuestiones:

- ¿Por qué `ToString()` y no `Imprimir(IEscritor&)`? ¿Y por qué C++ no tiene el
  `toString()` implícito de Java/C#? → diferencia entre **sobrecarga** y
  **polimorfismo**, `operator<<`, `std::formatter`.
- En `DuplicateShapeCommand`, `handle_` es un puntero raw: ¿quién destruye el
  objeto? → **propiedad**: `unique_ptr` posee y destruye; el puntero raw solo
  observa, `.get()` no cede propiedad.
- ¿Es apropiado un `.exe` de 2,78 MB? → **enlazado estático** del runtime,
  el compromiso tamaño/autonomía.
- ¿Los módulos de C++20 reducirían eso? → qué resuelven los módulos
  (compilación, encapsulación) y qué **no** (el tamaño lo manda el enlazador).

**Ninguna de esas preguntas se puede formular sin saber C++ y diseño.** Quien no
tiene base no detecta nada de eso: acepta lo que sale y confía. Quien tiene base
revisa, corrige, y *entiende por qué*. Esa es toda la diferencia.

Un ejemplo concreto de esa sesión: al ejecutar, el programa se caía con un
*segfault*. La IA lo diagnosticó (un choque de runtimes de MinGW: el `.exe`
cargaba la `libstdc++` de Git en vez de la correcta) y lo arregló enlazando el
runtime de forma estática. Pero **entender por qué pasó** —y poder explicarlo en
clase— es conocimiento que la herramienta no te da: lo traes tú.

---

## 4. El mensaje, en una frase

La IA **amplifica** criterio; no lo **sustituye**. Te lleva más rápido a donde
ya sabías ir. Si no sabes a dónde vas, te lleva rápido a cualquier sitio.

Las decisiones que dieron forma a este proyecto —qué patrón, por qué, cuándo un
Singleton es legítimo y cuándo es pereza, quién es dueño de cada objeto en
memoria— son **agnósticas al lenguaje**. Son las mismas que tomarías en Java, en
C#, en Rust o en Go. El patrón Command es Command en todos. SOLID es SOLID en
todos. La propiedad de la memoria la razonas igual aunque el `unique_ptr` se
llame de otra forma.

> *"El código lo ha escrito la IA. Pero las decisiones las he tomado yo. Y esas
> decisiones son agnósticas al lenguaje. Aprended los fundamentos: la
> herramienta es lo de menos — y por eso, paradójicamente, es lo que más os va
> a potenciar."*

---

## 5. La tarea de mañana lo cierra todo

Mañana añadiréis la figura `triangle`. Veréis que es **una línea de registro**
en `src/ShapeRegistration.cpp` más su `.h`/`.cpp`, sin tocar la fábrica, ni el
lienzo, ni los comandos, ni nada más. Eso es **OCP** funcionando.

Y esa es la última lección: si los fundamentos están bien aplicados, extender es
trivial — lo hagas tú a mano o se lo pidas a una IA. Porque una IA sobre un
diseño sólido extiende bien; sobre un diseño podrido, multiplica la podredumbre.

**La base no es opcional. La base es lo único que escala.**
