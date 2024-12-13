// utenti.h
#ifndef UTENTI_H
#define UTENTI_H

#define MAX_USERS 100

// Carica gli utenti dal file all'avvio del server
int carica_utenti();

// Registra un nuovo utente 
int registra_utente(const char *username);

// Verifica se un utente è registrato
int utente_registrato(const char *username);

#endif // UTENTI_H