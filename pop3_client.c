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
#include <sys/types.h>

#define INVALID_SOCKET -1

int main(){
    char *domain = "pop.gmail.com";
    struct hostent* presult = gethostbyname(domain);
    if(presult != NULL){
        struct hostent host = *presult;
            int type = host.h_addrtype;
            int count = 0;
            struct in_addr inaddr;
            while(host.h_addr_list[count] != NULL){
                char* addr = host.h_addr_list[count];
                memcpy(&inaddr, addr, sizeof(inaddr));
                char* saddr = inet_ntoa(inaddr);
                printf("%s...\n", saddr);
                count += 1;
            }
            if(count > 0){
                int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if(s != INVALID_SOCKET){
                    struct sockaddr_in addr;
                    addr.sin_family = AF_INET;
                    addr.sin_addr = inaddr;
                    addr.sin_port = htons(995);
                    int xem = sizeof(addr);
                    int error = connect(s, (struct sockaddr*)&addr, sizeof(addr));
                    if(error == 0){
                        char buffer[1024] = { 0 };
                        char *hello = "GET / HTTP/1.1\r\nHost: nginx\r\n\r\n";
                        int sent = send(s, hello, strlen(hello), 0);
                        int r = recv(s, buffer, sizeof(buffer), 0);
                        printf("%d: %s\n", r, buffer);
                    }
                }
            }
    }
}