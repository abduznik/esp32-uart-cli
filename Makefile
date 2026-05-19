# ESP32 CLI Firmware Makefile (Host-Side Testing)

CC = gcc
CFLAGS = -O2 -Wall -Itests/mock_idf -Dapp_main=firmware_app_main

# Gathers all source files and test-suite files automatically
TEST_SRCS = $(wildcard main/*.c) $(wildcard tests/*.c)
TEST_BIN = tests/runner

.PHONY: all test clean

all: test

$(TEST_BIN): $(TEST_SRCS)
	@echo "Compiling safety-aware firmware test runner..."
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $(TEST_BIN)

test: $(TEST_BIN)
	@echo "Executing mock firmware test suite..."
	./$(TEST_BIN)

clean:
	@echo "Cleaning compiled host assets..."
	rm -f $(TEST_BIN)
