#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define PORT 8080

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

void convert_to_binary(uint32_t crc, char *binary_str) {
    for (int i = 31; i >= 0; i--) {
        binary_str[31 - i] = (crc & (1 << i)) ? '1' : '0';
    }
    binary_str[32] = '\0';  // Null-terminate the string
}

int main() {
    int sock = 0;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Connexion au serveur
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    printf("Connecté au serveur. Tapez vos messages.\n");

    while (1) {
        printf("Vous: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Enlever le '\n' de la fin du message

        // Calcul du CRC
        uint32_t crc = calculate_crc(buffer, strlen(buffer));
        
        // Convertir le CRC en binaire
        char crc_binary[33];  // 32 bits + '\0'
        convert_to_binary(crc, crc_binary);
        
        // Afficher le message original et le CRC binaire
        printf("Message original: %s\n", buffer);
        printf("CRC binaire: %s\n", crc_binary);
        
        // Créer le message avec CRC binaire
        char message_with_crc[BUFFER_SIZE];
        snprintf(message_with_crc, BUFFER_SIZE, "%s|%s", buffer, crc_binary);
        
        // Envoyer le message
        send(sock, message_with_crc, strlen(message_with_crc), 0);
        
        // Réception de la réponse du serveur
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        printf("Serveur: %s\n", buffer);
    }

    close(sock);
    return 0;
}
