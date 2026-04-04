#ifndef COMMIT_H
#define COMMIT_H

#include "init.h"

#define COMMIT_OK 0
#define COMMIT_ERROR -1
#define COMMIT_SUBCOMMAND "commit"
#define COMMIT_HEAD_FILE INIT_ENTRY_DIR "/head"

int commit(int argc, char *argv[]);

#endif // !COMMIT_H
