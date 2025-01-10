#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

#define REQ_CONNPEER 17
#define RES_CONNPEER 18
#define REQ_DISCPEER 19
#define REQ_CONN 20
#define RES_CONN 21
#define REQ_DISC 22

#define REQ_USRADD 33
#define REQ_USRACCESS 34
#define RES_USRACCESS 35
#define REQ_LOCREG 36
#define RES_LOCREG 37
#define REQ_USRLOC 38
#define RES_USRLOC 39
#define REQ_LOCLIST 40
#define RES_LOCLIST 41
#define REQ_USRAUTH 42
#define RES_USRAUTH 43

#define BUFSZ 500
#define MAX_USERS 30
#define MAX_CLIENTS 10
#define MAX_LOCATIONS 10

#define ERROR 255
#define OK 0

// Estruturas para armazenar dados
typedef struct {
    char user_id[11];
    int permission;
} User;

typedef struct {
    char user_id[11];
    int location;
} Location;

typedef struct {
    uint8_t code;        
    char payload[BUFSZ]; 
} Message;

extern User users[MAX_USERS];
extern int user_count;

extern Location locations[MAX_LOCATIONS];
extern int location_count;


void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);

void sendMessage(int socket, uint8_t code, const char *payload);

int receiveMessage(int socket, Message *message);

int parseMessageToUser(User *user, const char *payload);
