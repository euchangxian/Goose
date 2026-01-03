.PHONY: all configure build test clean

BUILD_DIR = build

all: build

init:
	cmake -G Ninja -B $(BUILD_DIR)

build:
	cmake --build $(BUILD_DIR) -j8

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf $(BUILD_DIR)

