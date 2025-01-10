#include "common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSZ 500

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <user server port> <location server port> <location code>\n", argv[0]);
    exit(EXIT_FAILURE);
}

int connect_to_server(const char *ip, uint16_t port) {
    struct sockaddr_storage storage;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%u", port);

    if (addrparse(ip, port_str, &storage) != 0) {
        logexit("addrparse");
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)&storage;
    if (connect(s, addr, sizeof(storage)) != 0) {
        logexit("connect");
    }

    return s;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage(argc, argv);
    }

    char *server_ip = argv[1];
    uint16_t user_port = atoi(argv[2]);
    uint16_t location_port = atoi(argv[3]);
    int location_code = atoi(argv[4]);

    if (location_code < 1 || location_code > 10) {
        fprintf(stderr, "Invalid location code. Must be between 1 and 10.\n");
        exit(EXIT_FAILURE);
    }

    int su_socket = connect_to_server(server_ip, user_port);
    int sl_socket = connect_to_server(server_ip, location_port);

    printf("Connected to servers. Location code: %d\n", location_code);

    char command[BUFSZ];
    while (1) {
        printf("Enter command: ");
        memset(command, 0, BUFSZ);
        if (fgets(command, BUFSZ, stdin) == NULL) break;

        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "kill") == 0) {
            sendMessage(su_socket, REQ_DISC, "Client disconnect");
            printf("Disconnected from user server.\n");
            break;
        }

        if (strncmp(command, "add", 3) == 0) {
            sendMessage(su_socket, REQ_USRADD, command + 4);
            printf("DEBUG -> ENVIOU.\n");

            Message message;
            if (receiveMessage(su_socket, &message) > 0) {
                printf("Server response: %s\n", message.payload);
            }
        } else {
            printf("Unknown command.\n");
        }
    }

    close(su_socket);
    close(sl_socket);
    return 0;
}
