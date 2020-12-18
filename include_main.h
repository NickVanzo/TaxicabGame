
#define _GNU_SOURCE

#ifndef INCLUDE_STDIO
#define INCLUDE_STDIO
#include <stdio.h>

#endif

#ifndef INCLUDE_STDLIB
#define INCLUDE_STDLIB
#include <stdlib.h>

#endif

#ifndef INCLUDE_TIME
#define INCLUDE_TIME
#include <time.h>

#endif

#ifndef INCLUDE_STRINGS
#define INCLUDE_STRINGS
#include <string.h>

#endif

#ifndef INCLUDE_UNISTD
#define INCLUDE_UNISTD
#include <unistd.h>

#endif

#ifndef INCLUDE_SIGNAL
#define INCLUDE_SIGNAL
#include <signal.h>

#endif

#ifndef INCLUDE_TYPES
#define INCLUDE_TYPES
#include <sys/types.h>

#endif

#ifndef INCLUDE_IPC
#define INCLUDE_IPC
#include <sys/ipc.h>

#endif

#ifndef INCLUDE_SEM
#define INCLUDE_SEM
#include <sys/sem.h>

#endif

#ifndef INCLUDE_LIMITS
#define INCLUDE_LIMITS
#include <limits.h>

#endif

#ifndef INCLUDE_ERRNO
#define INCLUDE_ERRNO
#include <errno.h>

#endif

#ifndef INCLUDE_SHM
#define INCLUDE_SHM
#include <sys/shm.h>

#endif

#ifndef INCLUDE_IOCTL /*vedi pg 711 di advanced programming for unix enviroment*/ 
#define INCLUDE_IOCTL
#include <sys/ioctl.h>

#endif

#ifndef INCLUDE_MSG
#define INCLUDE_MSG
#include <sys/msg.h>

#endif

#ifndef INCLUDE_MATH
#define INCLUDE_MATH
#include <math.h>

#endif

#ifndef DEFINES
#define DEFINES
/*dimensione della mappa*/
/*colonne*/
#define SO_WIDTH 20
/*righe*/
#define SO_HEIGHT 10
#endif

#ifndef COLORS_SCHEME
#define COLORS_SCHEME
#define C_DEFAULT "\033[39m"
#define C_BLACK "\033[30m"
#define C_RED "\033[31m"
#define C_GREEN "\033[32m"
#define C_YELLOW "\033[33m"
#define C_BLUE "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN "\033[36m"
#define C_GRAY "\033[37m"
#define C_WHITE "\033[97m"
#define BG_C_DEFAULT "\033[49m"
#define BG_C_BLACK "\033[40m"
#define BG_C_RED "\033[41m"
#define BG_C_GREEN "\033[42m"
#define BG_C_YELLOW "\033[43m"
#define BG_C_BLUE "\033[44m"
#define BG_C_MAGENTA "\033[45m"
#define BG_C_CYAN "\033[46m"
#define BG_C_GRAY "\033[100m"
#define BG_C_WHITE "\033[107m"
#endif

/*
    Struct di supporto implementata da noi per utilizzare booleani.
*/
typedef enum
{
    FALSE, TRUE
}
boolean;

/*
    Variabili per il colore dello sfondo e del testo
*/

enum color
{
    DEFAULT, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, GRAY, WHITE
};

/*
    Variabili della mappa
*7

/*
    Tipo della singola cella. 
    Le celle devono essere di tre tipologie:
    1) ROAD - dove i taxi si possono spostare liberamente tenendo conto delle risorse messe a disposizione per il contenimento dei taxi nelle celle
    2) BLOCK - cella sopra la quale i taxi non possono transitare
    3) SOURCE - cella di origine delle richieste. Da qui i taxi prelevano un messaggio dalla coda e si recano verso la propria destinazione
*/
typedef enum
{
    ROAD, BLOCK, SOURCE
}
cell_type;

/*
    Struct che indica una singola cella della mappa.
    Per una destizione dell'utilizzo delle variabili vedere in seguito
    - waitForEveryone = variabile di tipo semaforo per far sì che prima di muoversi i taxi siano spawnati tutti inizializzato a SO_TAXI in master
    - availableSpace = indica quanti posti ci sono nella cella. E' una variabile usata come semaforo. Per avere un esempio pratico di utilizzo si guardi la funzione spawnTaxi in taxi.c
    - cellType = indica il tipo di cella.[Se ROAD, BLOCK, SOURCE]
    - availableForHoles = variabile utilizzata nella creazione delle celle di tipo BLOCK. Per vedere un esempio pratico si guardi la funzione spawnBlocks in master.c
    - taxiOnThisCell = variabile che tiene conto di quanti taxi ci sono all'interno di una determinata cella
    - totalNumberOfTaxiPassedHere = variabile che tiene conto del numero di taxi che sono passati sulla cella
    - timeRequiredToCrossCell = variabile che tiene conto del tempo di attraversamento della cella.
    - isInTopCell = variabile di tipo booleano usata nella individuazione delle celle SO_TOP_CELLS nel master.c
    - mutex = variabile di tipo semaforo usata per garantire la muta esclusione per l'accesso in memoria condivisa per evitare informazioni non corrette
*/
typedef struct
{
    int waitForEveryone;
    cell_type cellType;
    int availableSpace;
    int taxiOnThisCell;
    int availableForHoles;
    int totalNumberOfTaxiPassedHere;
    time_t timeRequiredToCrossCell;
    boolean isInTopSoCell;
    int mutex;
}
map_cell;

/*
    Struct dedita al mantenimento di informazioni riguardo la mappa. Queste informazioni sono:
    - matrice[][] = la struttura a matrice vera e propria della mappa usata far si' che i taxi si muovano
    - mutex = variabile di tipo semaforo usata per garantire la mutua esclusione per l'accesso in memoria condivisa per evitare informazioni non corrette
    - abordet_rides = variabile che tiene conto dei taxi che hanno ottenuto la richiesta da un cliente ma che non sono riusciti ad arrivare a destinazione
    - successes_rides = variabile che tiene conto delle corse andate a buon fine
*/
struct grigliaCitta
{
    map_cell matrice[SO_HEIGHT][SO_WIDTH];
    int aspettaTutti;
    int mutex;
    int aborted_rides;
    int successes_rides;
};

/*
    Struttura che specifica il corpo del messaggio da costruire e da mandare nella coda di messaggi. Il corpo del messaggio viene costruito in source.c e prelevato in taxi.c.
    - mtype = variabile che specifica il tipo di messaggio che viene letto. Il numero specificato corrisponde alla cella source in cui è possibile prelevare il messaggio
    - xDest e yDest = variabili che indicano al taxi dove deve andare per completare la corsa. Il sistema di riferimento non è cartesiano ma è quello che si utilizza all'interno di una matrice[][]
*/

struct msgBuf
{
    long mtype;
    int xDest;
    int yDest;
};

/*
    Struttura contente la variabili di cui deve essere tenuto conto quando si il master stampa il pid del taxi con:
    + servite (la variabile corrispondente è "clienti_serviti")
    + celle attraversate (la variabile corrispondente è "strada_fatta")
    + tempo in strada (la variabile corrispondente è "tempo_in_strada")
    - le variabili pid_nome_variabile contengono il pid del taxi con le caratteristiche sopra citate più alte.
    - mutex = variabile di tipo semaforo usata per garantire la mutua esclusione quando si lavora con le variabili presente nella struct (struct che viene caricata in memoria condivisa
    e che viene consultata dai taxi ad ogni spostamento che fanno)
*/
struct taxiStatistiche
{
    int strada_fatta;
    int clienti_serviti;
    unsigned long int tempo_in_strada;
    int pid_strada_fatta;
    int pid_clienti_serviti;
    int pid_tempo_in_strada;
    int mutex;
};

#ifndef DEFINE_CUSTOM_FUNCTIONS
#define DEFINE_CUSTOM_FUNCTIONS
/*qua dentro definiamo tutti i prototipi delle funzioni che andiamo a scrivere*/

void colorPrintf(char *message, enum color colore, enum color bgcolore);

/*
    FUNZIONI DI GESTIONE DEI SEMAFORI
    sono defineite dentro il file utils.c
*/
void P(int semaphore);
void V(int semaphore);
void VwaitForZero(int semaphore);
#endif