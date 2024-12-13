#ifndef BACHECA_H
#define BACHECA_H

#include <pthread.h>

// Definizione della struttura del messaggio nella bacheca
typedef struct
{
    char autore[11]; // fino a 10 caratteri + null terminatore
    char testo[129]; // Testo del messaggio (fino a 128 caratteri + null terminatore)
} messaggio_bacheca_t;

// Inizializza la bacheca
int inizializza_bacheca();

// Aggiunge un messaggio alla bacheca
int aggiungi_messaggio(const char *autore, const char *testo);

// Recupera i messaggi della bacheca in formato CSV
int recupera_bacheca_csv(char *buffer, size_t size);

// Pulisce le risorse della bacheca
void pulisci_bacheca();

#endif // BACHECA_H