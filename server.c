#include "common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


void usage(int argc, char **argv) {
    printf("usage: %s <peer_port> <client_port>\n", argv[0]);
    printf("example: %s 40000 50000\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Função para adicionar usuário
int add_user(const char *uid, int permission) {
    // Verificar se o usuário já existe
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].user_id, uid) == 0) {
            // Atualizar permissão de usuário existente
            users[i].permission = permission;
            return 1;  // Usuário atualizado
        }
    }

    // Verificar limite de usuários
    if (user_count >= MAX_USERS) {
        return -1;  // Limite de usuários atingido
    }

    // Adicionar novo usuário
    strncpy(users[user_count].user_id, uid, 10);
    users[user_count].user_id[10] = '\0';  // Garantir terminador nulo
    users[user_count].permission = permission;
    user_count++;

    return 0;  // Usuário adicionado com sucesso
}

void handleClient(int client_fd) {
    Message message;
    if (receiveMessage(client_fd, &message) <= 0) {
        printf("Client disconnected.\n");
        close(client_fd);
        return;
    }

    printf("DEBUG -> MENSAGEM CHEGOU.\n");

    switch (message.code) {
        case REQ_USRADD: {
            User user;
            if (parseMessageToUser(&user, message.payload)) {
                printf("Adding user: %s, Special: %d\n", user.user_id, user.permission);
                sendMessage(client_fd, OK, "User added successfully.");
            } else {
                sendMessage(client_fd, ERROR, "Invalid user format.");
            }
            break;
        }
        default:
            sendMessage(client_fd, ERROR, "Unknown request.");
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <peer port> <client port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uint16_t client_port = atoi(argv[2]);
    int client_socket = socket(AF_INET6, SOCK_STREAM, 0);

    struct sockaddr_in6 server_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(client_port)
    };

    if (bind(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(client_socket, MAX_CLIENTS) != 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", client_port);

    fd_set read_fds, active_fds;
    FD_ZERO(&active_fds);
    FD_SET(client_socket, &active_fds);

    while (1) {
        read_fds = active_fds;
        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            break;
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == client_socket) {
                    int client_fd = accept(client_socket, NULL, NULL);
                    if (client_fd >= 0) {
                        FD_SET(client_fd, &active_fds);
                        printf("New client connected.\n");
                    }
                } else {
                    handleClient(i);
                }
            }
        }
    }

    printf("saiu do while.\n");

    close(client_socket);
    return 0;
}