typedef struct client_t {
    int cfd;
    char* username;
    char* password;
    int room;
    int online;
}client;

int sendPacket(int fd, char* data, int len);
int recvPacket(int fd, char* data, int maxlen);
void append(char** pdst, const char* src);

int checkUsername(char* username, client* clients[], int client_count);
int login(char *username, char* password, client* clients[], int client_count);