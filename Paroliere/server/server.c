// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#define PORT 12345     // Porta di ascolto del server
#define BACKLOG 10     // Numero massimo di connessioni pendenti

void *gestisci_client(void *arg);

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    // Creazione del socket del server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Imposta le opzioni del socket
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Errore setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configura l'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Accetta connessioni da qualsiasi IP
    server_addr.sin_port = htons(PORT);

    // Associa il socket all'indirizzo
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nel binding del socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Ascolta le connessioni in arrivo
    if (listen(server_fd, BACKLOG) == -1) {
        perror("Errore nell'ascolto sul socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto sulla porta %d...\n", PORT);

    while (1) {
        int *client_fd = malloc(sizeof(int));
        if (client_fd == NULL) {
            perror("Errore nell'allocazione della memoria");
            continue;
        }

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Accetta una nuova connessione client
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (*client_fd == -1) {
            perror("Errore nell'accettazione della connessione");
            free(client_fd);
            continue;
        }

        printf("Connessione accettata da %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Crea un nuovo thread per gestire il client
        pthread_t tid;
        if (pthread_create(&tid, NULL, gestisci_client, client_fd) != 0) {
            perror("Errore nella creazione del thread");
            close(*client_fd);
            free(client_fd);
            continue;
        }

        // Stacca il thread per liberare le risorse al termine
        pthread_detach(tid);
    }

    // Chiudi il socket del server
    close(server_fd);
    return 0;
}

void *gestisci_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Ricevi dati dal client
        int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Client disconnesso\n");
            } else {
                perror("Errore nella ricezione dei dati");
            }
            break;
        }

        printf("Messaggio ricevuto: %s\n", buffer);

        // Esegue l'echo del messaggio al client
        if (send(client_fd, buffer, bytes_received, 0) == -1) {
            perror("Errore nell'invio dei dati");
            break;
        }
    }

    // Chiude il socket del client
    close(client_fd);
    pthread_exit(NULL);
}