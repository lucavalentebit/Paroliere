#define _XOPEN_SOURCE 700 // Per sigaction, perché sigaction fa parte di POSIX.1-2008
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>
#include <semaphore.h>
#include <sys/time.h>
#include <ctype.h> // Per tolower

#include "messaggio.h"
#include "paroliere_matrix.h"
#include "paroliere_utenti.h"
#include "paroliere_bacheca.h"
#include "paroliere_utils.h"

#define BACKLOG 10

// Semaforo per limitare i client connessi
static sem_t client_sem;

// Variabili globali per la gestione del tempo
pthread_mutex_t mutex_game = PTHREAD_MUTEX_INITIALIZER;
bool is_paused = false;
static int durata_partita = 180; // Durata in secondi, default 3 minuti
static int tempo_residuo = 0;
static pthread_t tempo_thread;

// Variabile per il timeout in secondi
static int timeout = 0;

// Variabili globali per la gestione dei parametri seed e file matrici
static unsigned int seed_fornito = 0;
static char *file_matrici = NULL;

// Variabili globali per la matrice condivisa
static char *shared_matrix = NULL;
static pthread_mutex_t mutex_matrix = PTHREAD_MUTEX_INITIALIZER;

// Variabile globale per il socket del server
int server_fd;

// Thread scorer e condition variable per la fine partita
static pthread_t scorer_thread;
pthread_cond_t cond_game_end = PTHREAD_COND_INITIALIZER;

// Inizializza i punti della partita
void inizializza_game(client_info_t *client)
{
    client->game.part_points = 0;
}

void *scorer_function(void *arg)
{
    (void)arg;
    while (!shutdown_server)
    {
        // Attendi segnalazione fine partita
        pthread_mutex_lock(&mutex_game);
        while (!is_paused && !shutdown_server)
        {
            pthread_cond_wait(&cond_game_end, &mutex_game);
        }

        if (shutdown_server)
        {
            pthread_mutex_unlock(&mutex_game);
            break;
        }
        pthread_mutex_unlock(&mutex_game);

        // Acquisisci lock sulla lista client
        pthread_mutex_lock(&mutex_client_list);

        // Verifica presenza client connessi
        if (client_list == NULL)
        {
            printf("Debug: Nessun client connesso\n");
            pthread_mutex_unlock(&mutex_client_list);
            continue;
        }

        // Buffer per la classifica
        char *classifica = calloc(2048, sizeof(char));
        if (!classifica)
        {
            printf("Debug: Errore allocazione memoria classifica\n");
            pthread_mutex_unlock(&mutex_client_list);
            continue;
        }

        bool punteggi_validi = false;

        // Scansiona client e costruisci classifica
        client_node_t *client = client_list;
        while (client)
        {
            // Verifica validità client e username
            if (client->client && strlen(client->client->username) > 0)
            {
                int punti = client->client->game.part_points;

                // Aggiungi alla classifica se ha punti
                if (punti > 0)
                {
                    char entry[256];
                    snprintf(entry, sizeof(entry), "%s,%d,",
                             client->client->username, punti);

                    if (strlen(classifica) + strlen(entry) < 2047)
                    {
                        strcat(classifica, entry);
                        punteggi_validi = true;

                        // Aggiorna punteggi nel database
                        if (aggiorna_punti_utente(client->client->username, punti) == 0)
                        {
                            printf("Debug: Punti aggiornati per utente %s: %d\n", client->client->username, punti);
                        }
                        else
                        {
                            printf("Debug: Errore aggiornamento punti per utente %s\n", client->client->username);
                        }
                    }
                }

                // Reset stato client
                client->client->game.part_points = 0;
                client->client->word_count = 0;
                client->client->score_submitted = true;
            }
            client = client->next;
        }

        pthread_mutex_unlock(&mutex_client_list);

        // Invia classifica se ci sono punteggi
        if (punteggi_validi && strlen(classifica) > 0)
        {
            printf("Debug: Invio classifica: %s\n", classifica);
            broadcast_message(MSG_PUNTI_FINALI, classifica);
            salva_punti();
        }
        else
        {
            printf("Debug: Nessun punteggio da inviare\n");
        }

        free(classifica);

        // Segnala fine elaborazione punteggi
        pthread_mutex_lock(&mutex_game);
        is_paused = false;                   // Reset flag pausa
        pthread_cond_signal(&cond_game_end); // Segnala al thread temporale
        pthread_mutex_unlock(&mutex_game);
    }
    return NULL;
}

// Funzione per gestire la durata delle partite e le pause
void *gestione_temporale(void *arg)
{
    (void)arg;
    while (!shutdown_server)
    {
        // Attendi 5 secondi prima di pulire la classifica
        sleep(5);

        // Pulizia della classifica
        printf("Debug: Avvio della pulizia della classifica.\n");
        pulisci_classifica();
        printf("Debug: Pulizia della classifica completata.\n");

        // Inizio della partita
        pthread_mutex_lock(&mutex_game);
        is_paused = false;
        tempo_residuo = durata_partita;

        // Genera nuova matrice
        pthread_mutex_lock(&mutex_matrix);
        if (shared_matrix != NULL)
        {
            free(shared_matrix);
        }
        shared_matrix = ottieni_prossima_matrice();
        printf("Debug: shared_matrix ottenuta: %s\n", shared_matrix);
        pthread_mutex_unlock(&mutex_matrix);

        pthread_mutex_unlock(&mutex_game);

        // Countdown durata partita
        while (tempo_residuo > 0 && !shutdown_server)
        {
            sleep(1); // Aspetta un secondo
            pthread_mutex_lock(&mutex_game);
            tempo_residuo--;
            pthread_mutex_unlock(&mutex_game);
        }

        if (shutdown_server)
            break;

        // Fine della partita
        pthread_mutex_lock(&mutex_game);
        is_paused = true;
        
        // Attendiamo che lo scorer finisca di elaborare i punteggi
        pthread_cond_signal(&cond_game_end);
        pthread_cond_wait(&cond_game_end, &mutex_game);
        
        pthread_mutex_unlock(&mutex_game);

        // Inizio della pausa
        pthread_mutex_lock(&mutex_game);
        tempo_residuo = 60; // Pausa fra le partite
        pthread_mutex_unlock(&mutex_game);

        // Countdown pausa
        while (tempo_residuo > 0 && !shutdown_server)
        {
            sleep(1);
            pthread_mutex_lock(&mutex_game);
            tempo_residuo--;
            pthread_mutex_unlock(&mutex_game);
        }
    }
    return NULL;
}

void *handle_client(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    int client_fd = client->client_fd;
    char current_username[MAX_USERNAME] = "";

    if (timeout > 0)
    {
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            perror("Errore nell'impostazione del timeout sul socket");
        }
    }

    // Inizializza la lista delle parole proposte e i punti
    client->word_count = 0;
    inizializza_game(client);

    aggiungi_client(client);

    while (1)
    {
        messaggio_t *msg = ricevi_messaggio(client_fd);
        if (!msg)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                printf("Client %s disconnesso per inattività dopo %d minuti\n",
                       current_username[0] ? current_username : "non autenticato",
                       timeout / 60);
                if (current_username[0])
                    rimuovi_utente_loggato(current_username);
                break;
            }
            printf("Client %s disconnesso\n",
                   current_username[0] ? current_username : "non autenticato");
            if (current_username[0])
                rimuovi_utente_loggato(current_username);
            break;
        }

        pthread_mutex_lock(&mutex_game);
        bool paused = is_paused;
        int tempo = tempo_residuo;
        pthread_mutex_unlock(&mutex_game);

        printf("Ricevuto messaggio %s - Tipo: %c, Payload: %s\n",
               current_username[0] ? current_username : "non autenticato",
               msg->tipo,
               msg->payload ? msg->payload : "vuoto");

        // Durante la pausa, limita i messaggi accettati
        if (paused && msg->tipo != MSG_REGISTRA_UTENTE && msg->tipo != MSG_MATRICE)
        {
            invia_messaggio(client_fd, MSG_ERR, "Server in pausa. Riprova più tardi.");
            libera_messaggio(msg);
            continue;
        }

        switch (msg->tipo)
        {
        case MSG_REGISTRA_UTENTE:
            if (registra_utente(msg->payload) == 0)
            {
                strncpy(current_username, msg->payload, MAX_USERNAME - 1);
                current_username[MAX_USERNAME - 1] = '\0';
                invia_messaggio(client_fd, MSG_OK, "Utente registrato con successo");
            }
            else
            {
                invia_messaggio(client_fd, MSG_ERR, "Username già in uso");
            }
            break;

        case MSG_LOGIN_UTENTE:
            if (current_username[0] != '\0')
            {
                invia_messaggio(client_fd, MSG_ERR, "Già autenticato");
                break;
            }
            if (utente_registrato(msg->payload))
            {
                if (utente_gia_loggato(msg->payload))
                {
                    invia_messaggio(client_fd, MSG_ERR, "Utente già loggato");
                }
                else
                {
                    strncpy(current_username, msg->payload, MAX_USERNAME - 1);
                    current_username[MAX_USERNAME - 1] = '\0';
                    aggiungi_utente_loggato(current_username);
                    invia_messaggio(client_fd, MSG_OK, "Login effettuato");
                }
            }
            else
            {
                invia_messaggio(client_fd, MSG_ERR, "Utente non trovato");
            }
            break;

        case MSG_MATRICE:
            if (paused)
            {
                char tempo_str[16];
                snprintf(tempo_str, sizeof(tempo_str), "%d", tempo);
                invia_messaggio(client_fd, MSG_TEMPO_ATTESA, tempo_str);
            }
            else
            {
                pthread_mutex_lock(&mutex_matrix);
                if (shared_matrix)
                {
                    char buffer[1024];
                    snprintf(buffer, sizeof(buffer), "%s\nTempo rimanente: %d secondi", shared_matrix, tempo);
                    invia_messaggio(client_fd, MSG_MATRICE, buffer);
                }
                else
                {
                    invia_messaggio(client_fd, MSG_ERR, "Matrice non disponibile");
                }
                pthread_mutex_unlock(&mutex_matrix);
            }
            break;

        case MSG_PAROLA:
            if (paused)
            {
                invia_messaggio(client_fd, MSG_ERR, "Server in pausa. Riprova più tardi.");
                break;
            }
            if (current_username[0] == '\0')
            {
                invia_messaggio(client_fd, MSG_ERR, "Autenticazione richiesta");
                break;
            }
            if (!msg->payload || strlen(msg->payload) < 4)
            {
                invia_messaggio(client_fd, MSG_ERR, "La parola deve contenere almeno 4 caratteri");
                break;
            }
            if (strlen(msg->payload) > 16)
            {
                invia_messaggio(client_fd, MSG_ERR, "Parola troppo lunga");
                break;
            }
            if (!cerco(msg->payload))
            {
                invia_messaggio(client_fd, MSG_ERR, "Parola non valida o non presente nella matrice");
                break;
            }
            if (is_word_submitted(client, msg->payload))
            {
                invia_messaggio(client_fd, MSG_PUNTI_PAROLA, "0");
                break;
            }
            if (add_submitted_word(client, msg->payload) != 0)
            {
                invia_messaggio(client_fd, MSG_ERR, "Limite parole raggiunto");
                break;
            }
            {
                // Calcola i punti
                int punti = 0;
                for (size_t i = 0; i < strlen(msg->payload); i++)
                {
                    if (tolower(msg->payload[i]) == 'q' && i + 1 < strlen(msg->payload) && tolower(msg->payload[i + 1]) == 'u')
                    {
                        punti += 1; // La coppia 'qu' conta come una lettera
                        i++;        // Salta la 'u'
                    }
                    else
                    {
                        punti += 1;
                    }
                }

                // Aggiorna i punti della partita
                client->game.part_points += punti;

                // Aggiorna i punti cumulativi
                aggiorna_punti_utente(current_username, punti);

                // Invia i punti assegnati
                char punti_str[16];
                snprintf(punti_str, sizeof(punti_str), "%d", punti);
                invia_messaggio(client_fd, MSG_PUNTI_PAROLA, punti_str);

                // Log dell'evento di parola inviata
                char details[256];
                snprintf(details, sizeof(details), "Utente: %s Parola: %s", current_username, msg->payload);
                log_event("PAROLA_INVIATA", details);

                // Log dell'evento di punteggio assegnato
                snprintf(details, sizeof(details), "Utente: %s Punteggio: %d", current_username, punti);
                log_event("PUNTEGGIO", details);
            }
            break;

        case MSG_POST_BACHECA:
            if (paused)
            {
                invia_messaggio(client_fd, MSG_ERR, "Server in pausa. Riprova più tardi.");
                break;
            }
            if (current_username[0] == '\0')
            {
                invia_messaggio(client_fd, MSG_ERR, "Autenticazione richiesta");
                break;
            }
            if (strlen(msg->payload) > 128)
            {
                invia_messaggio(client_fd, MSG_ERR, "Messaggio troppo lungo");
                break;
            }
            if (aggiungi_messaggio(current_username, msg->payload) == 0)
            {
                invia_messaggio(client_fd, MSG_OK, "Messaggio pubblicato con successo");
            }
            else
            {
                invia_messaggio(client_fd, MSG_ERR, "Errore nella pubblicazione del messaggio");
            }
            break;

        case MSG_SHOW_BACHECA:
        {
            char buffer[1024];
            if (recupera_bacheca_csv(buffer, sizeof(buffer)) == 0)
            {
                invia_messaggio(client_fd, MSG_SHOW_BACHECA, buffer);
            }
            else
            {
                invia_messaggio(client_fd, MSG_ERR, "Errore nel recupero della bacheca");
            }
        }
        break;

        case MSG_PUNTI_FINALI:
            if (current_username[0] == '\0')
            {
                invia_messaggio(client_fd, MSG_ERR, "Autenticazione richiesta");
                break;
            }
            break;

        case MSG_CANCELLA_UTENTE:
            if (current_username[0] == '\0')
            {
                invia_messaggio(client_fd, MSG_ERR, "Autenticazione richiesta");
                break;
            }
            if (cancella_utente(msg->payload) == 0)
            {
                invia_messaggio(client_fd, MSG_OK, "Utente cancellato con successo");
            }
            else
            {
                invia_messaggio(client_fd, MSG_ERR, "Errore nella cancellazione utente");
            }
            break;

        case MSG_SERVER_SHUTDOWN:
            invia_messaggio(client_fd, MSG_OK, "Shutdown ricevuto");
            break;

        default:
            invia_messaggio(client_fd, MSG_ERR, "Comando non riconosciuto");
            break;
        }

        libera_messaggio(msg);
    }

    rimuovi_client(client);
    close(client_fd);
    free(client);

    // Libera uno slot nel semaforo
    sem_post(&client_sem);

    return NULL;
}

int main(int argc, char *argv[])
{
    // Configura il gestore del segnale SIGINT
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("Errore nella configurazione di sigaction");
        exit(EXIT_FAILURE);
    }

    int port = 0;

    // Opzioni per getopt_long
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"durata", required_argument, 0, 'd'},
        {"seed", required_argument, 0, 's'},
        {"matrici", required_argument, 0, 'm'},
        {"diz", required_argument, 0, 'z'},
        {"disconnetti-dopo", required_argument, 0, 't'},
        {0, 0, 0, 0}};

    // Valori di default
    int opt;
    int option_index = 0;
    unsigned int seed = 0;
    char *dictionary_file = "dictionary_ita.txt";

    while ((opt = getopt_long(argc, argv, "p:d:s:m:z:", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            durata_partita = atoi(optarg) * 60;
            break;
        case 's':
            seed = (unsigned int)atoi(optarg);
            seed_fornito = 1;
            break;
        case 'm':
            file_matrici = strdup(optarg);
            break;
        case 'z':
            dictionary_file = strdup(optarg);
            break;
        case 't':
            timeout = atoi(optarg) * 60;
            break;
        default:
            fprintf(stderr, "Uso: %s --port <porta> [--matrici <data_filename>] [--durata <minuti>] [--seed <rnd_seed>] [--diz <nome_dizionario.txt>]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Inizializza seed e caricamento matrici
    if (seed_fornito)
    {
        imposta_seed_matrice(seed);
    }
    else
    {
        imposta_seed_matrice((unsigned int)time(NULL));
    }

    if (file_matrici)
    {
        if (carica_matrici_da_file(file_matrici) != 0)
        {
            fprintf(stderr, "Errore nel caricamento delle matrici dal file\n");
            exit(EXIT_FAILURE);
        }
    }

    if (port == 0)
    {
        fprintf(stderr, "Uso: %s --port <porta> [--durata <minuti>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Carica utenti, dizionario e inizializza la bacheca
    if (carica_utenti() != 0)
    {
        fprintf(stderr, "Errore nel caricamento degli utenti\n");
        exit(EXIT_FAILURE);
    }

    if (carica_dizionario(dictionary_file) != 0)
    {
        fprintf(stderr, "Errore nel caricamento del dizionario\n");
        exit(EXIT_FAILURE);
    }

    if (inizializza_bacheca() != 0)
    {
        fprintf(stderr, "Errore nell'inizializzazione della bacheca\n");
        exit(EXIT_FAILURE);
    }

    // Inizializza il semaforo con valore MAX_CLIENTS
    if (sem_init(&client_sem, 0, MAX_CLIENTS) != 0)
    {
        perror("Errore nell'inizializzazione del semaforo");
        exit(EXIT_FAILURE);
    }

    int server_fd;
    struct sockaddr_in server_addr;

    // Creazione del socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Errore nella creazione del socket");
        sem_destroy(&client_sem);
        exit(EXIT_FAILURE);
    }

    // Impostazione delle opzioni del socket
    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        perror("Errore setsockopt");
        close(server_fd);
        sem_destroy(&client_sem);
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Binding del socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Errore nel binding");
        close(server_fd);
        sem_destroy(&client_sem);
        exit(EXIT_FAILURE);
    }

    // Ascolto delle connessioni in arrivo
    if (listen(server_fd, BACKLOG) == -1)
    {
        perror("Errore nella listen");
        close(server_fd);
        sem_destroy(&client_sem);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto sulla porta %d...\n", port);

    // Creazione del thread scorer
    if (pthread_create(&scorer_thread, NULL, scorer_function, NULL) != 0)
    {
        perror("Errore nella creazione del thread scorer");
        close(server_fd);
        sem_destroy(&client_sem);
        exit(EXIT_FAILURE);
    }

    // Creazione del thread di gestione temporale
    if (pthread_create(&tempo_thread, NULL, gestione_temporale, NULL) != 0)
    {
        perror("Errore nella creazione del thread di gestione temporale");
        close(server_fd);
        sem_destroy(&client_sem);
        exit(EXIT_FAILURE);
    }

    // Ciclo principale del server
    while (!shutdown_server)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (!client_info)
        {
            perror("Errore allocazione memoria");
            continue;
        }

        // Accettazione di una nuova connessione
        client_info->client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_info->client_fd == -1)
        {
            if (shutdown_server || errno == EINTR)
            {
                free(client_info);
                break; // Esci dal ciclo se shutdown o interrupt
            }
            perror("Errore accept");
            free(client_info);
            continue;
        }

        // Verifica del numero di client connessi usando il semaforo
        if (sem_trywait(&client_sem) != 0)
        {
            printf("Rifiutata connessione da %s:%d - Server pieno.\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
            invia_messaggio(client_info->client_fd, MSG_ERR, "Server pieno. Riprova più tardi.");
            close(client_info->client_fd);
            free(client_info);
            continue;
        }

        client_info->username[0] = '\0';
        client_info->word_count = 0; // Inizializza il conteggio delle parole

        printf("Nuova connessione da %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Creazione di un thread per gestire il client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_info) != 0)
        {
            perror("Errore creazione thread");
            close(client_info->client_fd);
            free(client_info);
            sem_post(&client_sem); // Libera il semaforo in caso di errore
            continue;
        }
        pthread_detach(thread);
    }
    if (file_matrici)
    {
        free(file_matrici);
        libera_matrici();
    }

    free(dictionary_file);

    printf("Shutdown del server in corso...\n");
    // Invia messaggio di shutdown a tutti i client
    invia_shutdown_a_tutti();

    // Pulisce la lista dei client
    pulisci_client_list();

    // Pulisce altre risorse
    pulisci_bacheca();

    libera_trie(NULL); // Assicurati che 'radice' sia accessibile
    pthread_mutex_destroy(&mutex_client_list);
    pthread_mutex_destroy(&mutex_logged_users);

    // Attesa della terminazione del thread temporale
    pthread_join(tempo_thread, NULL);

    // Attesa terminazione thread scorer
    pthread_cond_destroy(&cond_game_end);

    // Distrugge il semaforo
    sem_destroy(&client_sem);

    // Libera la matrice condivisa
    free(shared_matrix);
    pthread_mutex_destroy(&mutex_matrix);

    close(server_fd);
    printf("Server terminato correttamente.\n");
    return 0;
}