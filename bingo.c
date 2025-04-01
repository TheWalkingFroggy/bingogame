#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define N_ROWS 3
#define N_COLS 5
#define SIZE_CARD 15
#define TOTAL_NUMBERS 75

typedef struct {
    int card[SIZE_CARD];
    int* randomValues;
    int lastNumber;
    bool cinquina;
    bool endGame;
    int counter_for_starting; // mi serve se voglio risolvere il problema usando solo DUE semafori
    int counter_for_dealer;

    pthread_mutex_t mutex;
    sem_t player;
    sem_t dealer;

} shared_data;

typedef struct {
    pthread_t tid;
    int id;
    int** myCards;
    bool** myCheckCards;
    int numberOfPlayers;
    int numberOfCards;
    shared_data* sh;

} player_data;

typedef struct {
    pthread_t tid;
    int id;
    int numberOfPlayers;
    int numberOfCards;
    shared_data* sh;

} dealer_data;

int* createCard() {
    // crea dinamicamente un array di 75 indici, in ordine crescente
    int* arr = malloc(sizeof(int) * TOTAL_NUMBERS);
    for(int i=0; i<TOTAL_NUMBERS; i++) arr[i] = i+1;

    // mescola i valori (in sostanza questo metodo "simula" la generazione casuali di valori univoci da 1 a 75)
    for(int i=TOTAL_NUMBERS-1; i > 0; i--) {
        int j = rand() % (i+1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
    return arr;
}

void printCard(int* arr, int size) {
    for(int i=0; i<size; i++) {
        if(i % 5 == 0 && i !=0) printf("/ ");
        printf("%d ", arr[i]);
        if(i == size-1) printf(")\n");
    }
}

int checkCinquina(bool arr[]) {
    if((arr[0] && arr[1] && arr[2] && arr[3] && arr[4]) == true) return 1;
    if((arr[5] && arr[6] && arr[7] && arr[8] && arr[9]) == true) return 2;
    if((arr[10] && arr[11] && arr[12] && arr[13] && arr[14]) == true) return 3;
}

int checkTombola(bool arr[]) {
    for(int i=0; i<SIZE_CARD; i++) {
        if(arr[i] == false) {
            return 0;
        }
    }
    return 4;
}

void initShared(shared_data* sh) {
    pthread_mutex_init(&sh->mutex, NULL);
    sem_init(&sh->dealer, 0, 1);
    sem_init(&sh->player, 0, 0);
}

void freeShared(shared_data* sh) {
    pthread_mutex_destroy(&sh->mutex);
    sem_destroy(&sh->dealer);
    sem_destroy(&sh->player);
    free(sh->randomValues);
}

void* dealerFunction(void* args) {
    dealer_data* dd = (dealer_data*) args;
    int numPlayers = dd->numberOfPlayers;
    int numCards = dd->numberOfCards;
    srand(time(NULL));

    for(int i=0; i<(numPlayers*numCards); i++) {
        sem_wait(&dd->sh->dealer);
        dd->sh->randomValues = createCard();            // genera un array casuale con 75 numeri
        for(int i=0; i<SIZE_CARD; i++) {                // inserisci i primi 15 valori in una cartellina
            dd->sh->card[i] = dd->sh->randomValues[i];
        }
        sem_post(&dd->sh->player);                      // sveglia un player: dovrà copiare i dati nella sua cartellina
    }

    // --- SE IL DEALER ARRIVA QUI, HA DISTRIBUITO TUTTE LE CARTELLINE ED INIZIA LA PARTITA! ---
    sleep(1);
    printf("\nIl dealer ha finito le distribuzioni delle cartelline...\n");
    printf("[D] Inizio l'estrazione dei numeri...\n");

    // codice per estrazione (prepara l'array da 75 valori univoci)
    int arr[TOTAL_NUMBERS];
    for (int i=0; i<TOTAL_NUMBERS; i++) {
        arr[i] = i+1;
    }
    for (int i=TOTAL_NUMBERS-1; i>0; i--) {
        int j = rand() % (i+1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
    // fine codice per estrazione

    int i = 0;
    while(dd->sh->endGame == false && i<TOTAL_NUMBERS) {
        sem_wait(&dd->sh->dealer);

        // dato che il dealer si rimette subito in attesa ad inizio ciclo, potrebbe capitare che estrae un ultimo numero prima di terminare
        // la partita nonostante un thread player abbia ottenuto tombola, allora rimetto un controllo per uscire dal while...
        if(dd->sh->endGame == true) break;               
                                                          
        printf("[D] Numero estratto: %d\n", arr[i]);
        dd->sh->lastNumber = arr[i];
        i++;
        if(i==TOTAL_NUMBERS) printf("[D] Dovrei aver estratto tutti i numeri...\n");
        sleep(1);
    
        // mando tot sveglie pari al numero dei player, che controlleranno il numero appena estratto
        // se è presente nella propria cartellina: non ho race condition
        for (int i=0; i<numPlayers; i++) sem_post(&dd->sh->player);
        printf("[D] Aspetto esito dai partecipanti...\n");

    }
    for (int i=0; i<numPlayers-1; i++) sem_post(&dd->sh->player); // evito possibili problemi di sync post-tombola
    printf("[D] FINE DEL GIOCO!\n");
    return NULL;
}

void* playerFunction(void* args) {
    player_data* pd = (player_data*) args;

    int prize;
    int counter_for_dealer;

    // Alloca dinamicamente un array di puntatori
    pd->myCards = (int**)malloc(sizeof(int*) * pd->numberOfCards);
    for(int i=0; i<pd->numberOfCards; i++) {
        pd->myCards[i] = (int*)malloc(sizeof(int) * SIZE_CARD);
    }

    pd->myCheckCards = (bool**)malloc(sizeof(bool*) * pd->numberOfCards);
    for(int i=0; i<pd->numberOfCards; i++) pd->myCheckCards[i] = (bool*)malloc(sizeof(bool) * SIZE_CARD);

    for(int i=0; i<pd->numberOfCards; i++) {
        sem_wait(&pd->sh->player);

        for(int j=0; j<SIZE_CARD; j++) {
            pd->myCards[i][j] = pd->sh->card[j];
        }

        printf("[P%d]: ricevuta card n%d con numeri: ( ", pd->id+1, i+1);
        printCard(pd->myCards[i], SIZE_CARD);
        sem_post(&pd->sh->dealer);
    }

    printf("[P%d]: Ho ricevuto tutte le cartelline.\n", pd->id+1);
    pthread_mutex_lock(&pd->sh->mutex);
    pd->sh->counter_for_starting++;
    pthread_mutex_unlock(&pd->sh->mutex);

    while(pd->sh->counter_for_starting != pd->numberOfPlayers) {
        // attendo tutti abbiano le proprie cartelline...
    }

    // --- SE PLAYER ARRIVA QUI, HA LA PROPRIE CARTELLINE E INIZIA IL GIOCO! ---
    while(pd->sh->endGame == false) {
        sem_wait(&pd->sh->player);
        if(pd->sh->endGame == true) break;
        sleep(1);

        for(int i=0; i<pd->numberOfCards; i++) {
            for(int j=0; j<SIZE_CARD; j++) {
                if(pd->myCards[i][j] == pd->sh->lastNumber) { 
                
                    pd->myCheckCards[i][j] = true;
                    printf("[P%d] ha il %d! (card n%d)\n", pd->id+1, pd->sh->lastNumber, i+1);
                    
                    pthread_mutex_lock(&pd->sh->mutex);
                    if(pd->sh->cinquina == false) {
                        prize = checkCinquina(pd->myCheckCards[i]);
                        if(prize == 1) {
                            printf("\n[P%d] ha fatto cinquina! (card n%d) (%d, %d, %d, %d, %d)\n\n", pd->id+1, i+1, pd->myCards[i][0], pd->myCards[i][1], pd->myCards[i][2], pd->myCards[i][3], pd->myCards[i][4]);
                            pd->sh->cinquina = true;
                        }
                        else if(prize == 2) {
                            printf("\n[P%d] ha fatto cinquina! (card n%d) (%d, %d, %d, %d, %d)\n\n", pd->id+1, i+1, pd->myCards[i][5], pd->myCards[i][6], pd->myCards[i][7], pd->myCards[i][8], pd->myCards[i][9]);
                            pd->sh->cinquina = true;
                        }
                        else if(prize == 3) {
                            printf("\n[P%d] ha fatto cinquina! (card n%d) (%d, %d, %d, %d, %d)\n\n", pd->id+1, i+1, pd->myCards[i][10], pd->myCards[i][11], pd->myCards[i][12], pd->myCards[i][13], pd->myCards[i][14]);
                            pd->sh->cinquina = true;
                        }
                    }
                    
                    if(pd->sh->endGame == false) {
                        prize = checkTombola(pd->myCheckCards[i]);
                        if(prize == 4) {
                            printf("[P%d] ha fatto tombola! (card n%d) ( ", pd->id+1, i+1);
                            printCard(pd->myCards[i], SIZE_CARD);
                            pd->sh->endGame = true;
                        } 
                    }
                    pthread_mutex_unlock(&pd->sh->mutex);
                }
            }
        }
        //printf("[P%d] ha finito di controllare!\n", pd->id+1);
        pthread_mutex_lock(&pd->sh->mutex);
        pd->sh->counter_for_dealer++;
        if(pd->sh->counter_for_dealer == pd->numberOfPlayers) {
            sem_post(&pd->sh->dealer);
            pd->sh->counter_for_dealer = 0;
        }
        pthread_mutex_unlock(&pd->sh->mutex);
    }

    free(pd->myCards);
    free(pd->myCheckCards);
    printf("[P%d] E' uscito correttamente dalla partita...\n", pd->id+1);
    return NULL;
}


int main(int argc, char const *argv[])
{
    if(argc < 3) exit(1); // conta numero di input su riga di comando. Ricorda che: a.exe (0) numeroPlayer (1) numeroCarte (2)
    int nPlayers = atoi(argv[1]);
    int mCards = atoi(argv[2]);

    // Devo inizializzare le mie strutture scritte all'inizio!
    shared_data* shared = (shared_data*) malloc(sizeof(shared_data));
    dealer_data* dd = (dealer_data*) malloc(sizeof(dealer_data));
    player_data* pd = (player_data*) malloc(sizeof(player_data) * nPlayers);

    initShared(shared); // inizializza le strutture inserite dentro la struct shared..

    // creiamo i thread
    // thread dealer (ne abbiamo solo uno!)
    if(pthread_create(&dd[0].tid, NULL, &dealerFunction, &dd[0]) !=0)
        exit(1);
    dd[0].id = 0;
    dd[0].sh = shared;
    dd[0].numberOfPlayers = nPlayers;
    dd[0].numberOfCards = mCards;

    // fine thread dealer

    // thread giocatori
    for(int i=0; i<nPlayers; i++) {
        if(pthread_create(&pd[i].tid, NULL, &playerFunction, &pd[i]) !=0)
            exit(1);
        pd[i].id = i;
        pd[i].numberOfPlayers = nPlayers;
        pd[i].numberOfCards = mCards;
        pd[i].sh = shared;
    }

    // fine thread giocatori

    // facciamo le join...
    // join del thread dealer
    if(pthread_join(dd[0].tid, NULL) !=0)
        exit(1);

    // join dei thread giocatori
    for(int i=0; i<nPlayers; i++) {
        if(pthread_join(pd[i].tid, NULL) !=0)
            exit(1);
    }

    freeShared(shared);
    return 0;
}