#include "game.h"
#include <stdlib.h>
#include <stddef.h>

/*
Takes as input a pointer to a connect4 game board, a player (0/1), and a int column.
Places a new piece in `col` on the game board.
*/
int make_move(struct game* g, int player, int col) {
	if (g == NULL || player < 0 || player > 1 || col < 0 || col >= BOARD_COLS) {
		return -1;
	}

	for (int row = BOARD_ROWS - 1; row >= 0; row--) {
		if (g->board.cells[row][col] == -1) {
			g->board.cells[row][col] = player;
			g->currentPlayer = 1 - player;
			return 0;
		}
	}

	return -1;
}
 
/*
Takes as input a pointer to a connect4 game board.
Returns 0 or 1 if player 0 or 1 has won respectively.
Returns -1 if there is no winner.
*/
int check_win(struct game* g) {
	if (g == NULL) {
		return -1;
	}

	const int directions[4][2] = {
		{0, 1},
		{1, 0},
		{1, 1},
		{1, -1}
	};

	for (int row = 0; row < BOARD_ROWS; row++) {
		for (int col = 0; col < BOARD_COLS; col++) {
			int player = g->board.cells[row][col];
			if (player != 0 && player != 1) {
				continue;
			}

			for (int direction = 0; direction < 4; direction++) {
				int end_row = row + 3 * directions[direction][0];
				int end_col = col + 3 * directions[direction][1];
				if (end_row < 0 || end_row >= BOARD_ROWS ||
					end_col < 0 || end_col >= BOARD_COLS) {
					continue;
				}

				int match = 1;
				for (int offset = 1; offset < 4; offset++) {
					int next_row = row + offset * directions[direction][0];
					int next_col = col + offset * directions[direction][1];
					if (g->board.cells[next_row][next_col] != player) {
						match = 0;
						break;
					}
				}

				if (match) {
					return player;
				}
			}
		}
	}

	return -1;
}
 
/*
Initializes an existing game with an empty board.
*/
void new_game(struct game* g, int gameId) {
	g->gameId = gameId;
	g->currentPlayer = 0;

	for (int row = 0; row < BOARD_ROWS; row++) {
		for (int col = 0; col < BOARD_COLS; col++) {
			g->board.cells[row][col] = -1;
		}
	}
}


