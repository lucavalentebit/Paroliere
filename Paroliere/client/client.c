#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345 // Porta su cui il server è in ascolto
#define SERVER_IP "127.0.0.1" // IP del server

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Creazione del socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
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

    printf("Connesso al server %s:%d\n", SERVER_IP, PORT);

    // Invio e ricezione messaggi
    while (1) {
        printf("Inserisci un messaggio (fine per uscire): ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // Rimuove il newline

        if (strcmp(buffer, "fine") == 0) {
            printf("Chiusura del client...\n");
            break;
        }

        if (send(sock, buffer, strlen(buffer), 0) == -1) {
            perror("Errore nell'invio");
            break;
        }

        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            printf("Connessione chiusa dal server o errore\n");
            break;
        }

        printf("Risposta dal server: %s\n", buffer);
    }

    // Chiusura del socket
    close(sock);

    return 0;
}
