#!/usr/bin/env python3
"""
harness.py — primer orquestador del Test Harness de Apotris SNES.

Lanza BizHawk desde Python, carga la ROM y el script Lua del PoC de forma
automatica, espera a que el script demuestre que esta leyendo memoria SNES
frame a frame, y reporta PASS / FAIL.

No modifica el juego, no crea TestStatus, no usa OCR / captura de pantalla /
automatizacion de teclado o raton. Todo el control es via los argumentos de
linea de comandos oficiales de BizHawk mas el archivo de log que el propio
script Lua escribe.

Argumentos de BizHawk usados (verificados en las cadenas UTF-16 de
tools/BizHawk-2.11.1-linux-x64/dll/BizHawk.Client.Common.dll, donde vive el
ArgParser de esta build):

    --lua=<ruta>    carga y ejecuta un script Lua al arrancar
    --luaconsole    abre la ventana Lua Console
    <ruta>          argumento posicional: la ROM a cargar

Limitaciones conocidas: ver tools/harness/README.md
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import signal
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

BIZHAWK_LAUNCHER = REPO_ROOT / "tools/BizHawk-2.11.1-linux-x64/EmuHawkMono.sh"
DEFAULT_ROM = REPO_ROOT / "snes/apotris.sfc"
DEFAULT_LUA = REPO_ROOT / "tools/lua/poc_read_memory.lua"

# El script Lua del PoC escribe aqui (ruta fijada dentro del propio .lua).
DEFAULT_LUA_LOG = REPO_ROOT / "tools/lua/poc_read_memory.log"

ARTIFACT_DIR = REPO_ROOT / "tools/harness/artifacts"

POLL_INTERVAL_S = 0.25
MIN_SAMPLES = 2  # INIT + al menos una linea CHANGE/IDLE posterior

# Frames de emulacion que deben transcurrir para aceptar que el bucle Lua
# realmente avanza frame a frame y no solo arranco.
MIN_FRAMES_ADVANCED = 180

# bsnes rellena la WRAM sin inicializar con 0x55. Si `gs` sigue en 0x55 no
# estamos leyendo estado del juego, solo RAM virgen.
UNINIT_FILL = 0x55

RE_DOMAIN = re.compile(r"^Dominio usado \. (?P<domain>.+)$", re.MULTILINE)
RE_SAMPLE = re.compile(
    r"^(?P<kind>INIT|CHANGE|IDLE)\s+f=(?P<frame>\d+)\s+pad=(?P<pad>[0-9A-Fa-f]{4})\s+"
    r"piece\[type=(?P<type>-?\d+) rot=(?P<rot>-?\d+) x=(?P<x>-?\d+) y=(?P<y>-?\d+)\]\s+"
    r"lines=(?P<lines>-?\d+)$",
    re.MULTILINE,
)


# Variables que un terminal empaquetado como snap (p.ej. el snap de VS Code)
# inyecta apuntando a librerias del propio snap. Heredarlas hace que GTK cargue
# una glibc distinta a la del sistema y Mono muera con
# "undefined symbol: __libc_pthread_init". Se limpian antes de lanzar BizHawk.
SNAP_LEAKY_VARS = (
    "LD_LIBRARY_PATH",
    "LD_PRELOAD",
    "GTK_PATH",
    "GTK_EXE_PREFIX",
    "GTK_MODULES",
    "GTK_IM_MODULE_FILE",
    "GIO_MODULE_DIR",
    "GSETTINGS_SCHEMA_DIR",
    "LOCPATH",
    "GDK_PIXBUF_MODULE_FILE",
    "GDK_PIXBUF_MODULEDIR",
)

SNAP_ORIG_SUFFIX = "_VSCODE_SNAP_ORIG"


def clean_env() -> tuple[dict[str, str], list[str]]:
    """Entorno sin contaminacion de snap. Devuelve (env, notas)."""
    env = dict(os.environ)
    notes: list[str] = []

    # El snap de VS Code guarda el valor original en <VAR>_VSCODE_SNAP_ORIG.
    # Restaurarlo es mas fiable que borrar a ciegas.
    for key in [k for k in env if k.endswith(SNAP_ORIG_SUFFIX)]:
        base = key[: -len(SNAP_ORIG_SUFFIX)]
        original = env.pop(key)
        if original:
            env[base] = original
        else:
            env.pop(base, None)
        notes.append(f"restaurado {base} desde {key}")

    for key in SNAP_LEAKY_VARS:
        value = env.get(key)
        if value and "/snap/" in value:
            env.pop(key)
            notes.append(f"eliminado {key} (apuntaba a /snap)")

    return env, notes


@dataclass
class Verdict:
    """Resultado de evaluar el log del script Lua."""

    ok: bool
    reasons: list[str] = field(default_factory=list)
    domain: str | None = None
    samples: int = 0
    first_frame: int | None = None
    last_frame: int | None = None
    frames_advanced: int = 0
    game_state_seen: bool = False


def log(msg: str) -> None:
    print(f"[harness] {msg}", flush=True)


def preflight(rom: Path, lua: Path) -> list[str]:
    """Comprueba lo que debe existir antes de arrancar. Devuelve fallos."""
    problems: list[str] = []

    if not BIZHAWK_LAUNCHER.is_file():
        problems.append(f"no existe el lanzador de BizHawk: {BIZHAWK_LAUNCHER}")
    elif not os.access(BIZHAWK_LAUNCHER, os.X_OK):
        problems.append(f"el lanzador no es ejecutable: {BIZHAWK_LAUNCHER}")

    if not rom.is_file():
        problems.append(f"no existe la ROM: {rom} (correr `make` en snes/)")
    if not lua.is_file():
        problems.append(f"no existe el script Lua: {lua}")

    if shutil.which("mono") is None:
        problems.append("`mono` no esta en PATH; BizHawk en Linux corre bajo Mono")

    # BizHawk es una app WinForms: necesita un servidor grafico. Esta build no
    # trae modo headless y el sistema no tiene Xvfb, asi que DISPLAY es
    # obligatorio.
    if not os.environ.get("DISPLAY") and not os.environ.get("WAYLAND_DISPLAY"):
        problems.append(
            "no hay DISPLAY ni WAYLAND_DISPLAY; BizHawk necesita sesion grafica "
            "(esta build no tiene modo headless)"
        )

    return problems


def evaluate(log_text: str) -> Verdict:
    """Decide PASS/FAIL a partir del contenido del log del script Lua."""
    v = Verdict(ok=False)

    m = RE_DOMAIN.search(log_text)
    if m:
        v.domain = m.group("domain").strip()
    else:
        v.reasons.append("el log no declara el dominio de memoria elegido")

    samples = list(RE_SAMPLE.finditer(log_text))
    v.samples = len(samples)

    if samples:
        v.first_frame = int(samples[0].group("frame"))
        v.last_frame = int(samples[-1].group("frame"))

    if v.samples < MIN_SAMPLES:
        v.reasons.append(
            f"solo {v.samples} lectura(s) en el log, se esperaban >= {MIN_SAMPLES}"
        )

    if v.first_frame is not None and v.last_frame is not None:
        v.frames_advanced = v.last_frame - v.first_frame
        if v.frames_advanced < MIN_FRAMES_ADVANCED:
            v.reasons.append(
                f"la emulacion solo avanzo {v.frames_advanced} frame(s) "
                f"(de {v.first_frame} a {v.last_frame}), se esperaban "
                f">= {MIN_FRAMES_ADVANCED}"
            )

    # Al menos una lectura debe salir del patron de RAM virgen: prueba de que
    # el juego escribio en `gs` y que estamos viendo estado real, no 0x55.
    for s in samples:
        fields = [int(s.group(k)) for k in ("type", "rot", "x", "y", "lines")]
        if any(f != UNINIT_FILL for f in fields):
            v.game_state_seen = True
            break

    if not v.game_state_seen:
        v.reasons.append(
            f"`gs` sigue en el patron de WRAM sin inicializar (0x{UNINIT_FILL:02X}); "
            "no hay evidencia de estado de juego escrito"
        )

    v.ok = not v.reasons
    return v


def wait_for_evidence(
    proc: subprocess.Popen, lua_log: Path, timeout_s: float
) -> tuple[Verdict, bool]:
    """Sondea el log hasta que hay evidencia suficiente, timeout, o BizHawk muere.

    Devuelve (veredicto, murio_bizhawk).
    """
    deadline = time.monotonic() + timeout_s
    last_verdict = Verdict(ok=False, reasons=["el script Lua nunca escribio el log"])
    announced = False

    while time.monotonic() < deadline:
        if proc.poll() is not None:
            # BizHawk termino solo. Evaluamos lo que haya quedado escrito.
            if lua_log.is_file():
                last_verdict = evaluate(lua_log.read_text(errors="replace"))
            return last_verdict, True

        if lua_log.is_file():
            if not announced:
                log(f"el script Lua esta escribiendo {lua_log}")
                announced = True
            last_verdict = evaluate(lua_log.read_text(errors="replace"))
            if last_verdict.ok:
                return last_verdict, False

        time.sleep(POLL_INTERVAL_S)

    return last_verdict, False


def shutdown(proc: subprocess.Popen) -> None:
    """Cierra BizHawk. El script Lua del PoC hace bucle infinito y no llama a
    client.exit(), asi que el harness es quien tiene que terminarlo."""
    if proc.poll() is not None:
        return
    log("cerrando BizHawk")
    proc.send_signal(signal.SIGTERM)
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        log("SIGTERM ignorado; enviando SIGKILL")
        proc.kill()
        proc.wait(timeout=10)


def tail(path: Path, lines: int = 15) -> str:
    if not path.is_file():
        return f"(sin {path.name})"
    content = path.read_text(errors="replace").splitlines()
    if not content:
        return f"({path.name} vacio)"
    return "\n".join(f"    | {ln}" for ln in content[-lines:])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    ap.add_argument("--rom", type=Path, default=DEFAULT_ROM, help="ROM .sfc a cargar")
    ap.add_argument("--lua", type=Path, default=DEFAULT_LUA, help="script Lua a ejecutar")
    ap.add_argument(
        "--lua-log",
        type=Path,
        default=DEFAULT_LUA_LOG,
        help="archivo de log que escribe el script Lua",
    )
    ap.add_argument(
        "--timeout",
        type=float,
        default=45.0,
        help="segundos maximos de espera antes de declarar FAIL (def: 45)",
    )
    ap.add_argument(
        "--keep-open",
        action="store_true",
        help="no cerrar BizHawk al terminar (para inspeccion manual)",
    )
    ap.add_argument(
        "--no-lua-console",
        action="store_true",
        help="no pasar --luaconsole (la ventana Lua Console no se abre)",
    )
    args = ap.parse_args()

    rom = args.rom.resolve()
    lua = args.lua.resolve()
    lua_log = args.lua_log.resolve()

    problems = preflight(rom, lua)
    if problems:
        for p in problems:
            log(f"preflight: {p}")
        print("\nFAIL")
        return 1

    ARTIFACT_DIR.mkdir(parents=True, exist_ok=True)
    stdout_path = ARTIFACT_DIR / "bizhawk_stdout.txt"
    stderr_path = ARTIFACT_DIR / "bizhawk_stderr.txt"

    # Log viejo fuera: si no reaparece, es que el script no lo escribio ahora.
    if lua_log.exists():
        lua_log.unlink()

    cmd = [str(BIZHAWK_LAUNCHER), f"--lua={lua}"]
    if not args.no_lua_console:
        cmd.append("--luaconsole")
    cmd.append(str(rom))

    log(f"ROM ...... {rom}")
    log(f"Lua ...... {lua}")
    log(f"comando .. {' '.join(cmd)}")

    env, env_notes = clean_env()
    for note in env_notes:
        log(f"entorno: {note}")

    with open(stdout_path, "wb") as out, open(stderr_path, "wb") as err:
        proc = subprocess.Popen(
            cmd,
            stdout=out,
            stderr=err,
            stdin=subprocess.DEVNULL,
            cwd=str(BIZHAWK_LAUNCHER.parent),
            env=env,
        )
        log(f"BizHawk lanzado (pid {proc.pid}); esperando hasta {args.timeout:g}s")
        verdict, died = wait_for_evidence(proc, lua_log, args.timeout)

        if not args.keep_open:
            shutdown(proc)
        elif verdict.ok:
            log("--keep-open: BizHawk sigue abierto")

    print()
    print("-" * 62)
    if verdict.domain:
        print(f"dominio de memoria .. {verdict.domain}")
    print(f"lecturas en el log .. {verdict.samples}")
    if verdict.first_frame is not None:
        print(
            f"frames observados ... {verdict.first_frame} -> {verdict.last_frame} "
            f"(+{verdict.frames_advanced})"
        )
    print(f"estado de juego ..... {'si' if verdict.game_state_seen else 'no'}")
    print("-" * 62)

    if verdict.ok:
        print("PASS")
        return 0

    if died:
        code = proc.returncode
        log(f"BizHawk termino por su cuenta con codigo {code}")
    else:
        log("timeout esperando evidencia del script Lua")

    for reason in verdict.reasons:
        log(f"motivo: {reason}")

    print()
    print(f"BizHawk stdout ({stdout_path}):")
    print(tail(stdout_path))
    print(f"BizHawk stderr ({stderr_path}):")
    print(tail(stderr_path))
    print()
    print("FAIL")
    return 1


if __name__ == "__main__":
    sys.exit(main())
