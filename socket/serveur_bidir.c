#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define MAX 200
#define PORT 8080
#define SA struct sockaddr

char fanion[] = "00000000";
char adrS[] = "S000000S";
char adrD[] = "D000000D";
char trame[MAX] = "";

void initial() {
    strcpy(trame, "");
    strcat(trame, fanion);
    strcat(trame, adrS);
    strcat(trame, adrD);
}

char* CRC2(char* msg) {
    int i, j, step = 1;
    char crc[] = "10001"; // Polynôme utilisé pour le calcul
    int n = strlen(crc);
    static char encoded[200];
    strcpy(encoded, msg);

    for (i = 0; i < n - 1; i++)
        strcat(encoded, "0");

    printf("\nDivision pour calcul du CRC côté serveur :\n");
    for (i = 0; i <= strlen(encoded) - n;) {
        printf("R(%d): %s\n", step++, encoded);
        for (j = 0; j < n; j++)
            encoded[i + j] = (encoded[i + j] == crc[j]) ? '0' : '1';
        while (i < strlen(encoded) && encoded[i] != '1')
            i++;
    }
    return &encoded[strlen(encoded) - (n - 1)];
}

bool CRC(char* msg) {
    int i, j, step = 1;
    char crc[] = "10001"; // Même polynôme utilisé ici
    int n = strlen(crc);
    static char encoded[200];
    strcpy(encoded, msg);

    printf("\nDivision pour vérification CRC côté serveur :\n");
    for (i = 0; i <= strlen(encoded) - n;) {
        printf("R(%d): %s\n", step++, encoded);
        for (j = 0; j < n; j++)
            encoded[i + j] = (encoded[i + j] == crc[j]) ? '0' : '1';
        while (i < strlen(encoded) && encoded[i] != '1')
            i++;
    }
    printf("Reste final côté serveur : %s\n", &encoded[strlen(encoded) - (n - 1)]);

    for (i = strlen(encoded) - (n - 1); i < strlen(encoded); i++) {
        if (encoded[i] != '0') {
            return false;
        }
    }
    return true;
}

void func(int connfd) {
    char buff[MAX];
    int n;

    for (;;) {
        bzero(buff, MAX);
        read(connfd, buff, sizeof(buff));
        printf("Trame reçue du client : %s\n", buff);

        if (strncmp(buff, "exit", 4) == 0) {
            printf("Client déconnecté.\n");
            break;
        }

        char messageWithCrc[MAX];
        int len = strlen(buff) - 28;
        memcpy(messageWithCrc, &buff[24], len);
        messageWithCrc[len] = '\0';

        if (CRC(messageWithCrc)) {
            printf("Message valide.\n");
        } else {
            printf("Erreur détectée dans le message.\n");
        }

        bzero(buff, MAX);
        printf("Entrez le message binaire à envoyer au client : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n');
        buff[n - 1] = '\0';

        char* fcrc = CRC2(buff);
        initial();
        strcat(trame, buff);
        strcat(trame, fcrc);
        strcat(trame, "0000");

        write(connfd, trame, strlen(trame));
        printf("Trame envoyée au client : %s\n", trame);
    }
}

int main() {
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Échec de la création du socket...\n");
        exit(0);
    }
    printf("Socket créé avec succès.\n");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("Échec du bind...\n");
        exit(0);
    }
    printf("Socket bindé avec succès.\n");

    if ((listen(sockfd, 5)) != 0) {
        printf("Échec de l'écoute...\n");
        exit(0);
    }
    printf("Serveur en écoute...\n");

    len = sizeof(cli);
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("Échec de l'acceptation...\n");
        exit(0);
    }
    printf("Client accepté.\n");

    func(connfd);
    close(sockfd);
}
