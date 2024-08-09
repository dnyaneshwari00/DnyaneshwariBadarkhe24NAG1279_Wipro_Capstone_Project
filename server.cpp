#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

#define PORT 8080
#define BUFFER_SIZE 1024

std::mutex mtx;
std::vector<int> client_sockets;
int client_counter = 0; // To track client numbers

void handle_client(int client_socket, int client_id) {
    char buffer[BUFFER_SIZE] = {0};
    std::cout << "Client " << client_id << " connected." << std::endl;

    while (true) {
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0) {
            std::cerr << "Client " << client_id << " disconnected." << std::endl;
            close(client_socket);

            // Remove client socket from the list
            {
                std::lock_guard<std::mutex> lock(mtx);
                client_sockets.erase(std::remove(client_sockets.begin(), client_sockets.end(), client_socket), client_sockets.end());
            }
            return;
        }

        // Broadcast the message to all other clients
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (int socket : client_sockets) {
                if (socket != client_socket) {
                    send(socket, buffer, bytes_read, 0);
                }
            }
        }

        std::cout << "Received message from Client " << client_id << ": " << buffer << std::endl;
        memset(buffer, 0, BUFFER_SIZE); // Clear buffer after processing
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        int client_id;
        {
            std::lock_guard<std::mutex> lock(mtx);
            client_id = ++client_counter;
            client_sockets.push_back(new_socket);
        }

        std::cout << "Client " << client_id << " connected." << std::endl;
        std::thread(handle_client, new_socket, client_id).detach(); // Handle client in a new thread
    }

    return 0;
}
