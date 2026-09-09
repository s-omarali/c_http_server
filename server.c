#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
int main()
{
    char buffer[BUFFER_SIZE];
    // create socket file descriptor. need domain, type, protocol
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = default protocol
    if (server_fd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    printf("Socket created successfully. socket fd = %d\n", server_fd);

    // allow address reuse to prevent "Address already in use" error when restarting the server
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // returns 0 if succesful
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    printf("Set socket options successfully. server reuse status = %d\n", opt);
    
    // bind socket to a specific port and address
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address)); // initialize address structure to zero
    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // bind to all available interfaces
    address.sin_port = htons(8080); // port number in network byte order

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket bound to address successfully.\n");

    // listen for incoming connections
    if (listen(server_fd, 3) < 0) // backlog of 3
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket is listening for incoming connections on port %d.\n", ntohs(address.sin_port));

    // accept incoming connection
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    // wrap in a loop to accept multiple clients
    while(1)
    {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_len);
        if (client_fd < 0)
        {
            perror("accept failed for client connection");
            printf("Failed to accept client connection from IP = %s, Port = %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            continue; // continue to accept next connection
        }
        printf("Client connected successfully. Client IP = %s, Client Port = %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        
        memset(buffer, 0, BUFFER_SIZE); // clear buffer
        int valread = read(client_fd, buffer, sizeof(buffer)-1); // read data from client
        printf("Data received from client: %s\n", buffer);
        if (valread < 0)
        {
            perror("read failed");
            close(client_fd);
            continue;
        }
        // null terminate the buffer to make it a valid string. thats why we read sizeof(buffer)-1 bytes to leave space for the null terminator
        buffer[valread] = '\0';
        printf("Data received from client: %s\n", buffer);

        // parse request and route based on the path
        char method[16], path[256];
        char content_type[64];
        sscanf(buffer, "%s %s", method, path); // parse method and path from the request line
        printf("Request method: %s, Request path: %s\n", method, path);

        // routing
        if (strcmp(path, "/") == 0)
        {
            strcpy(path, "index.html"); // serve index.html for root path
            content_type[0] = '\0'; // clear content_type
            strcat(content_type, "text/html"); // set content type for HTML file
        }
        else if (strcmp(path, "/index.js") == 0)
        {
            strcpy(path, "index.js"); // serve index.js for /index.js path
            content_type[0] = '\0'; // clear content_type
            strcat(content_type, "application/javascript"); // set content type for JS file
        }
        else
        {
            // send 404 response to client
            // const char *not_found_response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\n404 Not Found";
            char not_found_response[BUFFER_SIZE];
            size_t file_size = strlen("404 Not Found");
            snprintf(not_found_response, sizeof(not_found_response), "HTTP/1.1 404 Not Found\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", content_type, file_size); // build HTTP response header with content length and type
            write(client_fd, not_found_response, strlen(not_found_response));  // send 404 response to client socket
            close(client_fd);
            continue;
        }


        // build response from html file
        // open file and read its contents
        FILE *file = fopen(path, "rb"); // open in binary mode to read raw bytes to prevent issues with line endings on different platforms
        if (file == NULL)
        {
            perror("fopen failed");
            // send 404 response to client
            char not_found_response[BUFFER_SIZE];
            size_t file_size = strlen("404 Not Found");
            snprintf(not_found_response, sizeof(not_found_response), "HTTP/1.1 404 Not Found\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", content_type, file_size);
            write(client_fd, not_found_response, strlen(not_found_response));  // send 404 response to client socket
            close(client_fd);
            continue;
        }

        // get file size before reading
        fseek(file, 0, SEEK_END);
        long int file_size = ftell(file); // get current file pointer position which is the size of the file
        printf("File size of %s = %ld bytes\n", path, file_size);
        fseek(file, 0, SEEK_SET); // reset file pointer to beginning of file

        // read file contents into response buffer
        char *response = malloc(file_size + 1); // allocate memory for response. +1 for null terminator
        if (response == NULL)
        {
            perror("malloc failed");
            fclose(file);
            close(client_fd);
            continue;
        }
        fread(response, 1, file_size, file); // read file contents into response buffer
        response[file_size] = '\0'; // null terminate the response buffer

        // two seperate write calls to send the response header and body separately
        // send response header to client
        char header[BUFFER_SIZE];
        snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", content_type, file_size); // build HTTP response header with content length and type
        if (write(client_fd, header, strlen(header)) < 0)
        {
            perror("write failed");
            close(client_fd);
            free(response);
            fclose(file);
            continue;
        }

        // send response body to client
        if (write(client_fd, response, file_size) < 0)
        {
            perror("write failed");
            close(client_fd);
            free(response);
            fclose(file);
            continue;
        }
        free(response); // free allocated memory for response
        fclose(file);
        close(client_fd); // close immediately after accepting for this example, in a real server you would handle the client connection

    }
    

}