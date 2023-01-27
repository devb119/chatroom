#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "utils.c"

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

void *listen_from_server(void *arg)
{
    int sfd = *(int *)arg;
    free(arg);
    while (0 == 0)
    {
        char buffer[1024] = {0};
        int n = recv(sfd, buffer, sizeof(buffer) - 1, 0);
        if (n != 0)
        {
            printf("%s", buffer);
        }
    }
}

int main()
{
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr, caddr;
    int clen = sizeof(caddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5000);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(sfd, (SOCKADDR *)&saddr, sizeof(saddr));
    pthread_t pid;
    int *arg = (int *)calloc(1, sizeof(int));
    *arg = sfd;
    pthread_create(&pid, NULL, listen_from_server, arg);
    // bind(sfd, (SOCKADDR *)&saddr, sizeof(saddr));
    // listen(sfd, 20);
    while (1)
    {
        char mes[1024] = {0};
        fgets(mes, 1024, stdin);
        if (strncmp(mes, "END", 3) == 0)
        {
            break;
        }
        int sent = 0;
        while (sent < strlen(mes))
        {
            int tmp = send(sfd, mes + sent, strlen(mes) - sent, 0);
            sent += tmp;
        }
    }
    close(sfd);
}