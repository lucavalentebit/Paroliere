// paroliere_cl.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "messaggio.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <nome_server> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock;
    struct sockaddr_in server_addr;

    // Creazione del socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        perror("Errore nella conversione dell'indirizzo IP");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connessione al server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Errore nella connessione al server");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connesso al server %s:%d\n", server_ip, port);

    // Loop principale
    while (1)
    {
        char input[1024];
        printf("[PROMPT PAROLIERE] --> ");
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\nErrore nella lettura dell'input.\n");
            break;
        }
        input[strcspn(input, "\n")] = '\0'; // Rimozione carattere di nuova linea

        if (strcmp(input, "fine") == 0)
        {
            printf("Chiusura del client.\n");
            break;
        }

        if (strncmp(input, "registra_utente ", 16) == 0)
        {
            // Estrai il nome utente dopo "registra_utente "
            char *username = input + 16;
            if (strlen(username) > 0)
            {
                // Invia il comando di registrazione
                if (invia_messaggio(sock, MSG_REGISTRA_UTENTE, username) < 0)
                {
                    perror("Errore nell'invio del messaggio");
                    break;
                }

                // Ricevi la risposta
                messaggio_t *response = ricevi_messaggio(sock);
                if (!response)
                {
                    printf("Connessione chiusa dal server\n");
                    break;
                }

                // Processa la risposta
                if (response->tipo == MSG_OK && response->payload)
                {
                    printf("Successo: %s\n", response->payload);
                }
                else if (response->tipo == MSG_ERR && response->payload)
                {
                    printf("Errore: %s\n", response->payload);
                }
                else
                {
                    printf("Risposta inaspettata dal server (tipo: %c)\n", response->tipo);
                }
                libera_messaggio(response);
            }
            else
            {
                printf("Nome utente non valido.\n");
            }
        }
        else if (strncmp(input, "login_utente ", 13) == 0)
        {
            // Estrai il nome utente dopo "login_utente "
            char *username = input + 13;
            if (strlen(username) > 0)
            {
                // Invia il comando di login
                if (invia_messaggio(sock, MSG_LOGIN_UTENTE, username) < 0)
                {
                    perror("Errore nell'invio del messaggio");
                    break;
                }

                // Ricevi la risposta
                messaggio_t *response = ricevi_messaggio(sock);
                if (!response)
                {
                    printf("Connessione chiusa dal server\n");
                    break;
                }

                // Processa la risposta
                if (response->tipo == MSG_OK && response->payload)
                {
                    printf("Successo: %s\n", response->payload);
                }
                else if (response->tipo == MSG_ERR && response->payload)
                {
                    printf("Errore: %s\n", response->payload);
                }
                else
                {
                    printf("Risposta inaspettata dal server (tipo: %c)\n", response->tipo);
                }
                libera_messaggio(response);
            }
            else
            {
                printf("Nome utente non valido.\n");
            }
        }
        else if (strcmp(input, "matrice") == 0)
        {
            // Invia il comando "matrice" al server
            if (invia_messaggio(sock, MSG_MATRICE, "matrice") < 0)
            {
                perror("Errore nell'invio del messaggio");
                break;
            }

            // Ricevi la risposta
            messaggio_t *response = ricevi_messaggio(sock);
            if (!response)
            {
                printf("Connessione chiusa dal server\n");
                break;
            }

            // Processa la risposta
            if (response->tipo == MSG_MATRICE && response->payload)
            {
                printf("Matrice ricevuta:\n%s\n", response->payload);
            }
            else if (response->tipo == MSG_ERR && response->payload)
            {
                printf("Errore: %s\n", response->payload);
            }
            else
            {
                printf("Risposta inaspettata dal server (tipo: %c)\n", response->tipo);
            }
            libera_messaggio(response);
        }
        else
        {
            // Invia altri comandi al server come MSG_PAROLA
            if (invia_messaggio(sock, MSG_PAROLA, input) < 0)
            {
                perror("Errore nell'invio del messaggio");
                break;
            }

            // Ricevi la risposta
            messaggio_t *response = ricevi_messaggio(sock);
            if (!response)
            {
                printf("Connessione chiusa dal server\n");
                break;
            }

            // Processa la risposta
            if (response->payload)
            {
                printf("Risposta dal server (tipo: %c): %s\n", response->tipo, response->payload);
            }
            else
            {
                printf("Risposta dal server (tipo: %c)\n", response->tipo);
            }
            libera_messaggio(response);
        }
    }

    close(sock);
    return 0;
}