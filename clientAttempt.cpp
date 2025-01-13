#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <SFML/Graphics.hpp>
#include "main.cpp"

#define SERVER_IP "192.168.1"
#define SERVER_PORT 2908
#define BUFFER_SIZE 256

int serverSocket;
bool isRunning = true;           // To control program flow
pthread_t chessThread;           // Separate thread for DrawChess
bool isChessRunning = false;     // To prevent multiple instances of DrawChess

void *listener(void *arg);
void sendMessageToServer();
void *startChess(void *arg);     // Function to handle DrawChess in a thread

int main() {
    struct sockaddr_in serverAddr;

    // Create the socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Set up the server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to the server
    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Connection to server failed");
        close(serverSocket);
        return 1;
    }

    std::cout << "Connected to the server." << std::endl;

    // Start the listener thread
    pthread_t listenerThread;
    if (pthread_create(&listenerThread, NULL, listener, NULL) != 0) {
        perror("Failed to create listener thread");
        close(serverSocket);
        return 1;
    }

    // Handle user input in the main thread
    sendMessageToServer();

    // Stop the listener thread and clean up
    isRunning = false;
    pthread_cancel(listenerThread);
    pthread_join(listenerThread, NULL);

    if (isChessRunning) {
        pthread_cancel(chessThread);
        pthread_join(chessThread, NULL);
    }

    close(serverSocket);
    return 0;
}

void *listener(void *arg) {
    char buffer[BUFFER_SIZE];

    while (isRunning) {
        int bytesRead = read(serverSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';

            if (strstr(buffer, "connected.")) {
                std::cout << "You are now in a private game." << std::endl;

                // Start the chess game in a separate thread
                if (!isChessRunning) {
                    isChessRunning = true;
                    if (pthread_create(&chessThread, NULL, startChess, NULL) != 0) {
                        perror("[ERROR] Failed to create chess thread");
                        isChessRunning = false;
                    }
                }
            } else {
                std::cout << "\n[Server]: " << buffer << std::endl;
            }
        } else if (bytesRead == 0) {
            std::cout << "\nServer disconnected" << std::endl;
            close(serverSocket);
            isRunning = false;
            pthread_exit(NULL); // Exit the thread
        } else {
            perror("[ERROR] Reading from server failed");
            close(serverSocket);
            isRunning = false;
            pthread_exit(NULL); // Exit the thread
        }
    }

    return NULL;
}

// Thread function to start DrawChess
void *startChess(void *arg) {
    DrawChess(); // Run the chess game
    isChessRunning = false; // Reset flag when DrawChess finishes
    return NULL;
}

void sendMessageToServer() {
    std::string command;

    while (isRunning) {
        std::cout << "\nEnter a command (login/invite/response/message): ";
        std::getline(std::cin, command);

        if (write(serverSocket, command.c_str(), command.size() + 1) == -1) {
            perror("[ERROR] Failed to send command to server");
            break;
        }
    }
}
