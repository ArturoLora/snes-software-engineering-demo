# Deep-dive: núcleo de gameplay (MVP) — tetrisEngine + tetromino

Alcance: `include/tetrisEngine.h`, `source/tetrisEngine.cpp`, `include/tetromino.hpp`, `source/tetromino.cpp`. Solo sistemas del MVP (tablero, pieza activa, spawn, movimiento horizontal, gravedad, colisión, lock, siguiente pieza, line clear). Rotation system asumido: **SRS** (default de `Game::rotationSystem`). NRS/ARS/CLASSIC y todo lo demás se documentan solo donde aparecen entrelazados en el mismo código y deben separarse.

Clasificación usada:
- 🟢 **reutilizable casi directo**
- 🟡 **requiere adaptación C++ → C**
- 🔴 **dependiente de GBA/plataforma, reemplazar**

---

## Tablero

- **Estado:** `int** board` (`Game`, tetrisEngine.h:358), tamaño `lengthY=40 × lengthX=10` (tetrisEngine.h:356-357). 40 filas = 20 visibles + 20 buffer superior oculto (spawn/game-over se validan en filas 21-23, tetrisEngine.cpp:1239-1248). Valor de celda: 0 = vacío; !=0 = id de pieza + bits de textura de borde (ver `getShape`, no afecta colisión).
- **Alloc:** `new int*[lengthY]` + `new int[lengthX]` por fila (tetrisEngine.h:487-493), liberado en `~Game()` (tetrisEngine.h:603-613).
- 🟡 Convención 40×10 con buffer superior → reutilizable como constante de diseño.
- 🔴 Allocación dinámica heap → reemplazar por `int board[40][10]` estático.
- 🟡 Todo el código de tablero está entreverado con la variante `pawn.big` (modo 2x2 escalado, fuera de MVP) — hay que eliminar esas ramas, no solo ignorarlas.

## Pieza activa (Pawn)

- **Estado:** `class Pawn` (tetrisEngine.h:207-238): `x,y,type,current,rotation,lowest,big`, y `board[4][4][4]` = las 4 rotaciones pre-calculadas de la pieza actual.
- **Función clave:** `Pawn::setBlock(int system)` (tetrisEngine.cpp:91-105) — llama a `getShape()` para las 4 rotaciones y las cachea en `pawn.board`.
- **`getShape(piece, rotation, rotationSystem)`** (tetrisEngine.cpp:1448, declarada en tetrisEngine.h:16) — indexa la tabla de datos (`tetraminos`/`classic`/`ars` según sistema) y luego (si no es `bigMode`) añade bits de textura de borde vía `getNbr()` (tetrisEngine.cpp:107-129) — **puramente visual**, no se usa en colisión (`checkRotation` solo comprueba `!=0`).
- **Datos:** `GameInfo::tetraminos[7][4][4][4]` (tetromino.cpp:19) — única tabla necesaria para MVP (SRS). `classic`/`ars` son para NRS/ARS, fuera de MVP.
- 🟢 Tabla `tetraminos` (forma de las 7 piezas × 4 rotaciones): dato puro, reutilizable tal cual.
- 🟡 `getShape` usa `new int*[4]`/`new int[4]` (alloc dinámica) → pasar a buffer estático o llenar `pawn.board` directo sin capa intermedia.
- 🟡 Separar el cálculo de bits de borde (rendering) de la tabla de colisión — para MVP se puede omitir del todo ese post-proceso.

## Spawn (siguiente pieza entra al tablero)

- **Función:** `Game::next()` (tetrisEngine.cpp:1202-1274). Orden: posiciona `pawn.x = lengthX/2-2`, `pawn.y = lengthY/2`, `rotation=0` (rama IRS/ARS no aplica en SRS puro) → toma pieza de `queue.front()`, `pop_front()`, `fillQueue(1)` para reponer → `pawn.setBlock(rotationSystem)` → chequea filas 21-23 ocupadas o colisión inmediata; si bloqueado intenta subir 1 fila, si sigue bloqueado → `lost=1`.
- **Bag/cola:** `fillBag()` (tetrisEngine.cpp:1178) llena `bag` con 0..6 (7-bag) cuando está vacía. `fillQueue(count)` (tetrisEngine.cpp:1276) saca pieza aleatoria del bag vía `qran() % bag.size()`, la mete a `queue`, rellena bag si se vació.
- **Dependencia directa:** `qran()`/`sqran()` — RNG de hardware de **libtonc** (`tonc.h`), no existe en SNES/PVSnesLib.
- 🟢 Algoritmo 7-bag en sí (lógica pura, independiente del RNG concreto).
- 🔴 `qran()`/`sqran()` (libtonc) → sustituir por RNG propio SNES/PVSnesLib.
- 🟡 `bag`/`queue` son `std::list<int>` → pasar a arrays/ring buffer fijo (sin STL en C).
- 🟡 Ramas NRS/ARS/CLASSIC/IRS/IHS/drought-history dentro de `next()`/`fillBag()`/`fillQueue()` deben eliminarse (fuera de MVP), no solo dejarse inactivas.

## Movimiento horizontal

- **Estado:** `left,right,das,maxDas,arr,arrCounter` (miembros privados de `Game`, tetrisEngine.h:290-304).
- **Funciones:** `keyLeft(int)`/`keyRight(int)` (tetrisEngine.cpp:1546,1575) fijan intención de movimiento; `moveLeft()`/`moveRight()` (tetrisEngine.cpp:286,310) aplican `checkRotation(±1,0,rotation)` y mueven si es válido. DAS/ARR (auto-repeat) se resuelve en `update()`.
- 🟢 Patrón colisión-antes-de-mover y contadores DAS/ARR: enteros simples, portan casi literal.
- 🟡 `moveHistory` (`std::list<int>`, usado para finesse — fuera de MVP) debe eliminarse de estas funciones, no solo ignorarse.

## Gravedad

- **Estado:** `speed` (float, viene de `GameInfo::gravity[level]`), `speedCounter` (acumulador float) — en `update()` (tetrisEngine.cpp:543-559): `speedCounter += speed; n = (int)speedCounter;` luego aplica `n` pasos de caída (`checkRotation(0,1,rot)` → `pawn.y++`), `speedCounter -= n`.
- **Datos:** `GameInfo::gravity[19]` (tetromino.cpp:648) — curva de velocidad por nivel (float, filas/frame).
- 🟢 Algoritmo acumulador (técnica estándar sub-frame) y curva de valores: reutilizable en concepto.
- 🔴 Todo es `float` — SNES/65C816 sin FPU. Convertir tabla y acumulador a **fixed-point**.

## Colisión

- **Función:** `Game::checkRotation(dx, dy, r)` (tetrisEngine.cpp:17-52) — recorre la matriz 4×4 de la pieza en la rotación `r`, para cada celda ocupada valida límites de tablero y `board[y][x] != 0`.
- 🟢 El núcleo (rama `!pawn.big`) es lógica de índices simple, casi copiable línea a línea a C.
- 🟡 Convertir de método de clase a función libre `checkRotation(Game* g, int dx, int dy, int r)`.
- 🔴 Rama `pawn.big` (escalado 2×2, fuera de MVP) — eliminar, no dejar como dead code.

## Lock (fijar pieza)

- **Estado:** `lockTimer,maxLockTimer,lockMoveCounter` (tetrisEngine.h:300,426-427).
- **Funciones:** `lockCheck()` (tetrisEngine.cpp:1539-1544) — regla SRS "15 move reset": cada vez que la pieza toca superficie y quedan resets (`lockMoveCounter>0`), reinicia `lockTimer`. Se llama desde `rotatePlace`, `moveLeft`, `moveRight`.
- **`place()`** (tetrisEngine.cpp:655-812) — función grande: escribe celdas de la pieza en `board`, chequea derrota (pieza por encima de fila `lengthY/2-2` → `lost=1`), llama `calculateDrop()` + `clear()`, y luego mezcla lógica **no-MVP** (combo, T-spin scoring, modos DIG/BATTLE/MASTER, garbage) con lo esencial (reset de timers, `next()`).
- 🟢 Mecánica de lock-delay + 15-move-reset (estándar SRS): reutilizable tal cual.
- 🟡 `place()` debe reducirse a: escribir celdas → chequear derrota → `clear()` → `next()`. El resto (scoring, modos, combo) hay que extraerlo, no adaptarlo.

## Siguiente pieza (cola/preview)

- **Estado:** `queue` (`std::list<int>`, tetrisEngine.h:359). `held`/`hold()` (hold piece) **fuera de MVP**, no documentado más allá de esto.
- Reutiliza el mismo `fillQueue()`/7-bag descrito en "Spawn" — mantiene 1 pieza de lookahead (`fillQueue(1)` en cada `next()`).
- 🟡 `std::list<int>` → array/ring buffer fijo.

## Line clear

- **Función:** `Game::clear(Drop drop)` (tetrisEngine.cpp:814-1000+). Escanea filas completas → `linesToClear` (lista). Detección de T-spin (líneas 823-862) es scoring, **no imprescindible para MVP** pero es barata de mantener si se desea puntaje básico.
- **Colapso real:** NO ocurre en `clear()` — se difiere vía `clearLock` hasta `removeClearLock()` (tetrisEngine.cpp:1624-1672), que hace el shift-down (`board[j][k] = board[j-1][k]`, cascada desde la línea limpiada hacia arriba) y vuelve a llamar `next()`.
- 🟢 Bucle de detección de fila completa y bucle de colapso (shift-down): lógica de índices simple, portable casi directa.
- 🟡 Para MVP: colapsar en el mismo `clear()`/`place()`, sin el estado diferido `clearLock`/`maxClearDelay` (ese delay existe solo para animación).
- 🟡 `linesToClear` (`std::list<int>`) → array fijo (máx. 4 líneas simultáneas).
- 🟡 Ramas de zone-lines/garbage/perfect-clear/T-spin scoring dentro de `clear()` deben extraerse (fuera de MVP).

---

## Orden de actualización por frame (MVP, de `Game::update()` tetrisEngine.cpp:430-610, recortado)

1. Si `lost` → no hacer nada.
2. Si no hay pieza activa (`pawn.current == -1`) → `next()` (spawn).
3. Input horizontal: actualizar contador DAS desde flags `left/right`; al llegar a `maxDas`, aplicar auto-repeat (ARR) vía `moveLeft()`/`moveRight()`.
4. Gravedad: `speedCounter += speed`; aplicar `n = floor(speedCounter)` pasos de caída con `checkRotation(0,1,rot)`.
5. Recalcular `pawn.lowest = lowest()` (fila de aterrizaje, usada para lock-timer y hard/ghost).
6. Lock-delay: si la pieza está apoyada, decrementar `lockTimer`; en 0 → `place()`.
7. Soft drop (si `down` activo): mover hacia abajo vía `moveDown()` al ritmo de `softDropSpeed`.

(Se omiten del orden: timers de modos DIG/BATTLE/MASTER/SURVIVAL, animación `disappearing`/créditos, zone — todo fuera de MVP.)

---

## Dependencias directas a reemplazar (transversal a todo el núcleo)

- 🔴 `qran()` / `sqran()` — RNG de **libtonc** (incluido vía `tonc.h`), usado en `fillQueue`/`fillBag`/`generateGarbage`. Sustituir por RNG propio en C/PVSnesLib.
- 🔴 `tonc.h` / `tonc_types.h` (tipos `u16`/`u8`, no usados en la lógica core más allá de tipos) → reemplazar por `stdint.h` (`uint16_t`, `uint8_t`).
- 🟡 `def.h`, `logging.h`, `posprintf.h` — incluidos por `tetrisEngine.cpp` pero sin uso directo detectado en la lógica de estos 4 archivos; no son necesarios para el port del núcleo MVP.
- 🟡 `Game` (clase) → struct + funciones libres que reciben `Game*`. `Pawn`, `Drop`, `Score`, `SoundFlags` (clases con constructores de copia) → structs planos, sin lógica de copia (no aplica en C).
- 🟡 Contenedores STL (`std::list<int>` para `bag`, `queue`, `linesToClear`, `moveHistory`; `std::tuple`) → arrays/ring buffers de tamaño fijo.
- 🔴 `new`/`delete` (tablero, `disappearTimers`) → memoria estática.

## Fuera de MVP pero entreverado en el mismo código (extraer, no adaptar)

`pawn.big` (modo 2×2), rotationSystem NRS/ARS/CLASSIC + IRS/IHS/drought-history, T-spin/combo/zone/garbage/modos DIG-BATTLE-MASTER-SURVIVAL-CLASSIC, animación `disappearing`/créditos, clase `Bot` (IA, no depende de ella el núcleo), finesse (`moveHistory`, `getBestFinesse`), `connectedConversion`/`connectedFix` (tablas de conexión de tiles para render, no gameplay).

## Siguiente paso técnico sugerido

Escribir un `Game` en C reducido a los campos/funciones marcados 🟢/🟡 de arriba, con RNG y tipos SNES desde el inicio (evita reescribir dos veces). No hay bloqueo técnico identificado que justifique ASM en este núcleo — todo es aritmética entera simple salvo la tabla de gravedad (float → fixed-point).
