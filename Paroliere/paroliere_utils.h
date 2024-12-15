#ifndef PAROLIERE_UTILS_H
#define PAROLIERE_UTILS_H

#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include "paroliere_utenti.h"

#define MAX_USERNAME 50
#define MAX_LOGGED_USERS 100
#define MAX_CLIENTS 32
#define MAX_STORED_WORDS 100
#define MAX_WORD_LENGTH 17

// Struttura per i punteggi dei giocatori
typedef struct score_node
{
    char username[MAX_USERNAME];
    int score;
    struct score_node *next;
} score_node_t;

// Coda condivisa tra i thread client e lo scorer
typedef struct
{
    score_node_t *head;
    score_node_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
} score_queue_t;

// Variabile globale per la coda dei punteggi
extern score_queue_t score_queue;

// Funzioni per gestire la coda dei punteggi
void init_score_queue();
void enqueue_score(const char *username, int score);
score_node_t *dequeue_score();
void destroy_score_queue();

// Funzione per inviare un messaggio a tutti i client
void broadcast_message(char tipo, const char *payload);

// Funzione per ottenere il numero di client connessi
int get_number_of_clients();

// Struttura per i punti della partita
typedef struct
{
    int part_points;
} game_info_t;

// Struttura per le informazioni del client
typedef struct
{
    int client_fd;
    char username[MAX_USERNAME];
    char submitted_words[MAX_STORED_WORDS][MAX_WORD_LENGTH];
    int word_count;
    game_info_t game;
    bool score_submitted; // Flag per indicare se il punteggio è stato inviato
} client_info_t;

// Struttura per tracciare gli utenti loggati
typedef struct
{
    char username[MAX_USERNAME];
    bool in_use;
} logged_user_t;

// Struttura per tracciare i client connessi
typedef struct client_node
{
    client_info_t *client;
    struct client_node *next;
} client_node_t;

// Funzioni per la gestione degli utenti loggati
int aggiungi_utente_loggato(const char *username);
void rimuovi_utente_loggato(const char *username);
int utente_gia_loggato(const char *username);

// Variabili globali condivise
extern volatile sig_atomic_t shutdown_server;
extern pthread_mutex_t mutex_client_list;
extern pthread_mutex_t mutex_logged_users;
extern int server_fd;
extern pthread_mutex_t mutex_game;
extern pthread_cond_t cond_game_end;
extern bool is_paused;

// Funzioni per la gestione dei client connessi
void aggiungi_client(client_info_t *client);
void rimuovi_client(client_info_t *client);
void invia_shutdown_a_tutti();
void pulisci_client_list();

// Funzioni per la gestione delle parole proposte
int add_submitted_word(client_info_t *client, const char *word);
int is_word_submitted(client_info_t *client, const char *word);

// Funzioni per la gestione dei segnali
void handle_sigint(int sig);

// Funzioni per l'aggiornamento dei punti
int aggiorna_punti_utente(const char *username, int punti);

// Salva i punti aggiornati
int salva_punti();

//Funzione di logging
int log_event(const char *operation, const char *details);

#endif // PAROLIERE_UTILS_H