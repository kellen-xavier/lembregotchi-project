# Arquitetura do hardware — Waveshare ESP32-S3 1.54" LCD

Placa usada no projeto **Lembregotchi**.

## 🗺️ Mapa da placa (esquema elétrico e documentação)

- **Esquema elétrico oficial (PDF):** https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.54/ESP32-S3-LCD-1.54-Schematic.pdf
- **Manual / wiki oficial:** https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54
- **Guia Arduino da Waveshare:** https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54/Arduino
- **Códigos de exemplo oficiais:** https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54
- **Página do produto:** https://www.waveshare.com/esp32-s3-lcd-1.54.htm

> O arquivo `docs/my-board.pdf` **não é desta placa**: é o datasheet de uma NVIDIA Jetson Orin NX.

---

## Visão geral

```
                         ┌──────────────────────────────────────────┐
   USB-C ───────────────►│                ESP32-S3R8                │
 (energia + gravação     │  2 núcleos 240 MHz · Wi-Fi · BLE 5       │
  + serial /dev/ttyACM0) │  8 MB PSRAM (interna) · 16 MB flash      │
                         └──┬──────────┬──────────┬──────────┬──────┘
                            │ SPI      │ I2C      │ I2S      │ SDMMC
                            ▼          ▼          ▼          ▼
                     ┌──────────┐ ┌─────────┐ ┌─────────┐ ┌──────────┐
                     │ Tela LCD │ │ QMI8658 │ │ ES8311  │ │ microSD  │
                     │ ST7789   │ │ (IMU)   │ │ (som)   │ │ (TF)     │
                     │ 240×240  │ │ ES7210  │ │   │     │ └──────────┘
                     └──────────┘ │ (micros)│ │ NS4150B │
                                  │ CST816* │ │ (ampli) │──► alto-falante
                                  └─────────┘ └─────────┘
          Botões: BOOT · PLUS · PWR        Bateria 3,7 V (conector MX1.25)
          * toque só na versão "Touch"
```

## Componentes

| Bloco | Chip | Função |
|---|---|---|
| Processador | **ESP32-S3R8** (rev v0.2) | 2 núcleos Xtensa LX7 a 240 MHz, Wi-Fi 2,4 GHz, Bluetooth LE 5 |
| Memória | 8 MB PSRAM + **16 MB flash** | PSRAM para imagens/buffers; flash para o programa e arquivos |
| Tela | **ST7789** | IPS 1,54", 240×240, 262K cores, SPI 4 fios |
| Toque (versão Touch) | **CST816** | toque capacitivo via I2C |
| Áudio (saída) | **ES8311** + **NS4150B** | codec de áudio + amplificador para o alto-falante |
| Áudio (entrada) | **ES7210** | 2 microfones com cancelamento de eco |
| Movimento | **QMI8658** | acelerômetro + giroscópio (6 eixos) |
| Armazenamento | slot **microSD (TF)** | arquivos, imagens, estado salvo |
| Energia | conector de bateria 3,7 V | carregamento pelo USB-C |
| USB | USB-Serial/JTAG nativo | aparece como `/dev/ttyACM0` (ID `303a:1001`) |

> ⚠️ Não há relógio de tempo real (RTC) externo. Para saber a hora certa, use a internet (NTP via Wi-Fi).

---

## Mapa de pinos (GPIO)

Todos conferidos no código oficial da Waveshare.

### Tela (SPI)

| Sinal | GPIO |
|---|---|
| SCK (relógio) | 38 |
| MOSI (dados) | 39 |
| CS (seleção) | 21 |
| DC (dado/comando) | 45 |
| RST (reset) | 40 |
| BL (luz de fundo) | 46 |

### Botões (ativos em LOW)

| Botão | GPIO | Observação |
|---|---|---|
| BOOT | 0 | segurar ao ligar = modo de gravação |
| PLUS | 4 | livre |
| PWR | 5 | livre (no firmware de fábrica, segurar = desligar) |

### Energia e bateria

| Função | GPIO | Observação |
|---|---|---|
| Manter ligada na bateria | 2 | HIGH = ligada, LOW = desliga |
| Leitura da tensão da bateria (ADC) | 1 | |
| Indicação de carregamento | 3 | entrada com pull-up |

### Barramento I2C (sensores, áudio e toque)

| Sinal | GPIO |
|---|---|
| SDA | 42 |
| SCL | 41 |

| Dispositivo | Endereço I2C |
|---|---|
| ES8311 (codec de áudio) | `0x18` |
| ES7210 (microfones) | `0x40` |
| QMI8658 (IMU) | `0x6B` |
| CST816 (toque) | `0x15` |

| Toque (versão Touch) | GPIO |
|---|---|
| TP_INT (interrupção) | 48 |
| TP_RST (reset) | 47 |

### Áudio (I2S)

| Sinal | GPIO |
|---|---|
| MCLK | 8 |
| BCLK | 9 |
| LRCK / WS | 10 |
| DIN (do microfone) | 11 |
| DOUT (para o alto-falante) | 12 |
| PA_CTRL (liga o amplificador) | 7 |

### Cartão microSD (SDMMC 4 bits)

| Sinal | GPIO |
|---|---|
| CLK | 16 |
| CMD | 15 |
| D0 | 17 |
| D1 | 18 |
| D2 | 13 |
| D3 | 14 |

---

## Linguagem de programação

- **Linguagem:** **C++** (arquivos `.ino`, sketches do Arduino).
- **Framework:** **Arduino-ESP32** (pacote `esp32 by Espressif Systems` **3.3.12**), que roda por cima
  do **ESP-IDF** (o SDK oficial da Espressif, em C) e do sistema de tempo real **FreeRTOS**.
- **Ferramentas:** Arduino IDE 2 (ou o `arduino-cli` que vem junto com ela), `esptool` para gravar.

### Configuração da placa (Arduino IDE → Tools)

| Opção | Valor |
|---|---|
| Board | **ESP32S3 Dev Module** |
| USB CDC On Boot | Enabled |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| Porta | `/dev/ttyACM0` |

FQBN (para `arduino-cli`):
```
esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB
```

---

## Bibliotecas

### Instaladas e em uso no projeto

| Biblioteca | Versão | Para que serve |
|---|---|---|
| **GFX Library for Arduino** (Arduino_GFX) | 1.6.8 | desenhar na tela ST7789 (formas, texto, imagens) |
| **OneButton** | 2.6.2 | reconhecer clique, duplo clique e clique longo nos botões |

> A versão 1.6.0 da GFX (usada nos exemplos da Waveshare) **não compila** com o pacote ESP32 3.3.x.
> Use a 1.6.8 ou mais nova.

### Já incluídas no pacote ESP32 (não precisa instalar)

| Biblioteca | Para que serve |
|---|---|
| `WiFi` | conectar à internet |
| `BLE` | Bluetooth Low Energy |
| `SD_MMC` | cartão microSD |
| `Wire` | barramento I2C |
| `ESP_I2S` | áudio digital |
| `Preferences` | salvar dados pequenos na flash |
| `LittleFS` | sistema de arquivos na flash |

### Usadas nos exemplos da Waveshare (instalar quando precisar)

| Biblioteca | Versão nos exemplos | Para que serve |
|---|---|---|
| lvgl | 8.4.0 / 9.3.0 | interface gráfica completa (menus, botões na tela) |
| U8g2 | 2.35.30 | fontes extras para a tela |
| SensorLib | 0.3.1 | sensor QMI8658 e toque CST816 |
| ESP32-audioI2S | 3.4.0 | tocar MP3/WAV pelo alto-falante |
