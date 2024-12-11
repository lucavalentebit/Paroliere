#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

int A[4][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
int visite[4][4];
int valido(int c, int r, int indice, const char *parola)
{
    return (c >= 0 && c < 4 && // controllo range
            r >= 0 && r < 4 &&
            !visite[c][r] && A[c][r] == parola[indice]); // controllo se non ho visitato quella cella e se è valida
}

int controllo(int c, int r, int indice, const char *parola)
{

    int length = strlen(parola);

    if (indice == length)
    {
        return 1; // ho trovato la parola
    }
    if (!valido(c, r, indice, parola))
    {
        return 0; // non trovato
    }

    visite[c][r] = 1; // ho visitato

    int found = controllo(c + 1, r, indice + 1, parola) ||     // dx
                controllo(c - 1, r, indice + 1, parola) ||     // sx
                controllo(c, r + 1, indice + 1, parola) ||     // su
                controllo(c, r - 1, indice + 1, parola) ||     // giù
                controllo(c + 1, r + 1, indice + 1, parola) || // giù dx
                controllo(c + 1, r - 1, indice + 1, parola) || // su dx
                controllo(c - 1, r + 1, indice + 1, parola) || // giù sx
                controllo(c - 1, r - 1, indice + 1, parola);   // su sx

    visite[c][r] = 0; // ripristino

    return found;
}

int cerco(const char *parola)
{
    for (int c = 0; c < 4; c++)
    {
        for (int r = 0; r < 4; r++)
        {
            if (controllo(c, r, 0, parola))
            {
                return 1;
            }
        }
    }
    return 0;
}

void creazioneMatrice()
{
    int min = 97, max = 122; // codice ascii dei caratteri in minuscolo
    srand(time(NULL));       // cambio seed di generazione casuale basato sul tempo attuale
    for (int r = 0; r < 4; r++)
    {
        for (int c = 0; c < 4; c++)
        {
            A[r][c] = (rand() % (max - min + 1)) + min;
            if (A[r][c] == 113)
            {
                printf("%c%c  ", (char)A[r][c], (char)117); // mi assicuro che nella visualizzazione q e u siano insieme
                continue;
            }
            printf("%c   ", (char)A[r][c]); // stampo i caratteri in ascii
        }
        printf("\n");
    }
}

void stampaparola(const char *parola)
{
    printf("Parola cercata: ");
    for (int i = 0; i < strlen(parola); i++)
    {
        printf("%c", parola[i]); // Stampa ogni carattere
    }
    printf("\n");
}

int main()
{
    creazioneMatrice();
    char *parola = NULL;
    size_t len = 0;
    printf("Inserisci la parola da cercare: ");
    if (getline(&parola, &len, stdin) == -1)
    {
        perror("Errore nella lettura della parola");
        exit(EXIT_FAILURE);
    }
    parola[strcspn(parola, "\n")] = '\0';// rimozione carattere di nuova linea
    stampaparola(parola);
    if (cerco(parola))
    {
        printf("\ntrovato\n");
    }
    else
        printf("\nnon trovato\n");
}