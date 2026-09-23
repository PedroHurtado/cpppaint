# Día 4 — Cierre del curso

Último día. Una sola jornada de **8:45 a 15:00** (6h 15min en total, ~5h 45min
efectivas descontando dos pausas cortas). Toca cumplir las cinco promesas
pendientes del día 2 (Strategy, Command, Observer, Memento, Visitor),
completar los cuatro estructurales restantes (Bridge, Facade, Proxy,
Flyweight), pasar los cuatro patrones de comportamiento sin promesa
(Chain of Responsibility, Mediator, State, Template Method), y cerrar
con el mapa enterprise como referencia conceptual.



## Cómo respirar el día

Los markdowns siguen siendo material extendido (el alumno se los lleva).
**En clase aprietas el ritmo** como has hecho todo el curso: lo extenso
es para que se lleven a casa, no para discurso oral.

Tres bloques en el día:

- **Bloque 1 (08:45–10:20):** los 4 estructurales restantes.
  Cierra la familia estructural. Bridge primero porque resuelve algo
  parecido a Abstract Factory por otra vía, y nos sirve de puente
  conceptual con el día 3.
- **Bloque 2 (10:35–12:50):** los 5 prometidos de comportamiento.
  El corazón del día. Aquí se rellenan los `TODO` que dejamos en
  `Lienzo::anadir` y `Lienzo::limpiar`. Strategy y Command primero
  (cambios visibles en el Paint), luego Observer (cose la GUI),
  Memento (alternativa a Command para undo) y Visitor (cierra OCP
  por el otro lado).
- **Bloque 3 (13:05–15:00):** los 4 de comportamiento sin promesa,
  recap, enterprise y cierre del curso.

## Si el ritmo se desvía

El bloque 3 es el más comprimido. Plan de contingencia:

- Si vas con retraso al llegar a Mediator/State/TM, **acórtalos a
  15 min** cada uno (no a 20). El material extendido se lo llevan
  igualmente.
- **Patrones enterprise** se da rápido (es un glosario, no patrones
  para aplicar). Puede comprimirse de 20 a 12-15 min si hace falta.
- El **recap "Trabajando con patrones"** no es opcional: es lo que
  cierra el arco SOLID → patrones. Defiéndelo, aunque sea breve.
- El **cierre del curso** (las 13 promesas, las 3 ideas para llevarse)
  cierra el arco completo. **Siempre se da**, aunque sea con prisa.

## Decisión de diseño del día

Igual que el día 3, **todos los GoF van sobre el mismo Paint**. Lo que
empezó con `Shape` el día 1 termina hoy con Singleton + Observer +
Command + Memento conviviendo sin pisarse. Esa es la prueba final de
que SOLID + patrones es una arquitectura, no una colección de trucos.

Los **patrones enterprise** son distintos: vienen de Java EE / .NET,
resuelven problemas de aplicaciones web y de tres capas, y **no aplican
naturalmente a C++**. Los vemos como mapa conceptual para que el alumno
los reconozca si los encuentra en otros entornos. Sin código C++.

## Mantra del día

> *"Hoy coordinamos. Los objetos que creamos ayer hablan entre ellos
> hoy. Y el Paint sigue siendo el mismo Paint."*
