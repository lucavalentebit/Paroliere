// messaggio.c
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "messaggio.h"

int invia_messaggio(int socket, char tipo, const char* payload) {
    unsigned int lunghezza = payload ? strlen(payload) : 0;
    
    // Invia lunghezza
    if (write(socket, &lunghezza, sizeof(lunghezza)) != sizeof(lunghezza))
        return -1;
        
    // Invia tipo
    if (write(socket, &tipo, sizeof(tipo)) != sizeof(tipo))
        return -1;
        
    // Invia payload se presente
    if (lunghezza > 0) {
        if (write(socket, payload, lunghezza) != lunghezza)
            return -1;
    }
    
    return 0;
}

messaggio_t* ricevi_messaggio(int socket) {
    messaggio_t* msg = malloc(sizeof(messaggio_t));
    if (!msg) return NULL;
    
    // Leggi lunghezza
    if (read(socket, &msg->lunghezza, sizeof(msg->lunghezza)) != sizeof(msg->lunghezza)) {
        free(msg);
        return NULL;
    }
    
    // Leggi tipo
    if (read(socket, &msg->tipo, sizeof(msg->tipo)) != sizeof(msg->tipo)) {
        free(msg);
        return NULL;
    }
    
    // Leggi payload se presente
    if (msg->lunghezza > 0) {
        msg->payload = malloc(msg->lunghezza + 1);
        if (!msg->payload) {
            free(msg);
            return NULL;
        }
        
        if (read(socket, msg->payload, msg->lunghezza) != msg->lunghezza) {
            free(msg->payload);
            free(msg);
            return NULL;
        }
        msg->payload[msg->lunghezza] = '\0';
    } else {
        msg->payload = NULL;
    }
    
    return msg;
}

void libera_messaggio(messaggio_t* msg) {
    if (msg) {
        free(msg->payload);
        free(msg);
    }
}