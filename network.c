#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#define BUF_SIZE 256
#define EXIT 'x'

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
    printf("Connected with server successfully\n");
    sender->desc = socket_desc;
}

void clean_buffer(char* cli_msg) {
    memset(cli_msg, '\0', sizeof(cli_msg));
}

int handler(char* msg) {
    printf("Msg from client: %s\n", msg);
    return (msg[0] == EXIT);
}

int handle_msgs(int client_desc, int (*handler)(char*)) {
    char client_message[BUF_SIZE];
//    char exit_message = EXIT;
    int exit = 0;
    printf("Handling messages...\n");
    while (exit == 0) {
        clean_buffer(client_message);
        // Receive client's message:
        if (recv(client_desc, client_message, sizeof(client_message), 0) < 0){
            printf("Couldn't receive\n");
            return -1;
        }
        exit = (*handler)(client_message);
    }
    printf("Connected host has closed connection. Press \"x\" to exit.\n");
}

int send_msgs(int send_client_desc) {
    char send_message[BUF_SIZE];
    int exit = 0;
    printf("sending messages...\n");
    while (1) {
        clean_buffer(send_message);
        // Get input from the user:
        printf("Enter message: ");
        // Send the message to server:
        fgets(send_message, BUF_SIZE, stdin);
        send(send_client_desc, send_message, strlen(send_message), 0);
        if (send_message[0] == EXIT) {
            printf("Exiting...\n");
            return 0;
        }
    }
}

void * rec_thread(void *args) {
    struct socket_wrapper* wrappers = (struct socket_wrapper*)args;
    init_receiver(&(wrappers[0]));
    accept_client(wrappers[0].desc, &(wrappers[1]));
    handle_msgs(wrappers[1].desc, handler);
    free(wrappers);
}

void * send_thread(void *args) {
    struct socket_wrapper* sender = (struct socket_wrapper*)args;
    connect_to_client(sender);
    send_msgs(sender->desc);
}

struct network {
    pthread_t rec;
    pthread_t send;
    struct socket_wrapper sender;
    struct socket_wrapper receiver;
    struct socket_wrapper client;
};

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
    return nw;
}

void network_start(struct network* nw) {
    struct socket_wrapper receiver = nw->receiver;
    struct socket_wrapper client = nw->client;
    struct socket_wrapper* wrappers = malloc(sizeof(struct socket_wrapper)*2);
    wrappers[0] = receiver;
    wrappers[1] = client;
    pthread_create(&(nw->rec), NULL, rec_thread, wrappers);
    pthread_create(&(nw->send), NULL, send_thread, &(nw->sender));
}

void network_end(struct network* nw) {
    pthread_join(nw->rec, NULL);
    pthread_join(nw->send, NULL);
    
    // Closing the sockets:
    close(nw->receiver.desc);
    close(nw->sender.desc);
}

// MESSAGE types must be defined in the file that imports this library.
struct MESSAGE;

int network_send(struct network* nw) {
    return 0;
}

struct MESSAGE* network_get(struct network* nw) {
    struct MESSAGE* m;
    return m;
}
