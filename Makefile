# Lembregotchi — tarefas do projeto
#
#   make test                  → todos os testes no PC: C++ (doctest + ASan/UBSan), ponte (node:test)
#                                e ferramentas (Python unittest)
#   make compilar              → compila o firmware e os testes de placa (sem gravar)
#   make gravar                → grava o firmware na placa
#   make gravar-teste T=passo1_tela   → grava um teste de placa (test/placa/<T>)
#   make ide                   → deixa a biblioteca visível na Arduino IDE (link em ~/Arduino/libraries)
#   make imagem IMG=foto.jpg   → troca a imagem do Pomodoro (recorte central; ou CORTE="x0 y0 x1 y1")
#   make clean                 → apaga o que foi compilado
#
# Testes unitários: só a lógica pura (firmware/libraries/Lembregotchi/src/logica/) — ela não
# usa Arduino. Sanitizers: o teste para com erro se houver acesso fora de memória ou
# comportamento indefinido, mesmo quando o resultado "parece" certo.

CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -g -O1 \
            -fsanitize=address,undefined -fno-omit-frame-pointer
BIBLIOTECA := firmware/libraries/Lembregotchi
INCLUDES := -Itest/vendor -I$(BIBLIOTECA)/src

LOGICA   := $(wildcard $(BIBLIOTECA)/src/logica/*.cpp)
TESTES   := $(wildcard test/unit/*.cpp)
BUILD    := build/test

# Placa (arduino-cli da Arduino IDE)
ARDUINO_CLI ?= /opt/arduino-ide/resources/app/lib/backend/resources/arduino-cli
ARDUINO_CFG ?= $(HOME)/.arduinoIDE/arduino-cli.yaml
FQBN        := esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB
PORTA       ?= /dev/ttyACM0
CLI         := $(ARDUINO_CLI) --config-file $(ARDUINO_CFG)
LIBS        := --libraries firmware/libraries
TESTES_PLACA := $(notdir $(wildcard test/placa/*))

.PHONY: test test-unit test-apps-script test-tools compilar gravar gravar-teste ide imagem clean

test: test-unit test-apps-script test-tools

# tools/ (conversor de imagem)
test-tools:
	python3 -m unittest discover -s test/tools

# Ponte apps-script/Codigo.gs com Google falso (test/apps-script/)
test-apps-script:
	node --test test/apps-script/

test-unit: $(BUILD)/unit
	./$(BUILD)/unit

$(BUILD)/unit: $(LOGICA) $(TESTES) $(wildcard $(BIBLIOTECA)/src/logica/*.h) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LOGICA) $(TESTES) -o $@

compilar:
	$(CLI) compile --fqbn $(FQBN) $(LIBS) firmware/lembregotchi
	@for t in $(TESTES_PLACA); do echo "== test/placa/$$t"; \
	  $(CLI) compile --fqbn $(FQBN) $(LIBS) test/placa/$$t || exit 1; done

gravar:
	$(CLI) compile --fqbn $(FQBN) $(LIBS) firmware/lembregotchi
	$(CLI) upload -p $(PORTA) --fqbn $(FQBN) firmware/lembregotchi

gravar-teste:
	@test -n "$(T)" || { echo "Use: make gravar-teste T=<$(TESTES_PLACA)>"; exit 1; }
	$(CLI) compile --fqbn $(FQBN) $(LIBS) test/placa/$(T)
	$(CLI) upload -p $(PORTA) --fqbn $(FQBN) test/placa/$(T)

# Imagem do timer do Pomodoro (220×150). Qualquer JPG/PNG serve: sem CORTE, usa o centro.
IMAGEM_PADRAO := firmware/lembregotchi/image/descansa-guerreiro.jpeg
IMG ?= $(IMAGEM_PADRAO)
ifeq ($(IMG),$(IMAGEM_PADRAO))
CORTE ?= 210 0 728 353
endif

imagem:
	python3 tools/imagem_rgb565.py $(IMG) firmware/lembregotchi/src/imagens/foco.h img_foco 220 150 $(CORTE)

# A Arduino IDE só enxerga bibliotecas em ~/Arduino/libraries: cria um link para a do projeto
ide:
	mkdir -p $(HOME)/Arduino/libraries
	ln -sfn $(CURDIR)/$(BIBLIOTECA) $(HOME)/Arduino/libraries/Lembregotchi
	@echo "Biblioteca ligada: ~/Arduino/libraries/Lembregotchi -> $(BIBLIOTECA)"

$(BUILD):
	mkdir -p $@

clean:
	rm -rf build
