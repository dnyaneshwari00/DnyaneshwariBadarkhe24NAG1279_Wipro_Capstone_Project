#include <iostream>
#include <cstring>  // For memset
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>  // For inet_pton and inet_ntoa
#include <thread>

#define PORT 8080
#define BUFFER_SIZE 1024

void receive_messages(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0) {
            std::cerr << "Disconnected from server" << std::endl;
            close(client_socket);
            return;
        }
        std::cout << "Message from server: " << buffer << std::endl;
        memset(buffer, 0, BUFFER_SIZE); // Clear buffer after processing
    }
}

int main() {
    int client_fd;
    struct sockaddr_in serv_addr;

    // Creating socket file descriptor
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(client_fd);
        return -1;
    }

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(client_fd);
        return -1;
    }

    std::cout << "Connected to server" << std::endl;

    std::thread receive_thread(receive_messages, client_fd);

    while (true) {
        std::string message;
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        send(client_fd, message.c_str(), message.length(), 0);
    }

    receive_thread.join();
    close(client_fd);

    std::cout << "Disconnected from server" << std::endl;

    return 0;
}
