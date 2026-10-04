#!/usr/bin/env python3
"""Converte uma imagem (JPG/PNG) num header C com pixels RGB565 para a tela ST7789.

    python3 tools/imagem_rgb565.py ENTRADA SAIDA.h NOME LARGURA ALTURA [x0 y0 x1 y1]

- recorta a caixa x0,y0,x1,y1 da imagem original; sem caixa, recorta o CENTRO na proporção
  LARGURA:ALTURA (a imagem nunca fica esticada ou achatada);
- redimensiona para LARGURA×ALTURA (filtro LANCZOS);
- grava `const uint16_t NOME[] PROGMEM` + NOME_W / NOME_H.

RGB565 = 5 bits de vermelho, 6 de verde, 5 de azul (2 bytes por pixel) — o formato
do gfx->draw16bitRGBBitmap(). Precisa do Pillow (pip install pillow).
"""
import sys
from PIL import Image


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def recorte_central(tamanho, largura, altura):
    """Maior caixa no centro da imagem com a proporção largura:altura."""
    w, h = tamanho
    if w * altura > h * largura:          # imagem mais "larga" que o alvo: corta as laterais
        novo_w = h * largura // altura
        x0 = (w - novo_w) // 2
        return (x0, 0, x0 + novo_w, h)
    novo_h = w * altura // largura        # mais "alta": corta em cima e embaixo
    y0 = (h - novo_h) // 2
    return (0, y0, w, y0 + novo_h)


def main():
    if len(sys.argv) not in (6, 10):
        sys.exit(__doc__)
    entrada, saida, nome = sys.argv[1:4]
    largura, altura = int(sys.argv[4]), int(sys.argv[5])

    img = Image.open(entrada).convert("RGB")
    if len(sys.argv) == 10:
        img = img.crop(tuple(int(v) for v in sys.argv[6:10]))
    else:
        img = img.crop(recorte_central(img.size, largura, altura))
    img = img.resize((largura, altura), Image.LANCZOS)

    dados = img.tobytes()   # R,G,B,R,G,B,… (3 bytes por pixel)
    pixels = [rgb565(dados[i], dados[i + 1], dados[i + 2]) for i in range(0, len(dados), 3)]
    linhas = []
    for i in range(0, len(pixels), 12):
        linhas.append("  " + ", ".join(f"0x{p:04X}" for p in pixels[i:i + 12]) + ",")

    with open(saida, "w") as f:
        f.write("#pragma once\n")
        f.write(f"// GERADO por tools/imagem_rgb565.py a partir de {entrada} — não edite à mão.\n")
        f.write(f"// {largura}x{altura} pixels RGB565 ({largura * altura * 2} bytes na flash).\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"#define {nome.upper()}_W {largura}\n#define {nome.upper()}_H {altura}\n\n")
        f.write(f"const uint16_t {nome}[] PROGMEM = {{\n")
        f.write("\n".join(linhas))
        f.write("\n};\n")
    print(f"{saida}: {largura}x{altura}, {largura * altura * 2} bytes")


if __name__ == "__main__":
    main()
