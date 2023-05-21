#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // for inet_addr
#include <arpa/inet.h> // for sockets
#include <unistd.h> // just for the close() function
#include <pthread.h>
#define BUF_SIZE 256
#define EXIT 'x'
#define BEGIN 'b'

// Reference: https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c

struct socket_wrapper {
    int port;
    int desc;
    struct sockaddr_in socket;
};

void init_receiver(struct socket_wrapper* receiver) {
    // Create socket:
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    receiver->socket.sin_family = AF_INET;
    receiver->socket.sin_port = receiver->port;
    receiver->socket.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&(receiver->socket), sizeof((receiver->socket)))<0){
        printf("Couldn't bind to the port\n");
        return;
    }
    printf("Done with binding: address %d\n", 
           receiver->socket.sin_port);
    
    // Listen for clients:
    if(listen(socket_desc, 1) < 0){
        printf("Error while listening\n");
        return;
    }
    printf("\nListening socket created\n");
    receiver->desc = socket_desc;
}

void accept_client(int receiver_desc, struct socket_wrapper* client) {
    int client_size;
    int client_sock;
    // Accept an incoming connection:
    client_size = sizeof(client->socket);
    client_sock = accept(receiver_desc, (struct sockaddr*)&(client->socket), &client_size);
    printf("Accepted client\n");
    
    if (client_sock < 0){
        printf("Can't accept\n");
        return;
    }
    printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client->socket.sin_addr), ntohs(client->socket.sin_port));
    client->desc = client_sock;
}

void connect_to_client(struct socket_wrapper* sender) {
    int socket_desc;
    // Create socket:
    printf("Creating socket\n");
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Unable to create socket\n");
        return;
    }
    
    printf("Socket created successfully\n");
    
    // Set port and IP the same as server-side:
    sender->socket.sin_family = AF_INET;
    sender->socket.sin_port = sender->port;
    sender->socket.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    printf("Connecting to client at %d\n", sender->socket.sin_port);
    // Send connection request to server:
    while(connect(socket_desc, (struct sockaddr*)&(sender->socket), sizeof(sender->socket)) < 0){
    }
    fprintf(stderr, "Connected with server successfully\n");
    sender->desc = socket_desc;
}

void clean_buffer(char* cli_msg) {
    memset(cli_msg, '\0', sizeof(cli_msg));
}

// MESSAGE types must be defined in the file that imports this library.
struct MESSAGE;
char* SERIALIZE_MESSAGE(struct MESSAGE* m);
void DESERIALIZE_MESSAGE(struct MESSAGE* m, char* msg);

struct network {
    pthread_t rec;
    pthread_t send;
    int connected;
    int accepted;
    struct socket_wrapper sender;
    struct socket_wrapper receiver;
    struct socket_wrapper client;
    char last_msg[BUF_SIZE];
    struct network* self;
};


int handler(char* msg, struct network* nw) {
    fprintf(stderr, "Msg from client: %s\n", msg);
    strcpy(nw->last_msg, msg);
    return (msg[0] == EXIT);
}

int handle_msgs(struct network* nw, int (*handler)(char*, struct network*)) {
    char client_message[BUF_SIZE];
    int exit = 0;
    fprintf(stderr, "Handling messages...\n");
    while (exit == 0) {
        clean_buffer(client_message);
        // Receive client's message:
        if (recv(nw->client.desc, client_message, sizeof(client_message), 0) < 0){
            fprintf(stderr, "Couldn't receive\n");
            return -1;
        }
        exit = (*handler)(client_message, nw);
    }
    fprintf(stderr, "Connected host has closed connection. Closing listener thread\n");
    return 0;
}

void * rec_thread(void *args) {
//    struct socket_wrapper* wrappers = (struct socket_wrapper*)args;
    struct network* nw = (struct network*)args;
    init_receiver(&(nw->receiver));
    accept_client(nw->receiver.desc, &(nw->client));
    nw->accepted = 1;
    handle_msgs(nw, handler);
}

void * send_thread(void *args) {
    struct network* nw = (struct network*)args;
    connect_to_client(&(nw->sender));
    nw->connected = 1;
//    send_msgs(sender->desc);
}

struct network new_network(int rec_port, int send_port) {
    struct network nw;
    if (rec_port == send_port) {
        fprintf(stderr, "Receive and Send ports must not have the same value (here set to %d)\n",
              rec_port);
        exit(0);
    }
    if (rec_port == 0 || send_port == 0) {
        fprintf(stderr, "Receive or Send port must not be set to 0\n");
        exit(0);
    }
    nw.receiver.port = rec_port;
    nw.sender.port = send_port;
    nw.self = &nw;
    sprintf(nw.last_msg, "%c", EXIT);
    nw.connected = 0;
    nw.accepted = 0;
    return nw;
}

void network_start(struct network* nw) {
    pthread_create(&(nw->rec), NULL, rec_thread, nw);
    pthread_create(&(nw->send), NULL, send_thread, nw);
}

void network_end(struct network* nw) {
    while(nw->connected == 0);
    while(nw->accepted == 0);
    char exit[2];
    sprintf(exit, "%c", EXIT);
    send(nw->sender.desc, exit, strlen(exit), 0);
//    printf("exit signal sent\n");
//    pthread_join(nw->rec, NULL);
//    printf("closed listener\n");
//    pthread_join(nw->send, NULL);
    
    // Closing the sockets:
    close(nw->receiver.desc);
    close(nw->sender.desc);
}

int network_send(struct network* nw, struct MESSAGE* m) {
    while(nw->connected == 0);
    char send_message[BUF_SIZE];
    strcpy(send_message, SERIALIZE_MESSAGE(m));
//    sprintf(send_message, "%s\n", send_message);
    fprintf(stderr, "sending message: %s\n", send_message);
    return send(nw->sender.desc, send_message, strlen(send_message), 0);
}

struct MESSAGE* network_get(struct network* nw) {
    if (nw->last_msg[0] == BEGIN) {
        return NULL;
    }
    struct MESSAGE* m;
    DESERIALIZE_MESSAGE(m, nw->last_msg);
    return m;
}
