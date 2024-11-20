#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8081
#define BUFFER_SIZE 1024

// Fonction pour envoyer la réponse HTTP
void send_response(int client_sock) {
    char *response = "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Connection: close\r\n\r\n"
                     "<html><body><h1>Bienvenue sur mon serveur web!</h1></body></html>";
    send(client_sock, response, strlen(response), 0);
}

// Fonction pour analyser la requête HTTP
int parse_request(char *request) {
    // Vérifier si la requête commence par "GET /"
    if (strncmp(request, "GET /", 5) == 0) {
        return 1;  // Requête valide
    }
    return 0;  // Requête invalide
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];

    // Création du socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Erreur lors de la création du socket");
        exit(1);
    }

    // Configuration de l'adresse du serveur
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accepter toutes les adresses
    server_addr.sin_port = htons(PORT); // Port 8081

    // Lier le socket à l'adresse
    if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur de binding");
        exit(1);
    }

    // Écouter les connexions entrantes
    if (listen(server_sock, 5) < 0) {
        perror("Erreur lors de l'écoute des connexions");
        exit(1);
    }

    printf("Serveur en écoute sur le port %d...\n", PORT);

    // Accepter les connexions entrantes
    client_len = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_len);
    if (client_sock < 0) {
        perror("Erreur lors de l'acceptation de la connexion");
        exit(1);
    }

    printf("Client connecté\n");

    // Recevoir la requête HTTP
    recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[sizeof(buffer) - 1] = '\0'; // Assurer la terminaison de la chaîne

    // Afficher la requête reçue (pour débogage)
    printf("Requête reçue:\n%s\n", buffer);

    // Vérifier si la requête est valide (par exemple, "GET / HTTP/1.1")
    if (parse_request(buffer)) {
        // Envoyer la réponse HTTP
        send_response(client_sock);
    } else {
        // Réponse HTTP 400 Bad Request
        char *error_response = "HTTP/1.1 400 Bad Request\r\n"
                               "Content-Type: text/html\r\n"
                               "Connection: close\r\n\r\n"
                               "<html><body><h1>400 Bad Request</h1></body></html>";
        send(client_sock, error_response, strlen(error_response), 0);
    }

    // Fermer la connexion proprement
    close(client_sock);
    close(server_sock);

    return 0;
}
