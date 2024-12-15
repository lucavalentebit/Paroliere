#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "paroliere_matrix.h"

#define MATRIX_SIZE 4
#define MAX_WORD_LENGTH 17    // 16 caratteri + terminatore
#define MAX_MATRIXES_FILE 100 // Numero massimo di matrici in un file

// Matrice di gioco e visite
static int A[MATRIX_SIZE][MATRIX_SIZE] = {{0}};
static int visite[MATRIX_SIZE][MATRIX_SIZE] = {{0}};

// Array per le matrici caricate da file
static char *matrixes_file[MAX_MATRIXES_FILE];
static int numero_matrixes_file = 0;
static int indice_matrice_file = 0;

// Radice del trie
static TrieNode *radice = NULL;

// Crea nuovo nodo trie
TrieNode *crea_nodo()
{
    TrieNode *nodo = malloc(sizeof(TrieNode));
    if (nodo)
    {
        nodo->fine_parola = 0;
        for (int i = 0; i < 26; i++)
        {
            nodo->figli[i] = NULL;
        }
    }
    return nodo;
}

// Inserisce parola nel trie
void inserisci_parola(const char *parola)
{
    if (!radice)
    {
        radice = crea_nodo();
    }

    TrieNode *corrente = radice;
    for (int i = 0; parola[i]; i++)
    {
        int indice = tolower(parola[i]) - 'a';
        if (indice < 0 || indice >= 26)
            continue;

        if (!corrente->figli[indice])
        {
            corrente->figli[indice] = crea_nodo();
        }
        corrente = corrente->figli[indice];
    }
    corrente->fine_parola = 1;
}

// Cerca parola nel trie
int cerca_nel_trie(const char *parola)
{
    if (!radice)
        return 0;

    TrieNode *corrente = radice;
    for (int i = 0; parola[i]; i++)
    {
        int indice = tolower(parola[i]) - 'a';
        if (indice < 0 || indice >= 26)
            return 0;

        if (!corrente->figli[indice])
        {
            return 0;
        }
        corrente = corrente->figli[indice];
    }
    return corrente && corrente->fine_parola;
}

// Libera memoria del trie
void libera_trie(TrieNode *nodo)
{
    if (!nodo)
        return;

    for (int i = 0; i < 26; i++)
    {
        if (nodo->figli[i])
        {
            libera_trie(nodo->figli[i]);
        }
    }
    free(nodo);
}

int carica_dizionario(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Errore nell'apertura del dizionario");
        return -1;
    }

    char parola[MAX_WORD_LENGTH];
    while (fgets(parola, sizeof(parola), file))
    {
        parola[strcspn(parola, "\n")] = '\0';
        if (strlen(parola) <= 16)
        { // Verifica lunghezza massima
            inserisci_parola(parola);
        }
    }

    fclose(file);
    return 0;
}

int parola_nel_dizionario(const char *parola)
{
    if (strlen(parola) > 16)
        return 0;
    return cerca_nel_trie(parola);
}

int valido(int r, int c, int indice, const char *parola)
{
    return (r >= 0 && r < MATRIX_SIZE && // controllo riga
            c >= 0 && c < MATRIX_SIZE && // controllo colonna
            !visite[r][c] &&             // cella non visitata
            A[r][c] == parola[indice]);  // lettera corrispondente
}

int controllo(int r, int c, int indice, const char *parola)
{
    int length = strlen(parola);

    if (indice == length)
        return 1;
    if (!valido(r, c, indice, parola))
        return 0;

    visite[r][c] = 1; // marca visitata

    // Cerca in tutte le 8 direzioni
    int trovata = controllo(r + 1, c, indice + 1, parola) ||     // giù
                  controllo(r - 1, c, indice + 1, parola) ||     // su
                  controllo(r, c + 1, indice + 1, parola) ||     // destra
                  controllo(r, c - 1, indice + 1, parola) ||     // sinistra
                  controllo(r + 1, c + 1, indice + 1, parola) || // diagonale giù-destra
                  controllo(r - 1, c + 1, indice + 1, parola) || // diagonale su-destra
                  controllo(r + 1, c - 1, indice + 1, parola) || // diagonale giù-sinistra
                  controllo(r - 1, c - 1, indice + 1, parola);   // diagonale su-sinistra

    visite[r][c] = 0; // ripristina
    return trovata;
}

int cerco(const char *parola)
{
    if (!parola || strlen(parola) > 16)
        return 0;
    if (!parola_nel_dizionario(parola))
        return 0;

    for (int r = 0; r < MATRIX_SIZE; r++)
    {
        for (int c = 0; c < MATRIX_SIZE; c++)
        {
            if (controllo(r, c, 0, parola))
                return 1;
        }
    }
    return 0;
}

void imposta_seed_matrice(unsigned int seed)
{
    srand(seed);
}

/*Di seguito le funzioni per la gestione delle matrici, che vengono 
  sia generate casualmente che caricate da file:
  
    - genera_matrice_stringa: genera una matrice casuale di 4x4 caratteri
    - formatta_matrice_stringa: formatta la matrice in una stringa
    - converti_stringa_in_matrice: converte la stringa in una matrice
    - carica_matrici_da_file: carica le matrici da un file
    - ottieni_prossima_matrice: restituisce la prossima matrice da usare
    - libera_matrici: libera la memoria allocata per le matrici

*/

char *genera_matrice_stringa(int righe, int colonne)
{
    if (righe != MATRIX_SIZE || colonne != MATRIX_SIZE)
        return NULL;

    char *matrice = malloc((righe * (colonne * 2 + 1)) + 1);
    if (!matrice)
        return NULL;

    int pos = 0;
    for (int r = 0; r < righe; r++)
    {
        for (int c = 0; c < colonne; c++)
        {
            int char_val = (rand() % (122 - 97 + 1)) + 97; // Da 'a' a 'z'
            A[r][c] = char_val;                            // Aggiorna la matrice interna

            if (char_val == 113)
            { // 'q'
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


char *formatta_matrice_stringa(const char *input)
{
    if (!input)
        return NULL;

    // Alloca spazio per la matrice formattata (4 righe * (8 caratteri + newline) + null terminator)
    char *output = malloc(4 * 9 + 1);
    if (!output)
        return NULL;

    int input_pos = 0;
    int output_pos = 0;

    // Processa 4 righe
    for (int r = 0; r < 4; r++)
    {
        // Processa 4 colonne
        for (int c = 0; c < 4; c++)
        {
            if (input[input_pos] == 'q' || input[input_pos] == 'Q')
            {
                // Gestione speciale per Qu
                output[output_pos++] = input[input_pos++];
                output[output_pos++] = input[input_pos++];
            }
            else
            {
                output[output_pos++] = input[input_pos++];
            }
            // Aggiungi spazio dopo ogni carattere (o Qu)
            output[output_pos++] = ' ';
        }
        // Aggiungi newline alla fine di ogni riga
        output[output_pos++] = '\n';
    }

    output[output_pos] = '\0';
    return output;
}


void converti_stringa_in_matrice(const char *matrice_stringa)
{
    int r = 0, c = 0;
    int i = 0;

    while (matrice_stringa[i] != '\0' && r < MATRIX_SIZE)
    {
        if (matrice_stringa[i] == 'q' || matrice_stringa[i] == 'Q')
        {
            A[r][c] = tolower(matrice_stringa[i]);
            i += 2; // Salta 'u'/'U'
        }
        else
        {
            A[r][c] = tolower(matrice_stringa[i]);
            i++;
        }

        c++;
        if (c == MATRIX_SIZE)
        {
            c = 0;
            r++;
        }
    }
}


int carica_matrici_da_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Errore nell'apertura del file delle matrici");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file) && numero_matrixes_file < MAX_MATRIXES_FILE)
    {
        line[strcspn(line, "\n")] = '\0';

        // Verifica la lunghezza della matrice
        int len = strlen(line);
        int expected_len = 16; // Lunghezza standard

        // Conta le occorrenze di 'q' o 'Q' per gestire "Qu"
        for (int i = 0; i < len; i++)
        {
            if ((line[i] == 'q' || line[i] == 'Q') && i + 1 < len &&
                (line[i + 1] == 'u' || line[i + 1] == 'U'))
            {
                expected_len++;
                i++; // Salta la 'u'
            }
        }

        if (len == expected_len)
        {
            matrixes_file[numero_matrixes_file] = strdup(line);
            if (!matrixes_file[numero_matrixes_file])
            {
                fclose(file);
                return -1;
            }
            numero_matrixes_file++;
        }
    }

    fclose(file);
    return 0;
}

char *ottieni_prossima_matrice()
{
    if (numero_matrixes_file > 0)
    {
        char *matrice_raw = matrixes_file[indice_matrice_file];
        converti_stringa_in_matrice(matrice_raw);

        indice_matrice_file = (indice_matrice_file + 1) % numero_matrixes_file;
        return formatta_matrice_stringa(matrice_raw);
    }
    else
    {
        return genera_matrice_stringa(4, 4);
    }
}

void libera_matrici()
{
    for (int i = 0; i < numero_matrixes_file; i++)
    {
        free(matrixes_file[i]);
    }
    numero_matrixes_file = 0;
    indice_matrice_file = 0;
}
