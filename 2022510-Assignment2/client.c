#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Client connection function to communicate with the server
void *client_connection(void *arg) {
    int client_socket;
    struct sockaddr_in serv_addr;
    char *request = "Requesting CPU process info";
    char buffer[BUFFER_SIZE] = {0};

    // Create a socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Client: Socket creation error\n");
        return NULL;
    }

    // Set up server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 or IPv6 address to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Client: Invalid address / Address not supported\n");
        return NULL;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Client: Connection failed\n");
        return NULL;
    }

    printf("Client: Connected to server\n");

    // Send request to the server
    send(client_socket, request, strlen(request), 0);
    printf("Client: Request sent to server\n");

    // Read response from the server
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        printf("Client: Server response:\n%s\n", buffer);
    }

    // Close the socket
    close(client_socket);
    return NULL;
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_clients>\n", argv[0]);
        return -1;
    }

    int num_clients = atoi(argv[1]); // Convert the argument to an integer

    pthread_t thread_id[num_clients]; // Array of threads

    // Create multiple client threads based on the number of clients passed
    for (int i = 0; i < num_clients; i++) {
        // sleep(4);
        if (pthread_create(&thread_id[i], NULL, client_connection, NULL) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    // Wait for all client threads to finish
    for (int i = 0; i < num_clients; i++) {
        pthread_join(thread_id[i], NULL);
    }

    return 0;
}
