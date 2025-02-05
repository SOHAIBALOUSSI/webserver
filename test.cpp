#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

/*
    for i in {1..997}; do
    ./a.out &
    done
    wait
*/
int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to connect to server\n";
        close(sockfd);
        return 1;
    }

    std::string request = "POST /path/to/resource HTTP/1.1\r\n";

    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Failed to send request\n";
        close(sockfd);
        return 1;
    }
    sleep(2);
    request = "Host: example.com\r\n"
                "Content-length: 1000\r\n";
    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Failed to send request\n";
        close(sockfd);
        return 1;
    }
    sleep(1);
    request = "\r\n\r\n"                          
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Failed to send request\n";
        close(sockfd);
        return 1;
    }
    sleep(3);
    request =  "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
                "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
        if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Failed to send request\n";
        close(sockfd);
        return 1;
    }


    char buffer[4096];
    ssize_t bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "Server Response:\n" << buffer << std::endl;
    } else {
        std::cerr << "Failed to receive response\n";
    }

    close(sockfd);
    return 0;
}