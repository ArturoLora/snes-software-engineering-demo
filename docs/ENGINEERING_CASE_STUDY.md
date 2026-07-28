# SNES Technical Demo — Caso de estudio de ingeniería

| | |
|---|---|
| **Proyecto** | Demostración técnica nativa para Super Nintendo (65816, C, PVSnesLib) |
| **Estado** | Alcance mínimo jugable completo, a la espera de una validación manual |
| **Stack** | C · PVSnesLib · 65C816 · Python · Lua |
| **Fecha del documento** | 2026-07-25 |

> Este documento describe dos cosas construidas en paralelo: un juego para una consola
> de 1990, y el proceso de ingeniería que lo produjo. La segunda resultó ser la parte
> interesante.

---

## 1. Resumen del proyecto

**SNES Technical Demo** es una demostración técnica nativa para Super Nintendo: un
juego de bloques que sigue la guía estándar del género, escrito desde cero en C sobre
PVSnesLib.

La lógica de juego se apoya en [Apotris](https://akouzoukos.itch.io/apotris) (Game Boy
Advance, C++, GPLv3) como **origen histórico**, no como identidad del proyecto. Este
repositorio **no es un port mecánico**: reutiliza y adapta selectivamente los algoritmos
y las tablas de datos del original cuando resulta técnicamente viable, y los reescribe
cuando no. Cada función adaptada cita la original en un comentario de código.

El objetivo declarado es una ROM que funcione en hardware real, minimizando esfuerzo
humano y complejidad. No elegancia arquitectónica, no portabilidad.

### Por qué la SNES es un reto técnico

Para quien nunca haya trabajado con consolas retro, estas son las restricciones que
cambian cómo se escribe el código:

- **No hay sistema operativo.** No hay asignación dinámica de memoria, no hay hilos, no
  hay sistema de archivos. Toda la memoria es estática y se decide en tiempo de
  compilación.
- **La memoria de vídeo no es accesible en cualquier momento.** Escribir en VRAM
  mientras el haz de electrones dibuja la pantalla no falla con un error: **se descarta
  en silencio**. Hay que esperar al intervalo de borrado vertical, unos pocos
  microsegundos por frame.
- **No hay depurador conectado al código.** No hay `printf` a una consola, ni puntos de
  ruptura, ni stack traces. La única forma de saber qué pasa dentro es escribir valores
  en memoria y leerlos desde fuera.
- **El compilador es poco convencional.** El toolchain de PVSnesLib compila C a
  ensamblador 65C816 pasando por varias herramientas encadenadas. Algunas suposiciones
  que cualquier programador de C da por sentadas no se cumplen — este proyecto
  documentó una: **la memoria estática no se inicializa a cero**.
- **El presupuesto de recursos es fijo y pequeño.** 128 KB de RAM de trabajo, 64 KB de
  VRAM, una ROM de 256 KB.

Nada de esto es exótico para quien viene del mundo embebido. Es exactamente ese mundo,
con treinta años de antigüedad y sin las herramientas modernas de diagnóstico.

### Tecnologías

| Área | Herramienta |
|---|---|
| Lenguaje | C (subconjunto que acepta `816-tcc`) |
| SDK | PVSnesLib |
| Ensamblador / enlazador | `wla-65816`, `wlalink` |
| Conversión de gráficos | `gfx4snes` |
| Verificación automática | BizHawk en modo headless, dirigido por Lua, orquestado desde Python |
| Validación manual | Ares |
| Referencia | Código fuente original de Apotris (GBA, C++, libtonc) |

---

## 2. Arquitectura

El principio que ordena todo el diseño es una **separación estricta entre simulación y
presentación**.

```
                 ┌──────────────────────────────┐
   pad físico ──▶│  input.c                     │──▶ InputIntent
                 │  única frontera con el SDK   │    {left, right, down}
                 └──────────────────────────────┘
                                │
                 ┌──────────────▼───────────────┐
                 │  main.c — bucle de frame     │
                 │  ordena, no decide reglas    │
                 └──────────────┬───────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        ▼                       ▼                       ▼
  ┌───────────┐          ┌───────────┐          ┌─────────────┐
  │ board.c   │          │ piece.c   │          │ queue.c     │
  │ tablero,  │◀────────▶│ pieza     │◀────────▶│ bolsa de 7  │
  │ colisión, │          │ activa,   │          │             │
  │ líneas    │          │ gravedad, │          └─────────────┘
  └───────────┘          │ lock      │
        │                └───────────┘
        │                       │
        └───────────┬───────────┘
                    ▼
            ┌───────────────┐
            │  GameState    │  estado único, estático
            └───────┬───────┘
                    │ solo lectura
                    ▼
            ┌───────────────────────────────┐
            │  render.c                     │
            │  ÚNICO módulo que escribe     │
            │  VRAM y OAM                   │
            └───────────────────────────────┘
```

**`GameState`** es una única estructura estática que contiene todo el estado del juego:
el tablero, la pieza activa, las filas pendientes de eliminar y la bolsa de piezas. No
hay estado de juego en ningún otro sitio.

**Los módulos de lógica** —tablero, pieza, bolsa— no incluyen ninguna cabecera de vídeo
ni de entrada. No saben que existe una pantalla. Esto no es purismo: es lo que permite
verificar las reglas del juego leyendo memoria, sin mirar píxeles.

**`render.c`** es el único módulo que escribe en memoria de vídeo, y solo lee
`GameState`. Nunca lo modifica.

**`main.c`** ordena el frame y no contiene ninguna regla de juego. Cuando hizo falta
decidir si una pieza había aterrizado, la función se escribió en el módulo de la pieza,
no en el bucle.

### Dos decisiones de rendimiento que condicionan el diseño

**La pieza que cae se dibuja con sprites; el tablero fijo, con un mapa de tiles.** La
pieza activa se mueve cada frame, y mover cuatro sprites es barato. El tablero solo
cambia cuando una pieza se fija: como mucho una vez cada varios segundos.

**El mapa de tiles se mantiene duplicado en RAM.** El SDK no ofrece forma de escribir
una celda suelta de la pantalla. El patrón —tomado de un ejemplo oficial de la
librería— es mantener una copia en RAM, modificarla libremente, y transferirla entera
al hardware por acceso directo a memoria justo después del borrado vertical. En este
juego esa transferencia ocurre **solo al fijar una pieza**, nunca por movimiento ni por
gravedad.

### Infraestructura de pruebas dentro de la ROM

La ROM incluye dos módulos que no son juego: publican un bloque de estado en una
dirección de memoria conocida para que un proceso externo pueda leerlo mientras el
juego corre. Es la respuesta al problema de "no hay depurador": si no puedes observar
el programa desde fuera, haz que el programa se describa a sí mismo.

---

## 3. Framework de desarrollo — el Game Dev Native Loop

### El problema

Un agente de IA que escribe código tiene un sesgo estructural: **tiende a declarar
terminado lo que compila**. En desarrollo web eso se corrige rápido, porque los tests y
el navegador contradicen al agente en segundos. En una consola retro no hay nada que lo
contradiga. El compilador dice que sí, la ROM existe, y nadie ha comprobado que el
juego haga algo.

El segundo problema es más sutil: **quien implementa no puede ser quien verifica.** Un
agente que revisa su propio trabajo reproduce sus propias suposiciones. Si asumió mal
algo al escribir el código, asumirá lo mismo al revisarlo.

El Native Loop es un proceso construido para que ninguna de esas dos cosas sea posible.

### Cómo funciona, a alto nivel

Cinco ideas:

1. **Cada unidad de trabajo se declara antes de empezar.** Alcance, criterios de
   aceptación, lista explícita de archivos que se pueden tocar, y el estado exacto del
   repositorio contra el que se mide. Todo escrito antes de la primera línea de código.
2. **La verificación tiene niveles, y el nivel lo determina la naturaleza del cambio,
   no la opinión de quien lo hizo.** Una tabla convierte "¿qué clase de trabajo es
   esto?" en "¿qué comprobaciones son obligatorias?".
3. **La revisión la hace un contexto independiente**, que no ha visto el razonamiento
   del ejecutor y que vuelve a ejecutar las comprobaciones por su cuenta en lugar de
   creerse el informe.
4. **La contención de alcance es mecánica, no disciplinada.** Una herramienta calcula
   qué archivos cambiaron realmente y los compara con la lista declarada. Instruir a un
   agente para que no toque algo no equivale a impedírselo.
5. **Los problemas encontrados se clasifican y se registran**, incluidos los que se
   decide no arreglar. Un problema conocido y anotado es información; uno silenciado es
   deuda.

### Por qué se diseñó así

Cada regla del proceso viene de un fallo concreto, no de una teoría. Dos ejemplos
reales de este proyecto:

**El baseline sucio.** Una Story declaraba compararse contra un commit concreto, pero
el árbol de trabajo ya tenía cambios de antes. El diff mezclaba el trabajo de la Story
con suciedad previa, y la revisión no podía afirmar "no se tocó nada fuera de alcance",
solo "veo cosas que no sé a quién atribuir". Ocurrió tres veces antes de convertirse en
una herramienta que declara la suciedad previa y la resta al medir.

**La compilación que no compila nada.** Ejecutar `make` sobre un árbol ya construido
devuelve "no hay nada que hacer" con código de éxito. Un revisor que ejecutaba `make`
para "reproducir la compilación por su cuenta" estaba firmando una operación vacía. La
solución fue una herramienta que compila en una copia desechable, partiendo de cero, y
compara el binario resultante con el que hay en el árbol.

Ambas nacieron de una Story dedicada exclusivamente a arreglar el proceso, que pasó por
**cuatro rondas de revisión independiente** antes de darse por buena. Las tres primeras
encontraron defectos bloqueantes en las herramientas que debían garantizar la calidad
de todo lo demás.

### Cómo evolucionó

| Versión | Qué introdujo |
|---|---|
| **v1** | Los tres niveles de verificación. Traslada la validación manual de "cada cambio" a "cambios que afectan a lo perceptible". |
| **v1.1** | Convierte la elección de nivel en una tabla mecánica de trece clases de trabajo. El ejecutor deja de poder decidir cuánta verificación necesita su propio trabajo. |
| **v1.2** | Añade tres profundidades de revisión y un modo de ejecución continua. Motivo: revisar un cambio de interfaz con el mismo esfuerzo que un cambio de motor no es rigor, es desproporción. |
| **v1.3** | Endurece los límites de las revisiones ligeras y añade un principio de coste. Establece que **escalar una revisión es decisión del proceso, no del revisor**: sin esa regla, cualquier revisor que encuentre algo interesante convierte su revisión en la más cara, y el presupuesto deja de ser predecible. |

La dirección del cambio es constante: **cada versión quita decisiones discrecionales y
las sustituye por reglas comprobables**.

---

## 4. Flujo de trabajo

```
  ROADMAP
     │  se elige la siguiente unidad de trabajo por dependencias y valor
     ▼
  STORY ─────────────────────────────────────────────────────────┐
     │  alcance, criterios de aceptación, lista de archivos       │
     │  permitidos, clase de trabajo, estado inicial del repo     │
     │  TODO ESTO ANTES DE ESCRIBIR CÓDIGO                        │
     ▼                                                            │
  IMPLEMENTACIÓN                                                  │
     │  solo lo que la Story declara                              │
     ▼                                                            │
  VALIDACIONES                                                    │
     │  compilación → comprobación automática → validación humana │
     ▼                                                            │
  REVISIÓN INDEPENDIENTE                                          │
     │  contexto limpio · re-ejecuta todo · profundidad según     │
     │  el riesgo del cambio                                      │
     ├── problemas con arreglo claro ──▶ se aplican y se re-valida │
     ├── problemas fuera de alcance ───▶ se registran, no se hacen │
     └── decisión ambigua ────────────▶ para y pregunta            │
     ▼                                                            │
  COMMIT                                                          │
     │  acotado a los archivos realmente atribuibles              │
     ▼                                                            │
  SIGUIENTE STORY ──────────────────────────────────────────────┘
```

El detalle que hace funcionar el ciclo: **la lista de archivos permitidos se comprueba
con una herramienta, no leyendo el diff**. La herramienta calcula qué cambió realmente
respecto al estado declarado, resta la suciedad previa que la Story registró, y falla
si aparece cualquier cosa fuera de la lista. También detecta archivos escondidos en
directorios ignorados y archivos marcados para que el control de versiones deje de
reportarlos — dos formas de sacar un cambio del alcance sin que se note.

---

## 5. Validación en tres niveles

| Nivel | Qué es | Qué cuesta | Qué demuestra |
|---|---|---|---|
| **V0** | Compilación completa desde cero, en una copia desechable, comparando el binario resultante con el del árbol | Segundos de máquina | Que el código compila y que **la ROM corresponde a las fuentes** |
| **V1** | El juego corre en un emulador sin interfaz, un script lee su memoria mientras corre, y un proceso externo comprueba los valores | ~40 s de máquina | Que las reglas del juego se comportan como deben, sin mirar la pantalla |
| **V2** | Una persona abre el emulador y mira | Minutos de atención humana | Que lo que se ve es lo que debía verse |

### Por qué tres y no uno

Porque miden cosas distintas y **ninguno puede sustituir a otro**:

- V0 no sabe nada de comportamiento. Un juego que compila puede no arrancar.
- V1 no puede ver. Una pieza dibujada 24 píxeles más arriba de donde debería tiene
  exactamente los mismos valores en memoria que una bien dibujada.
- V2 no escala ni es reproducible. Depende de una persona disponible, y no puede
  ejecutarse cientos de veces.

La regla operativa es que **V0 y V1 son obligatorios para cualquier cambio que pueda
alterar la ROM**, sin excepción por tipo de trabajo. Su coste es tiempo de máquina, no
atención humana. Lo que la clase de trabajo determina es si además hace falta V2.

Un caso concreto de por qué V1 no es negociable: una Story cambió **un único valor de
color de una paleta**. No podía romper nada. V1 fue lo único que demostró que la lógica
de caída de las piezas no se había movido.

---

## 6. Sprint Mode — ejecución continua

El modo continuo permite encadenar varias unidades de trabajo sin autorización entre
ellas.

**Lo que hace el agente sin preguntar:**

- Elegir la siguiente unidad de trabajo según dependencias, roadmap y valor.
- Escribir la Story: alcance, criterios de aceptación, clase, lista de archivos.
- Implementar.
- Ejecutar compilación y comprobación automática.
- Lanzar la revisión independiente al nivel de profundidad que corresponda.
- Aplicar las correcciones de la revisión que tengan un arreglo claro, y volver a
  validar.
- Registrar los problemas que se decide no arreglar.
- Actualizar la documentación.
- Hacer commit, acotado a los archivos atribuibles.
- Empezar la siguiente.

**Lo que sigue siendo del desarrollador:**

- **La validación visual.** Nadie puede firmar por otro que algo se ve bien.
- **Las decisiones ambiguas.** Cuando existe más de una respuesta defendible, el proceso
  para y pregunta en lugar de elegir.
- **Aceptar o rechazar compromisos.** Un límite conocido puede ser aceptable o no; eso
  no lo decide quien escribió el código.
- **El presupuesto.** Un sprint sin límite declarado no existe.

El modo se detiene por exactamente cuatro causas: hace falta validación humana, hay una
decisión que tomar, hay un bloqueo real, o se agotó el presupuesto. Ninguna otra cosa
lo interrumpe: los problemas con arreglo claro se arreglan y se sigue; los diferidos se
anotan y se sigue.

---

## 7. Estado actual

### Implementado

Tablero lógico con detección de colisión sobre la forma completa de la pieza · las
siete piezas, repartidas con una bolsa que garantiza que cada tipo salga una vez por
cada siete · aparición de pieza · movimiento horizontal con el mando · caída automática
· fijado al aterrizar · tablero persistente dibujado en pantalla · aparición de la
pieza siguiente · eliminación de filas completas con recolocación del resto.

Fuera del juego: infraestructura de pruebas dentro de la ROM, un arnés de verificación
automática, y dos herramientas de proceso.

### Pendiente

- **Fin de partida.** Cuando la pila alcanza la zona donde aparecen las piezas, la
  simulación deja de avanzar. Desde fuera es indistinguible de un cuelgue. Está
  registrado, confirmado por observación directa, y **no pertenece a ninguna unidad de
  trabajo planificada**: el roadmap original no lo contemplaba.
- Rotación de piezas, caída acelerada, puntuación, niveles, interfaz, audio.
- Velocidad de caída variable por nivel: implementada como intervalo fijo, pendiente la
  versión completa.

### Grado de avance

El alcance mínimo declarado al inicio del proyecto —tablero, pieza activa, aparición,
movimiento horizontal, gravedad, colisión, fijado, pieza siguiente, eliminación de
líneas— **está completo**, a la espera de una validación visual.

Lo que separa al proyecto de una partida jugable de principio a fin es el fin de
partida. Lo que lo separa de un juego completo es considerablemente más.

### Cifras verificables

| | |
|---|---|
| Unidades de trabajo documentadas | 24 |
| Líneas de C en la ROM | ~1 400 |
| Líneas de herramientas de proceso (Python) | ~1 000 |
| Commits | 25 |
| Tamaño de la ROM | 256 KB |

---

## 8. Lecciones aprendidas

Solo las respaldadas por evidencia del proyecto.

### Un instrumento de medición puede romperse en silencio

Una Story añadió un búfer de 2 KB en memoria. Eso desplazó la posición de las variables
que el script de verificación leía por dirección fija. El script pasó a leer el búfer
nuevo creyendo que era el estado del juego, e **imprimió valores congelados durante
toda la ejecución**. El arnés siguió devolviendo éxito: sus criterios —hay lecturas,
avanzaron frames, se vio algo— se cumplen igual de bien leyendo basura.

La única señal fue que las cifras cambiaron respecto a ejecuciones anteriores. No hubo
ningún error.

De ahí salió una regla concreta: **cualquier cambio que altere la disposición de la
memoria escala automáticamente a la revisión más profunda**. Y la comprobación de que
las direcciones siguen siendo válidas pasó a formar parte del cierre de cada Story.

### Las suposiciones del lenguaje no siempre se cumplen

El estándar de C garantiza que las variables estáticas empiezan a cero. **Este
toolchain no lo hace.** Se midió directamente: una variable estática leía `0x55` —el
patrón de relleno de la memoria sin inicializar— durante toda la ejecución, sin cambiar
nunca.

El código que dependía de esa garantía tenía una rama inalcanzable y un comportamiento
que dependía del contenido de la memoria al arrancar. Nada lo señalaba. Ahora es una
regla del proyecto: **toda variable estática se inicializa explícitamente**.

### Escribir en memoria de vídeo puede no hacer nada

Una transferencia de memoria colocada antes del primer intervalo de borrado, con la
pantalla ya encendida, es descartada por el hardware. En este caso resultó inocua **por
accidente**: el estado inicial del tablero era vacío, así que la transferencia escribía
exactamente los mismos bytes que ya había. El defecto se habría manifestado el día que
el juego empezara con el tablero no vacío.

Lo encontró una revisión independiente leyendo el orden de las llamadas, no una prueba.

### La revisión independiente rinde de forma medible

En cuatro Stories revisadas por un contexto sin acceso al razonamiento del ejecutor:

- Se tumbó un criterio de aceptación que el ejecutor daba por cumplido.
- Se encontraron dos formas de blanquear cambios fuera de alcance en la herramienta que
  existía precisamente para impedirlo.
- Se demostró que una afirmación de la documentación —"se verifica el árbol entero por
  hash"— era falsa: se verificaba un archivo.
- Se detectó que la evidencia de un criterio era engañosa el 79 % del tiempo, con el
  código perfectamente correcto.

Ninguno de esos hallazgos venía de conocimiento que el ejecutor no tuviera. Venían de
**no compartir sus suposiciones**.

### Un revisor con instrumentos propios encuentra cosas distintas

En la revisión más profunda de este proyecto, el revisor construyó su propia sonda,
leyó memoria del juego y memoria de vídeo durante unos 1 600 frames, y verificó ocho
criterios de aceptación **empíricamente** en lugar de por lectura de código. Otro
enumeró exhaustivamente las 5 040 secuencias posibles del algoritmo de barajado para
comprobar que era uniforme.

Ese nivel de esfuerzo no es sostenible en cada cambio, y de ahí salió la política de
profundidad proporcional al riesgo.

### El coste de la revisión se puede acotar sin perder independencia

Medición directa de una misma sesión, tres revisiones:

| Profundidad | Coste observado | Resultado |
|---|---|---|
| Profunda | ~105 000 tokens | 8 hallazgos |
| Profunda | ~92 000 tokens | 5 hallazgos |
| Acotada | ~42 000 tokens | 0 hallazgos, cambio de 17 líneas |

Son tres puntos de datos, no una medición estadística. Lo que sí se sostiene es la
decisión de diseño que los acompaña: **lo que se recorta en una revisión ligera es la
investigación, nunca la ejecución de las comprobaciones ni la independencia del
revisor**. Una revisión que se salta la comprobación automática porque el cambio era
pequeño no es una revisión ligera: no es una revisión.

### Un criterio de aceptación puede afirmar de más

Uno de los criterios de este proyecto pedía que "la ROM no cambie". No es verificable
de forma independiente: la ROM no está bajo control de versiones, no existe un hash
previo de origen ajeno a quien hizo el cambio, y ninguna herramienta del proyecto puede
cerrarlo. **Un criterio que solo el ejecutor puede afirmar no es un criterio de
aceptación.**

Hizo falta reformularlo dos veces. La primera reformulación seguía afirmando de más, y
lo demostró la tercera revisión construyendo la ROM desde dos estados distintos y
obteniendo hashes distintos.

### Registrar lo que no se arregla es parte del trabajo

El proyecto acumula problemas conocidos y no resueltos, cada uno con su motivo. Ninguno
está silenciado. El valor no está en la lista: está en que **la decisión de no arreglar
algo fue explícita y quedó firmada**, en lugar de disolverse en el olvido.

---

## 9. Conclusiones

Este proyecto demuestra tres cosas.

**Que un proceso de verificación puede diseñarse para un entorno sin herramientas de
diagnóstico.** Cuando no hay depurador, ni tests unitarios que corran en la máquina de
destino, ni forma de observar el programa desde fuera, la infraestructura de
observación hay que construirla: dentro de la ROM, alrededor del emulador, y en el
proceso que decide cuándo algo está terminado.

**Que la separación entre implementar y verificar produce resultados distintos aunque
ambas partes sean el mismo modelo.** No es una cuestión de capacidad, sino de
suposiciones compartidas. Un revisor que no ha visto el razonamiento del autor
encuentra los errores que ese razonamiento no podía ver, y este proyecto tiene el
registro de cada uno.

**Que el rigor y la velocidad son parametrizables por separado.** La versión final del
proceso permite encadenar trabajo de forma continua sin renunciar a que cada cambio se
compile desde cero, se verifique automáticamente y lo revise un contexto independiente.
Lo que se ajusta según el riesgo es la profundidad de la investigación, no la existencia
de las comprobaciones.

El juego, por ahora, es un tablero donde caen bloques que se pueden mover, se fijan al
llegar abajo, y desaparecen cuando completan una fila. Es poco. Pero cada una de esas
piezas de comportamiento está declarada antes de escribirse, verificada en tres niveles,
revisada por un contexto independiente y registrada con la evidencia que la respalda —
incluidas las veces en que la evidencia demostró que el trabajo estaba mal.

---

## Apéndice — Dónde está cada cosa

| Documento | Contenido |
|---|---|
| `BOOTSTRAP.md` | Punto de entrada único: comandos de verificación, convenciones, estado del proceso |
| `docs/BMAD_NATIVE_LOOP.md` | Especificación del framework base |
| `docs/BMAD_GAMEDEV_NATIVE_LOOP.md` | Capa de adaptación: niveles de validación, niveles de revisión, modo continuo |
| `_bmad-output/game-architecture.md` | Diseño de módulos, orden del bucle de frame, estrategia de memoria de vídeo |
| `_bmad-output/planning-artifacts/epics.md` | Roadmap original |
| `_bmad-output/implementation-artifacts/` | Una unidad de trabajo por archivo, con su evidencia completa |
| `tools/loop/` | Herramientas de contención de alcance y compilación reproducible |
| `tools/harness/` | Arnés de verificación automática |
| `snes/` | El juego |
