#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "paroliere_bacheca.h"

#define MAX_MESSAGGI 8

// Array circolare per memorizzare i messaggi
static messaggio_bacheca_t bacheca[MAX_MESSAGGI];
static int indice_inserimento = 0;

// Mutex per la sincronizzazione
static pthread_mutex_t mutex_bacheca;

// Inizializza la bacheca e il mutex
int inizializza_bacheca() {
    memset(bacheca, 0, sizeof(bacheca));
    indice_inserimento = 0;
    if (pthread_mutex_init(&mutex_bacheca, NULL) != 0) {
        perror("Errore nell'inizializzazione del mutex");
        return -1;
    }
    return 0;
}

// Aggiunge un messaggio alla bacheca in modo thread-safe
int aggiungi_messaggio(const char *autore, const char *testo) {
    if (!autore || !testo) {
        return -1;
    }

    pthread_mutex_lock(&mutex_bacheca);

    // Copia l'autore e il testo nel messaggio corrente
    strncpy(bacheca[indice_inserimento].autore, autore, sizeof(bacheca[indice_inserimento].autore) - 1);
    bacheca[indice_inserimento].autore[sizeof(bacheca[indice_inserimento].autore) - 1] = '\0';

    strncpy(bacheca[indice_inserimento].testo, testo, sizeof(bacheca[indice_inserimento].testo) - 1);
    bacheca[indice_inserimento].testo[sizeof(bacheca[indice_inserimento].testo) - 1] = '\0';

    // Aggiorna l'indice di inserimento
    indice_inserimento = (indice_inserimento + 1) % MAX_MESSAGGI;

    pthread_mutex_unlock(&mutex_bacheca);
    return 0;
}

// Recupera i messaggi in formato CSV
int recupera_bacheca_csv(char *buffer, size_t size) {
    if (!buffer) {
        return -1;
    }

    pthread_mutex_lock(&mutex_bacheca);

    buffer[0] = '\0';
    int piena = 1;
    
    // Verifica se la bacheca è piena
    for (int i = 0; i < MAX_MESSAGGI; i++) {
        if (strlen(bacheca[i].autore) == 0 && strlen(bacheca[i].testo) == 0) {
            piena = 0;
            break;
        }
    }

    int count = piena ? MAX_MESSAGGI : indice_inserimento;
    
    // Si parte dall'ultimo messaggio inserito e andiamo indietro
    for (int i = 0; i < count; i++) {
        // Calcolo l'indice da cui partire
        int index = (indice_inserimento - 1 - i + MAX_MESSAGGI) % MAX_MESSAGGI;
        
        if (strlen(bacheca[index].autore) > 0 && strlen(bacheca[index].testo) > 0) {
            char temp[256];
            snprintf(temp, sizeof(temp), "%s,%s", 
                    bacheca[index].autore, 
                    bacheca[index].testo);
            
            if (strlen(buffer) + strlen(temp) + 2 < size) {
                if (strlen(buffer) > 0) {
                    strcat(buffer, ",");
                }
                strcat(buffer, temp);
            }
        }
    }

    pthread_mutex_unlock(&mutex_bacheca);
    return 0;
}

// Pulisce le risorse della bacheca
void pulisci_bacheca() {
    pthread_mutex_destroy(&mutex_bacheca);
    // Non è necessario liberare la memoria per l'array fisso
}