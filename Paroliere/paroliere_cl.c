#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "messaggio.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <nome_server> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock;
    struct sockaddr_in server_addr;

    // Creazione del socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Errore nella conversione dell'indirizzo IP");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connessione al server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nella connessione al server");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connesso al server %s:%d\n", server_ip, port);

    // Loop principale
    while (1) {
        char input[1024];
        printf("Inserisci un messaggio (fine per uscire): ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "fine") == 0) {
            break;
        }

        // Invia il messaggio usando il nostro protocollo
        if (invia_messaggio(sock, MSG_PAROLA, input) < 0) {
            perror("Errore nell'invio del messaggio");
            break;
        }

        // Ricevi la risposta
        messaggio_t* response = ricevi_messaggio(sock);
        if (!response) {
            printf("Connessione chiusa dal server\n");
            break;
        }

        // Processa la risposta
        printf("Risposta dal server (tipo: %c): ", response->tipo);
        if (response->payload) {
            printf("%s\n", response->payload);
        }
        libera_messaggio(response);
    }

    close(sock);
    return 0;
}