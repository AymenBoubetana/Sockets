#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define ROUTER_PORT 8083  // Port du routeur principal
#define CLIENT_PORT 8080  // Nouveau port pour router2
#define BUFFER_SIZE 1024
#include <stdint.h>

// Calcul du CRC
uint32_t calculate_crc(const char *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF; // Initialiser le CRC
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320; // Polynôme utilisé pour le calcul
            } else {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF; // XOR final
}



int verify_crc(const char *data, size_t length, uint32_t received_crc) {
    uint32_t calculated_crc = calculate_crc(data, length);
    return calculated_crc == received_crc;
}


// Fonction qui va transmettre les messages vers router
void *forward_to_router(void *client_sock) {
    int client_socket = *(int *)client_sock;
    free(client_sock);  // Libérer la mémoire allouée pour le client_sock
    char buffer[BUFFER_SIZE];

    int server_socket;
    struct sockaddr_in server_addr;

    // Créer le socket pour communiquer avec le serveur
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur de création du socket serveur.");
        close(client_socket);
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ROUTER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Connecter au serveur
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connexion au serveur échouée.");
        close(client_socket);
        close(server_socket);
        return NULL;
    }

    printf("Routeur connecté au serveur.\n");

    while (1) {
        // Lire depuis le client
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            if (valread < 0)
                perror("Erreur de lecture depuis le client.");
            else
                printf("Client déconnecté du routeur.\n");
            break;
        }

        // Envoyer au serveur
        if (send(server_socket, buffer, valread, 0) < 0) {
            perror("Erreur d'envoi au serveur.");
            break;
        }

        // Lire depuis le serveur
        memset(buffer, 0, BUFFER_SIZE);
        valread = read(server_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            if (valread < 0)
                perror("Erreur de lecture depuis le serveur.");
            else
                printf("Serveur déconnecté.\n");
            break;
        }

        // Envoyer la réponse au client
        if (send(client_socket, buffer, valread, 0) < 0) {
            perror("Erreur d'envoi au client.");
            break;
        }
    }

    // Fermer les sockets
    close(client_socket);
    close(server_socket);
    printf("Connexion fermée.\n");

    return NULL;
}


int main() {
    int router2_fd, client_socket;
    struct sockaddr_in router2_addr;
    int addrlen = sizeof(router2_addr);

    // Créer le socket pour écouter les clients sur router2
    if ((router2_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Erreur de création du socket router2");
        exit(EXIT_FAILURE);
    }

    router2_addr.sin_family = AF_INET;
    router2_addr.sin_addr.s_addr = INADDR_ANY;
    router2_addr.sin_port = htons(CLIENT_PORT);  // Port de router2

    if (bind(router2_fd, (struct sockaddr *)&router2_addr, sizeof(router2_addr)) < 0) {
        perror("Erreur de bind sur router2");
        exit(EXIT_FAILURE);
    }

    if (listen(router2_fd, 3) < 0) {
        perror("Erreur de listen sur router2");
        exit(EXIT_FAILURE);
    }

    printf("router2 en attente de connexion...\n");

    while (1) {
        if ((client_socket = accept(router2_fd, (struct sockaddr *)&router2_addr, (socklen_t *)&addrlen)) < 0) {
            perror("Erreur d'acceptation sur router2");
            exit(EXIT_FAILURE);
        }

        printf("Client connecté à router2.\n");

        pthread_t thread_id;
        int *client_sock = malloc(sizeof(int));
        *client_sock = client_socket;
        pthread_create(&thread_id, NULL, forward_to_router, (void *)client_sock);
    }

    return 0;
}