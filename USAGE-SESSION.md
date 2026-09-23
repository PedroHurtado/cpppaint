# Análisis de *usage* de una sesión de agente (Claude Code)

> Material didáctico — curso de C++ / patrones.
> Fuente: `session.jsonl` · Sesión `51e5ff4d-b8de-46b4-b3e7-11538301ba1b` · Modelo `claude-opus-4-8` · 2026-06-04 (12:11 → 14:10).

---

## 1. Qué es el *usage* y por qué importa

Cada vez que el agente "piensa" hace **una llamada a la API**. En esa llamada el modelo:

1. **Lee** un contexto de entrada (instrucciones, archivos, historial de la conversación, resultados de herramientas).
2. **Escribe** una salida (texto + llamadas a herramientas).

El consumo (`usage`) se mide en **tokens** y se reparte en cuatro contadores:

| Campo | Qué es | Coste relativo |
|---|---|---|
| `input_tokens` | Entrada **nueva**, procesada desde cero (sin caché) | normal |
| `cache_creation_input_tokens` | Entrada que se **escribe por primera vez** en la caché | algo más caro que un read |
| `cache_read_input_tokens` | Entrada **reutilizada** desde la caché | ~10× más barato que entrada nueva |
| `output_tokens` | Lo que el modelo **genera** | el más caro por token |

La clave del **prompt caching**: el contexto de un agente es enorme y se repite turno a turno. En lugar de reprocesarlo entero cada vez (carísimo y lento), se cachea: la primera vez se "escribe" (`cache_creation`) y a partir de ahí se "lee" barato (`cache_read`).

---

## 2. Un aviso metodológico importante

El archivo `session.jsonl` tiene **302 líneas** y **118** contienen un bloque `usage`. Pero **no son 118 llamadas reales**.

Cuando un turno del asistente incluye varios bloques (texto + varias llamadas a herramientas), Claude Code escribe **una línea por bloque**, y **todas repiten la misma copia del `usage`**. Si sumas las 118 líneas, inflas el total **~2,8×**.

Lo correcto es **deduplicar por `message.id`** (el identificador de cada respuesta de la API). Al hacerlo quedan **39 llamadas reales**.

| | Sumando las 118 líneas (MAL) | Deduplicado por `message.id` (BIEN) |
|---|--:|--:|
| Llamadas | 118 | **39** |
| TOTAL tokens | 11 562 309 | **4 132 141** |

> Lección: cuidado al medir consumo a partir de logs en bruto — hay que entender la estructura del fichero antes de sumar.

---

## 3. Totales reales de la sesión (39 llamadas)

| Métrica | Tokens | % del total |
|---|--:|--:|
| `input_tokens` (entrada nueva) | 3 511 | 0,08 % |
| `output_tokens` (generado) | 70 815 | 1,71 % |
| `cache_creation_input_tokens` | 115 764 | 2,80 % |
| `cache_read_input_tokens` | 3 942 051 | 95,40 % |
| **TOTAL** | **4 132 141** | 100 % |

---

## 4. Desglose llamada por llamada

`Contexto entrada = In + CacheCreate + CacheRead` = todo lo que el modelo "leyó" en esa llamada.

| # | Hora | In | Out | CacheCreate | CacheRead | Contexto entrada |
|--:|---|--:|--:|--:|--:|--:|
| 1 | 12:11:18 | 2459 | 611 | 4191 | 21 333 | 27 983 |
| 2 | 12:11:40 | 2 | 492 | 3764 | 25 524 | 29 290 |
| 3 | 12:12:02 | 2 | 347 | 16 110 | 29 288 | 45 400 |
| 4 | 12:16:12 | 124 | **24 542** | 13 679 | 45 398 | 59 201 |
| 5 | 12:17:08 | 2 | 2573 | 25 463 | 59 077 | 84 542 |
| 6 | 12:18:04 | 124 | 2258 | 3290 | 84 540 | 87 954 |
| 7 | 12:18:42 | 2 | 2210 | 2880 | 87 830 | 90 712 |
| 8 | 12:19:43 | 124 | 1625 | 2727 | 90 710 | 93 561 |
| 9 | 12:20:11 | 2 | 2870 | 1943 | 93 437 | 95 382 |
| 10 | 12:20:55 | 2 | 2407 | 3265 | 95 380 | 98 647 |
| 11 | 12:21:45 | 2 | 235 | 2479 | 98 645 | 101 126 |
| 12 | 12:23:14 | 124 | 229 | 320 | 101 124 | 101 568 |
| 13 | 12:24:01 | 2 | 372 | 1359 | 101 444 | 102 805 |
| 14 | 12:25:19 | 2 | 897 | 718 | 102 803 | 103 523 |
| 15 | 12:25:56 | 2 | 344 | 974 | 103 521 | 104 497 |
| 16 | 12:26:26 | 2 | 189 | 356 | 104 495 | 104 853 |
| 17 | 12:29:14 | 124 | 1063 | 585 | 104 851 | 105 560 |
| 18 | 12:29:52 | 2 | 1301 | 1746 | 105 436 | 107 184 |
| 19 | 12:30:21 | 2 | 478 | 1406 | 107 182 | 108 590 |
| 20 | 12:30:50 | 2 | 333 | 509 | 108 588 | 109 099 |
| 21 | 12:31:36 | 2 | 479 | 732 | 109 097 | 109 831 |
| 22 | 12:32:52 | 2 | 1946 | 518 | 109 829 | 110 349 |
| 23 | 12:36:14 | 124 | 898 | 1988 | 110 347 | 112 459 |
| 24 | 12:36:54 | 2 | 1221 | 1110 | 112 335 | 113 447 |
| 25 | 12:37:30 | 2 | 766 | 1459 | 113 445 | 114 906 |
| 26 | 12:38:08 | 2 | 223 | 1025 | 114 904 | 115 931 |
| 27 | 12:42:55 | 2 | 573 | 263 | 115 929 | 116 194 |
| 28 | 12:43:42 | 124 | 536 | 605 | 116 192 | 116 921 |
| 29 | 12:44:24 | 2 | 870 | 694 | 116 797 | 117 493 |
| 30 | 12:45:05 | 2 | 875 | 876 | 117 491 | 118 369 |
| 31 | 12:48:10 | 2 | 1635 | 915 | 118 367 | 119 284 |
| 32 | 13:36:42 | 2 | 4801 | 1830 | 119 282 | 121 114 |
| 33 | 13:39:26 | 2 | 37 | 4822 | 121 112 | 125 936 |
| 34 | 13:42:45 | 2 | 3184 | 197 | 125 934 | 126 133 |
| 35 | 13:46:31 | 2 | 21 | 3222 | 126 131 | 129 355 |
| 36 | 14:02:57 | 2 | 1446 | 194 | 129 353 | 129 549 |
| 37 | 14:04:05 | 124 | 2022 | 2043 | 129 547 | 131 714 |
| 38 | 14:09:31 | 2 | 3261 | 2173 | 131 590 | 133 765 |
| 39 | 14:10:26 | 2 | 645 | 3334 | 133 763 | 137 099 |

---

## 5. Qué pasó realmente (lectura de la tabla)

### 5.1 El contexto solo crece
La columna **Contexto entrada** va de **~28 000** (llamada 1) a **~137 000** tokens (llamada 39): casi **×5**. Es la naturaleza de un agente conversacional: cada turno **arrastra todo lo anterior**. Nada se "olvida" mientras dura la sesión; el historial completo se reenvía en cada llamada.

### 5.2 Casi nada se procesa "fresco"
Fíjate en la columna **In**: salvo la **primera** llamada (2459, porque aún no había nada cacheado) y las que valen **124** (turnos donde el usuario aporta texto nuevo), el resto vale **2**. Es decir: **prácticamente todo el contexto se sirve desde la caché**, no se reprocesa. Eso es lo que hace viable (en coste y velocidad) trabajar con un contexto de 137 K tokens.

### 5.3 La danza CacheCreate ↔ CacheRead
Cuando entra material nuevo al contexto, se **escribe** en caché (`CacheCreate` sube) y en los turnos siguientes ya se **lee** barato (`CacheRead`). Compáralo fila a fila:

- El **`CacheRead` de cada fila ≈ el `Contexto entrada` de la fila anterior**. La caché va "absorbiendo" todo lo que se acumuló.
- Picos de `CacheCreate`: llamada **3** (16 110) y llamada **5** (25 463) — ahí se incorporaron bloques grandes de contexto (archivos / instrucciones / código).

### 5.4 El modelo *lee mucho y escribe poco*
- Entrada total (creation + read + in): **~4,06 millones** de tokens.
- Salida total: **70 815** tokens.
- Relación **≈ 57 : 1**. El trabajo de un agente de código es sobre todo **comprender contexto**, no generar texto.

### 5.5 ¿Dónde se "trabajó" de verdad?
La columna **Out** marca los momentos de generación intensa:
- **Llamada 4 (12:16): 24 542 tokens** — con diferencia el pico. Aquí el agente produjo el grueso de la respuesta/código.
- Llamadas **32 (4801)**, **38 (3261)**, **34 (3184)**, **5 (2573)** — otros bloques sustanciales de salida.
- Llamadas con `Out` muy bajo (**33: 37**, **35: 21**) son turnos "de transición": el modelo apenas dice algo y encadena con una herramienta.

### 5.6 El parón de la sesión
Hay un salto temporal entre la **llamada 31 (12:48)** y la **32 (13:36)**: ~48 minutos. La sesión siguió viva (el contexto se conservó: `CacheRead` continúa donde lo dejó, ~119 K). El caching tiene un TTL corto, así que tras una pausa larga parte del contexto se **re-escribe** en caché — por eso ves repuntar `CacheCreate` en la 33 (4822) y la 35 (3222).

---

## 6. Resumen en una frase

> Una sesión de agente **lee muchísimo y escribe poco**, y ese "muchísimo" se paga barato gracias a la caché: en esta sesión el **95 %** de los tokens fueron **lecturas de caché**, mientras que la entrada realmente nueva fue apenas el **0,08 %**.
