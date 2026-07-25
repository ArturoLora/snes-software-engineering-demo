# BOOTSTRAP — Apotris SNES

| | |
|---|---|
| **Rol** | Project Bootstrap del BMAD Native Loop. Propiedad del **proyecto** |
| **Estado** | Vigente |
| **Fecha** | 2026-07-25 |
| **Framework** | `docs/BMAD_NATIVE_LOOP.md` v1.3 |
| **Capa de adaptación** | `docs/BMAD_GAMEDEV_NATIVE_LOOP.md` v1.1 |

Este archivo es el **único punto de entrada** que el BMAD Native Loop consume para obtener el contexto de este proyecto. No redefine el framework: lo alimenta.

Es un **índice con los datos operativos**, no una copia de la documentación existente. Cuando un dato ya vive en otro artefacto, aquí se referencia; no se duplica. Si este archivo y el artefacto referenciado se contradicen, **manda el artefacto referenciado** y este archivo está desactualizado.

---

# 1. Comandos de verificación

Los tres niveles de la política de validación (`docs/BMAD_GAMEDEV_NATIVE_LOOP.md` → "Política de niveles de validación"). Constituyen el **nivel 2 del orden de autoridad** del framework.

## V0 — Compilación

```bash
cd snes && make
```

- Requiere la variable de entorno `PVSNESLIB_HOME`. En esta máquina: `/home/arturo/Tools/pvsneslib`.
- Produce `snes/apotris.sfc` y `snes/apotris.sym`.
- **No usar `make clean`** durante `dev-story`: solo `make`, dejando la ROM lista para el emulador.
- `make` regenera `snes/linkfile` y `snes/apotris.sym` en cada enlace, y recoge automáticamente cualquier `.c` nuevo bajo `snes/source/` (`snes_rules` usa `wildcard`). Añadir un módulo no requiere tocar el `Makefile`.

## V1 — Harness automático

```bash
DISPLAY=:0 python3 tools/harness/harness.py
```

- Sin dependencias más allá de la stdlib de Python 3.
- Veredicto por **código de salida**: `0` = PASS, `1` = FAIL.
- Lanza BizHawk con la ROM y un script Lua, comprueba que se están leyendo lecturas por frame de la memoria del sistema emulado, y cierra el emulador.
- Requiere sesión gráfica: esta build de BizHawk no tiene modo headless y el sistema no tiene `Xvfb`.
- Artefactos: `tools/harness/artifacts/` y `tools/lua/poc_read_memory.log`. Ignorados por Git; se sobrescriben en cada corrida.
- Documentación: `tools/harness/README.md` (uso y limitaciones del emulador), `tools/harness/TEST_STATUS.md` (contrato de memoria), `tools/harness/TEST_RUNNER.md` (máquina de estados y registro de pruebas por `test_id`).

**V1 forma parte del comando de verificación del proyecto. No es un extra opcional.** Esta declaración es lo que lo convierte en evidencia del nivel 2 del orden de autoridad, y es un requisito de adopción de la capa de adaptación.

## V2 — Validación manual

```bash
cd snes && ares apotris.sfc
```

Solo obligatoria para las clases de Story que la exigen. La tabla que lo determina está en `docs/BMAD_GAMEDEV_NATIVE_LOOP.md`; el resumen operativo, en `CLAUDE.md` → "Validation".

## Reproducción independiente de V0

`make` sobre un árbol ya construido devuelve `No se hace nada para 'all'` con código
0. **Eso no reproduce nada.** Un revisor que corra `make` para "re-ejecutar V0 por su
cuenta" firma un no-op. Y la regla de no usar `make clean` cierra la salida obvia,
porque limpiaría la ROM que queda lista para el emulador.

Procedimiento correcto, cuando hace falta una compilación reproducida de verdad —
Review, gate de Epic, o duda sobre si la ROM corresponde a las fuentes:

```bash
python3 tools/loop/rebuild_v0.py
```

Copia `snes/` a un directorio desechable, borra allí los artefactos generados
—usando como fuente la lista que `.gitignore` ya declara—, compila desde cero, y
compara la ROM resultante con la del árbol principal.

**Nunca toca el árbol principal**, y lo comprueba tomando el md5 de **todos** los
archivos bajo `snes/` antes y después: reporta cualquier archivo que aparezca,
desaparezca o cambie, no solo la ROM.

Tras purgar, verifica de forma independiente de `.gitignore` que no sobrevivió
ningún `.obj`, `.sfc`, `.sym`, `.pic` ni `.pal`. Sin esa comprobación, un
`.gitignore` incompleto dejaría objetos viejos en la copia y la ROM "reproducida"
vendría en parte de binarios que nunca se recompilaron — un PASS falso.

Código de salida `0` solo si la compilación limpia tuvo éxito, la ROM resultante
coincide con la del árbol, y el árbol quedó intacto. Con `--keep` conserva el
directorio de compilación para inspeccionarlo.

Una ROM que no se puede reproducir desde las fuentes es un hallazgo, no un detalle:
significa que lo validado en el emulador no es lo que el repositorio describe.

## Lo que NO existe

Declarado explícitamente para que ningún rol lo busque ni lo suponga:

- **No hay suite de tests unitarios.** El código de la ROM no se compila para host.
- **No hay linter ni formateador configurado** en el repositorio.
- **No hay CI.** Toda verificación se corre localmente.
- **V1 no mide nada perceptual:** no ve la pantalla ni oye el audio. Esa es exactamente la frontera que separa V1 de V2.

---

# 2. Convenciones de rutas y estructura

Base para derivar allowlists acotadas. Una allowlist se construye eligiendo de esta tabla, nunca ampliándola por conveniencia.

| Ruta | Contenido | Quién la toca |
|---|---|---|
| `snes/source/*.c`, `*.h` | Código de la ROM. Un módulo por sistema (`board`, `piece`, `render`, `input`, `test_status`, `test_runner`) | Ejecutor, en Stories de código |
| `snes/*.png` | Assets fuente. `make` los convierte con `gfx4snes` | Ejecutor, en Stories de clase `render` |
| `snes/Makefile`, `snes/hdr.asm`, `snes/data.asm` | Build y datos embebidos | Ejecutor, solo si la Story lo declara |
| `tools/loop/` | Herramientas del BMAD Native Loop: gate de árbol sucio, reproducción de V0 | Ejecutor, en Stories de clase `herramientas` |
| `tools/harness/` | Harness Python y su documentación | Ejecutor, en Stories de clase `herramientas` |
| `tools/lua/` | Scripts Lua que el harness ejecuta dentro de BizHawk | Ejecutor, en Stories de clase `herramientas` |
| `docs/` | Framework y capa de adaptación del Native Loop | Solo por decisión del orquestador humano |
| `_bmad-output/implementation-artifacts/` | Archivos de Story y sus investigaciones | Autor de Story; el ejecutor solo las secciones que el workflow permite |
| `_bmad-output/planning-artifacts/` | `epics.md` | No se toca durante una Story |
| `_bmad-output/project-context.md`, `game-architecture.md` | Artefactos BMAD | **No modificar.** Se mantienen mediante workflows BMAD |
| `_bmad/` | Configuración de BMAD | No es código del proyecto |
| `reference/apotris/` | Fuente GBA original, solo lectura | **Gitignored, no versionado.** No asumir que existe en un clon nuevo |

## Rutas generadas — nunca se editan a mano

`snes/apotris.sfc`, `snes/apotris.sym`, `snes/linkfile`, `snes/*.pic`, `snes/*.pal`, `snes/*.map`, `snes/*.inc`, `snes/*_data.as`, `snes/source/*.obj`, `tools/harness/artifacts/`, `tools/lua/*.log`.

Todas ignoradas por Git (ver `.gitignore`). Un diff que las incluya es señal de que algo se versionó por error.

## Binarios no versionados

`tools/BizHawk-2.11.1-linux-x64/` y su tarball (243 MB) están gitignorados. El harness los espera en esa ruta; un clon nuevo debe instalarlos ahí.

---

# 3. Fuente de candidatos de trabajo

| Artefacto | Rol |
|---|---|
| `_bmad-output/planning-artifacts/epics.md` | **Fuente primaria.** FR/NFR y desglose de epics y Stories con criterios de aceptación |
| `_bmad-output/implementation-artifacts/*.md` | Stories ya redactadas, una por archivo |
| `_bmad-output/implementation-artifacts/investigations/` | Investigaciones que pueden generar candidatos |

> **Pendiente declarado.** Este proyecto **no tiene `sprint-status.yaml`**. Los workflows GDS lo esperan en `_bmad-output/implementation-artifacts/sprint-status.yaml` y, al no encontrarlo, degradan a seguimiento solo en el archivo de Story. El comando para generarlo es `/gds-sprint-planning`. Mientras no exista, la selección de la siguiente Story es una decisión explícita del orquestador humano, no un dato del repositorio.

---

# 4. Stack y restricciones

## Stack

| Elemento | Valor |
|---|---|
| Plataforma objetivo | SNES (65816), LoROM + SlowROM |
| Lenguaje | C |
| Librería | PVSnesLib (`PVSNESLIB_HOME`) |
| Toolchain | tcc-816 → wla-dx (`wla-65816`, `wlalink`) |
| Conversión de gráficos | `gfx4snes`, invocado desde el `Makefile` |
| Emulador de validación manual | ares |
| Emulador del harness | BizHawk 2.11.1 (Mono), en `tools/BizHawk-2.11.1-linux-x64/` |
| Host | Ubuntu, Python 3 para el harness |

## Restricciones vinculantes

La lista completa está en `_bmad-output/project-context.md` y `CLAUDE.md`. Las que más condicionan a un ejecutor:

1. **No es un port mecánico.** `reference/apotris/` es C++/devkitARM/libtonc para GBA: se adaptan algoritmos y tablas de datos, nunca la sintaxis. Sin clases, sin `std::list`, sin `std::tuple`.
2. **Solo la Story actual.** No adelantar Stories futuras ni introducir abstracciones especulativas. La implementación más pequeña que satisfaga los AC.
3. **Sin Internet por defecto.** La investigación se resuelve con los artefactos locales y `reference/apotris/`. La excepción, y el gate para invocar `/gds-investigate`, están en `CLAUDE.md` → "Investigación externa".
4. **La investigación se detiene en el primer límite del proyecto.** Si responder exige inspeccionar PVSnesLib o el toolchain, detenerse y proponer `/gds-investigate`.
5. **Git solo lo ejecuta Arturo.** Ningún agente hace `add`, `commit`, `push`, `merge`, `rebase`, ni crea o borra ramas, salvo instrucción explícita en la conversación.
6. **Presupuesto de tokens acotado.** Workflows pequeños e incrementales; evitar subagentes salvo petición explícita. Ver `CLAUDE.md` → "Token Budget".
7. **Assets:** una Story que solo cambia gráficos debe verificar que el asset generado cambió, que el `.obj` que lo incorpora se regeneró y que la ROM cambió realmente. `make` terminando sin errores no lo demuestra.

## Restricciones de toolchain que ya causaron errores

Registradas aquí porque un ejecutor que las ignore produce código que compila y se comporta mal:

- **`u8`/`s8` no se promueven a 16 bits como varargs.** `consoleDrawText` consume 2 bytes por argumento: hacen falta casts explícitos de ampliación o cada argumento se desplaza un byte.
- **Los globales con inicializador exigen copia ROM→RAM al arrancar.** El valor inicial se escribe en una función de init, no en la declaración.
- **Los `static` salen al `.sym` como `tccs_<archivo>.asm_<nombre>`**; los globales no-`static`, con su nombre plano. Relevante para localizar símbolos desde el harness.
- **Las direcciones de WRAM no son estables entre cambios de fuentes.** Se leen de `snes/apotris.sym` en cada corrida; nunca se fijan en el código. Procedimiento en `tools/harness/TEST_STATUS.md`.

---

# 5. Definición de desviación aceptable

Qué puede ajustar el ejecutor sin romper la intención de una Story, y qué no.

## Aceptable

- Renombrar una variable local o reorganizar el cuerpo de una función que la Story ya autoriza a tocar.
- Elegir la forma concreta de una estructura de datos cuando la Story fija el comportamiento pero no el layout.
- Añadir comentarios que expliquen una decisión no obvia, con la densidad del código circundante.
- Corregir un error propio introducido durante la misma Story.
- Ajustar una prueba de depuración existente para que siga siendo legible en pantalla, sin cambiar lógica.

## No aceptable

- Tocar cualquier ruta fuera de la allowlist declarada, por correcta que sea la mejora.
- Introducir un mecanismo, abstracción o capa que ningún AC exige.
- Cambiar gravedad, movimiento, colisiones o generación de piezas cuando la Story no lo declara.
- **Cambiar la clase de validación declarada en la Story.** Si parece equivocada, es material para `/gds-correct-course`.
- Modificar `_bmad-output/project-context.md` o cualquier artefacto BMAD.
- Declarar una Story terminada porque compila, o porque el harness dio PASS. Ver `CLAUDE.md` → "Validation".

## Cuando la Story resulta inviable

Detenerse y reportar. No expandir el alcance para hacerla viable: eso es `/gds-correct-course`, o una Story nueva.

---

# 6. Baseline de comparación

El framework exige que cada Story declare el estado del repositorio contra el que se mide su diff.

**Mecanismo:** `/gds-dev-story` escribe `baseline_commit` en el frontmatter YAML del archivo de Story al empezar, con el `git rev-parse HEAD` de ese momento, y lo preserva si ya existe.

La Review compara contra ese hash — no contra `HEAD`, que puede haber avanzado.

## Gate de árbol sucio

Un commit como baseline no basta si el **árbol de trabajo** ya tenía cambios antes de empezar la Story: el diff mezcla el alcance real con la suciedad previa, y la Review solo puede decir *"veo cosas que no sé a quién atribuir"* en vez de *"no se tocó nada fuera de alcance"*.

**Preferido:** empezar con el árbol limpio. Commitear o revertir lo pendiente antes de `Create Story`.

**Cuando no es posible**, la suciedad se declara en lugar de ignorarse. En `Create Story`:

```bash
python3 tools/loop/story_baseline.py snapshot
```

Emite el bloque de frontmatter con `baseline_commit` y `baseline_dirty` — cada ruta que ya difería de `HEAD`, **con el md5 de su contenido en ese momento** —, obtenido del estado real del repositorio y no a mano. Avisa por stderr si el árbol está sucio y recomienda limpiarlo primero. Con el árbol limpio, `baseline_dirty` queda vacío y no hay ceremonia extra.

```yaml
baseline_dirty:
  - path: CLAUDE.md
    md5: 9b491b3f9b61a0a1df1b9ab4f610ec80
```

En la Review:

```bash
python3 tools/loop/story_baseline.py check <archivo-de-Story>
```

Calcula el **conjunto atribuible**: los archivos que difieren del baseline menos los declarados en `baseline_dirty`. **Ese conjunto, y no el diff completo, es lo que se audita contra la allowlist.**

La resta es **por contenido, no solo por ruta**:

| Estado de una ruta declarada | Qué hace `check` |
|---|---|
| Sigue difiriendo, md5 igual al declarado | Se resta. Es suciedad previa genuina |
| Sigue difiriendo, **md5 distinto** | **No se resta.** Entra en el conjunto atribuible: el cambio es de la Story |
| **Borrada** durante la Story | No se resta. Entra en el conjunto atribuible, con el md5 mostrado como `(borrado)` |
| Ya **no** difiere del baseline | **FAIL con código 1.** Declaración obsoleta: podría estar ocultando un cambio real |
| Declarada **sin `md5`** | **FAIL con código 1.** Sin hash no hay contenido que comparar, y restar por ruta devolvería el mecanismo al estado que el hash existe para cerrar. La clave debe ser exactamente `md5:`, en minúsculas y sin espacio antes de los dos puntos — `md5 :` y `MD5:` no se reconocen. La sangría es libre: espacios o tabuladores dan igual |

Cualquiera de esos fallos devuelve código 1 y **no** imprime el `OK` final. Un `OK` junto a un hallazgo impreso encima sería peor que no comprobar nada.

### Archivos que Git ignora

`check` también lista los archivos ignorados que **no** son artefactos de build conocidos, y falla si encuentra alguno. Motivo: las rutas ignoradas son invisibles al conjunto atribuible, así que un archivo dentro de `build/`, `out/`, o de un directorio creado con un `.gitignore` anidado que se ignora a sí mismo, podría esconder cambios fuera de alcance sin aparecer en ninguna parte.

Los artefactos legítimamente ignorados están declarados en `tools/loop/story_baseline.py`. Añadir una clase nueva exige declararla ahí, de forma explícita y revisable. Dos categorías, con alcances distintos a propósito:

- **Salidas del build**, acotadas al prefijo `snes/`. Un sufijo válido en cualquier ruta convertiría a cualquier archivo llamado `x.map` o `x.obj` en invisible en todo el repositorio.
- **Directorios de material no-código**: `tools/harness/artifacts/`, `tools/BizHawk-*`, `reference/`, `_bmad/custom/`, más `__pycache__/` y los `.log`. Son **puntos ciegos aceptados y declarados**, no descuidos: contienen material arbitrario que no es código del proyecto. Cualquier cosa depositada ahí queda fuera del conjunto atribuible.

### Archivos ocultos a `git status`

`git update-index --assume-unchanged` y `--skip-worktree` sacan un archivo trackeado de `git diff` y de `git status`. Un `.c` fuera de alcance modificado bajo cualquiera de las dos banderas desaparecería del conjunto atribuible — la misma invisibilidad que las rutas ignoradas, por otra vía.

`check` lista esos archivos y falla si encuentra alguno. Se quita la marca con `git update-index --no-assume-unchanged <ruta>` (o `--no-skip-worktree`) y se repite.

### Detección de renames

El cálculo usa `git diff --no-renames`. Con la detección activada, Git colapsa un par origen/destino en la ruta destino, de modo que **mover un archivo fuera de alcance a un directorio permitido lo hace desaparecer del conjunto atribuible**. Sin renames se ven las dos rutas, que es lo que la contención necesita.

### Límite conocido del mecanismo

Queda una vía: nada ata la declaración a un instante. Un ejecutor puede volver a ejecutar `snapshot` a mitad de Story y pegar el resultado, y los hashes se actualizarían al estado nuevo, blanqueando el cambio.

Con hashes ese fraude deja de ser silencioso —exige reescribir a mano N valores en el frontmatter, y es un cambio visible y deliberado en el archivo de Story— pero **no es imposible**. Cerrarlo del todo exigiría commitear el frontmatter en `Create Story`, antes de que empiece la ejecución. Está registrado como trabajo diferido.

---

# 7. Estado registrado del loop

El framework exige que el estado de una Story permita distinguir la fase activa y el rol que la tiene en curso, y que el vocabulario lo defina el proyecto.

Vocabulario actual, tomado de los workflows GDS: `ready-for-dev`, `in-progress`, `review`.

> **Pendiente declarado.** Ese vocabulario **no distingue** *"el ejecutor sigue trabajando"* de *"la Review está corriendo"*: ambas caben en `in-progress`, y `review` solo indica que la ejecución acabó. El framework exige la distinción. Resolverlo — ampliando el vocabulario o registrando el rol activo por separado — está pendiente y es requisito de adopción de la capa de adaptación.

---

# 8. Registro de trabajo diferido

Destino durable exigido por el framework para los hallazgos con disposición `defer`, y por la capa de adaptación para la deuda de validación manual.

> **Pendiente declarado.** No existe todavía. Hasta que se defina, ambos se anotan en el archivo de la Story correspondiente. Un archivo de Story es durable, pero no es consultable de forma agregada, así que no cumple la función que el framework le pide a un registro de trabajo diferido: se resuelve antes del primer cierre de Epic.

---

# 9. Índice de artefactos

| Artefacto | Qué contiene |
|---|---|
| `docs/BMAD_NATIVE_LOOP.md` | Especificación del framework. **No se modifica** |
| `docs/BMAD_GAMEDEV_NATIVE_LOOP.md` | Capa de adaptación a GDS: equivalencias de comandos y política de niveles de validación |
| `CLAUDE.md` | Instrucciones operativas para agentes en este repositorio |
| `_bmad-output/project-context.md` | Reglas y anti-patrones vinculantes del proyecto |
| `_bmad-output/game-architecture.md` | Diseño de módulos C, orden del frame loop, estrategia de VRAM |
| `_bmad-output/planning-artifacts/epics.md` | FR/NFR, epics y Stories con criterios de aceptación |
| `_bmad-output/implementation-artifacts/` | Archivos de Story e investigaciones |
| `tools/loop/story_baseline.py` | Gate de árbol sucio: `snapshot` en Create Story, `check` en Review |
| `tools/loop/rebuild_v0.py` | Reproducción independiente de V0 en copia desechable |
| `tools/harness/README.md` | Uso del harness y limitaciones del emulador encontradas |
| `tools/harness/TEST_STATUS.md` | Contrato de memoria ROM ↔ harness, y cómo localizarlo vía `.sym` |
| `tools/harness/TEST_RUNNER.md` | Máquina de estados del Test Runner y registro de pruebas por `test_id` |
| `tools/lua/README.md` | Script Lua del PoC de lectura de WRAM |

---

# 10. Vigencia

"Vigente" significa que este archivo refleja el estado actual del proyecto. El framework evalúa el gate al entrar al loop y antes de cada `Create Story` — no de forma continua.

Este archivo queda desactualizado, y hay que revisarlo antes del siguiente `Create Story`, cuando:

- cambian los comandos de V0, V1 o V2;
- aparece o desaparece un directorio de los listados en las convenciones de rutas;
- se resuelve cualquiera de los tres **pendientes declarados** (`sprint-status.yaml`, vocabulario de estados, registro de trabajo diferido);
- cambia el stack, el toolchain o una restricción vinculante;
- se adopta una versión nueva del framework o de la capa de adaptación.

Los tres pendientes declarados **no** invalidan la vigencia de este archivo: están declarados, que es lo contrario de estar ausentes. Lo que invalidaría la vigencia es que se resolvieran y este archivo siguiera diciendo que están pendientes.
