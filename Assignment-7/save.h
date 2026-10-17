#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"

int savegame(struct gameDB* game, char* filepath);
int loadgame(struct gameDB* game, char* filepath);