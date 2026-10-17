#include <arpa/inet.h>
#include <sys/socket.h>


#ifndef COMMON_H
#define COMMON_H

#define BOARD_ROWS 6
#define BOARD_COLS 7
#define MAX_GAMES 100

struct board {
	int cells[BOARD_ROWS][BOARD_COLS];
};

struct game {
	int gameId;
	struct board board;
	int currentPlayer;
};

struct game_session {
	struct game game;
	struct sockaddr_in player_addr[2];
	int active;
	int players_joined;
	int last_move_seq[2];
	int revision;
	int outcome;
};

struct gameDB {
	int next_game_id;
	struct game_session games[MAX_GAMES];
};

enum request_type {
	REQUEST_CREATE = 1,
	REQUEST_JOIN,
	REQUEST_MOVE,
	REQUEST_POLL
};

enum response_type {
	RESPONSE_STATE = 1,
	RESPONSE_ERROR
};

struct client_request {
	int type;
	int game_id;
	int player;
	int column;
	int move_seq;
};

struct server_response {
	int type;
	int error;
	int player;
	int players_joined;
	int revision;
	int outcome;
	struct game game;
};

#endif