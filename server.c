#include "common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PEERS 2

void usage(int argc, char **argv) {
    printf("usage: %s <peer_port> <client_port>\n", argv[0]);
    printf("example: %s 40000 50000\n", argv[0]);
    exit(EXIT_FAILURE);
}

int add_user(const char *uid, int permission) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].user_id, uid) == 0) {
            users[i].permission = permission;
            return 1; // Usuário atualizado
        }
    }

    if (user_count >= MAX_USERS) {
        return -1; // Limite de usuários atingido
    }

    strncpy(users[user_count].user_id, uid, 10);
    users[user_count].user_id[10] = '\0';
    users[user_count].permission = permission;
    user_count++;

    return 0; // Usuário adicionado com sucesso
}

void handleClient(int client_fd) {
    Message message;
    if (receiveMessage(client_fd, &message) <= 0) {
        printf("Client disconnected.\n");
        close(client_fd);
        return;
    }

    switch (message.code) {
        case REQ_USRADD: {
            User user;
            if (parseMessageToUser(&user, message.payload)) {
                printf("Adding user: %s, Special: %d\n", user.user_id, user.permission);
                int result = add_user(user.user_id, user.permission);
                if (result == -1) {
                    sendMessage(client_fd, ERROR, "User limit reached.");
                } else if (result == 1) {
                    sendMessage(client_fd, OK, "User updated successfully.");
                } else {
                    sendMessage(client_fd, OK, "User added successfully.");
                }
            } else {
                sendMessage(client_fd, ERROR, "Invalid user format.");
            }
            break;
        }
        default:
            sendMessage(client_fd, ERROR, "Unknown client request.");
            break;
    }
}

void handlePeerCommunication(int peer_fd, int *peer_connected) {
    Message message;
    if (receiveMessage(peer_fd, &message) <= 0) {
        printf("Peer disconnected.\n");
        close(peer_fd);
        *peer_connected = 0;
        return;
    }

    switch (message.code) {
        case REQ_CONNPEER:
            printf("Peer connected: %d\n", peer_fd);
            sendMessage(peer_fd, RES_CONNPEER, "Connection established.");
            break;
        case REQ_DISCPEER:
            printf("Peer disconnection request received.\n");
            sendMessage(peer_fd, OK, "Peer disconnected.");
            close(peer_fd);
            *peer_connected = 0;
            break;
        default:
            printf("Unknown peer message received: %d\n", message.code);
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argc, argv);
    }

    uint16_t peer_port = atoi(argv[1]);
    uint16_t client_port = atoi(argv[2]);

    int client_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Failed to create client socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in6 client_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(client_port)
    };

    if (bind(client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) != 0) {
        perror("Bind failed for client socket");
        exit(EXIT_FAILURE);
    }

    if (listen(client_socket, MAX_CLIENTS) != 0) {
        perror("Listen failed for client socket");
        exit(EXIT_FAILURE);
    }

    int peer_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (peer_socket < 0) {
        perror("Failed to create peer socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in6 peer_addr = {
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(peer_port)
    };

    if (connect(peer_socket, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) != 0) {
        printf("No peer found, starting to listen...\n");
        if (bind(peer_socket, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) != 0) {
            perror("Bind failed for peer socket");
            exit(EXIT_FAILURE);
        }

        if (listen(peer_socket, MAX_PEERS) != 0) {
            perror("Listen failed for peer socket");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("New Peer ID: %d\n", getpid());
    }

    fd_set read_fds, active_fds;
    FD_ZERO(&active_fds);
    FD_SET(client_socket, &active_fds);
    FD_SET(peer_socket, &active_fds);

    int peer_connected = 0;

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
                } else if (i == peer_socket) {
                    int new_peer_fd = accept(peer_socket, NULL, NULL);
                    if (new_peer_fd >= 0) {
                        if (peer_connected >= MAX_PEERS) {
                            printf("Peer limit exceeded\n");
                            sendMessage(new_peer_fd, ERROR, "Peer limit exceeded");
                            close(new_peer_fd);
                        } else {
                            printf("Peer connected.\n");
                            FD_SET(new_peer_fd, &active_fds);
                            peer_connected++;
                        }
                    }
                } else {
                    if (peer_connected && i == peer_socket) {
                        handlePeerCommunication(i, &peer_connected);
                    } else {
                        handleClient(i);
                    }
                }
            }
        }
    }

    close(peer_socket);
    close(client_socket);
    return 0;
}
