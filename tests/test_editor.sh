#!/usr/bin/env bash
# Batería de pruebas para Shell Text Editor.
# Ejecutar desde la raíz del proyecto: ./tests/test_editor.sh

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/sys_shell"
TMP="$(mktemp -d)"

PASS=0
FAIL=0
SKIP=0

trap 'rm -rf "$TMP"' EXIT

pass() {
    PASS=$((PASS + 1))
    printf '[PASS] %s\n' "$1"
}

fail() {
    FAIL=$((FAIL + 1))
    printf '[FAIL] %s -- %s\n' "$1" "${2:-}"
}

skip() {
    SKIP=$((SKIP + 1))
    printf '[SKIP] %s\n' "$1"
}

run() {
    RUN_OUT="$(cd "$ROOT" && printf '%s\n' "$1" | "$BIN" 2>&1)"
    RUN_STATUS=$?
}

contains() {
    local name="$1"
    local text="$2"
    local needle="$3"

    if grep -Fq -- "$needle" <<< "$text"; then
        pass "$name"
    else
        fail "$name" "No aparece: $needle"
    fi
}

file_eq() {
    local name="$1"
    local file="$2"
    local expected="$3"
    local actual

    if [ ! -f "$file" ]; then
        fail "$name" "No existe $file"
        return
    fi

    actual="$(cat "$file")"

    if [ "$actual" = "$expected" ]; then
        pass "$name"
    else
        fail "$name" "Contenido incorrecto"
    fi
}

printf '\n=== TESTS SHELL TEXT EDITOR ===\n'

if [ ! -x "$BIN" ]; then
    echo "[INFO] Compilando proyecto..."
    (
        cd "$ROOT" &&
        make clean &&
        make
    ) || {
        echo '[FATAL] No se pudo compilar.'
        exit 1
    }
fi


# ============================================================
# 1. Shell/editor
# ============================================================

run $'help\nexit'

contains \
    'Shell: help' \
    "$RUN_OUT" \
    'Shell editora de texto'

contains \
    'Shell: exit' \
    "$RUN_OUT" \
    'Saliendo del shell'

run $'editar\nq\nexit'

contains \
    'Editor: entrada' \
    "$RUN_OUT" \
    'Abriendo editor de texto'

contains \
    'Editor: salida con q' \
    "$RUN_OUT" \
    'Saliendo del editor'


# ============================================================
# 2. Comandos sin archivo abierto
# ============================================================

for cmd in \
    'p' \
    'a "x"' \
    'd 1' \
    'i 1 "x"' \
    's "x"' \
    'm' \
    'y 1'
do
    INPUT="$(printf 'editar\n%s\nq\nexit\n' "$cmd")"
    run "$INPUT"

    contains \
        "Sin archivo: $cmd" \
        "$RUN_OUT" \
        'No hay ningún archivo abierto'
done

run $'editar\nx 1\nq\nexit'

contains \
    'Sin archivo: x 1' \
    "$RUN_OUT" \
    'No hay nada en el portapapeles'


# ============================================================
# 3. Crear/abrir y append
# ============================================================

F1="$TMP/basic.txt"
rm -f "$F1"

run "editar
o \"$F1\"
a \"Primera línea\"
a \"Segunda línea\"
a \"Tercera línea\"
q
exit"

[ -f "$F1" ] \
    && pass 'o: crea archivo inexistente' \
    || fail 'o: crea archivo inexistente'

file_eq \
    'a: agrega líneas al final' \
    "$F1" \
    $'Primera línea\nSegunda línea\nTercera línea'


# ============================================================
# 4. p [n] y p
# ============================================================

run "editar
o \"$F1\"
p 1
p 2
p 3
p
q
exit"

contains \
    'p 1: primera línea' \
    "$RUN_OUT" \
    'Primera línea'

contains \
    'p 2: segunda línea' \
    "$RUN_OUT" \
    'Segunda línea'

contains \
    'p 3: tercera línea' \
    "$RUN_OUT" \
    'Tercera línea'


# ============================================================
# 5. i
# ============================================================

F2="$TMP/insert.txt"

printf '%s\n' uno tres > "$F2"

run "editar
o \"$F2\"
i 2 \"dos\"
q
exit"

file_eq \
    'i 2: inserción' \
    "$F2" \
    $'uno\ndos\ntres'


# ============================================================
# 6. d, incluyendo última línea y truncado
# ============================================================

F3="$TMP/delete.txt"

printf '%s\n' uno dos tres cuatro > "$F3"

run "editar
o \"$F3\"
d 2
q
exit"

file_eq \
    'd 2: elimina y desplaza' \
    "$F3" \
    $'uno\ntres\ncuatro'

run "editar
o \"$F3\"
d 3
q
exit"

file_eq \
    'd última línea: trunca archivo' \
    "$F3" \
    $'uno\ntres'


# ============================================================
# 7. s
# ============================================================

F4="$TMP/search.txt"

printf '%s\n' \
    'hola mundo hola' \
    'adios mundo' \
    hola > "$F4"

run "editar
o \"$F4\"
s \"hola\"
s \"mundo\"
s \"inexistente\"
q
exit"

contains \
    's: cuenta hola' \
    "$RUN_OUT" \
    "Se encontraron 3 coincidencias en total de 'hola'"

contains \
    's: cuenta mundo' \
    "$RUN_OUT" \
    "Se encontraron 2 coincidencias en total de 'mundo'"

contains \
    's: sin coincidencias' \
    "$RUN_OUT" \
    "No se encontraron coincidencias para 'inexistente'"


# ============================================================
# 8. m
# ============================================================

run "editar
o \"$F4\"
m
q
exit"

contains \
    'm: inodo' \
    "$RUN_OUT" \
    'Inodo:'

contains \
    'm: permisos' \
    "$RUN_OUT" \
    'Permisos (oct):'

contains \
    'm: tamaño' \
    "$RUN_OUT" \
    'Tamaño:'

contains \
    'm: modificación' \
    "$RUN_OUT" \
    'MTime (Modif):'


# ============================================================
# 9. y/x y reemplazo de clipboard
# ============================================================

F5="$TMP/clipboard.txt"

printf '%s\n' uno dos tres > "$F5"

run "editar
o \"$F5\"
y 2
x 1
q
exit"

file_eq \
    'y 2 + x 1: copiar/pegar' \
    "$F5" \
    $'dos\nuno\ndos\ntres'

F6="$TMP/clipboard_replace.txt"

printf '%s\n' AAA BBB CCC > "$F6"

run "editar
o \"$F6\"
y 1
y 3
x 2
q
exit"

file_eq \
    'y repetido: reemplaza clipboard' \
    "$F6" \
    $'AAA\nCCC\nBBB\nCCC'


# ============================================================
# 10. Cambiar de archivo abierto
# ============================================================

F7="$TMP/one.txt"
F8="$TMP/two.txt"

printf '%s\n' uno > "$F7"
printf '%s\n' dos > "$F8"

run "editar
o \"$F7\"
o \"$F8\"
p
q
exit"

contains \
    'o dos veces: usa segundo archivo' \
    "$RUN_OUT" \
    'dos'


# ============================================================
# 11. Archivo vacío
# ============================================================

F9="$TMP/empty.txt"

: > "$F9"

run "editar
o \"$F9\"
p
s \"algo\"
y 1
d 1
q
exit"

contains \
    'Vacío: p' \
    "$RUN_OUT" \
    'El archivo está vacío'

contains \
    'Vacío: y' \
    "$RUN_OUT" \
    'El archivo está vacío'

contains \
    'Vacío: d' \
    "$RUN_OUT" \
    'El archivo está vacío'


# ============================================================
# 12. Líneas inválidas/inexistentes y sintaxis
# ============================================================

F10="$TMP/errors.txt"

printf '%s\n' uno dos > "$F10"

run "editar
o \"$F10\"
p 99
d 99
i 99 \"x\"
y 99
x 1
q
exit"

contains \
    'Errores: p línea inexistente' \
    "$RUN_OUT" \
    'La línea 99 no existe'

contains \
    'Errores: d línea inexistente' \
    "$RUN_OUT" \
    'La línea 99 no existe'

contains \
    'Errores: i línea inexistente' \
    "$RUN_OUT" \
    'La línea 99 no existe'

contains \
    'Errores: y línea inexistente' \
    "$RUN_OUT" \
    'La línea 99 no existe'

contains \
    'Errores: x sin clipboard' \
    "$RUN_OUT" \
    'No hay nada en el portapapeles'


run "editar
o \"$F10\"
p 0
d 0
i 0 \"x\"
y 0
q
exit"

contains \
    'Validación: p 0' \
    "$RUN_OUT" \
    'El número de línea debe ser 1 o mayor'

contains \
    'Validación: d 0' \
    "$RUN_OUT" \
    'El número de línea debe ser 1 o mayor'

contains \
    'Validación: i 0' \
    "$RUN_OUT" \
    'El número de línea debe ser 1 o mayor'

contains \
    'Validación: y 0' \
    "$RUN_OUT" \
    'El número de línea debe ser 1 o mayor'


run $'editar\no\np 1 2\na\nd\ni\ny\nx\ns\nm 1\nq\nexit'

contains \
    'Sintaxis: o' \
    "$RUN_OUT" \
    'Uso: o <archivo>'

contains \
    'Sintaxis: p' \
    "$RUN_OUT" \
    'Uso: p [n]'

contains \
    'Sintaxis: a' \
    "$RUN_OUT" \
    'Uso: a <texto>'

contains \
    'Sintaxis: d' \
    "$RUN_OUT" \
    'Uso: d <n>'

contains \
    'Sintaxis: i' \
    "$RUN_OUT" \
    'Uso: i <n> <texto>'

contains \
    'Sintaxis: y' \
    "$RUN_OUT" \
    'Uso: y <n>'

contains \
    'Sintaxis: x' \
    "$RUN_OUT" \
    'Uso: x <n>'

contains \
    'Sintaxis: s' \
    "$RUN_OUT" \
    'Uso: s <texto>'

contains \
    'Sintaxis: m' \
    "$RUN_OUT" \
    'Uso: m'


INPUT="$(printf 'editar\no "%s"\na "x"\ns ""\nq\nexit\n' "$TMP/e.txt")"
run "$INPUT"

contains \
    's: rechaza búsqueda vacía' \
    "$RUN_OUT" \
    'El texto de búsqueda no puede estar vacío'


# ============================================================
# 13. Comillas/espacios
# ============================================================

F11="$TMP/archivo con espacios.txt"

run "editar
o \"$F11\"
a \"texto con espacios\"
q
exit"

file_eq \
    'Parser: espacios y comillas' \
    "$F11" \
    'texto con espacios'


# ============================================================
# Resultado
# ============================================================

printf '\n============================================================\n'
printf 'RESULTADO: %d PASS | %d FAIL | %d SKIP\n' "$PASS" "$FAIL" "$SKIP"
printf '============================================================\n'

[ "$FAIL" -eq 0 ] && exit 0 || exit 1