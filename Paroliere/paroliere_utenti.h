// paroliere_utenti.h
#ifndef UTENTI_H
#define UTENTI_H

#define MAX_USERS 100

typedef struct {
    char username[11];
    int punti_totali;
} utente_t;

// Carica gli utenti dal file all'avvio del server
int carica_utenti();

// Registra un nuovo utente 
int registra_utente(const char *username);

// Cancella un utente dal sistema
int cancella_utente(const char *username);

// Verifica se un utente è registrato
int utente_registrato(const char *username);

// Ottiene i punti totali di un utente
int get_punti_totali(const char *username);

// Aggiunge punti a un utente
int aggiungi_punti(const char *username, int punti);

// Salva i punti cumulativi nel file
int salva_punti_utenti();

#endif // UTENTI_H