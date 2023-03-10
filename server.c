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
#include <ctype.h>

#include "utils.c"

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

#define MAX_CLIENT 1024
client *clients[MAX_CLIENT];
int g_clientcount = 0;
char *g_path = "./server_file_storage/";

void read_clients_file()
{
    FILE *f = fopen("clients.bin", "rt");
    while (!feof(f))
    {
        char s[1024] = {0};
        fgets(s, sizeof(s), f);
        if (strlen(s) < 2)
        {
            break;
        }
        printf("Read: %s", s);
        int cfd, room;
        char username[20] = {0};
        char password[20] = {0};
        sscanf(s, "%d %s %s %d", &cfd, username, password, &room);
        client *new_client = (client *)calloc(1, sizeof(client));
        new_client->cfd = cfd;
        new_client->username = strdup(username);
        new_client->password = strdup(password);
        new_client->room = room;
        new_client->online = 0;
        // LUU VAO DANH SACH
        clients[g_clientcount++] = new_client;
    }
    fclose(f);
}

void write_clients_file()
{
    FILE *f = fopen("clients.bin", "wt");
    for (int i = 0; i < g_clientcount; i++)
    {
        fprintf(f, "%d %s %s %d\n", clients[i]->cfd, clients[i]->username,
                clients[i]->password, clients[i]->room);
    }
    fclose(f);
}

void send_server_noti(int current_user_index, char *current_user_msg, char *room_msg)
{
    for (int i = 0; i < g_clientcount; i++)
    {
        if (current_user_index == i)
        {
            sendPacket(clients[i]->cfd, current_user_msg, strlen(current_user_msg));
            continue;
        };
        if (clients[i]->room == clients[current_user_index]->room &&
            clients[i]->online)
        {
            sendPacket(clients[i]->cfd, room_msg, strlen(room_msg));
        }
    }
}

int handle_register(int cfd, char *buffer)
{
    char REG[5] = {0};
    char username[20] = {0};
    char password[20] = {0};
    sscanf(buffer, "%s %s %s", REG, username, password);
    if (!checkUsername(username, clients, g_clientcount))
    {
        char *msg = "Username is already used.\n";
        sendPacket(cfd, msg, strlen(msg));
        return -1;
    }
    client *new_client = (client *)calloc(1, sizeof(client));
    new_client->cfd = cfd;
    new_client->username = strdup(username);
    new_client->password = strdup(password);
    new_client->room = 0;
    new_client->online = 1;
    // LUU VAO DANH SACH
    clients[g_clientcount++] = new_client;
    char *msg = "Registered successfully! Room list:\n1\n2\n3\n4\n5\n";
    sendPacket(cfd, msg, strlen(msg));
    write_clients_file();
    // TRA VE VI TRI TRONG DANH SACH
    return g_clientcount - 1;
}

int handle_login(int cfd, char *buffer)
{
    char LOGIN[10] = {0};
    char username[20] = {0};
    char password[20] = {0};
    sscanf(buffer, "%s %s %s", LOGIN, username, password);
    // NEU THANH CONG SE LA VI TRI CUA USER TRONG LIST
    int current_user_index = login(username, password, clients, g_clientcount);
    if (current_user_index == -1)
    {
        char *msg = "Wrong username or password!\n";
        sendPacket(cfd, msg, strlen(msg));
    }
    else
    {
        if (!clients[current_user_index]->room)
        {
            char *msg = "Logged in successfully! You haven't joined any room.\nRoom list:\n1\n2\n3\n4\n5\n";
            sendPacket(cfd, msg, strlen(msg));
        }
        else
        {
            char msg[1024] = {0};
            sprintf(msg, "Logged in successfully! Your current room: %d.\nRoom list:\n1\n2\n3\n4\n5\n", clients[current_user_index]->room);
            sendPacket(cfd, msg, strlen(msg));
            for (int i = 0; i < g_clientcount; i++)
            {
                if (current_user_index == i)
                    continue;
                char msg[1024] = {0};
                sprintf(msg, "%s has reconnected.\n", clients[current_user_index]->username);
                if (clients[i]->room == clients[current_user_index]->room &&
                    clients[i]->online)
                {
                    sendPacket(clients[i]->cfd, msg, strlen(msg));
                }
            }
        }
        // THAY DOI SOCKET CU
        clients[current_user_index]->cfd = cfd;
        clients[current_user_index]->online = 1;
        write_clients_file();
    }
    return current_user_index;
}

void handle_logout(int *current_user_index)
{
    int index = *current_user_index;
    clients[index]->online = 0;
    char room_msg[1024] = {0};
    sprintf(room_msg, "%s has disconnected.\n", clients[index]->username);
    send_server_noti(index, "Logged out successfully\n", room_msg);
    *current_user_index = -1;
}

void handle_register_room(int cfd, char *buffer, int current_user_index)
{
    char ROOM[10] = {0};
    int room = 0;
    sscanf(buffer, "%s %d", ROOM, &room);
    if (room > 5 || room < 1)
    {
        char *msg = "Non-exist room! Room list:\n1\n2\n3\n4\n5\n";
        sendPacket(cfd, msg, strlen(msg));
        return;
    }
    // XU LY CHON TRUNG ROOM
    if (clients[current_user_index]->room == room)
    {
        char msg[1024] = {0};
        sprintf(msg, "You are already in room %d\n", clients[current_user_index]->room);
        sendPacket(cfd, msg, strlen(msg));
        return;
    }
    // THONG BAO CHO CAC THANH VIEN CON LAI TRONG NHOM CHAT
    if (clients[current_user_index]->room)
    {
        char msg[1024] = {0};
        sprintf(msg, "%s has left the chat.\n", clients[current_user_index]->username);
        send_server_noti(current_user_index, "", msg);
    }
    // THONG BAO CHO CAC THANH VIEN RONG NHOM CHAT MOI
    clients[current_user_index]->room = room;
    char current_user_msg[1024] = {0};
    sprintf(current_user_msg, "You have joined room %d.\n", room);
    char room_msg[1024] = {0};
    sprintf(room_msg, "%s has joined the chat.\n", clients[current_user_index]->username);
    send_server_noti(current_user_index, current_user_msg, room_msg);
    write_clients_file();
    return;
}

void handle_leave_room(int current_user_index)
{
    // GUI THONG BAO TOI CAC THANH VIEN TRONG DOAN CHAT
    char msg[1024] = {0};
    sprintf(msg, "%s has left the chat.\n", clients[current_user_index]->username);
    send_server_noti(current_user_index, "Leave room successfully\n", msg);
    // DAT LAI SO PHONG CUA USER
    clients[current_user_index]->room = 0;
    write_clients_file();
}

void handle_recv_file(int cfd, char *buffer, int current_user_index)
{
    char POST[10] = {0};
    char filename[100] = {0};
    int content_length = 0;
    sscanf(buffer, "%s %s %d", POST, filename, &content_length);
    char *data = (char *)calloc(content_length, 1);
    recvPacket(cfd, data, content_length);
    char *saved_filename = create_save_path(g_path, filename);
    FILE *f = fopen(saved_filename, "wb");
    fwrite(data, 1, content_length, f);
    fclose(f);
    char room_msg[1024] = {0};
    sprintf(room_msg, "%s has uploaded a file. Use 'GET %s' to download.\n",
            clients[current_user_index]->username, saved_filename + strlen(g_path));
    send_server_noti(current_user_index, "File uploaded successfully\n", room_msg);
}

void handle_send_file(int cfd, char *buffer, int current_user_index)
{
    char GET[10] = {0};
    char filename[100] = {0};
    sscanf(buffer, "%s %s\n", GET, filename);
    char *path = NULL;
    append(&path, g_path);
    append(&path, filename);
    printf("%s\n", path);
    FILE *f = fopen(path, "rb");
    if (f != NULL)
    {
        fseek(f, 0, SEEK_END);
        int fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *data = (char *)calloc(fsize, 1);
        fread(data, 1, fsize, f);
        char msg[1024] = {0};
        sprintf(msg, "GET %s %d", filename, fsize);

        int sent = 0;
        while (sent < strlen(msg))
        {
            int tmp = send(cfd, msg + sent, strlen(msg) - sent, 0);
            sent += tmp;
        }
        sleep(0.1);

        // printf("%s", mes);

        sent = 0;
        do
        {
            sent += send(cfd, data + sent, fsize - sent, 0);
        } while (sent >= 0 && sent < fsize);
    }
    else
    {
        char *msg = "Cannot find file with that name.";
        sendPacket(cfd, msg, strlen(msg));
    }
}

void send_msg_to_room(int current_user_index, char *buffer)
{
    for (int i = 0; i < g_clientcount; i++)
    {
        if (current_user_index == i)
            continue;
        char *msg = NULL;
        append(&msg, clients[current_user_index]->username);
        append(&msg, ": ");
        append(&msg, buffer);
        if (clients[i]->room == clients[current_user_index]->room &&
            clients[i]->online)
        {
            sendPacket(clients[i]->cfd, msg, strlen(msg));
        }
    }
}

void *handle_client(void *arg)
{
    int cfd = *(int *)arg;
    free(arg);
    // BIEN DUNG DE KIEM TRA TRANG THAI DANG NHAP, = -1 NEU CHUA DANG NHAP DUOC
    int current_user_index = -1;
    while (1)
    {
        char buffer[1024] = {0};
        int r = recv(cfd, buffer, sizeof(buffer), 0);
        if (r > 0)
        {
            printf("Received: %s\n", buffer);
            // CHI CO REG VA LOGIN THUC HIEN DUOC KHI CHUA DANG NHAP
            if (strncmp(buffer, "REG", 3) == 0)
            {
                current_user_index = handle_register(cfd, buffer);
            }
            else if (strncmp(buffer, "LOGIN", 5) == 0)
            {
                current_user_index = handle_login(cfd, buffer);
            }
            else
            {
                if (current_user_index == -1)
                {
                    char *msg = "You have not logged in\n";
                    sendPacket(cfd, msg, strlen(msg));
                    // PHAN XU LY KHI DA LOGIN THANH CONG
                }
                else
                {
                    if (strncmp(buffer, "ROOM", 4) == 0)
                    {
                        handle_register_room(cfd, buffer, current_user_index);
                    }
                    else if (strncmp(buffer, "LEAVE", 5) == 0)
                    {
                        handle_leave_room(current_user_index);
                    }
                    else if (strncmp(buffer, "LOGOUT", 6) == 0)
                    {
                        handle_logout(&current_user_index);
                    }
                    else if (strncmp(buffer, "POST", 4) == 0)
                    {
                        handle_recv_file(cfd, buffer, current_user_index);
                    }
                    else if (strncmp(buffer, "GET", 3) == 0)
                    {
                        handle_send_file(cfd, buffer, current_user_index);
                    }
                    else
                    {
                        // KIEM TRA XEM CO PHONG CHUA
                        if (!clients[current_user_index]->room)
                        {
                            char *msg = "CANNOT chat! You have not registered room, use \"ROOM <number>\" to register.\n";
                            sendPacket(cfd, msg, strlen(msg));
                        }
                        else
                        {
                            send_msg_to_room(current_user_index, buffer);
                        }
                    }
                }
            }
        }
    }
}

void free_clients()
{
    for (int i = 0; i < g_clientcount; i++)
    {
        free(clients[i]->username);
        free(clients[i]->password);
        free(clients[i]);
    }
}

int main()
{
    read_clients_file();
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr, caddr;
    int clen = sizeof(caddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5000);
    saddr.sin_addr.s_addr = 0;
    bind(sfd, (SOCKADDR *)&saddr, sizeof(saddr));
    listen(sfd, 20);
    while (1)
    {
        int cfd = accept(sfd, (SOCKADDR *)&caddr, &clen);
        pthread_t pid;
        int *arg = (int *)calloc(1, sizeof(int));
        *arg = cfd;
        pthread_create(&pid, NULL, handle_client, arg);
    }
    write_clients_file();
    free_clients();
    close(sfd);
}