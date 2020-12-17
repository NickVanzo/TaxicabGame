#include "include_main.h"
#define TEST_ERROR
if (errno)
{
    fprintf(stderr, \
        "%s:%d: PID=%5d: Error %d (%s)\n", \
        __FILE__, \
        __LINE__, \
        getpid(), \
        errno, \
        strerror(errno));
}

/*
    funzione per la stampa in master delle statistiche della cella
    map_cell mappa[][] la mappa del gioco
    int statistiche[6]: array contenente le statistiche:
        0-> numero di corse completate con successo
        1-> numero di corse no completate
        2-> numero di corse abortite
        3-> pid processo con la somma di tempo di corsa maggiore tra tutte
        4-> pid processo che ha percorso più distanza
        5-> pid processo che ha servito più clienti
    boolean finalPrint indica se è la stampa finale della mappa e quindi evidenziare le celle più trafficate
*/
void stampaStatistiche(struct grigliaCitta *mappa, int *statistiche, boolean finalPrint, int SO_TOP_CELLS, int runningTime);

/*
    funzione per la stampa in master delle statistiche della cella ma solo con caratteri ASCII e con celle più piccole e le statistiche sotto...
    map_cell mappa[][] la mappa del gioco
    int statistiche[6]: array contenente le statistiche:
        0-> numero di corse completate con successo
        1-> numero di corse no completate
        2-> numero di corse abortite
        3-> pid processo con la somma di tempo di corsa maggiore tra tutte
        4-> pid processo che ha percorso più distanza
        5-> pid processo che ha servito più clienti
    boolean finalPrint indica se è la stampa finale della mappa e quindi evidenziare le celle più trafficate
*/
void stampaStatisticheAscii(struct grigliaCitta *mappa, int *statistiche, boolean finalPrint, int SO_TOP_CELLS, int runningTime);

/*
    Funzione per aggiornare le statistiche
*/
void aggiornaStatistiche(struct grigliaCitta *mappa, int *statistiche, int msgQueueId);

/*
    funzione per inizializzare la simulazione e tenere il codice del main meno sporco possibile
    questa funzione legge dal stdin o argv i parametri e li assegna.
    Inoltre stampa un riepilogo dei parametri e informa l'utente della dimensione consigliata dello schermo
    aspetta infine che l'utente prenma un tasto per iniziare  la simulazione per potere andare a stampare in continuo.
    in questo modo utente ha tempo di aggiustare lo schermo e controllare che i parametri inseriti sono validi

*/
void setupSimulation(int *SO_TAXI, int *SO_SOURCES, int *SO_HOLES, int *SO_CAP_MIN, int *SO_CAP_MAX, int *SO_TIMENSEC_MIN, int *SO_TIMENSEC_MAX, int *SO_TOP_CELLS, int *SO_TIMEOUT, int *SO_DURATION, int argc, char *argv[], boolean *printWithAscii);

/*
    funzione per posizionare i blocchi o case all'interno della griglia/citta'
    accetta come parametri la mappa in cui posizionare i buchi e il numero di buchi richiesto dal giocatore
    fino a quando ci sono blocchi da aggiungere nella mappa gli aggiungo
    se ci sono troppi blocchi da posizionare il programma cicla all'infinito
*/
void spawnBlocks(struct grigliaCitta *mappa, int SO_HOLES);

/*
    funzione per posizionare le SOURCES all'interno della griglia della citta'
    accetta come parametri la mappa in cui posizionare le SOURCES e il numero di SOURCES richiesto dal giocatore
    fino a quando ci sono SOURCES da piazzare la funzione cicla
    per piazzare una SOURCE dobbiamo assicurarci che stiamo cercando di posizionarla in una ROAD e non in un BLOCK
*/
void spawnSources(struct grigliaCitta *mappa, int SO_SOURCES);

/*funzione per potere verificare che una mappa non sia degenere*/
void checkForDegeneresMap();

/*questa funzione cerca nella mappa le top cells e le mette dentro la struttura dati topcellArray*/
void searchForTopCells(struct grigliaCitta *mappa, int SO_TOP_CELL);

/*inizializzo i campi della mappa*/
void initMap(struct grigliaCitta *mappa, int SO_CAP_MIN, int SO_CAP_MAX, int SO_TIMENSEC_MIN, int SO_TIMENSEC_MAX, int SO_HOLES, int SO_SOURCES);

boolean exitFromProgram;

/*
    funzione per gestire eventuali segnali guinti al processo
*/
void signalHandler(int signal);

struct taxiStatistiche * taxi_statistiche;

int main(int argc, char *argv[])
{
    int i, j; /*variabili iteratrici nei cicli. numerobuchi conta il numero di buchi che ho creato*/
    int SO_TAXI, SO_SOURCES, SO_HOLES, SO_CAP_MIN, SO_CAP_MAX, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TOP_CELLS, SO_TIMEOUT, SO_DURATION; /*parametri letti o inseriti a compilazione*/
    int mapStats[6]; /*Variabile contenente le statistiche della mappa*/
    key_t queueKey;
    int queue_id;
    int shmId, shmKey, shmKey_ForTaxi, shmKey_stats, shmID_stats;
    int *childSourceCreated;
    int *taxiCreated;
    char SO_TAXI_PARAM[10], SO_SOURCES_PARAM[10], SO_HOLES_PARAM[10], SO_CAP_MIN_PARAM[10], SO_CAP_MAX_PARAM[10], SO_TIMENSEC_MIN_PARAM[10], SO_TIMENSEC_MAX_PARAM[10], SO_TOP_CELLS_PARAM[10], SO_TIMEOUT_PARAM[10], SO_DURATION_PARAM[10];
    int runningTime = 0;
    int taxiSemaphore_id;
    int trashKillSignal; /*variabile per raccogliere stato segnale uscita*/
    int wait_pid;
    int child_status;
    boolean printWithAscii = FALSE; /*se lo schermo è piccolo stampo con ascii*/
    /*Variabili per memoria condivisa*/
    struct grigliaCitta * mappa;
    /*VARIABILI PER LA SINCRONIZZAZIONE DEI TAXI - DEVE ESSERE CARICATA IN MEMORIA CONDIVISA*/

    /*AVVIO SETUP SIMULAZIONE*/
    setupSimulation(&SO_TAXI, &SO_SOURCES, &SO_HOLES, &SO_CAP_MIN, &SO_CAP_MAX, &SO_TIMENSEC_MIN, &SO_TIMENSEC_MAX, &SO_TOP_CELLS, &SO_TIMEOUT, &SO_DURATION, argc, argv, &printWithAscii);

    /*CREAZIONE CODA DI MESSAGGI*/
    /*Ottengo la chiave per la coda di messaggi*/
    queueKey = ftok("ipcKey.key", 1);
    /*Ottengo l'id della coda di messaggi cosi' da disallocare in seguito la coda*/
    if ((queue_id = msgget(queueKey, IPC_CREAT | IPC_EXCL | 0666)) == -1)
    {
        fprintf(stderr, "Errore nella creazione della coda di messaggi. Codice errore: %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    /*CODA DI MESSAGGI CREATA CON SUCCESSO*/

    /*MEMORIA CONDIVISA*/
    shmKey = ftok("ipcKey.key", 2);
    shmId = shmget(shmKey, sizeof(struct grigliaCitta), IPC_CREAT | IPC_EXCL | 0666);
    if (shmId == -1)
    {
        system("./cleanIpcs.sh");
        shmId = shmget(shmKey, sizeof(struct grigliaCitta), IPC_CREAT | IPC_EXCL | 0666);
    }
    mappa = shmat(shmId, NULL, 0);
    if (mappa == (struct grigliaCitta *)(-1))
    {
        printf("Error at shmat! error code is %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    /*memoria condivisa per le statistiche dei taxi*/
    shmKey_stats = ftok("ipcKey.key", 5);
    shmID_stats = shmget(shmKey_stats, sizeof(struct taxiStatistiche), IPC_CREAT | IPC_EXCL | 0666);
    taxi_statistiche = shmat(shmID_stats, NULL, 0);
    /*MEMORIA CONDIVISA CREATA CON SUCCESSO*/

    /*IMPOSTAZIONE HANDLER SEGNALE TIMER*/
    signal(SIGALRM, signalHandler);

    /*controllo che la mappa non sia degenere ovvero che tutti i punti possano essere raggiunti -> mappa deve essere almeno 2x2*/
    checkForDegeneresMap();

    /*init della rand per la funzione di assegnazione*/
    srand(getpid());

    /*per debug solo DA TOGLIERE*/
    for (i = 0; i < 6; i++) mapStats[i] = 0;

    initMap(mappa, SO_CAP_MIN, SO_CAP_MAX, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_HOLES, SO_SOURCES);

    /*CREAZIONE FIGLI SOURCES*/
    /*preparo i parametri da mandare come argomenti alla execlp*/
    sprintf(SO_SOURCES_PARAM, "%d", SO_SOURCES);
    sprintf(SO_DURATION_PARAM, "%d", SO_DURATION);
    sprintf(SO_TIMEOUT_PARAM, "%d", SO_TIMEOUT);
    /*creo array contenente i pid dei figli source creati*/
    childSourceCreated = malloc(SO_SOURCES* sizeof(int));
    /*faccio la fork per poterer creare i processi che generano le richieste da  sources*/
    for (i = 0; i < SO_SOURCES; i++)
    {
        childSourceCreated[i] = fork();
        switch (childSourceCreated[i])
        {
            case -1:
                printf("Error while trying to fork()! %s\n", strerror(errno));
                exit(EXIT_FAILURE);
                break;
            case 0:
                /*cambio il programma in esecuzione*/
                execlp("./source", "source", SO_SOURCES_PARAM, SO_DURATION_PARAM, SO_TIMEOUT_PARAM, NULL);
                printf("Error loading new program %s!\n\n", strerror(errno));
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }
    /*CREAZIONE FIGLI SOURCES AVVENUTA CON SUCCESSO*/

    /*CREAZIONE FIGLI TAXI + INIZIALIZZAZIONE DEI SEMAFORI CHE VENGONO USATI E CONDIVISI IN MEMORIA CON TAXI*/

    shmKey_ForTaxi = ftok("ipcKey.key", 3);
    taxiSemaphore_id = semget(shmKey_ForTaxi, 1, IPC_CREAT | IPC_EXCL | 0666);
    semctl(taxiSemaphore_id, 0, SETVAL, SO_TAXI);

    /*Creao un array contenente i pid dei figli taxi creati*/
    taxiCreated = malloc(SO_TAXI* sizeof(int));
    for (i = 0; i < SO_TAXI; i++)
    {
        taxiCreated[i] = fork();
        switch (taxiCreated[i])
        {
            case -1:
                printf("Error while trying to fork()! %s\n", strerror(errno));
                exit(EXIT_FAILURE);
                break;
            case 0:
                execlp("./taxi", "taxi", SO_TIMEOUT_PARAM, SO_DURATION_PARAM, NULL);
                printf("Error loading new program (taxi)%s!\n\n", strerror(errno));
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }
    /*CREAZIONE DEI FIGLI TAXI AVVENUTA CON SUCCESSO*/

    /*Finche' e' FALSE continua ad eseguire la stampa delle statistiche, appena e' FALSE si esce dal programma*/
    exitFromProgram = FALSE;
    /*imposto durata simulazione*/
    alarm(SO_DURATION);
    for (i = 0; i < SO_HEIGHT; i++) {}

    while (!exitFromProgram)
    {

        sprintf(SO_DURATION_PARAM, "%d", SO_DURATION - runningTime);

        /*Se un taxi muore lo ricreiamo in un altra posizione della griglia*/
        while ((wait_pid = waitpid(-1, NULL, WNOHANG)) > 0)
        {
            for (i = 0; i < SO_TAXI; i++)
            {
                if (taxiCreated[i] == wait_pid)
                {
                    taxiCreated[i] = fork();
                    switch (taxiCreated[i])
                    {
                        case -1:
                            printf("Error while trying to fork()! %s\n", strerror(errno));
                            exit(EXIT_FAILURE);
                            break;
                        case 0:
                            execlp("./taxi", "taxi", SO_TIMEOUT_PARAM, SO_DURATION_PARAM, NULL);
                            printf("Error loading new program (taxi)%s!\n\n", strerror(errno));
                            exit(EXIT_FAILURE);
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        aggiornaStatistiche(mappa, mapStats, queue_id);

        if (printWithAscii)
        { /*se ho lo schermo piccolo stampo con ascii*/
            stampaStatisticheAscii(mappa, mapStats, FALSE, SO_TOP_CELLS, runningTime);
        }
        else
        {
            stampaStatistiche(mappa, mapStats, FALSE, SO_TOP_CELLS, runningTime);
        }

        runningTime++;
        sleep(1);
    }

    aggiornaStatistiche(mappa, mapStats, queue_id);
    /*RICORDARSI CHE QUA TUTTI ITAKI SONO DA KILLARE*/
    searchForTopCells(mappa, SO_TOP_CELLS); /*cerco e marco le SO_TOP_CELL*/

    /*Faccio terminare tutti i processi sources facendo anche smettere l'invio dei segnali*/
    for (i = 0; i < SO_SOURCES; i++)
    {
        kill(childSourceCreated[i], SIGUSR1);
    }

    if (printWithAscii)
    {
        stampaStatisticheAscii(mappa, mapStats, TRUE, SO_TOP_CELLS, runningTime);
    }
    else
    {
        stampaStatistiche(mappa, mapStats, TRUE, SO_TOP_CELLS, runningTime);
    }

    /*attendo la morte di tutti i figli*/
    for (i = 0; i < SO_TAXI; i++)
    {
        kill(taxiCreated[i], SIGKILL); /*da sostituire con un segnale specifico!!!*/
        waitpid(taxiCreated[i], &trashKillSignal);
    }

    for (i = 0; i < SO_SOURCES; i++)
    {
        kill(childSourceCreated[i], SIGKILL);
        waitpid(childSourceCreated[i], &trashKillSignal);
    }

    /*libero la memoria condivisa ED ELIMINO TUTTI I SEMAFORI*/
    for (i = 0; i < SO_HEIGHT; i++)
    {
        for (j = 0; j < SO_WIDTH; j++)
        {
            semctl(mappa->matrice[i][j].availableSpace, 0, IPC_RMID, 0); /*rimuovo i semafori*/
            semctl(mappa->matrice[i][j].mutex, j, IPC_RMID, 0); /*rimuovo i semafori*/

        }
    }

    free(childSourceCreated);
    free(taxiCreated);
    semctl(mappa->mutex, 0, IPC_RMID);
    semctl(taxiSemaphore_id, 0, IPC_RMID);
    semctl(taxi_statistiche->mutex, 0, IPC_RMID);
    shmdt(mappa);
    shmdt(taxi_statistiche);
    shmctl(shmID_stats, IPC_RMID, NULL);
    shmctl(shmId, IPC_RMID, NULL);
    msgctl(queue_id, IPC_RMID, NULL);

    return 0;
}

void spawnBlocks(struct grigliaCitta *mappa, int SO_HOLES)
{
    int k, q; /*Variabili usate per ciclare*/
    int numeroBuchi = 0;
    /*inizializzo il tipo di ogni cella della tabella ROAD e marco come 1(inteso come la variabile booleana TRUE) la disponibilita' di ospitare un buco*/
    for (k = 0; k < SO_HEIGHT; k++)
    {
        for (q = 0; q < SO_WIDTH; q++)
        {
            mappa->matrice[k][q].cellType = ROAD;
            mappa->matrice[k][q].availableForHoles = 1;

            /*solo per test colori*/
            mappa->matrice[k][q].totalNumberOfTaxiPassedHere = rand() % 13;
        }
    }

    /*Ciclo fino a quando non ho posizionato tutti i buchi richiesti*/
    while (numeroBuchi < SO_HOLES)
    {
        /*PROBLEMA DELL'ALGORITMO: TROPPI BUCHI LO FANNO CICLARE ALL'INFINITO PERCHE' NON TROVA COORDINATE ADATTE, NON DOVREBBERO ESSERCI ALTRI PROBLEMI NOTI AL MOMENTO*/
        /*Randomizzo le coordinate della cella che voglio far diventare buco*/
        k = rand() % SO_HEIGHT;
        q = rand() % SO_WIDTH;
        if (mappa->matrice[k][q].availableForHoles == 1)
        { /*La cella con le coordinate ottenute randomicamente vengono segnate come BLOCK*/
            mappa->matrice[k][q].cellType = BLOCK;
            mappa->matrice[k][q].availableForHoles = 0;

            #
            ifdef DEBUG_BLOCK
            fprintf(stdout, "%d - Posizionato buco in posizione %d%d\n", numeroBuchi, k, q);#
            endif
            /*La cella su cui posiziono il buco non potra' contenere altri buchi e nemmeno le 8 celle che ha intorno potranno*/

            /*marco come inutilizzabili per contenere buchi le celle intorno alla cella su cui ho posizionato il buco*/
            if (k > 0)
            {
                if (q > 0)
                {       /*Cella in alto a sinistra*/
                    mappa->matrice[k - 1][q - 1].availableForHoles = 0;
                }
                /*Cella in alto centrale*/
                mappa->matrice[k - 1][q].availableForHoles = 0;
                if (q < SO_WIDTH - 1)
                {       /*Cella in alto a destra*/
                    mappa->matrice[k - 1][q + 1].availableForHoles = 0;
                }
            }

            if (q > 0)
            {   /*Cella centrale a sinistra*/
                mappa->matrice[k][q - 1].availableForHoles = 0;
            }

            /*Per la cella centrale non ho bisogno di un ulteriore controllo perche' e' la cella su cui siamo seduti*/
            if (q < SO_WIDTH - 1)
            {   /*Cella centrale a destra*/
                mappa->matrice[k][q + 1].availableForHoles = 0;
            }

            if (k < SO_HEIGHT - 1)
            {
                if (q > 0)
                {       /*Cella in basso a sinistra*/
                    mappa->matrice[k + 1][q - 1].availableForHoles = 0;
                }
                /*Cella in basso centrale*/
                mappa->matrice[k + 1][q].availableForHoles = 0;

                if (q < SO_WIDTH - 1)
                {       /*Cella in basso a destra*/
                    mappa->matrice[k + 1][q + 1].availableForHoles = 0;
                }
            }
            /*Se il buco e' stato posizionato allora incremento, altrimenti non ho trovato una posizione valida, continuo il ciclo*/
            numeroBuchi++;
        }
    }
}

void spawnSources(struct grigliaCitta *mappa, int SO_SOURCES)
{
    int numeroSources = 0; /*variabile per tenere conto di quante Sources sono state piazzate nella mappa*/
    int n, l; /*variabili che determinano il posizionamento nella griglia*/

    /*La Source puo' essere posizionata solamente in celle della griglia che prima erano ROAD, un BLOCK non puo' diventare ROAD*/
    while (numeroSources < SO_SOURCES)
    {

        /*Assegno alle variabili n ed l un numero random che serve per posizionarmi sulla griglia*/
        n = rand() % SO_HEIGHT;
        l = rand() % SO_WIDTH;

        /*Se la cella che mi e' capitata e' di tipo ROAD allora posiziono la SOURCE e incremento il numero di SOURCE posizionate*/
        if (mappa->matrice[n][l].cellType == ROAD)
        {
            mappa->matrice[n][l].cellType = SOURCE;
            numeroSources++;
        }
        /*Se la cella che mi e' capitata non e' di tipo ROAD allora continuo il ciclo senza incrementare il numero di SOURCE posizionate*/
    }
}

void setupSimulation(int *SO_TAXI, int *SO_SOURCES, int *SO_HOLES, int *SO_CAP_MIN, int *SO_CAP_MAX, int *SO_TIMENSEC_MIN, int *SO_TIMENSEC_MAX, int *SO_TOP_CELLS, int *SO_TIMEOUT, int *SO_DURATION, int argc, char *argv[], boolean *printWithAscii)
{
    int screenHeight, screenWidth;
    char bufferTemp[1024]; /*buffer temporaneo per lettura dei parametri da tastiera*/
    char tmpChar; /*per leggere eventuali input non desiderati*/

    struct winsize size; /*struttura per ottenere la dimensione della finestra... advanced programming for unix pae 711. la uso per sapere se devo o meno stampare con asci o con int*/

    ioctl(STDIN_FILENO, TIOCGWINSZ, &size); /*ottengo la dimensione della finestra*/

    if (argc == 11)
    {
        /*1 nome comando + 10 parametri*/
        *SO_TAXI = atoi(argv[1]);
        *SO_SOURCES = atoi(argv[2]);
        *SO_HOLES = atoi(argv[3]);
        *SO_CAP_MIN = atoi(argv[4]);
        *SO_CAP_MAX = atoi(argv[5]);
        *SO_TIMENSEC_MIN = atoi(argv[6]);
        *SO_TIMENSEC_MAX = atoi(argv[7]);
        *SO_TOP_CELLS = atoi(argv[8]);
        *SO_TIMEOUT = atoi(argv[9]);
        *SO_DURATION = atoi(argv[10]);
    }
    else
    {

        printf("Insert number of taxi available for the simulation: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_TAXI = abs(atoi(bufferTemp)); /*converto da stringa a intero*/
        do {    printf("Insert number of sources available for the simulation(MAX = %d): ", SO_WIDTH *SO_HEIGHT);
            fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
            *SO_SOURCES = atoi(bufferTemp); /*converto da stringa a intero*/
            if (*SO_SOURCES >= SO_WIDTH *SO_HEIGHT)
            {
                printf("Insert again the number of SOURCES. Too many!!\n");
            }
        } while (*SO_SOURCES >= SO_WIDTH *SO_HEIGHT);
        do {    printf("Insert number of holes available for the simulation: ");
            fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
            *SO_HOLES = abs(atoi(bufferTemp)); /*converto da stringa a intero*/
            if (*SO_HOLES >= (SO_WIDTH *SO_HEIGHT - *SO_SOURCES))
            {
                printf("Insert again the number of HOLES. Too many holes.\n");
            }
        } while (*SO_HOLES >= (SO_WIDTH *SO_HEIGHT - *SO_SOURCES));

        printf("Insert minimum capacity for each cell: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_CAP_MIN = abs(atoi(bufferTemp)); /*converto da stringa a intero*/

        printf("Insert maximum capacity for each cell: ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_CAP_MAX = atoi(bufferTemp); /*converto da stringa a intero*/

        printf("Insert minimum crossing time for each cell (nanoseconds - max: 999999999): ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_TIMENSEC_MIN = atoi(bufferTemp); /*converto da stringa a intero*/

        printf("Insert maximum crossing time for each cell (nanoseconds - max: 999999999): ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_TIMENSEC_MAX = atoi(bufferTemp); /*converto da stringa a intero*/

        do {    printf("Insert number of top cells to be shown at the end of the simulation: ");
            fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
            *SO_TOP_CELLS = atoi(bufferTemp); /*converto da stringa a intero*/
            if (*SO_TOP_CELLS <= SO_WIDTH *SO_HEIGHT - *SO_HOLES)
            {
                printf("Insert again the number of SO_TOP_CELLS. Too many!!\n");
            }
        } while (*SO_TOP_CELLS <= SO_WIDTH *SO_HEIGHT - *SO_HOLES);

        printf("Insert taxi move timeout (milliseconds): ");
        fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_TIMEOUT = atoi(bufferTemp); /*converto da stringa a intero*/

        do {    printf("Insert simulation duration (seconds >= 2): ");
            fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
            *SO_DURATION = atoi(bufferTemp); /*converto da stringa a intero*/
            if (*SO_DURATION < 2)
            {
                printf("Insert again the number SO_DURATION value. Too small!!\n");
            }
        } while (*SO_DURATION < 2); /*la simulazione deve durare almeno 10 secondi*/

    }

    /*calcolo le dimensioni consigliate della finestra di terminale per esperienza ottimale*/
    if (SO_HEIGHT + 4 < 14) screenHeight = 14; /*se la dimensione della tabella è 9, diventa 13 (con 2 extra sopra e 2 extra sotto*/
    else screenHeight = SO_HEIGHT + 4;

    screenWidth = 4 *(2 + SO_WIDTH) + 55; /*lunghezza della singola cella per la dimenzione della mappa piu la len max delle stat*/

    if (*SO_DURATION < 2)
    {
        printf("Error: duration of simulation is less than 2 seconds...\nPlease insert valid inputs...\n");
        do {    printf("Insert simulation duration (seconds >= 2): ");
            fgets(bufferTemp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non idandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
            *SO_DURATION = atoi(bufferTemp); /*converto da stringa a intero*/
        } while (*SO_DURATION < 2); /*la simulazione deve durare almeno 10 secondi*/

        printf("Simulation will now start with thw following parameters:\nn\tSO_TAXI: %d\n\tSO_SOURCES: %d\n\tSO_HOLES: %d\n\tSO_CAP_MIN: %d\n\tSO_CAP_MAX: %d\n\tSO_TIMENSEC_MIN: %d\n\tSO_TIMENSEC_MAX: %d\n\tSO_TOP_CELLS: %d\n\tSO_TIMEOUT: %d\n\tSO_DURATION: %d\n\n%s For a better experience, a terminal with minimum %d char width and minimum %d character height is required %s\n\nAre you ok with the following parameters? (y/n, Default:y): ", *SO_TAXI, *SO_SOURCES, *SO_HOLES, *SO_CAP_MIN, *SO_CAP_MAX, *SO_TIMENSEC_MIN, *SO_TIMENSEC_MAX, *SO_TOP_CELLS, *SO_TIMEOUT, *SO_DURATION, C_YELLOW, screenWidth, screenHeight, C_DEFAULT);

        tmpChar = getc(stdin); /*leggo input per potere avviare simulazione, prima assicurandomi che la dimensione del terminale sia mantenuta delle dimansioni buone*/
        getc(stdin); /*pulisto da eventuali 7n che rompono le scatole*/
        /*verifico che la scelta sia negativa e se lo è riavvio la richiesta dei dati... con argc però a zero per farti modificare i numeri!*/
        if (tmpChar == 'n' || tmpChar == 'N') setupSimulation(SO_TAXI, SO_SOURCES, SO_HOLES, SO_CAP_MIN, SO_CAP_MAX, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TOP_CELLS, SO_TIMEOUT, SO_DURATION, 0, argv, printWithAscii);

        if (screenWidth > size.ws_col)
        {
            colorPrintf("\nWarning!", YELLOW, DEFAULT);
            printf(" screen size width might be too small to fit a beautified output version... would you like to print an ASCII rappresentation? (y/n - default: n): ");
            tmpChar = getc(stdin);
            getc(stdin); /*tolgo lo \n che senno fa partire subito la simulazione*/
            if (tmpChar == 'y') *printWithAscii = TRUE;
            else *printWithAscii = FALSE;
        }

        tmpChar = getc(stdin); /*leggo input per potere avviare simulazione, prima assicurandomi che la dimensione del terminale sia mantenuta delle dimansioni buone*/
        getc(stdin); /*pulisto da eventuali 7n che rompono le scatole*/

    }
    else
    {
        printf("Simulation will now start with thw following parameters:\n\n\tSO_TAXI: %d\n\tSO_SOURCES: %d\n\tSO_HOLES: %d\n\tSO_CAP_MIN: %d\n\tSO_CAP_MAX: %d\n\tSO_TIMENSEC_MIN: %d\n\tSO_TIMENSEC_MAX: %d\n\tSO_TOP_CELLS: %d\n\tSO_TIMEOUT: %d\n\tSO_DURATION: %d\n\n%s For a better experience, a terminal with minimum %d char width and minimum %d character height is required %s\n\nAre you ok with the following parameters? (y/n, Default:y): ", *SO_TAXI, *SO_SOURCES, *SO_HOLES, *SO_CAP_MIN, *SO_CAP_MAX, *SO_TIMENSEC_MIN, *SO_TIMENSEC_MAX, *SO_TOP_CELLS, *SO_TIMEOUT, *SO_DURATION, C_YELLOW, screenWidth, screenHeight, C_DEFAULT);
        tmpChar = getc(stdin); /*leggo input per potere avviare simulazione, prima assicurandomi che la dimensione del terminale sia mantenuta delle dimansioni buone*/
        getc(stdin); /*pulisto da eventuali 7n che rompono le scatole*/
        /*verifico che la scelta sia negativa e se lo è riavvio la richiesta dei dati... con argc però a zero per farti modificare i numeri!*/
        if (tmpChar == 'n' || tmpChar == 'N') setupSimulation(SO_TAXI, SO_SOURCES, SO_HOLES, SO_CAP_MIN, SO_CAP_MAX, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TOP_CELLS, SO_TIMEOUT, SO_DURATION, 0, argv, printWithAscii);

        if (screenWidth > size.ws_col)
        {
            colorPrintf("\nWarning!", YELLOW, DEFAULT);
            printf(" screen size width might be too small to fit a beautified output version... would you like to print an ASCII rappresentation? (y/n - default: n): ");
            tmpChar = getc(stdin);
            getc(stdin); /*tolgo lo \n che senno fa partire subito la simulazione*/
            if (tmpChar == 'y') *printWithAscii = TRUE;
            else *printWithAscii = FALSE;

            printf("\nSimulation will now start. Press any key to begin....");
            getc(stdin);
        }
    }
}

void stampaStatistiche(struct grigliaCitta *mappa, int *statistiche, boolean finalPrint, int SO_TOP_CELLS, int runningTime)
{
    int i, j, k, printedStats = 0, taxiOnTheCell, rowCount = 0; /*rowcount serve a contare quante righe sto stampando...*/
    char stats[13][128];
    const int numberOfStats = 13; /*numero di linee di statistiche da stampare*/
    char *strTmp = (char*) malloc(7); /*dichiaro una str temporanea d usare nella sprintf per poi passarla alla colorPrintf. uso la malloc perchè mi piace*/
    struct winsize size; /*struttura per ottenere la dimensione della finestra... advanced programming for unix pae 711*/
    struct sembuf sops;

    ioctl(STDIN_FILENO, TIOCGWINSZ, &size); /*ottengo la dimensione della finestra*/

    /*creo le stringhe da stampare*/
    sprintf(stats[0], "%s", " |\033[33m Statistics for running simulation \033[39m");
    sprintf(stats[1], "%s%d", " | Number of successful rides: ", statistiche[0]);

    if (!finalPrint) sprintf(stats[2], "%s%d", " | Number of pending rides: ", statistiche[1]);
    else sprintf(stats[2], "%s%d", " | Number of unsuccessful rides: ", statistiche[1]);

    sprintf(stats[3], "%s%d", " | Number of aborted rides: ", statistiche[2]);
    sprintf(stats[4], "%s%d", " | Cumulative longest driving taxi: ", statistiche[3]);
    sprintf(stats[5], "%s%d", " | Cumulative farthest driving taxi: ", statistiche[4]);
    sprintf(stats[6], "%s%d", " | Taxi with most succesfoul rides: ", statistiche[5]);
    sprintf(stats[7], "%s %d seconds...", " | Running time :", runningTime);
    sprintf(stats[8], "%s", " |\033[33m Colours legend: \033[39m");
    sprintf(stats[9], "%s", " | \033[40m  \033[49m -> Black shows blocked zones");
    sprintf(stats[10], "%s", " | \033[45m  \033[49m -> Magenta shows source points");
    sprintf(stats[11], "%s", " | \033[107m  \033[49m -> White shows roads");
    sprintf(stats[12], "%s", " | \033[43m  \033[49m -> Yellow shows SO_TOP_CELLS (only final print)");

    for (k = 0; k < 2; k++, printedStats++, rowCount++)
    {
        /*stampo il bordo superiore*/
        colorPrintf("    ", GRAY, GRAY);
        for (i = 0; i < SO_WIDTH + 1; i++)
        {
            sprintf(strTmp, "%-4d", i);
            if (k > 0 && i < SO_WIDTH) colorPrintf(strTmp, RED, GRAY);
            else colorPrintf("    ", GRAY, GRAY);
        }
        printf("%s\n", stats[printedStats]);
    }

    sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
    sops.sem_flg = 0; /*Comportamento di default*/

    for (i = 0; i < SO_HEIGHT; i++, rowCount++)
    {
        /*stampo il corpo della mappa*/
        sprintf(strTmp, "%4d", i);
        colorPrintf(strTmp, RED, GRAY); /*stampo bordo laterale sx*/
        for (j = 0; j < SO_WIDTH; j++)
        {

            P(mappa->mutex);
            sprintf(strTmp, "%-4d", mappa->matrice[i][j].taxiOnThisCell); /*preparo la stringa da stampare nella cella*/
            V(mappa->mutex);

            if (mappa->matrice[i][j].cellType == ROAD)
            {   /*se sono alla stampa finale e sono in una SO_TOP_CELL allora vado a mostrare i vari colori nelle celle altrimenti mostro solo l'occupazione...*/
                if ((finalPrint == TRUE) && (mappa->matrice[i][j].isInTopSoCell == TRUE))
                {
                    colorPrintf(strTmp, BLACK, YELLOW);
                } /*se sono alla stampa finale, mostro la most frquented cell...*/
                else
                {
                    colorPrintf(strTmp, BLACK, WHITE);
                } /*se non sono alla stampa finale allora devo solamente andare a stampare le srade bianche...*/

            }
            else if (mappa->matrice[i][j].cellType == SOURCE)
            {
                colorPrintf(strTmp, BLACK, MAGENTA);
            }
            else
            {
                colorPrintf("    ", BLACK, BLACK);
            }
        }

        colorPrintf("    ", GRAY, GRAY); /*stampo bordo laterale dx*/
        if (printedStats < numberOfStats)
        {
            printf("%s\n", stats[printedStats]);
            printedStats++;
        }
        else
        {
            printf("\n");
        }
    }

    /*stampo bordo inferiore*/
    for (k = 0; k < 2; k++, rowCount++)
    {
        for (i = 0; i < SO_WIDTH + 2; i++) colorPrintf("    ", GRAY, GRAY);
        if (printedStats < numberOfStats)
        { /*stampo statistica a financo banda grigia*/
            printf("%s\n", stats[printedStats]);
            printedStats++;
        }
        else
        {
            printf("\n");
        }
    }

    /*controllo che ho stampato tutti gli stats e se non lo ho fatto stampo quelli che mancano allineandoli agli altri sotto*/
    if (printedStats < numberOfStats)
    {
        for (i = printedStats; i < numberOfStats; i++, rowCount++)
        {
            for (j = 0; j < 7 *(SO_WIDTH + 2); j++) printf(" ");
            printf("%s\n", stats[i]);
        }
    }

    /*vado a stampare tanti \n in modo da fare sembrare che la pagina si stia aggiornando in continuazione...*/
    if (finalPrint == TRUE)
    {
        for (i = 0; i < (size.ws_row - rowCount - 3); i++) printf("\n"); /*stampo tanti \n fino ad ottenere di avere la finestra completamente pulita...*/
    }
    else
    {
        for (i = 0; i < (size.ws_row - rowCount - 1); i++) printf("\n");
    }

    free(strTmp); /*dealloco strTmp*/
}

void stampaStatisticheAscii(struct grigliaCitta *mappa, int *statistiche, boolean finalPrint, int SO_TOP_CELLS, int runningTime)
{
    int i, j, k, taxiOnTheCell, rowCount = 0; /*rowcount serve a contare quante righe sto stampando...*/
    char stats[12][128];
    const int numberOfStats = 12; /*numero di linee di statistiche da stampare*/
    char *strTmp = (char*) malloc(7); /*dichiaro una str temporanea d usare nella sprintf per poi passarla alla colorPrintf. uso la malloc perchè mi piace*/
    struct sembuf sops;
    struct winsize size; /*struttura per ottenere la dimensione della finestra... advanced programming for unix pae 711*/

    ioctl(STDIN_FILENO, TIOCGWINSZ, &size); /*ottengo la dimensione della finestra*/

    sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
    sops.sem_flg = 0; /*Comportamento di default*/

    /*creo le stringhe da stampare*/
    sprintf(stats[0], "%s", " |\033[33m Statistics for running simulation \033[39m");
    sprintf(stats[1], "%s%d", " | Number of successful rides: ", statistiche[0]);

    if (!finalPrint) sprintf(stats[2], "%s%d", " | Number of pending rides: ", statistiche[1]);
    else sprintf(stats[2], "%s%d", " | Number of unsuccessful rides: ", statistiche[1]);

    sprintf(stats[3], "%s%d", " | Number of aborted rides: ", statistiche[2]);
    sprintf(stats[4], "%s%d", " | Cumulative longest driving taxi: ", statistiche[3]);
    sprintf(stats[5], "%s%d", " | Cumulative farthest driving taxi: ", statistiche[4]);
    sprintf(stats[6], "%s%d", " | Taxi with most succesfoul rides: ", statistiche[5]);
    sprintf(stats[7], "%s %d seconds...", " | Running time :", runningTime);
    sprintf(stats[8], "%s", " |\033[33m Colours legend: \033[39m");
    sprintf(stats[9], "%s", " | \033[44m  \033[49m -> Blue shows blocked zones");
    sprintf(stats[10], "%s", " | \033[45m  \033[49m -> Magenta shows source points");
    sprintf(stats[11], "%s", " | \033[43m  \033[49m -> Yellow shows SO_TOP_CELLS (only final print)");

    printf("    ");
    for (k = 0; k < SO_WIDTH; k++)
    {
        /*stampo il bordo superiore*/
        sprintf(strTmp, " %-3d ", k);
        colorPrintf(strTmp, RED, DEFAULT);
    }
    printf("\n");
    rowCount++;

    for (i = 0; i < SO_HEIGHT; i++, rowCount++)
    {
        /*stampo il corpo della mappa*/
        sprintf(strTmp, " %-2d ", i);
        colorPrintf(strTmp, RED, DEFAULT); /*stampo bordo laterale sx*/
        for (j = 0; j < SO_WIDTH; j++)
        {

            P(mappa->matrice[i][j].mutex);
            sprintf(strTmp, " %-5d ", mappa->matrice[i][j].taxiOnThisCell); /*preparo la stringa da stampare nella cella*/
            V(mappa->matrice[i][j].mutex);

            if (mappa->matrice[i][j].cellType == ROAD)
            {   /*se sono alla stampa finale e sono in una SO_TOP_CELL allora vado a mostrare i vari colori nelle celle altrimenti mostro solo l'occupazione...*/
                if ((finalPrint == TRUE) && (mappa->matrice[i][j].isInTopSoCell == TRUE))
                {
                    colorPrintf(strTmp, BLACK, YELLOW);
                }
                else
                {
                    printf("%s", strTmp);
                } /*se non sono alla stampa finale allora devo solamente andare a stampare le srade bianche...*/

            }
            else if (mappa->matrice[i][j].cellType == SOURCE)
            {
                colorPrintf(strTmp, BLACK, MAGENTA);
            }
            else
            {
                colorPrintf(strTmp, BLUE, BLUE);
            }
        }
        printf("\n");
    }

    printf("\n");
    for (i = 0; i < numberOfStats; i++, rowCount++)
    {
        printf("%s\n", stats[i]);
    }

    /*vado a stampare tanti \n in modo da fare sembrare che la pagina si stia aggiornando in continuazione...*/
    if (finalPrint == TRUE)
    {
        for (i = 0; i < (size.ws_row - rowCount - 4); i++) printf("\n"); /*stampo tanti \n fino ad ottenere di avere la finestra completamente pulita...*/
    }
    else
    {
        for (i = 0; i < (size.ws_row - rowCount - 2); i++) printf("\n");
    }

    free(strTmp); /*dealloco strTmp*/
}

/*
funzione che controla se la mappa ha zone inaccessibili
*/
void checkForDegeneresMap()
{
    if (SO_HEIGHT < 2 || SO_WIDTH < 2)
    {
        colorPrintf("\n\nWARNING: map could have some places that are not reachable due to map size. \nI reccomend to recompile the program changing SO_WIDTH and SO_HEIGHT accordingly. \nSimulation will now proceed\n\n", YELLOW, DEFAULT);
        printf("Press any key to begin the simulation...");
        getc(stdin);
    }
}

/*
funzione che gestisce cosa succede se ricevo un segnale
in questo caso se ricvo sigalarm imposto esci dal programma a true ed esco
*/
void signalHandler(int signal)
{
    switch (signal)
    {
        case SIGALRM:
            exitFromProgram = TRUE;
            break;

        default:
            break;
    }
}

void searchForTopCells(struct grigliaCitta *mappa, int SO_TOP_CELL)
{
    int i, j;
    int maxValue, maxI, maxJ;
    int placedTopCell = 0;

    maxValue = INT_MIN;
    maxI = 0;
    maxJ = 0;

    while (SO_TOP_CELL > 0)
    {
        maxValue = INT_MIN;
        maxI = 0;
        maxJ = 0;

        for (i = 0; i < SO_HEIGHT; i++)
        {
            for (j = 0; j < SO_WIDTH; j++)
            {
                if ((mappa->matrice[i][j].cellType == ROAD) && (mappa->matrice[i][j].isInTopSoCell == FALSE) && (mappa->matrice[i][j].totalNumberOfTaxiPassedHere > maxValue))
                {
                    maxValue = mappa->matrice[i][j].totalNumberOfTaxiPassedHere;
                    maxI = i;
                    maxJ = j;
                }
            }
        }

        /*marco da stampare la cella a fine simulazione*/
        mappa->matrice[maxI][maxJ].isInTopSoCell = TRUE;

        SO_TOP_CELL--;
    }
}

/*controllare qua dentro che mentre assegno il semaforo non ho errori e che non ho alcun tipo di problema! (errno)*/
void initMap(struct grigliaCitta *mappa, int SO_CAP_MIN, int SO_CAP_MAX, int SO_TIMENSEC_MIN, int SO_TIMENSEC_MAX, int SO_HOLES, int SO_SOURCES)
{

    int i, j;
    int tmp;

    spawnBlocks(mappa, SO_HOLES);
    spawnSources(mappa, SO_SOURCES);

    for (i = 0; i < SO_HEIGHT; i++)
    {
        for (j = 0; j < SO_WIDTH; j++)
        { /*inizialisso il semaforo con lo spazio creto a random tra SO_CAP_MIN e MAX*/
            do {    /*posso fare in questo modo in quanto una volta che ho creto il semafor, esso non deve essere poi recuperato da altri processi in altre variabili in quanto condividono direttamente già il semaforo bello e pronto*/
                /*ciclare fino a che non ho un semaforo non valido!*/
                do {    tmp = mappa->matrice[i][j].availableSpace = semget(IPC_PRIVATE, 1, 0666);
                } while (tmp == -1); /*aspetto che il semaforo sia allocato*/
                tmp = -1;

                do {    tmp = mappa->matrice[i][j].mutex = semget(IPC_PRIVATE, 1, 0666);
                } while (tmp == -1); /*aspetto che il semaforo sia allocato*/

                /*initi di mutex a 1*/
                semctl(mappa->matrice[i][j].mutex, 0, SETVAL, 1); /*imposto mutex a 1*/

            } while (mappa->matrice[i][j].availableSpace == -1); /*fino a che non ottengo un semaforo valido allora continuo a tentare di ottenerne uno. potrebbe essere  che rand()%12000 dia un id già occupato ma ipc_excl ritornerebbe -1. quindi continuo fino a che ne ho uno valido*/

            /*imposto il valore del semaforo a un numero tra socap min e max*/
            if (SO_CAP_MAX > SO_CAP_MIN)
            {   /*con questo evito errori di divisioni per 0. metto > per evitare casi in cui max<min*/
                semctl(mappa->matrice[i][j].availableSpace, 0, SETVAL, SO_CAP_MIN + (rand() % (SO_CAP_MAX - SO_CAP_MIN)));
            }
            else
            {
                semctl(mappa->matrice[i][j].availableSpace, 0, SETVAL, SO_CAP_MIN);
            }

            mappa->matrice[i][j].taxiOnThisCell = 0;
            mappa->matrice[i][j].totalNumberOfTaxiPassedHere = 0;

            if (SO_TIMENSEC_MAX > SO_TIMENSEC_MIN)
            {   /*con questo evito errori di divisioni per 0*/
                mappa->matrice[i][j].timeRequiredToCrossCell = SO_TIMENSEC_MIN + (rand() % (SO_TIMENSEC_MAX - SO_TIMENSEC_MIN));
            }
            else
            {
                mappa->matrice[i][j].timeRequiredToCrossCell = SO_TIMENSEC_MIN;
            }
            /*fprintf(stdout,"[%d][%d]%d\n",i,j ,semctl(mappa->matrice[0][0].availableSpace,0,GETVAL));
            fprintf(stdout,"[%d][%d]%d\n",i,j ,semctl(mappa->matrice[0][0].mutex,0,GETVAL)); DEBUG*/
            mappa->matrice[i][j].isInTopSoCell = FALSE;
        }
    }
    /*Mutex per modificare le variabili contatore nella struct grigliacitta*/
    do {
        mappa->mutex = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | 0666);
    } while (tmp == -1); /*aspetto che il semaforo sia allocato*/
    tmp = -1;
    semctl(mappa->mutex, 0, SETVAL, 1);

    taxi_statistiche->strada_fatta = 0;
    taxi_statistiche->clienti_serviti = 0;
    taxi_statistiche->tempo_in_strada = 0;
    taxi_statistiche->pid_strada_fatta = 0;
    taxi_statistiche->pid_clienti_serviti = 0;
    taxi_statistiche->pid_tempo_in_strada = 0;
    taxi_statistiche->mutex = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | 0666);
    semctl(taxi_statistiche->mutex, 0, SETVAL, 1);
}

/*DECIDERE COME FARE CON NICK*/
void aggiornaStatistiche(struct grigliaCitta *mappa, int *statistiche, int msgQueueId)
{
    struct msqid_ds buffer;
    msgctl(msgQueueId, IPC_STAT, &buffer);
    statistiche[1] = buffer.msg_qnum;
    P(mappa->mutex);
    statistiche[2] = mappa->aborted_rides;
    statistiche[0] = mappa->successes_rides;
    V(mappa->mutex);
    P(taxi_statistiche->mutex);
    statistiche[3] = taxi_statistiche->pid_tempo_in_strada;
    statistiche[4] = taxi_statistiche->pid_strada_fatta;
    statistiche[5] = taxi_statistiche->pid_clienti_serviti;
    V(taxi_statistiche->mutex);
}