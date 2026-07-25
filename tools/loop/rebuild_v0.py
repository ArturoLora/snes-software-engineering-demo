#!/usr/bin/env python3
"""
rebuild_v0.py — reproduccion real de V0, sin tocar el arbol de trabajo principal.

Resuelve el hallazgo H4 de la Story de humo: `make` sobre un arbol ya construido
devuelve "No se hace nada para 'all'" con codigo 0. Un revisor que corra V0 asi
firma un no-op creyendo que reprodujo la compilacion, y la regla de re-ejecucion
independiente queda vacia. La regla de proyecto de no usar `make clean` cierra la
salida obvia, porque limpiaria la ROM que queda lista para el emulador.

Que hace:

  1. Copia `snes/` a un directorio desechable.
  2. Borra en la COPIA los artefactos generados, usando como fuente la lista que
     `.gitignore` ya declara para `snes/`. Nunca toca el arbol principal.
  3. Compila alli con `make`, partiendo de cero de verdad.
  4. Compara la ROM producida con la del arbol principal.

Una ROM que no se puede reproducir desde las fuentes es un hallazgo, no un detalle:
significa que lo que se valido en el emulador no es lo que el repositorio describe.

Solo stdlib. Ver BOOTSTRAP.md.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SNES_DIR = REPO_ROOT / "snes"
GITIGNORE = REPO_ROOT / ".gitignore"
ROM_NAME = "apotris.sfc"


def md5(path: Path) -> str:
    h = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def generated_patterns() -> list[str]:
    """Patrones de artefactos generados bajo snes/, leidos de .gitignore.

    Se lee del .gitignore en vez de duplicar la lista aqui: si manana el build
    genera un artefacto nuevo, se declara en un solo sitio.
    """
    patterns: list[str] = []
    for raw in GITIGNORE.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("snes/"):
            patterns.append(line[len("snes/") :])
    return patterns


# Clases de artefacto que NO pueden sobrevivir al purgado. Si alguna queda, la
# compilacion siguiente reutilizaria trabajo previo y el PASS seria falso: la ROM
# "reproducida" vendria en parte de binarios que no se recompilaron.
# Se comprueban de forma independiente de .gitignore, precisamente para que un
# .gitignore incompleto no degrade la reproduccion en silencio.
MUST_NOT_SURVIVE = ("**/*.obj", "**/*.sfc", "**/*.sym", "**/*.pic", "**/*.pal")


def purge_generated(root: Path, patterns: list[str]) -> list[str]:
    removed: list[str] = []
    for pat in patterns:
        for p in sorted(root.glob(pat)):
            if p.is_file():
                p.unlink()
                removed.append(str(p.relative_to(root)))
    return removed


def assert_purged(root: Path) -> None:
    """Falla si sobrevivio algun artefacto que invalidaria la reproduccion."""
    survivors: list[str] = []
    for pat in MUST_NOT_SURVIVE:
        survivors += [
            str(p.relative_to(root)) for p in sorted(root.glob(pat)) if p.is_file()
        ]
    if survivors:
        raise SystemExit(
            "el purgado dejo artefactos que harian falsa la reproduccion "
            f"({len(survivors)}): {', '.join(survivors[:10])}"
            + (" ..." if len(survivors) > 10 else "")
            + "\nRevisar los patrones de snes/ en .gitignore."
        )


def tree_fingerprint(root: Path) -> dict[str, str]:
    """md5 de todos los archivos bajo root, por ruta relativa."""
    return {
        str(p.relative_to(root)): md5(p)
        for p in sorted(root.rglob("*"))
        if p.is_file()
    }


def diff_fingerprints(before: dict[str, str], after: dict[str, str]) -> list[str]:
    problems = []
    for path in sorted(set(before) | set(after)):
        b, a = before.get(path), after.get(path)
        if b is None:
            problems.append(f"aparecio: {path}")
        elif a is None:
            problems.append(f"desaparecio: {path}")
        elif a != b:
            problems.append(f"cambio: {path}")
    return problems


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    ap.add_argument(
        "--keep",
        action="store_true",
        help="no borrar el directorio de compilacion (para inspeccionarlo)",
    )
    args = ap.parse_args()

    if not SNES_DIR.is_dir():
        raise SystemExit(f"no existe {SNES_DIR}")
    if not os.environ.get("PVSNESLIB_HOME"):
        raise SystemExit("PVSNESLIB_HOME no esta definida; el build la necesita")

    main_rom = SNES_DIR / ROM_NAME
    main_before = md5(main_rom) if main_rom.is_file() else None
    print(f"[v0] ROM del arbol principal .. {main_before or '(no existe)'}")

    # Huella del arbol completo, no solo de la ROM: la promesa es "no toca el
    # arbol principal", y comprobar un unico archivo no la respalda.
    fp_before = tree_fingerprint(SNES_DIR)
    print(f"[v0] huella del arbol ........ {len(fp_before)} archivos")

    patterns = generated_patterns()
    print(f"[v0] patrones generados en .gitignore: {len(patterns)}")

    tmp = Path(tempfile.mkdtemp(prefix="apotris-v0-"))
    work = tmp / "snes"
    try:
        shutil.copytree(SNES_DIR, work)
        removed = purge_generated(work, patterns)
        print(f"[v0] copia en {work}")
        print(f"[v0] artefactos borrados en la copia: {len(removed)}")

        assert_purged(work)
        rebuilt = work / ROM_NAME

        print("[v0] compilando desde cero...")
        proc = subprocess.run(
            ["make"], cwd=work, capture_output=True, text=True, env=os.environ.copy()
        )
        tail = (proc.stdout or "").strip().splitlines()[-3:]
        for line in tail:
            print(f"[v0]   {line}")

        if proc.returncode != 0:
            print(f"[v0] make fallo con codigo {proc.returncode}", file=sys.stderr)
            err = (proc.stderr or "").strip().splitlines()[-15:]
            for line in err:
                print(f"[v0]   {line}", file=sys.stderr)
            print("\nFAIL")
            return 1

        if not rebuilt.is_file():
            print("[v0] make termino en 0 pero no produjo ROM", file=sys.stderr)
            print("\nFAIL")
            return 1

        rebuilt_md5 = md5(rebuilt)
        print(f"[v0] ROM reproducida ......... {rebuilt_md5}")
    finally:
        if args.keep:
            print(f"[v0] copia conservada en {tmp}")
        else:
            shutil.rmtree(tmp, ignore_errors=True)

    fp_after = tree_fingerprint(SNES_DIR)
    drift = diff_fingerprints(fp_before, fp_after)
    intact = not drift
    print(
        f"[v0] arbol principal intacto .. {'si' if intact else 'NO'} "
        f"({len(fp_after)} archivos comparados)"
    )

    print()
    print("-" * 62)
    if not intact:
        print(f"FAIL: el arbol principal cambio durante la reproduccion ({len(drift)}):")
        for d in drift[:10]:
            print(f"       {d}")
        if len(drift) > 10:
            print(f"       ... y {len(drift) - 10} mas")
        print("-" * 62)
        return 1
    main_after = md5(main_rom) if main_rom.is_file() else None
    if main_before is None:
        print("FAIL: no habia ROM en el arbol principal con la que comparar")
        print("-" * 62)
        return 1
    if rebuilt_md5 != main_before:
        print("FAIL: la ROM del arbol NO es reproducible desde las fuentes")
        print(f"       arbol       {main_before}")
        print(f"       reproducida {rebuilt_md5}")
        print("-" * 62)
        return 1

    print(f"PASS: ROM reproducible desde fuentes ({rebuilt_md5})")
    print("-" * 62)
    return 0


if __name__ == "__main__":
    sys.exit(main())
