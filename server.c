#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <malloc.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "utils.h"

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

#define MAX_CLIENT 1024
client* clients[MAX_CLIENT];
int g_clientcount = 0;

void handle_register(int cfd, char* buffer){
    char REG[5] = { 0 };
    char username[20] = { 0 };
    char password[20] = { 0 };
    sscanf(buffer, "%s %s %s", REG, username, password);
    if(!checkUsername(username, clients, g_clientcount)){
        char* msg = "Username is already used.";
        sendPacket(cfd, msg, strlen(msg));
        return;
    }
    client *new_client = (client*)calloc(1, sizeof(client));
    new_client->cfd = cfd;
    new_client->username = strdup(username);
    new_client->password = strdup(password);
    // LUU VAO DANH SACH
    clients[g_clientcount++] = new_client;
}

void* handle_client(void* arg){
    int cfd = *(int*)arg;
    free(arg);
    while(1){
        char buffer[1024] = { 0 };
        int r = recv(cfd, buffer, sizeof(buffer), 0);
        if(r > 0){
            printf("Received: %s\n", buffer);\
            if(strncmp(buffer, "REG", 3) == 0){
                handle_register(cfd, buffer);
            }
        }
    }
}

int main(){
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr, caddr;
    int clen = sizeof(caddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5000);
    saddr.sin_addr.s_addr = 0;
    bind(sfd, (SOCKADDR*)&saddr, sizeof(saddr));
    listen(sfd, 10);
    while(1){
        int cfd = accept(sfd, (SOCKADDR*)&caddr, &clen);
        pthread_t pid;
        int* arg = (int*)calloc(1, sizeof(int));
        *arg = cfd;
        pthread_create(&pid, NULL, handle_client, arg);
    }
    close(sfd);
}