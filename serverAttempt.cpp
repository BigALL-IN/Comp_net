#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 2908
#define MAX_CLIENTS 100
#define BUFFER_SIZE 256

typedef struct {
    int socket;
    char username[50];
    int isPaired;
    int partnerSocket;
    int turn; // 1 if it's this client's turn, 0 otherwise
} Client;

Client clients[MAX_CLIENTS];
int clientCount = 0;
pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;

void* client(void *arg);
void Login(int clientSocket, char *username);
void Invite(int clientSocket, char *invitedUsername, char *starter);
void Response(int clientSocket, char *response);
void Message(int clientSocket, char *message);
void GameExit(int clientSocket);
Client *findClientByUsername(const char *username);
Client *findClientBySocket(int socket);

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        close(serverSocket);
        exit(1);
    }

    if (listen(serverSocket, 10) == -1) {
        perror("Listen failed");
        close(serverSocket);
        exit(1);
    }

    printf("Server running ...\n");

    while (1) {
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            perror("Client accept failed");
            continue;
        }

        pthread_t thread;
        int *newSocket = new int(clientSocket);
        *newSocket = clientSocket;

        pthread_create(&thread, NULL, client, newSocket);
    }

    close(serverSocket);
    return 0;
}

void *client(void *arg) {
    int clientSocket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int bytesRead;

    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        printf("[Server] Received: %s (socket: %d)\n", buffer, clientSocket);
        Client *client = findClientBySocket(clientSocket);
           if (client && client->isPaired) {
            if (strcmp(buffer, "chat_exit") == 0) {
                GameExit(clientSocket);
            } else if (client->turn) {
                if (strcmp(buffer, "WRONG MOVE") != 0) {
                    Message(clientSocket, buffer);
                    client->turn = 0;
                    Client *partner = findClientBySocket(client->partnerSocket);
                    if (partner) {
                        partner->turn = 1;
                    }
                } else {
                    char retryMsg[] = "WRONG MOVE, try again.\n";
                    write(clientSocket, retryMsg, strlen(retryMsg) + 1);
                }
            } else {
                char notYourTurnMsg[] = "Not your turn.\n";
                write(clientSocket, notYourTurnMsg, strlen(notYourTurnMsg) + 1);
            }
        }else {
            if (strncmp(buffer, "login ", 6) == 0) {

                Login(clientSocket, buffer + 6);
            } else if (strncmp(buffer, "invite ", 7) == 0) {
                char *starter = strstr(buffer, "starter:");
                if (starter) {
                 char *starterValue = starter + 8; // Skip "starter:" to get the actual value
                 *(starter - 1) = '\0'; // Null-terminate `invitedUsername`
                 Invite(clientSocket, buffer + 7, starterValue);
} else {
    Invite(clientSocket, buffer + 7, "partner");
} 
            } else if (strncmp(buffer, "response ", 9) == 0) {
                Response(clientSocket, buffer + 9);
            } else {
                char msg[] = "Unknown command. Please try again.\n";
                write(clientSocket, msg, strlen(msg) + 1);
            }
        }
    }

   
    pthread_mutex_lock(&clientsMutex);
    Client *client = findClientBySocket(clientSocket);
    if (client) {
        printf("[Server] Client %s disconnected (socket: %d).\n", client->username, clientSocket);
        for (int i = 0; i < clientCount; ++i) {
            if (clients[i].socket == clientSocket) {
                clients[i] = clients[--clientCount]; 
                break;
            }
        }
    }
    pthread_mutex_unlock(&clientsMutex);

    close(clientSocket);
    return NULL;
}

void Login(int clientSocket, char *username) {
    pthread_mutex_lock(&clientsMutex);

    for (int i = 0; i < clientCount; ++i) {
        if (strcmp(clients[i].username, username) == 0) {
            char msg[] = "User already exists. Please select another username.\n";
            write(clientSocket, msg, strlen(msg) + 1);
            pthread_mutex_unlock(&clientsMutex);
            return;
        }
    }

    strcpy(clients[clientCount].username, username);
    clients[clientCount].socket = clientSocket;
    clients[clientCount].isPaired = 0;
    clients[clientCount].partnerSocket = -1;
    clientCount++;

    char successMsg[] = "Login successful.\n";
    write(clientSocket, successMsg, strlen(successMsg) + 1);

    printf("[Server] User %s logged in (socket: %d).\n", username, clientSocket);

    pthread_mutex_unlock(&clientsMutex);
}

void Invite(int clientSocket, char *invitedUsername, char *starter) {
    pthread_mutex_lock(&clientsMutex);

    Client *invitedClient = findClientByUsername(invitedUsername);
    Client *invitingClient = findClientBySocket(clientSocket);

    if (!invitingClient || !invitedClient) {
        char msg[] = "User not found.\n";
        write(clientSocket, msg, strlen(msg) + 1);
        pthread_mutex_unlock(&clientsMutex);
        return;
    }

    if (invitingClient->username == invitedClient->username) {
        char msg[] = "You can't invite yourself!\n";
        write(clientSocket, msg, strlen(msg) + 1);
        pthread_mutex_unlock(&clientsMutex);
        return;
    }
    printf("starter: %s \n\n\n\n", starter);

   if (strcmp(starter, "me") == 0) {
    invitingClient->turn = 1; 
} else {
    invitingClient->turn = 0; 
}
    invitedClient->turn = !invitingClient->turn;

    char inviteMsg[BUFFER_SIZE];
    snprintf(inviteMsg, sizeof(inviteMsg), "You have an invite from %s. Respond with 'response accept' or 'response reject'.\n", invitingClient->username);
    write(invitedClient->socket, inviteMsg, strlen(inviteMsg) + 1);

    char notifyMsg[] = "Invite sent.\n";
    write(clientSocket, notifyMsg, strlen(notifyMsg) + 1);

    pthread_mutex_unlock(&clientsMutex);
}




void Response (int clientSocket, char *response) {
    pthread_mutex_lock(&clientsMutex);

    Client *respondingClient = findClientBySocket(clientSocket);
    if (!respondingClient) {
        pthread_mutex_unlock(&clientsMutex);
         return;
    }

    if (strcmp(response, "accept") == 0) {

        for (int i = 0; i < clientCount; ++i) {
            if (clients[i].partnerSocket == -1 && clients[i].socket != clientSocket) {
                
                clients[i].partnerSocket = clientSocket;
                respondingClient->partnerSocket = clients[i].socket;

                clients[i].isPaired = 1;
                respondingClient->isPaired = 1;

                char notifyClient1[] = "connected.\n";
                char notifyClient2[] = "connected.\n";
                write(clients[i].socket, notifyClient1, strlen(notifyClient1) + 1);
                write(clientSocket, notifyClient2, strlen(notifyClient2) + 1);

                break;
            }
        }
    } else if (strcmp(response, "reject") == 0) {
        char msg[] = "Invite rejected.\n";
        write(clientSocket, msg, strlen(msg) + 1);
    }

    pthread_mutex_unlock(&clientsMutex);
}





void Message(int clientSocket, char *message) {
    pthread_mutex_lock(&clientsMutex);

    Client *sender = findClientBySocket(clientSocket);
    if (sender && sender->partnerSocket != -1) {
        write(sender->partnerSocket, message, strlen(message) + 1);
    } else {
        char msg[] = "You are not connected to a game partner.\n";
        write(clientSocket, msg, strlen(msg) + 1);
    }

    pthread_mutex_unlock(&clientsMutex);
}

void GameExit(int clientSocket) {
    Client *client = findClientBySocket(clientSocket);
    if (client && client->partnerSocket != -1) {
        Client *partner = findClientBySocket(client->partnerSocket);

        if (partner) {
            char notifyPartner[] = "Your opponent has left the game.\n";
            write(partner->socket, notifyPartner, strlen(notifyPartner) + 1);
            partner->isPaired = 0;
            partner->partnerSocket = -1;
        }

        char notifyClient[] = "You have left the game.\n";
        write(clientSocket, notifyClient, strlen(notifyClient) + 1);

        client->isPaired = 0;
        client->partnerSocket = -1;
        
    }
}

Client* findClientByUsername(const char *username) {
    for (int i = 0; i < clientCount; ++i) {
        if (strcmp(clients[i].username, username) == 0) {
            return &clients[i];
        }
    }
    return NULL;
}

Client* findClientBySocket(int socket) {
    for (int i = 0; i < clientCount; ++i) {
        if (clients[i].socket == socket) {
            return &clients[i];
        }
    }
    return NULL;
}
