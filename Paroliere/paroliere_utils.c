#include "paroliere_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "messaggio.h"

// Variabili globali
static logged_user_t logged_users[MAX_LOGGED_USERS];
pthread_mutex_t mutex_logged_users = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

static client_node_t *client_list = NULL;
pthread_mutex_t mutex_client_list = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t shutdown_server = 0;

// Variabile globale per la coda dei punteggi
score_queue_t score_queue;

// Inizializza la coda dei punteggi
void init_score_queue()
{
    score_queue.head = NULL;
    score_queue.tail = NULL;
    score_queue.count = 0;
    pthread_mutex_init(&score_queue.mutex, NULL);
    pthread_cond_init(&score_queue.cond, NULL);
}

// Inserisce un punteggio nella coda
void enqueue_score(const char *username, int score)
{
    score_node_t *node = malloc(sizeof(score_node_t));
    if (!node)
        return;

    strncpy(node->username, username, MAX_USERNAME - 1);
    node->username[MAX_USERNAME - 1] = '\0';
    node->score = score;
    node->next = NULL;

    pthread_mutex_lock(&score_queue.mutex);
    if (score_queue.tail)
    {
        score_queue.tail->next = node;
    }
    else
    {
        score_queue.head = node;
    }
    score_queue.tail = node;
    score_queue.count++;
    pthread_cond_signal(&score_queue.cond);
    pthread_mutex_unlock(&score_queue.mutex);
}

// Estrae un punteggio dalla coda
score_node_t *dequeue_score()
{
    pthread_mutex_lock(&score_queue.mutex);
    while (score_queue.count == 0 && !shutdown_server)
    {
        pthread_cond_wait(&score_queue.cond, &score_queue.mutex);
    }

    if (shutdown_server)
    {
        pthread_mutex_unlock(&score_queue.mutex);
        return NULL;
    }

    score_node_t *node = score_queue.head;
    if (node)
    {
        score_queue.head = node->next;
        if (!score_queue.head)
        {
            score_queue.tail = NULL;
        }
        score_queue.count--;
    }
    pthread_mutex_unlock(&score_queue.mutex);
    return node;
}

// Distrugge la coda dei punteggi
void destroy_score_queue()
{
    pthread_mutex_lock(&score_queue.mutex);
    score_node_t *current = score_queue.head;
    while (current)
    {
        score_node_t *temp = current;
        current = current->next;
        free(temp);
    }
    pthread_mutex_unlock(&score_queue.mutex);
    pthread_mutex_destroy(&score_queue.mutex);
    pthread_cond_destroy(&score_queue.cond);
}

// Funzione per inviare un messaggio a tutti i client
void broadcast_message(char tipo, const char *payload)
{
    pthread_mutex_lock(&mutex_client_list);
    client_node_t *curr = client_list;
    while (curr)
    {
        invia_messaggio(curr->client->client_fd, tipo, payload);
        curr = curr->next;
    }
    pthread_mutex_unlock(&mutex_client_list);
}

// Funzione per ottenere il numero di client connessi
int get_number_of_clients()
{
    int count = 0;
    pthread_mutex_lock(&mutex_client_list);
    client_node_t *curr = client_list;
    while (curr)
    {
        count++;
        curr = curr->next;
    }
    pthread_mutex_unlock(&mutex_client_list);
    return count;
}

// Funzioni per la gestione degli utenti loggati
int aggiungi_utente_loggato(const char *username)
{
    pthread_mutex_lock(&mutex_logged_users);
    for (int i = 0; i < MAX_LOGGED_USERS; i++)
    {
        if (!logged_users[i].in_use)
        {
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

void rimuovi_utente_loggato(const char *username)
{
    pthread_mutex_lock(&mutex_logged_users);
    for (int i = 0; i < MAX_LOGGED_USERS; i++)
    {
        if (logged_users[i].in_use && strcmp(logged_users[i].username, username) == 0)
        {
            logged_users[i].in_use = false;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_logged_users);
}

int utente_gia_loggato(const char *username)
{
    pthread_mutex_lock(&mutex_logged_users);
    for (int i = 0; i < MAX_LOGGED_USERS; i++)
    {
        if (logged_users[i].in_use && strcmp(logged_users[i].username, username) == 0)
        {
            pthread_mutex_unlock(&mutex_logged_users);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_logged_users);
    return 0;
}

// Funzioni per la gestione dei client connessi
void aggiungi_client(client_info_t *client)
{
    client_node_t *node = malloc(sizeof(client_node_t));
    if (!node)
        return;

    node->client = client;
    node->next = NULL;

    pthread_mutex_lock(&mutex_client_list);
    node->next = client_list;
    client_list = node;
    pthread_mutex_unlock(&mutex_client_list);
}

void rimuovi_client(client_info_t *client)
{
    pthread_mutex_lock(&mutex_client_list);
    client_node_t *prev = NULL;
    client_node_t *curr = client_list;
    while (curr)
    {
        if (curr->client == client)
        {
            if (prev)
                prev->next = curr->next;
            else
                client_list = curr->next;
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&mutex_client_list);
}

void invia_shutdown_a_tutti()
{
    pthread_mutex_lock(&mutex_client_list);
    client_node_t *curr = client_list;
    while (curr)
    {
        invia_messaggio(curr->client->client_fd, MSG_SERVER_SHUTDOWN, "Server in chiusura");
        close(curr->client->client_fd);
        curr = curr->next;
    }
    pthread_mutex_unlock(&mutex_client_list);
}

void pulisci_client_list()
{
    pthread_mutex_lock(&mutex_client_list);
    client_node_t *curr = client_list;
    while (curr)
    {
        client_node_t *temp = curr;
        curr = curr->next;
        close(temp->client->client_fd);
        free(temp->client);
        free(temp);
    }
    client_list = NULL;
    pthread_mutex_unlock(&mutex_client_list);
}

// Funzioni per la gestione delle parole proposte
int add_submitted_word(client_info_t *client, const char *word)
{
    if (client->word_count >= MAX_STORED_WORDS)
        return -1; // Limite raggiunto

    strncpy(client->submitted_words[client->word_count], word, MAX_WORD_LENGTH - 1);
    client->submitted_words[client->word_count][MAX_WORD_LENGTH - 1] = '\0';
    client->word_count++;
    return 0;
}

int is_word_submitted(client_info_t *client, const char *word)
{
    for (int i = 0; i < client->word_count; i++)
    {
        if (strcmp(client->submitted_words[i], word) == 0)
            return 1; // Parola già proposta
    }
    return 0;
}

// Funzioni per la gestione dei segnali
void handle_sigint(int sig) {
    (void)sig;
    const char msg[] = "\nRicevuto segnale di interruzione. Chiusura del server in corso...\n";
    write(STDOUT_FILENO, msg, strlen(msg)); // write = signal safe
    
    shutdown_server = 1;
    
    // Sveglia il thread scorer
    pthread_mutex_lock(&mutex_game);
    pthread_cond_signal(&cond_game_end);
    pthread_mutex_unlock(&mutex_game);
    
    // Chiudi il socket principale per interrompere accept()
    if (server_fd > 0) {
        close(server_fd);
    }
}

// Funzioni per la gestione dei punti utente
int aggiorna_punti_utente(const char *username, int punti)
{
    return aggiungi_punti(username, punti);
}

int salva_punti()
{
    return salva_punti_utenti();
}

// Funzione per registrare eventi nel log
int log_event(const char *operation, const char *details) {
    time_t now;
    struct tm *tm_info;
    char timestamp[20];
    FILE *f;
    
    // Usa fprintf invece di snprintf per il logging
    pthread_mutex_lock(&mutex_log);
    
    f = fopen("paroliere.log", "a");
    if (!f) {
        pthread_mutex_unlock(&mutex_log);
        return -1;
    }

    now = time(NULL);
    tm_info = localtime(&now);
    if (!tm_info) {
        fclose(f);
        pthread_mutex_unlock(&mutex_log);
        return -1;
    }

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", tm_info);
    
    fprintf(f, "%s,%s,%s\n", timestamp, operation, details);
    fflush(f);  // Forza il flush del buffer
    fclose(f);
    
    pthread_mutex_unlock(&mutex_log);
    return 0;
}