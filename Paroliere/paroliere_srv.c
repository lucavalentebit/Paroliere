#define _XOPEN_SOURCE 700 // Per sigaction, perché sigaction fa parte di POSIX.1-2008
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include "messaggio.h"
#include "paroliere_matrix.h"
#include "paroliere_utenti.h"
#include "paroliere_bacheca.h"

#define BACKLOG 10
#define MAX_USERNAME 50
#define MAX_LOGGED_USERS 100

// Struttura per le informazioni del client
typedef struct {
    int client_fd;
    char username[MAX_USERNAME];
} client_info_t;

// Struttura per tracciare gli utenti loggati
typedef struct {
    char username[MAX_USERNAME];
    bool in_use;
} logged_user_t;

// Struttura per tracciare i client connessi
typedef struct client_node {
    client_info_t *client;
    struct client_node *next;
} client_node_t;

// Variabili globali
static logged_user_t logged_users[MAX_LOGGED_USERS];
static pthread_mutex_t mutex_logged_users = PTHREAD_MUTEX_INITIALIZER;

static client_node_t *client_list = NULL;
static pthread_mutex_t mutex_client_list = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t shutdown_server = 0;

// Funzione per aggiungere un utente alla lista degli utenti loggati
int aggiungi_utente_loggato(const char *username) {
    pthread_mutex_lock(&mutex_logged_users);
    for (int i = 0; i < MAX_LOGGED_USERS; i++) {
        if (!logged_users[i].in_use) {
            strncpy(logged_users[i].username, username, MAX_USERNAME - 1);
            logged_users[i].username[MAX_USERNAME - 1] = '\0';
            logged_users[i].in_use = true;
            pthread_mutex_unlock(&mutex_logged_users);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex_logged_users);
    return -1; // Lista piena
}

// Funzione per rimuovere un utente dalla lista degli utenti loggati
void rimuovi_utente_loggato(const char *username) {
    pthread_mutex_lock(&mutex_logged_users);
    for (int i = 0; i < MAX_LOGGED_USERS; i++) {
        if (logged_users[i].in_use && strcmp(logged_users[i].username, username) == 0) {
            logged_users[i].in_use = false;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_logged_users);
}

// Funzione per verificare se un utente è già loggato
int utente_gia_loggato(const char *username) {
    pthread_mutex_lock(&mutex_logged_users);
    for (int i = 0; i < MAX_LOGGED_USERS; i++) {
        if (logged_users[i].in_use && strcmp(logged_users[i].username, username) == 0) {
            pthread_mutex_unlock(&mutex_logged_users);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_logged_users);
    return 0;
}

// Funzione per aggiungere un client alla lista dei client connessi
void aggiungi_client(client_info_t *client) {
    client_node_t *node = malloc(sizeof(client_node_t));
    if (!node) return;

    node->client = client;
    node->next = NULL;

    pthread_mutex_lock(&mutex_client_list);
    node->next = client_list;
    client_list = node;
    pthread_mutex_unlock(&mutex_client_list);
}

// Funzione per rimuovere un client dalla lista dei client connessi
void rimuovi_client(client_info_t *client) {
    pthread_mutex_lock(&mutex_client_list);
    client_node_t *prev = NULL;
    client_node_t *curr = client_list;
    while (curr) {
        if (curr->client == client) {
            if (prev) {
                prev->next = curr->next;
            } else {
                client_list = curr->next;
            }
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&mutex_client_list);
}

// Funzione per inviare MSG_SERVER_SHUTDOWN a tutti i client
void invia_shutdown_a_tutti() {
    pthread_mutex_lock(&mutex_client_list);
    client_node_t *curr = client_list;
    while (curr) {
        invia_messaggio(curr->client->client_fd, MSG_SERVER_SHUTDOWN, "Server in chiusura");
        close(curr->client->client_fd);
        curr = curr->next;
    }
    pthread_mutex_unlock(&mutex_client_list);
}

// Handler per il segnale SIGINT
void handle_sigint(int sig) {
    shutdown_server = 1;
}

// Funzione per pulire la lista dei client
void pulisci_client_list() {
    pthread_mutex_lock(&mutex_client_list);
    client_node_t *curr = client_list;
    while (curr) {
        client_node_t *temp = curr;
        curr = curr->next;
        close(temp->client->client_fd);
        free(temp->client);
        free(temp);
    }
    client_list = NULL;
    pthread_mutex_unlock(&mutex_client_list);
}

// Funzione per gestire ogni connessione client
void *handle_client(void *arg) {
    client_info_t *client = (client_info_t *)arg;
    int client_fd = client->client_fd;
    char *current_username = client->username;

    aggiungi_client(client);

    while (1) {
        messaggio_t *msg = ricevi_messaggio(client_fd);
        if (!msg) {
            printf("Client %s disconnesso\n",
                   current_username[0] ? current_username : "non autenticato");
            if (current_username[0]) {
                rimuovi_utente_loggato(current_username);
            }
            break;
        }

        printf("Ricevuto messaggio %s - Tipo: %c, Payload: %s\n",
               current_username[0] ? current_username : "non autenticato",
               msg->tipo,
               msg->payload ? msg->payload : "vuoto");

        switch (msg->tipo) {
            case MSG_REGISTRA_UTENTE:
                if (registra_utente(msg->payload) == 0) {
                    strncpy(current_username, msg->payload, MAX_USERNAME - 1);
                    current_username[MAX_USERNAME - 1] = '\0';
                    invia_messaggio(client_fd, MSG_OK, "Utente registrato con successo");
                } else {
                    invia_messaggio(client_fd, MSG_ERR, "Username già in uso");
                }
                break;

            case MSG_LOGIN_UTENTE:
                if (current_username[0] != '\0') {
                    invia_messaggio(client_fd, MSG_ERR, "Già autenticato");
                    break;
                }
                if (utente_registrato(msg->payload)) {
                    if (utente_gia_loggato(msg->payload)) {
                        invia_messaggio(client_fd, MSG_ERR, "Utente già loggato");
                    } else {
                        strncpy(current_username, msg->payload, MAX_USERNAME - 1);
                        current_username[MAX_USERNAME - 1] = '\0';
                        aggiungi_utente_loggato(current_username);
                        invia_messaggio(client_fd, MSG_OK, "Login effettuato");
                    }
                } else {
                    invia_messaggio(client_fd, MSG_ERR, "Utente non trovato");
                }
                break;

            case MSG_MATRICE:
                if (current_username[0] == '\0') {
                    invia_messaggio(client_fd, MSG_ERR, "Autenticazione richiesta");
                    break;
                }
                {
                    char *matrice = genera_matrice_stringa(4, 4);
                    if (matrice) {
                        invia_messaggio(client_fd, MSG_MATRICE, matrice);
                        free(matrice);
                    } else {
                        invia_messaggio(client_fd, MSG_ERR, "Errore generazione matrice");
                    }
                }
                break;

            case MSG_PAROLA:
                if (current_username[0] == '\0') {
                    invia_messaggio(client_fd, MSG_ERR, "Autenticazione richiesta");
                    break;
                }
                if (strlen(msg->payload) > 16) {
                    invia_messaggio(client_fd, MSG_ERR, "Parola troppo lunga");
                    break;
                }
                if (cerco(msg->payload)) {
                    invia_messaggio(client_fd, MSG_OK, "Parola valida");
                } else {
                    invia_messaggio(client_fd, MSG_ERR, "Parola non valida o non presente nel dizionario");
                }
                break;

            case MSG_SERVER_SHUTDOWN:
                invia_messaggio(client_fd, MSG_OK, "Shutdown ricevuto");
                break;

            default:
                invia_messaggio(client_fd, MSG_ERR, "Comando non riconosciuto");
                break;
        }

        libera_messaggio(msg);
    }

    rimuovi_client(client);
    close(client_fd);
    free(client);
    return NULL;
}

int main(int argc, char *argv[]) {
    // Configura il gestore del segnale SIGINT
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Errore nella configurazione di sigaction");
        exit(EXIT_FAILURE);
    }

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Carica utenti e dizionario
    if (carica_utenti() != 0) {
        fprintf(stderr, "Errore nel caricamento degli utenti\n");
        exit(EXIT_FAILURE);
    }

    if (carica_dizionario("dictionary_ita.txt") != 0) {
        fprintf(stderr, "Errore nel caricamento del dizionario\n");
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int server_fd;
    struct sockaddr_in server_addr;

    // Creazione del socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    // Impostazione delle opzioni del socket
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Errore setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Binding del socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nel binding");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Ascolto delle connessioni in arrivo
    if (listen(server_fd, BACKLOG) == -1) {
        perror("Errore nella listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto sulla porta %d...\n", port);

    // Ciclo principale del server
    while (!shutdown_server) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (!client_info) {
            perror("Errore allocazione memoria");
            continue;
        }

        // Accettazione di una nuova connessione
        client_info->client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_info->client_fd == -1) {
            if (shutdown_server) {
                free(client_info);
                break;
            }
            perror("Errore accept");
            free(client_info);
            continue;
        }

        client_info->username[0] = '\0';

        printf("Nuova connessione da %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Creazione di un thread per gestire il client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_info) != 0) {
            perror("Errore creazione thread");
            close(client_info->client_fd);
            free(client_info);
            continue;
        }
        pthread_detach(thread);
    }

    printf("Shutdown del server in corso...\n");
    // Invia messaggio di shutdown a tutti i client
    invia_shutdown_a_tutti();

    // Pulisce la lista dei client
    pulisci_client_list();

    // Pulisce altre risorse
    pulisci_bacheca();
    libera_trie(NULL); // Assicurati che 'radice' sia accessibile
    pthread_mutex_destroy(&mutex_client_list);
    pthread_mutex_destroy(&mutex_logged_users);

    close(server_fd);
    printf("Server terminato correttamente.\n");
    return 0;
}