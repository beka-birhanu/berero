#ifndef INIT_H
#define INIT_H

#define INIT_SUBCOMMAND "init"

#define INIT_OK 0
#define INIT_ERROR -1
#define INIT_ENTRY_DIR "./.berero"
#define INIT_OBJECT_DIR "./.berero/objects"
#define INIT_BRANCH_DIR "./.berero/branches"
#define INIT_INDEX_FILE "./.berero/index"

int init(int argc, char *argv[]);

// returns 1 if initialized, 0 if not
int initialized();

#endif // INIT_H
