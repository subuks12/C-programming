#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "common.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

static int print_crlf(const char* format, ...) {
    char output[4096];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(output, sizeof(output), format, args);
    va_end(args);
    if (length < 0) {
        return length;
    }

    int written = length < (int)sizeof(output) ? length : (int)sizeof(output) - 1;
    for (int i = 0; i < written; i++) {
        if (output[i] == '\n') {
            fputc('\r', stdout);
        }
        fputc(output[i], stdout);
    }
    return length;
}

#define printf print_crlf
// prints the 7x6 flattened board array in the correct format
void print_board(int board[]) {
    printf("\n");
    for (int col = 0; col < BOARD_COLS; col++)
        printf(" %d", col);
    printf("\n");

    for (int row = 0; row < BOARD_ROWS; row++) {
        printf("|");
        for (int col = 0; col < BOARD_COLS; col++) {
            int cell = board[row * BOARD_COLS + col];
            printf("%c|", cell == 0 ? 'X' : cell == 1 ? 'O' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

static void print_game(const struct game* game) {
    printf("\nCurrent Game\n");
    printf("---------------------\n");
    printf("GameID: %d\n", game->gameId);
    print_board((int*)&game->board.cells[0][0]);
}

static int exchange(int sock_fd, const struct sockaddr_in* server_addr,
    const struct client_request* request, struct server_response* response) {
    for (;;) {
        if (sendto(sock_fd, request, sizeof(*request), 0,
            (const struct sockaddr*)server_addr, sizeof(*server_addr)) < 0) {
            continue;
        }

        ssize_t received = recvfrom(sock_fd, response, sizeof(*response), 0, NULL, NULL);
        if (received == (ssize_t)sizeof(*response) &&
            (response->type == RESPONSE_STATE || response->type == RESPONSE_ERROR)) {
            return response->type == RESPONSE_STATE ? 0 : -1;
        }
    }
}

static void poll_game(int sock_fd, const struct sockaddr_in* server_addr,
    int game_id, int player, struct server_response* state) {
    struct client_request request = {
        .type = REQUEST_POLL,
        .game_id = game_id,
        .player = player,
        .column = 0,
        .move_seq = 0
    };
    while (exchange(sock_fd, server_addr, &request, state) != 0) {
        usleep(100000);
    }
}

static int board_is_terminal(const struct server_response* state, int player) {
    if (state->outcome == 2) {
        printf("TIE\n");
        return 1;
    }
    if (state->outcome == 0 || state->outcome == 1) {
        printf(state->outcome == player ? "YOU WIN!\n" : "YOU LOSE!\n");
        return 1;
    }
    return 0;
}


int main() {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        return 1;
    }

    struct timeval timeout = { .tv_sec = 0, .tv_usec = 200000 };
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);


    printf("Connect 4\n");
    printf("  1) Create a new game\n");
    printf("  2) Join a game\n");
    printf("> ");
    fflush(stdout);
    int choice = 0;
    if (scanf("%d", &choice) != 1) {
        close(sock_fd);
        return 0;
    }

    struct client_request request;
    memset(&request, 0, sizeof(request));
    if (choice == 1) {
        request.type = REQUEST_CREATE;
    } else if (choice == 2) {
        printf("Game ID? >");
        fflush(stdout);
        if (scanf("%d", &request.game_id) != 1) {
            close(sock_fd);
            return 0;
        }
        request.type = REQUEST_JOIN;
    } else {
        close(sock_fd);
        return 0;
    }

    struct server_response state;
    while (exchange(sock_fd, &server_addr, &request, &state) != 0) {
        usleep(100000);
    }
    int game_id = state.game.gameId;
    int player = state.player;
    int seen_revision = state.revision;
    int move_seq = 0;

    print_game(&state.game);
    printf("waiting for other player...\n");
    for (;;) {
        poll_game(sock_fd, &server_addr, game_id, player, &state);
        if ((player == 0 && state.players_joined == 2) ||
            (player == 1 && state.revision > seen_revision)) {
            break;
        }
    }
    seen_revision = state.revision;
    print_game(&state.game);
    if (board_is_terminal(&state, player)) {
        close(sock_fd);
        return 0;
    }

    for (;;) {
        if (state.game.currentPlayer == player) {
            printf("\nInput Move Row: > ");
            fflush(stdout);
            int column;
            if (scanf("%d", &column) != 1) {
                close(sock_fd);
                return 0;
            }

            request.type = REQUEST_MOVE;
            request.game_id = game_id;
            request.player = player;
            request.column = column;
            request.move_seq = ++move_seq;
            while (exchange(sock_fd, &server_addr, &request, &state) != 0) {
                usleep(100000);
            }
            if (state.revision == seen_revision) {
                continue;
            }
            seen_revision = state.revision;
            print_game(&state.game);
            if (board_is_terminal(&state, player)) {
                break;
            }
            printf("waiting for other player...\n");
        }

        while (state.game.currentPlayer != player && state.outcome == -1) {
            poll_game(sock_fd, &server_addr, game_id, player, &state);
            if (state.revision > seen_revision) {
                seen_revision = state.revision;
                print_game(&state.game);
                if (board_is_terminal(&state, player)) {
                    close(sock_fd);
                    return 0;
                }
            }
        }
        if (state.outcome != -1) {
            board_is_terminal(&state, player);
            break;
        }
    }

    close(sock_fd);

    return 0;
}