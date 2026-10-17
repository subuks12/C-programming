#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"


/*
Saves server game data to specified filepath.
Returns 0 on success, -1 on failure.
*/
int savegame(struct gameDB* game, char* filepath) {
	if (game == NULL || filepath == NULL) {
		return -1;
	}

	int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0) {
		return -1;
	}

	const char* data = (const char*)game;
	size_t remaining = sizeof(*game);
	while (remaining > 0) {
		ssize_t written = write(fd, data, remaining);
		if (written < 0) {
			if (errno == EINTR) {
				continue;
			}
			close(fd);
			return -1;
		}
		data += written;
		remaining -= (size_t)written;
	}

	int result = fsync(fd);
	if (close(fd) < 0) {
		result = -1;
	}
	return result;
}

/*
Loads a file written by savegame into game.
Returns 0 on success, -1 on failure.
*/
int loadgame(struct gameDB* game, char* filepath) {
	if (game == NULL || filepath == NULL) {
		return -1;
	}

	int fd = open(filepath, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	struct gameDB loaded;
	char* data = (char*)&loaded;
	size_t remaining = sizeof(loaded);
	while (remaining > 0) {
		ssize_t count = read(fd, data, remaining);
		if (count < 0) {
			if (errno == EINTR) {
				continue;
			}
			close(fd);
			return -1;
		}
		if (count == 0) {
			close(fd);
			return -1;
		}
		data += count;
		remaining -= (size_t)count;
	}

	char extra;
	ssize_t count;
	do {
		count = read(fd, &extra, sizeof(extra));
	} while (count < 0 && errno == EINTR);
	int close_result = close(fd);
	if (count != 0 || close_result < 0 ||
		loaded.next_game_id < 0 || loaded.next_game_id > MAX_GAMES) {
		return -1;
	}

	*game = loaded;
	return 0;
}