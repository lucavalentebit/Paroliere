#define _XOPEN_SOURCE 700 // Per sigaction, perché sigaction fa parte di POSIX.1-2008
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>
#include "messaggio.h"

// Flag per il controllo della chiusura
static volatile sig_atomic_t running = 1;
static const char PROMPT[] = "[PROMPT PAROLIERE] --> ";

// Handler per SIGINT
void handle_sigint(int sig)
{
    (void)sig; // Cast a void per sopprimere il warning
    running = 0;
}

// Funzione per la chiusura pulita del client
void chiudi_client(int sock)
{
    printf("Chiusura della connessione...\n");
    close(sock);
    printf("Client terminato.\n");
}

// Funzione helper per estrarre parametri dai comandi
const char *estrai_parametro(const char *input, const char *comando)
{
    size_t len_comando = strlen(comando);
    if (strncmp(input, comando, len_comando) == 0)
    {
        const char *param = input + len_comando;
        return (strlen(param) > 0) ? param : NULL;
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // Configurazione del gestore SIGINT
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("Errore nella configurazione di sigaction");
        exit(EXIT_FAILURE);
    }

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
    printf("%s", PROMPT);
    fflush(stdout);

    fd_set read_fds;
    struct timeval tv;
    char input[1024];
    bool need_prompt = false;

    // Loop principale
    while (running)
    {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);

        // Timeout di 1 secondo per controllare il flag running
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(sock + 1, &read_fds, NULL, NULL, &tv);

        if (activity < 0)
        {
            if (errno == EINTR)
                continue;
            perror("Errore in select");
            break;
        }

        if (activity == 0)
            continue; // Timeout, no activity

        // Input da tastiera
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            if (fgets(input, sizeof(input), stdin) == NULL)
            {
                printf("\nErrore nella lettura dell'input.\n");
                break;
            }
            input[strcspn(input, "\n")] = '\0';

            if (strcmp(input, "fine") == 0)
            {
                printf("Chiusura richiesta dall'utente.\n");
                break;
            }

            // Estrazione del comando principale
            char *command = strtok(input, " ");
            const char *param = strtok(NULL, "");

            // Gestione dei comandi
            switch (command ? command[0] : '\0')
            {
            case 'r':
                if (strcmp(command, "registra_utente") == 0 && param)
                {
                    if (invia_messaggio(sock, MSG_REGISTRA_UTENTE, param) < 0)
                    {
                        perror("Errore nell'invio del messaggio");
                        running = 0;
                    }
                }
                break;
            case 'l':
                if (strcmp(command, "login_utente") == 0 && param)
                {
                    if (invia_messaggio(sock, MSG_LOGIN_UTENTE, param) < 0)
                    {
                        perror("Errore nell'invio del messaggio");
                        running = 0;
                    }
                }
                break;
            case 'm':
                if (strcmp(command, "matrice") == 0)
                {
                    if (invia_messaggio(sock, MSG_MATRICE, "matrice") < 0)
                    {
                        perror("Errore nell'invio del messaggio");
                        running = 0;
                    }
                }
                else if (strcmp(command, "msg") == 0 && param)
                {
                    if (invia_messaggio(sock, MSG_POST_BACHECA, param) < 0)
                    {
                        perror("Errore nell'invio del messaggio");
                        running = 0;
                    }
                }
                break;
            case 's':
                if (strcmp(command, "show-msg") == 0)
                {
                    if (invia_messaggio(sock, MSG_SHOW_BACHECA, "show-msg") < 0)
                    {
                        perror("Errore nell'invio del messaggio");
                        running = 0;
                    }
                }
                break;
            default:
                if (invia_messaggio(sock, MSG_PAROLA, input) < 0)
                {
                    perror("Errore nell'invio del messaggio");
                    running = 0;
                }
                break;
            }

            need_prompt = true;
        }

        // Gestione messaggi dal server
        if (FD_ISSET(sock, &read_fds))
        {
            messaggio_t *response = ricevi_messaggio(sock);
            if (!response)
            {
                printf("\nConnessione chiusa dal server\n");
                break;
            }

            switch (response->tipo)
            {
            case MSG_OK:
                printf("Successo: %s\n", response->payload ? response->payload : "");
                break;

            case MSG_ERR:
                printf("Errore: %s\n", response->payload ? response->payload : "");
                break;

            case MSG_MATRICE:
                printf("Matrice ricevuta:\n%s\n", response->payload ? response->payload : "");
                break;

            case MSG_SHOW_BACHECA:
                printf("Bacheca dei messaggi:\n");
                if (response->payload)
                {
                    char *token = strtok(response->payload, ",");
                    int index = 1;
                    while (token)
                    {
                        char *utente = token;
                        char *messaggio = strtok(NULL, ",");
                        if (messaggio)
                        {
                            printf("#%d %s: %s\n", index++, utente, messaggio);
                        }
                        token = strtok(NULL, ",");
                    }
                }
                break;

            case MSG_SERVER_SHUTDOWN:
                printf("Il server sta chiudendo. Chiusura del client in corso...\n");
                running = 0; // Imposta il flag per uscire dal loop
                break;

            default:
                printf("Risposta inaspettata dal server (tipo: %c)\n", response->tipo);
            }

            libera_messaggio(response);
            need_prompt = true;
        }

        if (need_prompt)
        {
            printf("%s", PROMPT);
            fflush(stdout);
            need_prompt = false;
        }
    }

    // Chiusura pulita
    chiudi_client(sock);
    return 0;
}