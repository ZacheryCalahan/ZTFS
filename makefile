CC = gcc
CFLAGS = -Wall -Wextra -g
SRC_DIR = src
RES_DIR = res
BUILD_DIR = build
SRCS = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/ztfs/*.c)
TARGETS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRCS))

.PHONY: all clean build

all: $(TARGETS)

build: $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/ztfs_util $(SRCS)
	cp -r $(RES_DIR)/* $(BUILD_DIR)/

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)
