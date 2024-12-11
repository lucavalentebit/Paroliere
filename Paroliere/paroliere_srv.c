// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "messaggio.h"

#define BACKLOG 3

void *handle_client(void *arg);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int server_fd;
    struct sockaddr_in server_addr;

    // Creazione socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Configurazione socket
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Errore setsockopt");
        exit(EXIT_FAILURE);
    }

    // Configurazione indirizzo server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Binding
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nel binding");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, BACKLOG) == -1) {
        perror("Errore nella listen");
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto sulla porta %d...\n", port);

    // Loop principale del server
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (*client_fd == -1) {
            perror("Errore accept");
            free(client_fd);
            continue;
        }

        printf("Nuova connessione da %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_fd) != 0) {
            perror("Errore creazione thread");
            close(*client_fd);
            free(client_fd);
            continue;
        }
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    while (1) {
        messaggio_t* msg = ricevi_messaggio(client_fd);
        if (!msg) {
            printf("Client disconnesso\n");
            break;
        }

        printf("Ricevuto messaggio tipo: %c, lunghezza: %u\n", 
               msg->tipo, msg->lunghezza);
        if (msg->payload) {
            printf("Payload: %s\n", msg->payload);
        }

        // Invia risposta
        invia_messaggio(client_fd, MSG_OK, "Messaggio ricevuto");
        libera_messaggio(msg);
    }

    close(client_fd);
    return NULL;
}