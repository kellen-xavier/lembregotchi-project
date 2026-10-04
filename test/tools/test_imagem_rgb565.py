"""Testes de tools/imagem_rgb565.py (rode com: make test, ou python3 -m unittest discover test/tools)."""
import importlib.util
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

from PIL import Image

RAIZ = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = RAIZ / "tools" / "imagem_rgb565.py"

spec = importlib.util.spec_from_file_location("imagem_rgb565", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def gerar(img, largura, altura, corte=None):
    """Roda o script de verdade e devolve (texto do .h, lista de pixels RGB565)."""
    with tempfile.TemporaryDirectory() as d:
        entrada = pathlib.Path(d) / "entrada.png"
        saida = pathlib.Path(d) / "saida.h"
        img.save(entrada)
        args = [sys.executable, str(SCRIPT), str(entrada), str(saida), "img_teste", str(largura), str(altura)]
        if corte:
            args += [str(v) for v in corte]
        subprocess.run(args, check=True, capture_output=True)
        texto = saida.read_text()
    return texto, [int(x, 16) for x in re.findall(r"0x([0-9A-F]{4})", texto)]


class TestRgb565(unittest.TestCase):
    def test_cores_basicas(self):
        self.assertEqual(mod.rgb565(255, 0, 0), 0xF800)
        self.assertEqual(mod.rgb565(0, 255, 0), 0x07E0)
        self.assertEqual(mod.rgb565(0, 0, 255), 0x001F)
        self.assertEqual(mod.rgb565(255, 255, 255), 0xFFFF)
        self.assertEqual(mod.rgb565(0, 0, 0), 0x0000)


class TestRecorteCentral(unittest.TestCase):
    def test_imagem_larga_corta_as_laterais(self):
        self.assertEqual(mod.recorte_central((440, 150), 220, 150), (110, 0, 330, 150))

    def test_imagem_alta_corta_em_cima_e_embaixo(self):
        self.assertEqual(mod.recorte_central((220, 450), 220, 150), (0, 150, 220, 300))

    def test_mesma_proporcao_nao_corta(self):
        self.assertEqual(mod.recorte_central((440, 300), 220, 150), (0, 0, 440, 300))


class TestGerarHeader(unittest.TestCase):
    def test_tamanho_nomes_e_quantidade_de_pixels(self):
        texto, pixels = gerar(Image.new("RGB", (100, 100), (255, 0, 0)), 22, 15)
        self.assertIn("#define IMG_TESTE_W 22", texto)
        self.assertIn("#define IMG_TESTE_H 15", texto)
        self.assertIn("const uint16_t img_teste[] PROGMEM", texto)
        self.assertEqual(len(pixels), 22 * 15)
        self.assertTrue(all(p == 0xF800 for p in pixels))   # tudo vermelho

    def test_sem_corte_usa_o_centro_sem_esticar(self):
        # Faixa larga: azul nas pontas, vermelho no meio. O recorte central só pega o vermelho.
        img = Image.new("RGB", (300, 50), (0, 0, 255))
        img.paste((255, 0, 0), (100, 0, 200, 50))
        _, pixels = gerar(img, 30, 15)
        self.assertTrue(all(p == 0xF800 for p in pixels))

    def test_corte_explicito(self):
        img = Image.new("RGB", (100, 100), (0, 0, 255))
        img.paste((0, 255, 0), (0, 0, 50, 50))           # canto superior esquerdo verde
        _, pixels = gerar(img, 10, 10, corte=(0, 0, 50, 50))
        self.assertTrue(all(p == 0x07E0 for p in pixels))

    def test_argumentos_errados_mostram_ajuda(self):
        r = subprocess.run([sys.executable, str(SCRIPT), "so-um-argumento"], capture_output=True, text=True)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("Converte uma imagem", r.stderr)


if __name__ == "__main__":
    unittest.main()
