#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>  // Ajouter cet en-tête pour les threads


#define BUFFER_SIZE 1024
#define SERVER_PORT 9090

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

void *handle_client(void *client_sock) {
    int sock = *(int *)client_sock;
    char buffer[BUFFER_SIZE];
    char *message;
    char *crc_binary;
    uint32_t received_crc;
    uint32_t calculated_crc;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread <= 0) {
            printf("Client déconnecté.\n");
            break;
        }

        // Extraire le message et le CRC binaire
        message = strtok(buffer, "|");
        crc_binary = strtok(NULL, "|");

        if (crc_binary == NULL) {
            printf("Erreur dans le format du message.\n");
            continue;
        }

        // Convertir le CRC binaire en entier
        received_crc = strtol(crc_binary, NULL, 2);
        printf("Message reçu: %s\n", message);
        printf("CRC reçu (binaire): %s\n", crc_binary);
        printf("CRC reçu (entier): %u\n", received_crc);

        // Calculer le CRC à partir du message reçu
        calculated_crc = calculate_crc(message, strlen(message));

        // Vérifier si le CRC reçu correspond au CRC calculé
        if (received_crc == calculated_crc) {
            printf("Message valide: %s\n", message);
            // Réponse du serveur
            const char *response = "Message reçu par le serveur.";
            send(sock, response, strlen(response), 0);
        } else {
            printf("Erreur de CRC, message corrompu.\n");
            const char *response = "Erreur de CRC, message corrompu.";
            send(sock, response, strlen(response), 0);
        }
    }

    close(sock);
    free(client_sock);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Création du socket serveur
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    // Liaison du socket à l'adresse
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Écouter les connexions entrantes
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Serveur en attente de connexion sur le port %d...\n", SERVER_PORT);

    // Acceptation des connexions clients
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Connexion établie avec un client.\n");

        pthread_t thread_id;
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;
        pthread_create(&thread_id, NULL, handle_client, (void *)client_sock);
    }

    return 0;
}
