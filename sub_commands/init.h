#ifndef INIT_H
#define INIT_H

#define INIT_SUBCOMMAND "init"

#define SUCCESS 0
#define FAILURE -1
#define ENTRY_DIR "./.berero"
#define OBJECT_DIR "./.berero/objects"
#define BRANCH_DIR "./.berero/branches"
#define INDEX_FILE "./.berero/index"

int init(int argc, char *argv[]);

#endif // INIT_H
