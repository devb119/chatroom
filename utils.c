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
#include <time.h>
#include <libgen.h>

#include "utils.h"

int sendPacket(int fd, char* data, int len)
{
    int sent = 0;
    do
    {
        sent += send(fd, data + sent, len - sent, 0);
    } while (sent >= 0 && sent < len);
    return sent;
}

int recvPacket(int fd, char* data, int maxlen)
{
    int received = 0;
    int block_size = 2;
    int tmp = 0;
    do
    {
        tmp = recv(fd, data + received, block_size, 0);
        received += tmp;
    } while (received >= 0 && received < maxlen && tmp == block_size);
    return received;
}

void append(char** pdst, const char* src){
    char *dst = *pdst; // Lay noi dung xau ky tu cu
    int oldLen = (dst == NULL ? 0 : strlen(dst));
    int newLen = oldLen + strlen(src) + 1; //Thua 1 byte \0
    dst = (char*)realloc(dst, newLen);
    // cap phat va memset ve 0 tu phan du lieu moi
    memset(dst + oldLen, 0, strlen(src) + 1);
    sprintf(dst + oldLen, "%s", src);
    *pdst = dst;
}

long int get_current_time(){
    long int seconds;
    seconds = (long int)time(NULL);
    return seconds;
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

char *get_filename(char* filename){
    return strtok(filename, ".");
}

char *create_save_path(char* save_dir, char* filename){
    long int time = get_current_time();
    char *ext = (char*)get_filename_ext(filename);
    char *saved_filename = NULL;
    append(&saved_filename, save_dir);
    append(&saved_filename, strdup(get_filename(filename)));
    append(&saved_filename, "-");
    char str_time[256];
    sprintf(str_time, "%ld", time);
    append(&saved_filename, str_time);
    append(&saved_filename, ".");
    append(&saved_filename, ext);
    printf("%s\n", saved_filename);
    return saved_filename;
}

int checkUsername(char* username, client* clients[], int client_count){
    for(int i = 0; i < client_count; i++){
        if(strcmp(clients[i]->username, username) == 0){
            return 0;
        }
    }
    return 1;
}

int login(char *username, char* password, client* clients[], int client_count){
    for(int i = 0; i < client_count; i++){
        if(strcmp(clients[i]->username, username) == 0 
        && strcmp(clients[i]->password, password) == 0){
            // TRA VE VI TRI HIEN TAI CUA USER TRONG LIST
            return i;
        }
    }
    return -1;
}