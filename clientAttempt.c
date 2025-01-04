#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2908
#define BUFFER_SIZE 256

int serverSocket;

void *listener(void *arg);
void sendMessageToServer();

int main() {
    struct sockaddr_in serverAddr;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Connection to server failed");
        close(serverSocket);
        exit(1);
    }

    printf("Connected to the server.\n");

    pthread_t listenerThread;
    pthread_create(&listenerThread, NULL, listener, NULL);

    sendMessageToServer();

    pthread_cancel(listenerThread);
    pthread_join(listenerThread, NULL);

    close(serverSocket);
    return 0;
}

void *listener(void *arg) {
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytesRead = read(serverSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; 
            printf("\n[Server]: %s\n", buffer);

          
           if (strstr(buffer, "connected")) {
                printf("You are now in a private game.\n");
           }

          fflush(stdout);
        } else if (bytesRead == 0) {
            printf("\nServer disconnected\n");
            close(serverSocket);
            exit(0);
        } else {
            perror("[ERROR] Reading from server failed");
            close(serverSocket);
            exit(1);
        }
    }

    return NULL;
}

void sendMessageToServer() {
    char command[BUFFER_SIZE];

    while (1) {
        printf("\nEnter a command (login/invite/response/message): ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0'; 

    
        if (write(serverSocket, command, strlen(command) + 1) == -1) {
            perror("[ERROR] Failed to send command to server");
            break;
        }
    }
}
