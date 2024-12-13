#ifndef PAROLIERE_MATRIX_H
#define PAROLIERE_MATRIX_H

// Struttura del nodo trie
typedef struct TrieNode {
    struct TrieNode *figli[26];  // array di puntatori per lettere a-z, 26 limite alfabeto
    int fine_parola;             // flag per indicare fine parola (1) o no (0)
} TrieNode;


//Funzione che libera la memoria allocata per il trie
void libera_trie(TrieNode *nodo);

// Genera matrice casuale
char* genera_matrice_stringa(int righe, int colonne);

// Cerca parola nella matrice e nel dizionario
int cerco(const char *parola);

// Carica dizionario nel trie
int carica_dizionario(const char *filename);

// Verifica parola nel dizionario usando trie
int parola_nel_dizionario(const char *parola);

#endif