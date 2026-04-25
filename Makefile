SRC_DIR=src
BUILD_DIR=build

all: bin

bin: $(BUILD_DIR)
	gcc ${SRC_DIR}/3dtictactoe.c -DTUI=0 -o ${BUILD_DIR}/3dtictactoe

tui: $(BUILD_DIR)
	gcc ${SRC_DIR}/3dtictactoe.c -DTUI=1 -o ${BUILD_DIR}/3dtictactoe

run_tui: tui
	./build/3dtictactoe

server: bin
	@test -d .env || python -m venv .env
	.env/bin/activate
	pip install -r requirements.txt

run_server: server
	fastapi run

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf ${BUILD_DIR}
	-deactivate
	rm -rf .env
