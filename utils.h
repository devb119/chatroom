typedef struct client_t {
    int cfd;
    char* username;
    char* password;
    int room;
}client;

int sendPacket(int fd, char* data, int len);
int recvPacket(int fd, char* data, int maxlen);

int checkUsername(char* username, client clients[], int client_count);