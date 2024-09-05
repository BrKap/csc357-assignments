typedef struct {
    int verbosity;
    int accept_all;
    int no_windowing;
    char *hostname;
    int port;
    int server_socket;
    int client_socket;
} MyTalkConfig;

void parse_args(int argc, char *argv[], MyTalkConfig *config);
void setup_server(MyTalkConfig *config);
void setup_client(MyTalkConfig *config);
void handle_server_connection(MyTalkConfig *config);
void handle_client_connection(MyTalkConfig *config);
void start_chat(MyTalkConfig *config);
void setup_signal_handler(void (*handler)(int));
void handle_sigint(int sig);