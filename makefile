CC = gcc
CFLAGS = -Wall -Wextra -g
SRC_DIR = src
RES_DIR = res
BUILD_DIR = build
TEST_DIR = tests

SRCS = $(wildcard $(SRC_DIR)/ztfs/*.c $(SRC_DIR)/ztfs/operations/*.c)
MAIN_SRC = $(SRC_DIR)/main.c
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TESTS = $(wildcard $(TEST_DIR)/*.c)
TARGETS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRCS))

.PHONY: all clean build tests

all: $(TARGETS)

build: $(MAIN_SRC) $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/ztfs_util $^
	cp -r $(RES_DIR)/* $(BUILD_DIR)/

tests: $(TEST_SRCS) $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/ztfs_util $^

clean:
	rm -rf $(BUILD_DIR)
