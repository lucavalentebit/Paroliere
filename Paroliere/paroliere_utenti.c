// paroliere_utenti.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "paroliere_utils.h"
#include "paroliere_utenti.h"

#define USERS_FILE "./users.txt"

static pthread_mutex_t mutex_utenti = PTHREAD_MUTEX_INITIALIZER;
static utente_t utenti_registrati[MAX_USERS];
static int num_utenti = 0;

// Carica gli utenti dal file all'avvio del server
int carica_utenti()
{
    FILE *file = fopen(USERS_FILE, "a+");
    if (!file)
    {
        perror("Errore nell'apertura del file utenti");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) > 0)
        {
            char *token = strtok(line, ":");
            if (token)
            {
                strncpy(utenti_registrati[num_utenti].username, token, 10);
                utenti_registrati[num_utenti].username[10] = '\0';
                token = strtok(NULL, ":");
                utenti_registrati[num_utenti].punti_totali = token ? atoi(token) : 0;
                num_utenti++;
                if (num_utenti >= MAX_USERS)
                    break;
            }
        }
    }

    fclose(file);
    return 0;
}

// Registra un nuovo utente
int registra_utente(const char *username)
{
    pthread_mutex_lock(&mutex_utenti);

    // Verifica se l'utente è già registrato
    for (int i = 0; i < num_utenti; i++)
    {
        if (strcmp(utenti_registrati[i].username, username) == 0)
        {
            printf("Utente '%s' già registrato.\n", username);
            pthread_mutex_unlock(&mutex_utenti);
            return -1;
        }
    }

    if (num_utenti >= MAX_USERS)
    {
        printf("Numero massimo di utenti raggiunto.\n");
        pthread_mutex_unlock(&mutex_utenti);
        return -1;
    }

    // Aggiungi l'utente alla lista in memoria con punti iniziali a 0
    strncpy(utenti_registrati[num_utenti].username, username, 10);
    utenti_registrati[num_utenti].username[10] = '\0';
    utenti_registrati[num_utenti].punti_totali = 0;
    num_utenti++;
    printf("Utente '%s' aggiunto con punti iniziali a 0.\n", username);

    // Scrivi l'utente nel file
    FILE *file = fopen(USERS_FILE, "a");
    if (!file)
    {
        perror("Errore nell'apertura del file utenti");
        pthread_mutex_unlock(&mutex_utenti);
        return -1;
    }

    fprintf(file, "%s:0\n", username);
    fclose(file);
    printf("Utente '%s' scritto nel file con punti iniziali a 0.\n", username);

    pthread_mutex_unlock(&mutex_utenti);

    // Log dell'evento di registrazione utente
    char details[128];
    snprintf(details, sizeof(details), "Nome utente: %s", username);
    log_event("REGISTRA_UTENTE", details);

    return 0;

    return 0;
}

// Verifica se un utente è registrato
int utente_registrato(const char *username)
{
    pthread_mutex_lock(&mutex_utenti);

    for (int i = 0; i < num_utenti; i++)
    {
        if (strcmp(utenti_registrati[i].username, username) == 0)
        {
            pthread_mutex_unlock(&mutex_utenti);
            return 1;
        }
    }

    pthread_mutex_unlock(&mutex_utenti);
    return 0;
}

// Ottiene i punti totali di un utente
int get_punti_totali(const char *username)
{
    pthread_mutex_lock(&mutex_utenti);

    for (int i = 0; i < num_utenti; i++)
    {
        if (strcmp(utenti_registrati[i].username, username) == 0)
        {
            int punti = utenti_registrati[i].punti_totali;
            pthread_mutex_unlock(&mutex_utenti);
            return punti;
        }
    }

    pthread_mutex_unlock(&mutex_utenti);
    return 0;
}

// Aggiunge punti a un utente
int aggiungi_punti(const char *username, int punti)
{
    pthread_mutex_lock(&mutex_utenti);

    for (int i = 0; i < num_utenti; i++)
    {
        if (strcmp(utenti_registrati[i].username, username) == 0)
        {
            utenti_registrati[i].punti_totali += punti;
            pthread_mutex_unlock(&mutex_utenti);
            return 0;
        }
    }

    pthread_mutex_unlock(&mutex_utenti);
    return -1;
}

// Resetta i punti totali di tutti gli utenti prima di una nuova partita
void reset_punti_totali()
{
    pthread_mutex_lock(&mutex_utenti);
    for (int i = 0; i < num_utenti; i++)
    {
        utenti_registrati[i].punti_totali = 0;
    }
    pthread_mutex_unlock(&mutex_utenti);
}

// Salva i punti cumulativi nel file
int salva_punti_utenti()
{
    pthread_mutex_lock(&mutex_utenti);

    FILE *file = fopen(USERS_FILE, "w");
    if (!file)
    {
        perror("Errore nell'apertura del file utenti per il salvataggio");
        pthread_mutex_unlock(&mutex_utenti);
        return -1;
    }

    for (int i = 0; i < num_utenti; i++)
    {
        fprintf(file, "%s:%d\n", utenti_registrati[i].username, utenti_registrati[i].punti_totali);
    }

    fclose(file);
    pthread_mutex_unlock(&mutex_utenti);
    return 0;
}

// Cancella un utente
int cancella_utente(const char *username)
{
    pthread_mutex_lock(&mutex_utenti);

    int found = -1;
    for (int i = 0; i < num_utenti; i++)
    {
        if (strcmp(utenti_registrati[i].username, username) == 0)
        {
            found = i;
            break;
        }
    }

    if (found == -1)
    {
        pthread_mutex_unlock(&mutex_utenti);
        return -1; // Utente non trovato
    }

    // Rimuovi l'utente dalla lista in memoria
    for (int i = found; i < num_utenti - 1; i++)
    {
        utenti_registrati[i] = utenti_registrati[i + 1];
    }
    num_utenti--;

    // Riscrivi il file users.txt senza l'utente eliminato
    FILE *file = fopen(USERS_FILE, "w");
    if (!file)
    {
        perror("Errore nella scrittura del file utenti");
        pthread_mutex_unlock(&mutex_utenti);
        return -1;
    }

    for (int i = 0; i < num_utenti; i++)
    {
        fprintf(file, "%s:%d\n", utenti_registrati[i].username, utenti_registrati[i].punti_totali);
    }

    fclose(file);
    pthread_mutex_unlock(&mutex_utenti);

    // Log dell'evento di cancellazione utente
    char details[128];
    snprintf(details, sizeof(details), "Nome utente: %s", username);
    log_event("CANCELLA_UTENTE", details);

    return 0;
}