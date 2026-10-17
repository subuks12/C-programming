#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "game.h"
#include "save.h"

#define PORT 8080


int main(int argc, char* argv[]) {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Message board server listening on port %d\n", PORT);

    //TODO: implement game server logic

    close(sock_fd);

    return 0;
}