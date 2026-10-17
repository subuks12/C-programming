#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"

#define MAX_GAMES 100  /* must match the malloc in server.c main() */

int savegame(struct gameDB* game, char* filepath);
int loadgame(struct gameDB* game, char* filepath);