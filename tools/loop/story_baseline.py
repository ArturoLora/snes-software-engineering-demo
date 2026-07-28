#!/usr/bin/env python3
"""
story_baseline.py — baseline de Story medible aunque el arbol arranque sucio.

Resuelve el hallazgo H3 de la Story de humo: el baseline es un commit, pero si el
arbol ya tenia cambios antes de empezar la Story, el diff contra ese commit mezcla
el alcance real con la suciedad previa, y la Review no puede afirmar que no se toco
nada fuera de alcance.

La solucion es declarar la suciedad previa en la propia Story, y restarla al medir.

Dos subcomandos:

    snapshot   En Create Story. Emite el frontmatter con baseline_commit y
               baseline_dirty, obtenidos del estado real del repositorio.

    check      En Review. Calcula el conjunto atribuible a la Story:
               (archivos que difieren del baseline) - (baseline_dirty declarado).
               Ese conjunto es el que se audita contra la allowlist.

Solo stdlib. Ver BOOTSTRAP.md.
"""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]

# Marca para una ruta que, al hacer el snapshot, ya estaba borrada respecto a HEAD.
DELETED = "(borrado)"


def file_md5(path: Path) -> str:
    """md5 del archivo, o DELETED si no existe."""
    if not path.is_file():
        return DELETED
    h = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def git(*args: str) -> str:
    out = subprocess.run(
        ["git", *args], cwd=REPO_ROOT, capture_output=True, text=True, check=True
    )
    return out.stdout


def dirty_paths() -> list[str]:
    """Rutas con cambios respecto a HEAD: modificadas, staged o sin trackear.

    Se usa --porcelain por estabilidad de formato, y -uall para que un directorio
    nuevo se liste archivo por archivo en vez de colapsado.

    En modo -z un rename/copy ocupa DOS campos consecutivos: primero
    "XY <ruta-nueva>" y despues, como campo aparte, "<ruta-original>". Hay que
    consumir el segundo explicitamente; buscar " -> " dentro del primero no
    funciona porque ese separador solo existe en el modo sin -z.
    """
    fields = [f for f in git("status", "--porcelain", "-uall", "-z").split("\0") if f]
    paths: set[str] = set()

    i = 0
    while i < len(fields):
        entry = fields[i]
        status, path = entry[:2], entry[3:]
        i += 1
        if status[0] in ("R", "C") or status[1] in ("R", "C"):
            # El campo siguiente es la ruta original. Ambas cuentan como sucias:
            # la vieja porque desaparecio, la nueva porque aparecio.
            if i < len(fields):
                paths.add(fields[i])
                i += 1
        paths.add(path)

    return sorted(paths)


# Rutas ignoradas por Git que SI son fuera-de-alcance legitimo. Cualquier otra ruta
# ignorada se reporta, porque un ejecutor podria esconder codigo dentro de un
# directorio ignorado y quedar invisible al alcance.
#
# Salidas del build. Acotadas a snes/ a proposito: si el sufijo valiera en
# cualquier ruta, un archivo con contenido arbitrario llamado x.map o x.obj seria
# invisible al alcance en todo el repositorio.
EXPECTED_IGNORED_BUILD = {
    "prefix": "snes/",
    "suffixes": (
        ".o", ".obj", ".sfc", ".smc", ".sym", ".pic", ".pal", ".map",
        ".inc", ".lst", ".elf", "_data.as", "/linkfile",
    ),
}

# Directorios que, por su naturaleza, contienen material arbitrario que no es
# codigo del proyecto. Son puntos ciegos ACEPTADOS y declarados, no descuidos:
# ver BOOTSTRAP.md, "Archivos que Git ignora".
EXPECTED_IGNORED_PREFIXES = (
    "tools/harness/artifacts/",  # salidas de ejecucion del harness
    "tools/BizHawk-",            # emulador instalado localmente
    "reference/",                # fuente GBA original, solo lectura
    "_bmad/custom/",             # configuracion local de BMAD
)
EXPECTED_IGNORED_SUFFIXES = (".log",)  # logs de ejecucion
EXPECTED_IGNORED_CONTAINS = ("__pycache__/",)


def changed_vs(commit: str) -> list[str]:
    """Archivos que difieren de un commit: trackeados con cambios + sin trackear.

    --no-renames es obligatorio: con deteccion de renames activada, `git diff`
    colapsa un par origen/destino en la ruta destino, de modo que mover un archivo
    fuera de alcance a un directorio permitido lo hace desaparecer del conjunto
    atribuible. Sin renames se ven las dos rutas, que es lo que la contencion de
    alcance necesita.
    """
    tracked = [
        p for p in git("diff", "--name-only", "--no-renames", commit).splitlines() if p
    ]
    untracked = [
        p for p in git("ls-files", "--others", "--exclude-standard").splitlines() if p
    ]
    return sorted(set(tracked) | set(untracked))


def hidden_from_status() -> list[str]:
    """Archivos trackeados marcados para que Git deje de reportar sus cambios.

    `git update-index --assume-unchanged` y `--skip-worktree` sacan un archivo de
    `git diff` y de `git status`, asi que un .c fuera de alcance modificado bajo
    cualquiera de las dos banderas desaparece del conjunto atribuible. Es la misma
    invisibilidad que las rutas ignoradas, por otra via.

    En `git ls-files -v`, la letra inicial es 'H' para un archivo normal; minusculas
    indican assume-unchanged y 'S' skip-worktree.
    """
    marked = []
    for line in git("ls-files", "-v").splitlines():
        if not line or len(line) < 3:
            continue
        tag, path = line[0], line[2:]
        if tag != "H":
            marked.append(f"{path}  [{tag}]")
    return sorted(marked)


def unexpected_ignored() -> list[str]:
    """Rutas ignoradas por Git que NO son artefactos de build conocidos.

    `git ls-files --others --exclude-standard` no ve nada ignorado, asi que un
    archivo dentro de un directorio ignorado —`build/`, `out/`, o uno creado con un
    .gitignore anidado que se ignora a si mismo— seria invisible al conjunto
    atribuible. Aqui se listan para que la Review los vea.
    """
    out = git("ls-files", "--others", "--ignored", "--exclude-standard", "-z")
    paths = [p for p in out.split("\0") if p]

    unexpected = []
    for p in paths:
        if p.startswith(EXPECTED_IGNORED_BUILD["prefix"]) and p.endswith(
            EXPECTED_IGNORED_BUILD["suffixes"]
        ):
            continue
        if p.endswith(EXPECTED_IGNORED_SUFFIXES):
            continue
        if p.startswith(EXPECTED_IGNORED_PREFIXES):
            continue
        if any(frag in p for frag in EXPECTED_IGNORED_CONTAINS):
            continue
        unexpected.append(p)
    return sorted(unexpected)


def parse_frontmatter(story: Path) -> dict:
    """Parser minimo del frontmatter YAML que estas Stories usan.

    Solo entiende `clave: valor` y listas de guiones. Evita una dependencia por
    algo que el formato de Story ya mantiene simple a proposito.
    """
    text = story.read_text(encoding="utf-8")
    if not text.startswith("---"):
        raise SystemExit(f"{story}: no tiene frontmatter YAML")
    end = text.index("\n---", 3)
    body = text[3:end]

    data: dict = {}
    key = None
    for raw in body.splitlines():
        line = raw.rstrip()
        if not line.strip():
            continue
        stripped = line.lstrip()

        if stripped.startswith("- ") and key:
            # Elemento de lista. Dos formas admitidas:
            #   - ruta/archivo            (heredada, sin hash)
            #   - path: ruta/archivo      (con hash en la linea siguiente)
            item = stripped[2:].strip()
            if item.startswith("path:"):
                data.setdefault(key, []).append(
                    {"path": item[len("path:") :].strip(), "md5": None}
                )
            else:
                data.setdefault(key, []).append({"path": item, "md5": None})
        elif stripped.startswith("md5:") and key and data.get(key):
            # Continuacion del ultimo elemento de la lista.
            last = data[key][-1]
            if isinstance(last, dict):
                last["md5"] = stripped[len("md5:") :].strip()
        elif ":" in line and not line.startswith(" "):
            key, _, value = line.partition(":")
            key = key.strip()
            value = value.strip()
            if value in ("", "[]"):
                # Lista vacia. Sin este caso, "[]" se guardaba como string y al
                # iterarlo producia los caracteres '[' y ']' como si fueran rutas.
                data[key] = []
            else:
                data[key] = value
        # cualquier otra forma se ignora a proposito
    return data


def cmd_snapshot(args) -> int:
    commit = git("rev-parse", "--short", "HEAD").strip()
    dirty = dirty_paths()

    print("---")
    print(f"baseline_commit: {commit}")
    if dirty:
        print("baseline_dirty:")
        for p in dirty:
            print(f"  - path: {p}")
            print(f"    md5: {file_md5(REPO_ROOT / p)}")
    else:
        print("baseline_dirty: []")
    print("story_class: <clase>")
    print("minimum_validation: <nivel derivado de la clase>")
    print("---")
    print()

    if dirty:
        print(
            f"[baseline] arbol SUCIO al crear la Story: {len(dirty)} archivo(s).",
            file=sys.stderr,
        )
        print(
            "[baseline] Pegar el bloque de arriba en el frontmatter. La Review "
            "restara esos archivos del diff antes de auditar el alcance.",
            file=sys.stderr,
        )
        print(
            "[baseline] Preferible: commitear o revertir esos cambios primero y "
            "volver a ejecutar, para que baseline_dirty quede vacio.",
            file=sys.stderr,
        )
    else:
        print("[baseline] arbol limpio. baseline_dirty vacio.", file=sys.stderr)
    return 0


def cmd_check(args) -> int:
    story = Path(args.story).resolve()
    if not story.is_file():
        raise SystemExit(f"no existe el archivo de Story: {story}")

    fm = parse_frontmatter(story)
    commit = fm.get("baseline_commit")
    if not commit:
        raise SystemExit(f"{story}: el frontmatter no declara baseline_commit")
    entries = fm.get("baseline_dirty") or []
    declared = {e["path"]: e.get("md5") for e in entries if isinstance(e, dict)}

    changed = set(changed_vs(commit))

    # Una ruta declarada se resta SOLO si su contenido sigue siendo el declarado.
    # Si cambio durante la Story, el cambio pertenece a la Story y entra en el
    # conjunto atribuible: restarlo seria blanquearlo.
    subtract: set[str] = set()
    modified_since: list[str] = []
    stale: list[str] = []
    unhashed: list[str] = []

    for path, declared_md5 in declared.items():
        if path not in changed:
            stale.append(path)
            continue
        if declared_md5 is None:
            # Sin md5 no hay nada que comparar, asi que restar por ruta devolveria
            # el mecanismo al estado que el hash existe para cerrar. Se rechaza en
            # vez de avisar: un `md5 :` con espacio de mas, en MAYUSCULAS o
            # simplemente omitido parece correcto a ojo y anularia la comparacion en
            # silencio. (La sangria si es libre: espacios o tabuladores dan igual.)
            # No hay frontmatter heredado en este repositorio, asi que la
            # retrocompatibilidad no protegeria nada.
            unhashed.append(path)
            continue
        current = file_md5(REPO_ROOT / path)
        if current == declared_md5:
            subtract.add(path)
        else:
            # DELETED no es un hash: truncarlo a 8 partiria el parentesis de cierre.
            # Vale para los dos lados: una ruta declarada como borrada puede reaparecer.
            short = current if current == DELETED else current[:8]
            was = declared_md5 if declared_md5 == DELETED else declared_md5[:8]
            modified_since.append(f"{path}  ({was} -> {short})")

    attributable = sorted(changed - subtract)
    stale.sort()

    try:
        shown = story.relative_to(REPO_ROOT)
    except ValueError:
        # Story fuera del repo (p.ej. un archivo de prueba); mostrar ruta absoluta
        shown = story
    print(f"Story ............. {shown}")
    print(f"baseline_commit ... {commit}")
    print(
        f"baseline_dirty .... {len(declared)} declarado(s), {len(subtract)} restado(s)"
    )
    print(f"difieren del base . {len(changed)}")
    print()
    print(f"CONJUNTO ATRIBUIBLE ({len(attributable)}) — auditar contra la allowlist:")
    for p in attributable:
        print(f"  {p}")
    if not attributable:
        print("  (ninguno)")

    failed = False

    if modified_since:
        print()
        print(f"DECLARADOS PERO MODIFICADOS ({len(modified_since)}):")
        for m in modified_since:
            print(f"  {m}")
        print()
        print(
            "[check] FAIL: estas rutas se declararon sucias pero su contenido cambio "
            "despues. No se restan —el cambio pertenece a la Story— y la declaracion "
            "ya no describe el estado inicial. Hay que reconciliarla: justificar el "
            "cambio en la allowlist y regenerar el frontmatter, o revertirlo.",
            file=sys.stderr,
        )
        failed = True

    if stale:
        print()
        print(f"DECLARACION OBSOLETA ({len(stale)}):")
        for p in stale:
            print(f"  {p}")
        print()
        print(
            "[check] FAIL: hay rutas en baseline_dirty que ya NO difieren del "
            "baseline. Una declaracion obsoleta puede estar ocultando un cambio "
            "real de la Story. Actualizar el frontmatter y repetir.",
            file=sys.stderr,
        )
        failed = True

    if unhashed:
        print()
        print(f"DECLARADOS SIN md5 ({len(unhashed)}):")
        for p in unhashed:
            print(f"  {p}")
        print()
        print(
            "[check] FAIL: sin md5 no hay contenido que comparar, asi que la ruta no "
            "se resta. La clave debe ser exactamente `md5:` en minusculas y sin "
            "espacio antes de los dos puntos (`md5 :` y `MD5:` no se reconocen; la "
            "sangria puede ser espacios o tabuladores). Regenerar el frontmatter con "
            "`snapshot`.",
            file=sys.stderr,
        )
        failed = True

    hidden = hidden_from_status()
    if hidden:
        print()
        print(f"OCULTOS A GIT STATUS ({len(hidden)}):")
        for h in hidden:
            print(f"  {h}")
        print()
        print(
            "[check] FAIL: hay archivos trackeados marcados con assume-unchanged o "
            "skip-worktree. Git deja de reportar sus cambios, asi que quedan fuera "
            "del conjunto atribuible y pueden esconder modificaciones fuera de "
            "alcance. Quitar la marca con `git update-index --no-assume-unchanged` "
            "o `--no-skip-worktree` y repetir.",
            file=sys.stderr,
        )
        failed = True

    ignored = unexpected_ignored()
    if ignored:
        print()
        print(f"IGNORADOS NO ESPERADOS ({len(ignored)}):")
        for p in ignored:
            print(f"  {p}")
        print()
        print(
            "[check] FAIL: hay archivos que Git ignora y que no son artefactos de "
            "build conocidos. Un archivo dentro de un directorio ignorado es "
            "invisible al conjunto atribuible, asi que puede esconder cambios fuera "
            "de alcance. Revisarlos, o declararlos como artefacto esperado en "
            "story_baseline.py.",
            file=sys.stderr,
        )
        failed = True

    if failed:
        return 1

    print()
    print("[check] OK: la declaracion de baseline_dirty es coherente.")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("snapshot", help="emitir frontmatter de baseline (Create Story)")

    c = sub.add_parser("check", help="calcular el conjunto atribuible (Review)")
    c.add_argument("story", help="ruta al archivo de Story")

    args = ap.parse_args()
    return {"snapshot": cmd_snapshot, "check": cmd_check}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
