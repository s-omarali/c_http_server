#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
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
        
        close(client_fd); // close immediately after accepting for this example, in a real server you would handle the client connection
    }
    
    



}