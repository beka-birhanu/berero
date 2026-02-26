CC      = cc
CFLAGS  = -Wall -Wextra -I/opt/homebrew/opt/openssl@3/include -I/opt/homebrew/opt/zlib/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -L/opt/homebrew/opt/zlib/lib
LDLIBS  = -lcrypto -lz
BUILD   = build

$(BUILD):
	mkdir -p $(BUILD)

run: $(BUILD)
	@$(CC) $(CFLAGS) utiles/chimek.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/chimek
	@./$(BUILD)/chimek .git/objects/c1/c13a4ade3797ca9808d20639dd2d5e12bfef15 compress x.txt

hash: $(BUILD)
	@$(CC) $(CFLAGS) utiles/hash.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/hash
	@./$(BUILD)/hash todo.md

test: $(BUILD)
	@echo "[build] Linking test runner..."
	@$(CC) $(CFLAGS) -I. tests/test.c utiles/hash.c utiles/blob.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/test
	@echo "[run] Executing test suite..."
	@./$(BUILD)/test
