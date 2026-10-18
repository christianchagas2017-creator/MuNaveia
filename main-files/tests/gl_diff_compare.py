#!/usr/bin/env python3
"""Compara os .rgba de referencia e de emulacao (gl_diff_main.cpp).

Uso: gl_diff_compare.py <dir_ref> <dir_emu> [--save-png dir]
Saida: por cena, diferenca maxima por canal, quantidade de pixels acima da tolerancia
e se a cena e uma das "diferencas conhecidas" (nao falham o teste).
"""
import os, sys, struct, zlib

W = H = 96
STRICT = "--strict" in sys.argv   # referencia com CLAMP_TO_EDGE: sem diferencas conhecidas
TOL = 3                 # diferenca por canal tolerada (arredondamento de interpolacao/neblina)
MAX_BAD_FRAC = 0.002    # fracao de pixels acima da tolerancia aceita (bordas de rasterizacao)
# cena -> fracao maxima aceita, motivo
KNOWN = {
    "clamp_linear": (1.0, "GL_CLAMP classico mistura a borda com a cor de borda (preto); GL_CLAMP_TO_EDGE nao"),
    "ui":           (0.06, "idem: borda do sprite com GL_CLAMP+GL_LINEAR"),
    "lines":        (0.02, "rasterizacao de linhas pode diferir em 1 pixel entre contextos"),
}

def png(path, px):
    raw = b"".join(b"\x00" + px[y * W * 4:(y + 1) * W * 4] for y in range(H - 1, -1, -1))
    def ch(t, d): return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + ch(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0)) + ch(b"IDAT", zlib.compress(raw)) + ch(b"IEND", b""))

def main():
    ref, emu = sys.argv[1], sys.argv[2]
    save = sys.argv[sys.argv.index("--save-png") + 1] if "--save-png" in sys.argv else None
    if save: os.makedirs(save, exist_ok=True)
    names = sorted(f[:-5] for f in os.listdir(ref) if f.endswith(".rgba"))
    if not names: print("nenhuma cena em", ref); return 2
    fail = 0
    for n in names:
        a = open(os.path.join(ref, n + ".rgba"), "rb").read()
        pe = os.path.join(emu, n + ".rgba")
        if not os.path.exists(pe): print(f"{n:16s} FALTA na emulacao"); fail += 1; continue
        b = open(pe, "rb").read()
        if len(a) != len(b): print(f"{n:16s} tamanhos diferentes"); fail += 1; continue
        mx = 0; bad = 0
        for i in range(0, len(a), 4):
            d = max(abs(a[i] - b[i]), abs(a[i + 1] - b[i + 1]), abs(a[i + 2] - b[i + 2]))
            if d > mx: mx = d
            if d > TOL: bad += 1
        frac = bad / (W * H)
        # a cena precisa ter conteudo (evita comparar dois quadros vazios iguais)
        distinct = len(set(a[i:i + 3] for i in range(0, len(a), 4)))
        lim, why = KNOWN.get(n, (MAX_BAD_FRAC, "")) if not STRICT else (MAX_BAD_FRAC, "")
        ok = frac <= lim and distinct >= 2
        print(f"{n:16s} max={mx:3d}  pixels>{TOL}: {bad:5d} ({frac*100:5.2f}%)  cores={distinct:4d}  {'OK' if ok else 'FALHA'}" + (f"   [{why}]" if why and bad else ""))
        if not ok: fail += 1
        if save:
            png(os.path.join(save, n + "_ref.png"), a); png(os.path.join(save, n + "_emu.png"), b)
    print("RESULTADO:", "FALHA" if fail else "OK")
    return 1 if fail else 0

sys.exit(main())
