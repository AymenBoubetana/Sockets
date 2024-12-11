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

char* CRC(char* msg) {
    int i, j, step = 1;
    char crc[] = "10001"; // Polynôme utilisé pour le calcul
    int n = strlen(crc);
    static char encoded[200];
    strcpy(encoded, msg);

    for (i = 0; i < n - 1; i++)
        strcat(encoded, "0");

    printf("\nDivision pour calcul du CRC côté client :\n");
    for (i = 0; i <= strlen(encoded) - n;) {
        printf("R(%d): %s\n", step++, encoded);
        for (j = 0; j < n; j++)
            encoded[i + j] = (encoded[i + j] == crc[j]) ? '0' : '1';
        while (i < strlen(encoded) && encoded[i] != '1')
            i++;
    }
    printf("Reste final côté client : %s\n", &encoded[strlen(encoded) - (n - 1)]);
    return &encoded[strlen(encoded) - (n - 1)];
}

bool CRC2(char* msg) {
    int i, j, step = 1;
    char crc[] = "10001"; // Même polynôme utilisé ici
    int n = strlen(crc);
    static char encoded[200];
    strcpy(encoded, msg);

    printf("\nDivision pour vérification CRC côté client :\n");
    for (i = 0; i <= strlen(encoded) - n;) {
        printf("R(%d): %s\n", step++, encoded);
        for (j = 0; j < n; j++)
            encoded[i + j] = (encoded[i + j] == crc[j]) ? '0' : '1';
        while (i < strlen(encoded) && encoded[i] != '1')
            i++;
    }
    printf("Reste final côté client après vérification : %s\n", &encoded[strlen(encoded) - (n - 1)]);

    for (i = strlen(encoded) - (n - 1); i < strlen(encoded); i++) {
        if (encoded[i] != '0') {
            return false;
        }
    }
    return true;
}

void func(int sockfd) {
    char buff[MAX];
    int n;

    for (;;) {
        initial();
        bzero(buff, sizeof(buff));
        printf("Entrez le message binaire (ou tapez 'exit' pour quitter) : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n');
        buff[n - 1] = '\0';

        if (strcmp(buff, "exit") == 0) {
            write(sockfd, buff, strlen(buff));
            printf("Client déconnecté.\n");
            break;
        }

        char* fcrc = CRC(buff);
        strcat(trame, buff);
        strcat(trame, fcrc);
        strcat(trame, "0000");

        write(sockfd, trame, strlen(trame));

        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("Message reçu du serveur : %s\n", buff);

        char messageWithCrc[MAX];
        int len = strlen(buff) - 28;
        memcpy(messageWithCrc, &buff[24], len);
        messageWithCrc[len] = '\0';

        if (CRC2(messageWithCrc)) {
            printf("Trame du serveur valide.\n");
        } else {
            printf("Erreur détectée dans la trame reçue.\n");
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Échec de la création du socket...\n");
        exit(0);
    }
    printf("Socket créé avec succès.\n");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Échec de la connexion au serveur...\n");
        exit(0);
    }
    printf("Connecté au serveur.\n");

    func(sockfd);
    close(sockfd);
}
