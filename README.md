# Paroliere

Gioco di parole multiplayer a turni, ispirato al classico gioco da tavolo. I giocatori si connettono a un server comune e devono trovare quante più parole italiane possibili all'interno di una matrice 4x4 di lettere, prima che scada il tempo.

Le parole vengono cercate seguendo lettere adiacenti (orizzontali, verticali o diagonali). Ogni parola valida assegna un punteggio pari alla sua lunghezza. Le parole devono avere almeno 4 caratteri, essere presenti nel dizionario italiano incluso nel progetto, e formare un percorso continuo nella matrice.

## Requisiti

- GCC
- Make
- Sistema POSIX (Linux, macOS)

## Compilazione

```bash
cd Paroliere
make
```

Vengono prodotti due eseguibili: `paroliere_srv` (server) e `paroliere_cl` (client).

## Avvio

**Server:**

```bash
./paroliere_srv --port <porta> [--durata <secondi>] [--matrici <file>] [--seed <n>] [--disconnetti-dopo <secondi>]
```

| Opzione | Descrizione | Default |
|---|---|---|
| `--port` | Porta di ascolto (obbligatoria) | - |
| `--durata` | Durata di ogni round in secondi | 180 |
| `--matrici` | File con matrici predefinite | - |
| `--seed` | Seme per la generazione casuale delle matrici | - |
| `--disconnetti-dopo` | Disconnette i client dopo N secondi di inattività | - |

**Client:**

```bash
./paroliere_cl --server <host> --port <porta>
```

## Comandi del client

| Comando | Descrizione |
|---|---|
| `registra_utente <nome>` | Registra un nuovo utente |
| `login_utente <nome>` | Effettua il login |
| `matrice` | Richiede la matrice corrente |
| `p <parola>` | Propone una parola |
| `punti_finali` | Mostra la classifica del round |
| `msg <testo>` | Pubblica un messaggio in bacheca |
| `show-msg` | Visualizza i messaggi in bacheca |
| `cancella_utente <nome>` | Cancella l'account |
| `fine` | Disconnette il client |

## Struttura del progetto

```
Paroliere/
├── paroliere_srv.c      # Server: gestione connessioni, logica di gioco, timer
├── paroliere_cl.c       # Client: interfaccia a riga di comando
├── paroliere_matrix.c   # Generazione matrici, ricerca parole, dizionario (Trie)
├── paroliere_utenti.c   # Gestione utenti e persistenza
├── paroliere_bacheca.c  # Bacheca messaggi
├── paroliere_utils.c    # Utilità condivise
├── messaggio.c          # Protocollo di comunicazione
└── dictionary_ita.txt   # Dizionario italiano (~49.000 parole)
```
