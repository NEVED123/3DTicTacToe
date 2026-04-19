SRC_DIR=src
BUILD_DIR=build

all: $(BUILD_DIR)
	gcc ${SRC_DIR}/3dtictactoe.c -o ${BUILD_DIR}/3dtictactoe

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf ${BUILD_DIR}

run: all
	./build/3dtictactoe