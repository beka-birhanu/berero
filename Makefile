CC      = cc
CFLAGS  = -Wall -Wextra -I. \
          -I/opt/homebrew/opt/openssl@3/include -I/opt/homebrew/opt/zlib/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -L/opt/homebrew/opt/zlib/lib
LDLIBS  = -lcrypto -lz
BUILD   = build

SRC     := $(filter-out tests/% utiles/dir.c,$(wildcard *.c sub_commands/*.c utiles/*.c))
OBJ     := $(addprefix $(BUILD)/,$(SRC:.c=.o))


$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/berero: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

run: force $(BUILD)/berero
	@./$(BUILD)/berero $(args)

hash: $(BUILD)
	@$(CC) $(CFLAGS) utiles/hash.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/hash
	@./$(BUILD)/hash todo.md

test: $(BUILD)
	@echo "[build] Linking test runner..."
	@$(CC) $(CFLAGS) -I. tests/test.c utiles/hash.c utiles/blob.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/test
	@echo "[run] Executing test suite..."
	@./$(BUILD)/test

test-blob: $(BUILD)
	@echo "[build] Linking blob test..."
	@$(CC) $(CFLAGS) -I. tests/blob_test.c utiles/blob.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/blob_test
	@echo "[run] Executing blob tests..."
	@./$(BUILD)/blob_test

clean:
	rm -rf $(BUILD)

force:

.PHONY: run hash test test-blob clean
