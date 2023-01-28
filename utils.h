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
long int get_current_time();
const char *get_filename_ext(const char *filename);
char *get_filename(char* path);
char *create_save_path(char *save_dir, char *filename);

int checkUsername(char* username, client* clients[], int client_count);
int login(char *username, char* password, client* clients[], int client_count);