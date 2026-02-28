CC      = cc
CFLAGS  = -Wall -Wextra -I. \
          -I/opt/homebrew/opt/openssl@3/include -I/opt/homebrew/opt/zlib/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -L/opt/homebrew/opt/zlib/lib
LDLIBS  = -lcrypto -lz
BUILD   = build

SRC     := $(filter-out tests/%,$(wildcard *.c sub_commands/*.c utiles/*.c))
OBJ     := $(addprefix $(BUILD)/,$(SRC:.c=.o))


$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/berero: $(OBJ)
	@$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

run: force $(BUILD)/berero
	@./$(BUILD)/berero $(args)

hash: $(BUILD)
	@$(CC) $(CFLAGS) utiles/hash.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/hash
	@./$(BUILD)/hash todo.md

test: test-hash test-blob test-index test-hash_table test-linked_list test-walker
	@echo "[run] All tests passed."

test-hash: $(BUILD)
	@$(CC) $(CFLAGS) -I. tests/hash.c utiles/hash.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/hash_test
	@./$(BUILD)/hash_test

test-blob: $(BUILD)
	@$(CC) $(CFLAGS) -I. tests/blob.c utiles/blob.c $(LDFLAGS) $(LDLIBS) -o $(BUILD)/blob_test
	@./$(BUILD)/blob_test

test-index: $(BUILD)
	@$(CC) $(CFLAGS) -I. tests/index.c utiles/index.c utiles/hash_table.c utiles/linked_list.c -o $(BUILD)/index_test
	@./$(BUILD)/index_test

test-hash_table: $(BUILD)
	@$(CC) $(CFLAGS) -I. tests/hash_table.c utiles/hash_table.c utiles/linked_list.c -o $(BUILD)/hash_table_test
	@./$(BUILD)/hash_table_test

test-linked_list: $(BUILD)
	@$(CC) $(CFLAGS) -I. tests/linked_list.c utiles/linked_list.c -o $(BUILD)/linked_list_test
	@./$(BUILD)/linked_list_test

test-walker: $(BUILD)
	@$(CC) $(CFLAGS) -I. tests/walker.c utiles/walker.c utiles/linked_list.c -o $(BUILD)/walker_test
	@./$(BUILD)/walker_test

clean:
	rm -rf $(BUILD)

force:

.PHONY: run hash test test-hash test-blob test-index test-hash_table test-linked_list test-walker clean
