#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

// Function to get file size
long get_file_size(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fclose(file);

    return filesize;
}

// Function to send file
int send_file(const char *filename, const char *host, int port) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Check file exists
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("File not found");
        return -1;
    }

    // Get file size
    long filesize = get_file_size(filename);
    if (filesize < 0) {
        fclose(file);
        return -1;
    }

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation error");
        fclose(file);
        return -1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        fclose(file);
        close(client_socket);
        return -1;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        fclose(file);
        close(client_socket);
        return -1;
    }

    // Prepare file metadata
    char filename_only[256];
    char *last_slash = strrchr(filename, '/');
    if (last_slash) {
        strcpy(filename_only, last_slash + 1);
    } else {
        strcpy(filename_only, filename);
    }

    char file_info[BUFFER_SIZE];
    snprintf(file_info, sizeof(file_info), "%s|%ld", filename_only, filesize);

    // Send file metadata
    if (send(client_socket, file_info, strlen(file_info), 0) < 0) {
        perror("Metadata send failed");
        fclose(file);
        close(client_socket);
        return -1;
    }

    // Send file contents
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) < 0) {
            perror("File send failed");
            fclose(file);
            close(client_socket);
            return -1;
        }
    }

    // Clean up
    fclose(file);
    close(client_socket);

    printf("File %s sent successfully!\n", filename);
    return 0;
}

int main() {
    char filename[512];
    char host[256] = "127.0.0.1";
    int port = 5000;

    // Get file path from user
    printf("Enter the path of the file you want to send: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = 0;  // Remove newline

    // Get server details
    printf("Enter server IP (default 127.0.0.1): ");
    char input[256];
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;  // Remove newline
    if (strlen(input) > 0) {
        strcpy(host, input);
    }

    printf("Enter server port (default 5000): ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;  // Remove newline
    if (strlen(input) > 0) {
        port = atoi(input);
    }

    // Send the file
    return send_file(filename, host, port);
}