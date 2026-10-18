#!/bin/sh
# Compila e roda o teste diferencial (referencia OpenGL classico x emulacao ES) com o Mesa llvmpipe.
# Uso: tests/run_gl_diff.sh <dir_khronos_gl_api> <dir_khronos_egl_api> <dir_de_trabalho>
#   (headers: https://github.com/KhronosGroup/OpenGL-Registry/tree/main/api e EGL-Registry/api)
set -e
GLAPI="$1"; EGLAPI="$2"; OUT="$3"
HERE="$(cd "$(dirname "$0")/.." && pwd)"
L=/usr/lib/x86_64-linux-gnu
INC="-I$HERE/source/Platform/GLES -I$HERE/tests -I$GLAPI -I$EGLAPI"
mkdir -p "$OUT"; rm -rf "$OUT/ref" "$OUT/ref_edge" "$OUT/emu"
rm -f "$OUT/gl_ref" "$OUT/gl_ref_edge" "$OUT/gl_emu"
g++ -std=c++17 -Wall -Wextra -DREF_GL            $INC "$HERE/tests/gl_diff_main.cpp" $L/libEGL.so.1 $L/libGL.so.1     -o "$OUT/gl_ref"
g++ -std=c++17 -Wall -Wextra -DREF_GL -DREF_CLAMP_EDGE $INC "$HERE/tests/gl_diff_main.cpp" $L/libEGL.so.1 $L/libGL.so.1 -o "$OUT/gl_ref_edge"
g++ -std=c++17 -Wall -Wextra                     $INC "$HERE/tests/gl_diff_main.cpp" $L/libEGL.so.1 $L/libGLESv2.so.2 -o "$OUT/gl_emu"
test -x "$OUT/gl_ref" && test -x "$OUT/gl_ref_edge" && test -x "$OUT/gl_emu"
export LIBGL_ALWAYS_SOFTWARE=1
"$OUT/gl_ref" "$OUT/ref"; "$OUT/gl_ref_edge" "$OUT/ref_edge"; "$OUT/gl_emu" "$OUT/emu"
echo "== GL_CLAMP classico x emulacao (diferencas de borda conhecidas) =="
python3 "$HERE/tests/gl_diff_compare.py" "$OUT/ref" "$OUT/emu" --save-png "$OUT/png"
echo "== CLAMP_TO_EDGE na referencia x emulacao (sem excecoes) =="
python3 "$HERE/tests/gl_diff_compare.py" "$OUT/ref_edge" "$OUT/emu" --strict
