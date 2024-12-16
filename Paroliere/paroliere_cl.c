#define _XOPEN_SOURCE 700 // Per sigaction
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>
#include <getopt.h>
#include <ctype.h>
#include "messaggio.h"

static volatile sig_atomic_t running = 1;
static const char PROMPT[] = "[PROMPT PAROLIERE] --> ";

void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

void chiudi_client(int sock)
{
    printf("Chiusura della connessione...\n");
    close(sock);
    printf("Client terminato.\n");
}

void mostra_aiuto()
{
    printf("Comandi disponibili:\n");
    printf("- aiuto: Mostra l'elenco dei comandi disponibili per il client e la loro sintassi.\n");
    printf("- registra_utente <nome_utente>: Registra un nuovo utente con il nome <nome_utente>. Il nome deve essere univoco e di massimo 10 caratteri alfanumerici.\n");
    printf("- cancella_utente <nome_utente>: Cancella un utente se esistente all'interno del db.\n");
    printf("- login_utente <nome_utente>: Permette di accedere ad un utente se pre-registrato.\n");
    printf("- matrice: Richiede la matrice di gioco al server.\n");
    printf("- p <parola>: Propone la parola <parola> al server.\n");
    printf("- msg <messaggio>: Pubblica un messaggio di massimo 128 caratteri sulla Bacheca.\n");
    printf("- show-msg: Mostra il contenuto della bacheca.\n");
    printf("- punti_finali: Richiede il punteggio totale dell'utente.\n");
    printf("- fine: Disconnette il client dal server e termina la connessione.\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("Errore nella configurazione di sigaction");
        exit(EXIT_FAILURE);
    }

    const char *server_ip = NULL;
    int port = 0;

    static struct option long_options[] = {
        {"server", required_argument, 0, 's'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}};

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "s:p:", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 's':
            server_ip = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Uso: %s --server <nome_server> --port <porta>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (!server_ip || port == 0)
    {
        fprintf(stderr, "Uso: %s --server <nome_server> --port <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        perror("Errore nella conversione dell'indirizzo IP");
        close(sock);
        exit(EXIT_FAILURE);
    }

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

    while (running)
    {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock, &read_fds);

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
            continue;

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

            char *command = strtok(input, " ");
            char *param = strtok(NULL, "");

            if (command)
            {
                if (strcmp(command, "registra_utente") == 0)
                {
                    if (param)
                    {
                        if (strlen(param) > 10)
                        {
                            printf("Errore: Il nome utente deve essere di massimo 10 caratteri alfanumerici.\n");
                        }
                        else
                        {
                            bool valido = true;
                            for (size_t i = 0; i < strlen(param); i++)
                            {
                                if (!isalnum(param[i]))
                                {
                                    valido = false;
                                    break;
                                }
                            }
                            if (!valido)
                            {
                                printf("Errore: Il nome utente deve contenere solo caratteri alfanumerici.\n");
                            }
                            else
                            {
                                if (invia_messaggio(sock, MSG_REGISTRA_UTENTE, param) < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    running = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        printf("Errore: Specificare il nome utente.\n");
                    }
                }
                else if (strcmp(command, "login_utente") == 0)
                {
                    if (param)
                    {
                        if (invia_messaggio(sock, MSG_LOGIN_UTENTE, param) < 0)
                        {
                            perror("Errore nell'invio del messaggio");
                            running = 0;
                        }
                    }
                    else
                    {
                        printf("Errore: Specificare il nome utente.\n");
                    }
                }
                else if (strcmp(command, "matrice") == 0)
                {
                    if (invia_messaggio(sock, MSG_MATRICE, "matrice") < 0)
                    {
                        perror("Errore nell'invio del messaggio");
                        running = 0;
                    }
                }
                else if (strcmp(command, "msg") == 0)
                {
                    if (param)
                    {
                        if (strlen(param) > 128)
                        {
                            printf("Errore: Il messaggio deve essere di massimo 128 caratteri.\n");
                        }
                        else
                        {
                            if (invia_messaggio(sock, MSG_POST_BACHECA, param) < 0)
                            {
                                perror("Errore nell'invio del messaggio");
                                running = 0;
                            }
                        }
                    }
                    else
                    {
                        printf("Errore: Specificare il messaggio.\n");
                    }
                }
                else if (strcmp(command, "show-msg") == 0)
                {
                    if (invia_messaggio(sock, MSG_SHOW_BACHECA, "show-msg") < 0)
                    {
                        perror("Errore nell'invio del messaggio");
                        running = 0;
                    }
                }
                else if (strcmp(command, "aiuto") == 0)
                {
                    mostra_aiuto();
                }
                else if (strcmp(command, "p") == 0)
                {
                    if (param)
                    {
                        if (invia_messaggio(sock, MSG_PAROLA, param) < 0)
                        {
                            perror("Errore nell'invio del messaggio");
                            running = 0;
                        }
                    }
                    else
                    {
                        printf("Errore: Specificare la parola da proporre.\n");
                    }
                }
                else if (strcmp(command, "punti_finali") == 0)
                {
                    if (invia_messaggio(sock, MSG_PUNTI_FINALI, "punti_finali") < 0)
                    {
                        perror("Errore nell'invio del messaggio");
                        running = 0;
                    }
                }
                else if (strcmp(command, "cancella_utente") == 0)
                {
                    if (param)
                    {
                        if (strlen(param) > 10)
                        {
                            printf("Errore: Il nome utente deve essere di massimo 10 caratteri.\n");
                        }
                        else
                        {
                            bool valido = true;
                            for (size_t i = 0; i < strlen(param); i++)
                            {
                                if (!isalnum(param[i]))
                                {
                                    valido = false;
                                    break;
                                }
                            }
                            if (!valido)
                            {
                                printf("Errore: Il nome utente deve contenere solo caratteri alfanumerici.\n");
                            }
                            else
                            {
                                if (invia_messaggio(sock, MSG_CANCELLA_UTENTE, param) < 0)
                                {
                                    perror("Errore nell'invio del messaggio");
                                    running = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        printf("Errore: Specificare il nome utente da cancellare.\n");
                    }
                }
                else if (strcmp(command, "aiuto") != 0 && strcmp(command, "registra_utente") != 0 && strcmp(command, "login_utente") != 0 && strcmp(command, "matrice") != 0 && strcmp(command, "msg") != 0 && strcmp(command, "show-msg") != 0 && strcmp(command, "p") != 0 && strcmp(command, "punti_finali") != 0 && strcmp(command, "cancella_utente") != 0)
                {
                    printf("Errore: Comando non riconosciuto. Digitare 'aiuto' per la lista dei comandi.\n");
                }
            }

            need_prompt = true;
        }

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

            case MSG_TEMPO_ATTESA:
                printf("La prossima partita inizierà tra %s secondi\n", response->payload ? response->payload : "");
                break;

            case MSG_PUNTI_PAROLA:
                if (response->payload && strcmp(response->payload, "0") != 0)
                {
                    printf("Punti assegnati: %s\n", response->payload);
                }
                else
                {
                    printf("Parola già proposta. Punti assegnati: 0\n");
                }
                break;

            case MSG_PUNTI_FINALI:
            {
                printf("\nClassifica finale:\n");
                char *token = strtok(response->payload, ",");
                int pos = 1;
                while (token != NULL)
                {
                    char *username = token;
                    token = strtok(NULL, ",");
                    if (!token)
                        break;
                    int score = atoi(token);
                    printf("%d° %s: %d punti\n", pos++, username, score);
                    token = strtok(NULL, ",");
                }
                break;
            }

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
                running = 0;
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

    chiudi_client(sock);
    return 0;
}