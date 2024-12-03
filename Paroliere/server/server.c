#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 12345 // Porta su cui il server ascolterà
#define BACKLOG 10 // Numero massimo di connessioni pendenti

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];

    // Creazione del socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Binding del socket all'indirizzo
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nel binding");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Avvio dell'ascolto
    if (listen(server_fd, BACKLOG) == -1) {
        perror("Errore nell'ascolto");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto sulla porta %d...\n", PORT);

    // Accettazione di una connessione
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
        perror("Errore nell'accettazione della connessione");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connessione accettata da %s:%d\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    // Ricezione e risposta al client
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            printf("Connessione chiusa dal client o errore\n");
            break;
        }

        printf("Messaggio ricevuto: %s\n", buffer);

        // Echo del messaggio ricevuto
        if (send(client_fd, buffer, bytes_received, 0) == -1) {
            perror("Errore nell'invio");
            break;
        }
    }

    // Chiusura dei socket
    close(client_fd);
    close(server_fd);

    return 0;
}
