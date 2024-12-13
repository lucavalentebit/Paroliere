// utenti.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "paroliere_utenti.h"

#define USERS_FILE "./users.txt"

static pthread_mutex_t mutex_utenti = PTHREAD_MUTEX_INITIALIZER;
static char utenti_registrati[MAX_USERS][11];
static int num_utenti = 0;

// Carica gli utenti dal file all'avvio del server
int carica_utenti()
{
    FILE *file = fopen(USERS_FILE, "a+"); // DEBUG -- Cambia "r" in "a+" per creare se non esiste
    if (!file)
    {
        perror("Errore nell'apertura del file utenti");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = '\0'; // Rimuove il carattere di nuova linea
        if (strlen(line) > 0)
        {
            strncpy(utenti_registrati[num_utenti], line, 10);
            utenti_registrati[num_utenti][10] = '\0';
            num_utenti++;
        }
    }

    fclose(file);
    return 0;
}

// Modifica della funzione registra_utente per aggiungere messaggi di debug
int registra_utente(const char *username)
{
    pthread_mutex_lock(&mutex_utenti);

    // Verifica se l'utente è già registrato
    for (int i = 0; i < num_utenti; i++)
    {
        if (strcmp(utenti_registrati[i], username) == 0)
        {
            printf("Utente '%s' già registrato.\n", username);
            pthread_mutex_unlock(&mutex_utenti);
            return -1; // Utente già registrato
        }
    }

    // Aggiungi l'utente alla lista in memoria
    strncpy(utenti_registrati[num_utenti], username, 10);
    utenti_registrati[num_utenti][10] = '\0';
    num_utenti++;
    printf("Utente '%s' aggiunto alla lista in memoria.\n", username);

    // Scrivi l'utente nel file
    FILE *file = fopen(USERS_FILE, "a");
    if (!file)
    {
        perror("Errore nell'apertura del file utenti");
        pthread_mutex_unlock(&mutex_utenti);
        return -1;
    }
    printf("File '%s' aperto correttamente.\n", USERS_FILE);

    fprintf(file, "%s\n", username);
    fclose(file);
    printf("Utente '%s' scritto nel file.\n", username);

    pthread_mutex_unlock(&mutex_utenti);
    return 0;
}

// Verifica se un utente è registrato
int utente_registrato(const char *username)
{
    pthread_mutex_lock(&mutex_utenti);

    for (int i = 0; i < num_utenti; i++)
    {
        if (strcmp(utenti_registrati[i], username) == 0)
        {
            pthread_mutex_unlock(&mutex_utenti);
            return 1; // Utente trovato
        }
    }

    pthread_mutex_unlock(&mutex_utenti);
    return 0; // Utente non trovato
}