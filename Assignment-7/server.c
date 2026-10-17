#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "game.h"
#include "save.h"

#define PORT 8080
#define SAVE_PATH ".connect4.save"

static int same_address(const struct sockaddr_in* left,
    const struct sockaddr_in* right) {
    return left->sin_family == right->sin_family &&
        left->sin_port == right->sin_port &&
        left->sin_addr.s_addr == right->sin_addr.s_addr;
}

static int find_game(struct gameDB* db, int game_id) {
    if (game_id < 0 || game_id >= db->next_game_id || game_id >= MAX_GAMES ||
        !db->games[game_id].active) {
        return -1;
    }
    return game_id;
}

static void set_state_response(struct server_response* response,
    const struct game_session* session, int player) {
    memset(response, 0, sizeof(*response));
    response->type = RESPONSE_STATE;
    response->player = player;
    response->players_joined = session->players_joined;
    response->revision = session->revision;
    response->outcome = session->outcome;
    response->game = session->game;
}

static int board_is_full(const struct game* game) {
    for (int row = 0; row < BOARD_ROWS; row++) {
        for (int col = 0; col < BOARD_COLS; col++) {
            if (game->board.cells[row][col] == -1) {
                return 0;
            }
        }
    }
    return 1;
}

static int send_response(int sock_fd, const struct server_response* response,
    const struct sockaddr_in* client_addr, socklen_t addr_len) {
    return sendto(sock_fd, response, sizeof(*response), 0,
        (const struct sockaddr*)client_addr, addr_len) == sizeof(*response) ? 0 : -1;
}


int main(int argc, char* argv[]) {
    struct gameDB games;
    memset(&games, 0, sizeof(games));
    if (argc > 1 && atoi(argv[1]) != 0) {
        if (loadgame(&games, SAVE_PATH) != 0) {
            memset(&games, 0, sizeof(games));
        }
    } else {
        unlink(SAVE_PATH);
    }

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    int reuse = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return 1;
    }

    printf("Message board server listening on port %d\n", PORT);

    for (;;) {
        struct client_request request;
        socklen_t client_len = sizeof(client_addr);
        ssize_t received = recvfrom(sock_fd, &request, sizeof(request), 0,
            (struct sockaddr*)&client_addr, &client_len);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            continue;
        }
        if ((size_t)received != sizeof(request)) {
            continue;
        }

        struct server_response response;
        memset(&response, 0, sizeof(response));
        response.type = RESPONSE_ERROR;
        response.error = 1;
        response.player = request.player;
        int game_id = request.game_id;

        if (request.type == REQUEST_CREATE) {
            int existing_game = -1;
            int existing_player = -1;
            for (int id = 0; id < games.next_game_id; id++) {
                struct game_session* session = &games.games[id];
                for (int player = 0; player < session->players_joined; player++) {
                    if (same_address(&session->player_addr[player], &client_addr)) {
                        existing_game = id;
                        existing_player = player;
                        break;
                    }
                }
                if (existing_game >= 0) {
                    break;
                }
            }

            if (existing_game >= 0) {
                game_id = existing_game;
                set_state_response(&response, &games.games[game_id], existing_player);
            } else if (games.next_game_id < MAX_GAMES) {
                game_id = games.next_game_id++;
                struct game_session* session = &games.games[game_id];
                memset(session, 0, sizeof(*session));
                session->active = 1;
                session->players_joined = 1;
            session->player_addr[0] = client_addr;
            session->outcome = -1;
                new_game(&session->game, game_id);
                set_state_response(&response, session, 0);
                savegame(&games, SAVE_PATH);
            }
        } else {
            int found = find_game(&games, game_id);
            if (found >= 0) {
                struct game_session* session = &games.games[found];
                if (request.type == REQUEST_JOIN) {
                    int player = -1;
                    for (int i = 0; i < session->players_joined; i++) {
                        if (same_address(&session->player_addr[i], &client_addr)) {
                            player = i;
                            break;
                        }
                    }
                    if (player < 0 && session->players_joined == 1) {
                        player = 1;
                        session->player_addr[player] = client_addr;
                        session->players_joined = 2;
                        savegame(&games, SAVE_PATH);
                    }
                    if (player >= 0) {
                        set_state_response(&response, session, player);
                    }
                } else if (request.type == REQUEST_POLL) {
                    if (request.player >= 0 && request.player < 2) {
                        set_state_response(&response, session, request.player);
                    }
                } else if (request.type == REQUEST_MOVE) {
                    int player = request.player;
                    if (player >= 0 && player < 2 &&
                        same_address(&session->player_addr[player], &client_addr)) {
                        if (request.move_seq <= session->last_move_seq[player]) {
                            set_state_response(&response, session, player);
                        } else if (session->outcome == -1 &&
                            session->game.currentPlayer == player &&
                            make_move(&session->game, player, request.column) == 0) {
                            session->last_move_seq[player] = request.move_seq;
                            session->revision++;
                            int winner = check_win(&session->game);
                            if (winner >= 0) {
                                session->outcome = winner;
                            } else if (board_is_full(&session->game)) {
                                session->outcome = 2;
                            }
                            savegame(&games, SAVE_PATH);
                            set_state_response(&response, session, player);
                        }
                    }
                }
            }
        }

        send_response(sock_fd, &response, &client_addr, client_len);
    }

    close(sock_fd);

    return 0;
}