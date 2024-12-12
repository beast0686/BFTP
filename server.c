#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 5000
#define BUFFER_SIZE 1024

// Function to create received files directory
void create_received_files_dir() {
    mkdir("received_files", 0777);
}

// Function to receive file
void receive_file(int client_socket) {
    char buffer[BUFFER_SIZE];
    char filename[256];
    long filesize;

    // Receive file metadata
    memset(buffer, 0, sizeof(buffer));
    if (recv(client_socket, buffer, BUFFER_SIZE, 0) <= 0) {
        perror("Error receiving file metadata");
        return;
    }

    // Parse filename and filesize
    char *token = strtok(buffer, "|");
    if (token == NULL) {
        perror("Invalid file metadata");
        return;
    }
    strcpy(filename, token);

    token = strtok(NULL, "|");
    if (token == NULL) {
        perror("Invalid file size");
        return;
    }
    filesize = atol(token);

    // Prepare full file path
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "received_files/%s", filename);

    // Open file for writing
    FILE *file = fopen(full_path, "wb");
    if (file == NULL) {
        perror("Error creating file");
        return;
    }

    // Receive file contents
    long bytes_received = 0;
    while (bytes_received < filesize) {
        int read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0) {
            break;
        }
        
        fwrite(buffer, 1, read_size, file);
        bytes_received += read_size;
    }

    fclose(file);
    printf("Received file: %s (%ld bytes)\n", filename, filesize);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation error");
        exit(1);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind error");
        exit(1);
    }

    // Create received files directory
    create_received_files_dir();

    // Listen for connections
    listen(server_socket, 5);
    printf("Server listening on port %d\n", PORT);

    while (1) {
        // Accept client connection
        client_socket = accept(server_socket, 
                               (struct sockaddr *)&client_addr, 
                               &client_addr_len);
        if (client_socket < 0) {
            perror("Accept error");
            continue;
        }

        // Convert client IP to string for display
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Connection from %s\n", client_ip);

        // Receive file
        receive_file(client_socket);

        // Close client socket
        close(client_socket);
    }

    // Close server socket (this will never be reached in this implementation)
    close(server_socket);

    return 0;
}