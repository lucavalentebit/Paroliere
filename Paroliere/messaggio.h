// messaggio.h
#ifndef MESSAGGIO_H
#define MESSAGGIO_H

#include <stddef.h>

#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'
#define MSG_SERVER_SHUTDOWN 'B'
#define MSG_POST_BACHECA 'H'
#define MSG_SHOW_BACHECA 'S'
#define MSG_CANCELLA_UTENTE 'D'
#define MSG_LOGIN_UTENTE 'L'

typedef struct {
    unsigned int lunghezza;  // lunghezza del payload
    char tipo;               // tipo di messaggio
    char* payload;           // dati effettivi
} messaggio_t;

// Invia un messaggio tramite socket
int invia_messaggio(int socket, char tipo, const char* payload);

// Ricevi un messaggio dal socket
messaggio_t* ricevi_messaggio(int socket);

// Libera la struttura del messaggio
void libera_messaggio(messaggio_t* msg);

#endif