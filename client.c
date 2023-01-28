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
            if (strncmp(buffer, "GET", 3) == 0)
            {
                char filename[1024] = {0};
                int fsize = 0;
                sscanf(buffer, "GET %s %d", filename, &fsize);
                char *data = (char *)calloc(fsize, 1);
                int rcv = 0;
                do
                {
                    n = recv(sfd, buffer, sizeof(buffer) - 1, 0);
                    data = strncpy(data + rcv, buffer, n);
                    rcv += n;
                } while (n >= 0 && rcv < fsize);
                FILE *f = fopen(filename, "wb");
                if (f != NULL)
                {
                    fwrite(data, 1, fsize, f);
                    fclose(f);
                }
                else
                    printf("Can not open file %s\n", filename);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char addr[1024] = {0};
    if (argc == 1)
    {
        strcpy(addr, "127.0.0.1");
    }
    else
    {
        strcpy(addr, argv[1]);
    }
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr, caddr;
    int clen = sizeof(caddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5000);
    saddr.sin_addr.s_addr = inet_addr(addr);
    int result = connect(sfd, (SOCKADDR *)&saddr, sizeof(saddr));
    if (result != 0)
    {
        printf("Can not connect to server\n");
        return -1;
    }
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
        else if (strncmp(mes, "POST", 4) == 0)
        {
            char fileName[100] = {0};
            sscanf(mes, "POST %s\n", fileName);
            // if (fileName[strlen(fileName) - 1] == '\n')
            // {
            //     fileName[strlen(fileName) - 1] = 0;
            // }
            // printf("File name: %s\n", fileName);
            FILE *f = fopen(fileName, "rb");
            if (f != NULL)
            {
                fseek(f, 0, SEEK_END);
                int fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                char *data = (char *)calloc(fsize, 1);
                fread(data, 1, fsize, f);
                sprintf(mes, "POST %s %d", fileName, fsize);
                int sent = 0;
                while (sent < strlen(mes))
                {
                    int tmp = send(sfd, mes + sent, strlen(mes) - sent, 0);
                    sent += tmp;
                }

                // printf("%s", mes);

                sent = 0;
                do
                {
                    sent += send(sfd, data + sent, fsize - sent, 0);
                } while (sent >= 0 && sent < fsize);
            }
            else
            {
                printf("Failed to load file\n");
            }
        }
        else
        {
            int sent = 0;
            while (sent < strlen(mes))
            {
                int tmp = send(sfd, mes + sent, strlen(mes) - sent, 0);
                sent += tmp;
            }
        }
    }
    close(sfd);
}