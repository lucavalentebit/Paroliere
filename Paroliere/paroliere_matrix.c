#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "paroliere_matrix.h"

#define MATRIX_SIZE 4
#define MAX_WORD_LENGTH 17  // 16 caratteri + terminatore

// Matrice di gioco e visite
static int A[MATRIX_SIZE][MATRIX_SIZE] = {{0}};
static int visite[MATRIX_SIZE][MATRIX_SIZE] = {{0}};

// Radice del trie
static TrieNode *radice = NULL;

// Crea nuovo nodo trie
TrieNode* crea_nodo() {
    TrieNode *nodo = malloc(sizeof(TrieNode));
    if (nodo) {
        nodo->fine_parola = 0;
        for (int i = 0; i < 26; i++) {
            nodo->figli[i] = NULL;
        }
    }
    return nodo;
}

// Inserisce parola nel trie
void inserisci_parola(const char *parola) {
    if (!radice) {
        radice = crea_nodo();
    }
    
    TrieNode *corrente = radice;
    for (int i = 0; parola[i]; i++) {
        int indice = tolower(parola[i]) - 'a';
        if (indice < 0 || indice >= 26) continue;
        
        if (!corrente->figli[indice]) {
            corrente->figli[indice] = crea_nodo();
        }
        corrente = corrente->figli[indice];
    }
    corrente->fine_parola = 1;
}

// Cerca parola nel trie
int cerca_nel_trie(const char *parola) {
    if (!radice) return 0;
    
    TrieNode *corrente = radice;
    for (int i = 0; parola[i]; i++) {
        int indice = tolower(parola[i]) - 'a';
        if (indice < 0 || indice >= 26) return 0;
        
        if (!corrente->figli[indice]) {
            return 0;
        }
        corrente = corrente->figli[indice];
    }
    return corrente && corrente->fine_parola;
}

// Libera memoria del trie
void libera_trie(TrieNode *nodo) {
    if (!nodo) return;
    
    for (int i = 0; i < 26; i++) {
        if (nodo->figli[i]) {
            libera_trie(nodo->figli[i]);
        }
    }
    free(nodo);
}

int carica_dizionario(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Errore nell'apertura del dizionario");
        return -1;
    }

    char parola[MAX_WORD_LENGTH];
    while (fgets(parola, sizeof(parola), file)) {
        parola[strcspn(parola, "\n")] = '\0';
        if (strlen(parola) <= 16) {  // Verifica lunghezza massima
            inserisci_parola(parola);
        }
    }

    fclose(file);
    return 0;
}

int parola_nel_dizionario(const char *parola) {
    if (strlen(parola) > 16) return 0;
    return cerca_nel_trie(parola);
}

int valido(int r, int c, int indice, const char *parola) {
    return (r >= 0 && r < MATRIX_SIZE &&  // controllo riga
            c >= 0 && c < MATRIX_SIZE &&  // controllo colonna
            !visite[r][c] &&              // cella non visitata
            A[r][c] == parola[indice]);   // lettera corrispondente
}

int controllo(int r, int c, int indice, const char *parola) {
    int length = strlen(parola);
    
    if (indice == length) return 1;
    if (!valido(r, c, indice, parola)) return 0;

    visite[r][c] = 1;  // marca visitata

    // Cerca in tutte le 8 direzioni
    int trovata = controllo(r+1, c, indice+1, parola) ||   // giù
                 controllo(r-1, c, indice+1, parola) ||   // su
                 controllo(r, c+1, indice+1, parola) ||   // destra
                 controllo(r, c-1, indice+1, parola) ||   // sinistra
                 controllo(r+1, c+1, indice+1, parola) || // diagonale giù-destra
                 controllo(r-1, c+1, indice+1, parola) || // diagonale su-destra
                 controllo(r+1, c-1, indice+1, parola) || // diagonale giù-sinistra
                 controllo(r-1, c-1, indice+1, parola);   // diagonale su-sinistra

    visite[r][c] = 0;  // ripristina
    return trovata;
}

int cerco(const char *parola) {
    if (!parola || strlen(parola) > 16) return 0;
    if (!parola_nel_dizionario(parola)) return 0;

    for (int r = 0; r < MATRIX_SIZE; r++) {
        for (int c = 0; c < MATRIX_SIZE; c++) {
            if (controllo(r, c, 0, parola)) return 1;
        }
    }
    return 0;
}

char *genera_matrice_stringa(int righe, int colonne) {
    if (righe != MATRIX_SIZE || colonne != MATRIX_SIZE) return NULL;

    char *matrice = malloc((righe * (colonne * 2 + 1)) + 1);
    if (!matrice) return NULL;

    srand(time(NULL));
    
    const int min = 97;  // 'a'
    const int max = 122; // 'z'
    
    int pos = 0;
    for (int r = 0; r < righe; r++) {
        for (int c = 0; c < colonne; c++) {
            int char_val = (rand() % (max - min + 1)) + min;
            A[r][c] = char_val;
            
            if (char_val == 113) { // 'q'
                matrice[pos++] = 'q';
                matrice[pos++] = 'u';
                matrice[pos++] = ' ';
                continue;
            }
            
            matrice[pos++] = (char)char_val;
            matrice[pos++] = ' ';
        }
        matrice[pos++] = '\n';
    }
    matrice[pos] = '\0';
    
    return matrice;
}