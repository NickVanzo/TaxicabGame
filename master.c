#include "include_main.h"


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
    boolean final_print indica se è la stampa finale della mappa e quindi evidenziare le celle più trafficate
*/
void stampaStatistiche(struct grigliaCitta *mappa, int *statistiche, boolean final_print, int SO_TOP_CELLS, int running_time);

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
    boolean final_print indica se è la stampa finale della mappa e quindi evidenziare le celle più trafficate
*/
void stampaStatisticheAscii(struct grigliaCitta *mappa, int *statistiche, boolean final_print, int SO_TOP_CELLS, int running_time);

/*
    Funzione per aggiornare le statistiche
    richiede la mappa su cui deve poter prendere i valori condivisi, 
    il veddore di lunghezza 6 in cui va a inserire il valore delle statistiche (per poi passare il vettore alle tampaStatistiche)
    e l'id della coda di messaggi da cui va a prelevare e statistiche per sapere quanti messaggi sono presenti nella coda di messaggi...
*/
void aggiornaStatistiche(struct grigliaCitta *mappa, int *statistiche, int msg_queue_id);

/*
    funzione per inizializzare la simulazione e tenere il codice del main meno sporco possibile
    questa funzione legge dal stdin o argv i parametri e li assegna.
    Inoltre stampa un riepilogo dei parametri e informa l'utente della dimensione consigliata dello schermo
    aspetta infine che l'utente prenma un tasto per iniziare  la simulazione per potere andare a stampare in continuo.
    in questo modo utente ha tempo di aggiustare lo schermo e controllare che i parametri inseriti sono validi

*/
void setupSimulation(int *SO_TAXI, int *SO_SOURCES, int *SO_HOLES, int *SO_CAP_MIN, int *SO_CAP_MAX, int *SO_TIMENSEC_MIN, int *SO_TIMENSEC_MAX, int *SO_TOP_CELLS, int *SO_TIMEOUT, int *SO_DURATION, int argc, char *argv[], boolean *print_with_ascii);

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

/*
    funzione per potere verificare che una mappa non sia degenere. nel caso che lo sia, genera un errore e interrompe la simulazione
*/
void checkForDegeneresMap();

/*
    questa funzione cerca nella mappa le so_top_cells. nel momento che ne identifica una, 
    marca il parametro della cella is_in_to_so_cells a true, in modo che nella stampa possa venire evidenziata...

*/
void searchForTopCells(struct grigliaCitta *mappa, int SO_TOP_CELL);


/*
    questa funzione procde a fare l'inizializzazione dei parametri delle celle della mappa,
    andando a impostare i valori casuali di SO_CAP e SO_TIMENSEC.
    Inoltre, per convenzione si è deciso che, se l valore del limite superiore è minore del limite inferiore, 
    il valore della cella viene settato al limite inferiore.
    EX: SO_CAP_MIN = 10, SO_CAP_MAX=5 => mappa->matric[x][y].timeRequiredToCrossCell = SO_CAP_MIN
*/
void initMap(struct grigliaCitta *mappa, int SO_CAP_MIN, int SO_CAP_MAX, int SO_TIMENSEC_MIN, int SO_TIMENSEC_MAX, int SO_HOLES, int SO_SOURCES);


/*
    funzione per gestire eventuali segnali guinti al processo
*/
void signalHandler(int signal);


boolean exit_from_program;

struct taxiStatistiche * taxi_statistiche;

int main(int argc, char *argv[])
{
    int i, j; /*variabili iteratrici nei cicli.*/

    int SO_TAXI, SO_SOURCES, SO_HOLES, SO_CAP_MIN, SO_CAP_MAX, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TOP_CELLS, SO_TIMEOUT, SO_DURATION; /*parametri letti o inseriti a compilazione*/

    int map_stats[6]; /*Variabile contenente le statistiche della mappa*/

    key_t queue_key; /*variabili per ottenere le risorse condivise*/
    int queue_id;
    int shm_id, shm_key, shm_key_ForTaxi, shm_key_stats, shm_id_stats;

    int *child_source_created; /*puntatori ad array che salveranno i PID dei ligli creati*/
    int *child_taxi_created;

    /*variabili per la conversione dei parametri letti a tempo di esecuzione in stringhe per poterli passare ai figli*/
    char  SO_SOURCES_PARAM[10], SO_TIMEOUT_PARAM[10], SO_DURATION_PARAM[10];

    int running_time = 0; /*variabile che indica il tempo in secondi di esecuzione. utile per distinguere le il processo master è bloccato o sta continuando l'esecuzione*/

    int taxi_semaphore_id; /*semaforo usato per fermare l'esecuzione dei taxi fino al momento in cui tutti possono partire insieme*/

    int trash_kill_signal; /*variabile per raccogliere stato segnale uscita*/

    int wait_pid; /*variabile usata per capire se un taxi è morto, e nel caso ricrearlo*/

    struct sigaction sa;
    
    boolean print_with_ascii = FALSE; /*variabile usata per capire se devo stampare con una modalità più compatta o meno*/

 
    struct grigliaCitta * mappa; /*struct contenente la mappa della città*/

    /*
        --INIZIO ESECUZIONE MAIN--
    */
    /*Impostazione dell'handler di sigalrm*/
    bzero(&sa, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigaction(SIGALRM, &sa, NULL);
     
    checkForDegeneresMap(); /*controllo che la mappa non sia degenere ovvero che tutti i punti possano essere raggiunti -> mappa deve essere almeno 2x2. SE NON è IL CASO TERMINO ESECUZIONE*/

   
    setupSimulation(&SO_TAXI, &SO_SOURCES, &SO_HOLES, &SO_CAP_MIN, &SO_CAP_MAX, &SO_TIMENSEC_MIN, &SO_TIMENSEC_MAX, &SO_TOP_CELLS, &SO_TIMEOUT, &SO_DURATION, argc, argv, &print_with_ascii);  /*AVVIO SETUP SIMULAZIONE*/

    /*
        --CREAZIONE RISORSE CONDIVISE--
    */

    /*Coda di messaggi contenente le richieste di trasporto per i taxi*/
    queue_key = ftok("ipcKey.key", 1);/*Ottengo la chiave per la coda di messaggi*/
    if ((queue_id = msgget(queue_key, IPC_CREAT | IPC_EXCL | 0666)) == -1)/*Ottengo l'id della coda di messaggi cosi' da disallocare in seguito la coda*/
    {
        fprintf(stderr, "Errore nella creazione della coda di messaggi. Codice errore: %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    

    /*Creazione memoria condivisa contenente la mappa con relativi controlli sulla mappa*/
    shm_key = ftok("ipcKey.key", 2);
    shm_id = shmget(shm_key, sizeof(struct grigliaCitta), IPC_CREAT | IPC_EXCL | 0666);
    mappa = shmat(shm_id, NULL, 0);
    if (mappa == (struct grigliaCitta *)(-1))
    {
        printf("Error at shmat! error code is %s", strerror(errno)); /*in caso di errore rilascio risorse precedenti ed esco dal programma*/
        msgctl(queue_id, IPC_RMID, NULL);
        shmctl(shm_id, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }


    /*Creazione memoria condivisa per le statistiche dei taxi*/
    shm_key_stats = ftok("ipcKey.key", 5);
    shm_id_stats = shmget(shm_key_stats, sizeof(struct taxiStatistiche), IPC_CREAT | IPC_EXCL | 0666);
    taxi_statistiche = shmat(shm_id_stats, NULL, 0);
    if(taxi_statistiche == (struct taxiStatistiche *)(-1))
    {
        printf("Error at shmat! error code is %s", strerror(errno)); /*in caso di errore rilascio risorse precedenti ed esco dal programma*/
        msgctl(queue_id, IPC_RMID, NULL);
        shmdt(mappa);
        shmctl(shm_id, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }


    srand(getpid()); /*init della rand per la funzione di assegnazione*/

    for (i = 0; i < 6; i++) map_stats[i] = 0; /*init del vettore statistiche, in qunto non essendo globale ha valori casuali*/

    initMap(mappa, SO_CAP_MIN, SO_CAP_MAX, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_HOLES, SO_SOURCES); /*init dei parametri delle celle della mappa e degli attributi condivisi della mappa*/


    /*
        --CREAZIONE FIGLI SOURCES--
    */
    
    sprintf(SO_SOURCES_PARAM, "%d", SO_SOURCES); /*preparo i parametri da mandare come argomenti alla execlp*/
    sprintf(SO_DURATION_PARAM, "%d", SO_DURATION);
    sprintf(SO_TIMEOUT_PARAM, "%d", SO_TIMEOUT);
 
    child_source_created = malloc(SO_SOURCES* sizeof(int));    /*creo array contenente i pid dei figli source creati*/
    
    for (i = 0; i < SO_SOURCES; i++) /*faccio la fork per poterer creare i processi che generano le richieste da  sources*/
    {
        child_source_created[i] = fork();
        switch (child_source_created[i])
        {
            case -1:
                printf("Error while trying to fork()! %s\n", strerror(errno)); /*in caso di errore esco disallocando le risorse condivise*/
                msgctl(queue_id, IPC_RMID, NULL);
                shmdt(mappa);
                shmdt(taxi_statistiche);
                shmctl(shm_id_stats, IPC_RMID, NULL);
                shmctl(shm_id, IPC_RMID, NULL);
                exit(EXIT_FAILURE);
                break;
            case 0:
                /*cambio il programma in esecuzione*/
                execlp("./source", "source", SO_SOURCES_PARAM, SO_DURATION_PARAM, SO_TIMEOUT_PARAM, NULL); /*se riesco avvio nuovo processo figlio, altrimenti esco deallocando le risorse condivise*/
                printf("Error loading new program %s!\n\n", strerror(errno));
                msgctl(queue_id, IPC_RMID, NULL);
                shmdt(mappa);
                shmdt(taxi_statistiche);
                shmctl(shm_id_stats, IPC_RMID, NULL);
                shmctl(shm_id, IPC_RMID, NULL);
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }

  
    /*creazione di variabile semaforo per potere fare aspettare tutti i taxi prima che inizino la loro esecuzione*/
    shm_key_ForTaxi = ftok("ipcKey.key", 3);
    taxi_semaphore_id = semget(shm_key_ForTaxi, 1, IPC_CREAT | IPC_EXCL | 0666);
    semctl(taxi_semaphore_id, 0, SETVAL, SO_TAXI);

    child_taxi_created = malloc(SO_TAXI* sizeof(int));     /*Creao un array contenente i pid dei figli taxi creati*/
    for (i = 0; i < SO_TAXI; i++)
    {
        child_taxi_created[i] = fork();
        switch (child_taxi_created[i])
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
 

    
    exit_from_program = FALSE; /*fino a che questa variabile è falsa, continuo a rimanere nel programma. se diventa vera allora devo uscire dal loop principale e posso terminare l'esecuzione andando a disallocare le risorse codivise*/
    
    alarm(SO_DURATION);/*imposto durata simulazione*/

    while (!exit_from_program)
    {

        sprintf(SO_DURATION_PARAM, "%d", SO_DURATION - running_time); /*preparo un eventuale parametro, per nuovi taxi che devono essere ricreati, causa morte di altri taxi*/

       /*Se un taxi muore lo ricreiamo in un altra posizione della griglia, andando a sostituire il pid del morto con quello dell'appena nato. So che non ho taxi morti se la waitpid mi ritorna 0. Inoltre la waitpid qua non è bloccante a causa del WNOHANG*/
        while ((wait_pid = waitpid(-1, NULL, WNOHANG)) > 0)  
        {
            for (i = 0; i < SO_TAXI; i++)
            {
                if (child_taxi_created[i] == wait_pid)
                {
                    child_taxi_created[i] = fork();
                    switch (child_taxi_created[i])
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

        aggiornaStatistiche(mappa, map_stats, queue_id); /*procedo ad aggiornare le statistiche*/

        if (print_with_ascii) /*se ho lo schermo piccolo stampo una versione più compatta dell'output*/
        { 
            stampaStatisticheAscii(mappa, map_stats, FALSE, SO_TOP_CELLS, running_time);
        }
        else
        {
            stampaStatistiche(mappa, map_stats, FALSE, SO_TOP_CELLS, running_time);
        }
        running_time++;
        sleep(1); /*in questo modo posso stampare con frequenza 1 stampa / secondo */
    }

    aggiornaStatistiche(mappa, map_stats, queue_id); /*aggiorno le statistiche della simulazione un ultima volta per la stampa finale*/
   
    searchForTopCells(mappa, SO_TOP_CELLS); /*cerco e marco le SO_TOP_CELL*/

    if (print_with_ascii) /*procedo alla tampa finale della simulazione*/
    {
        stampaStatisticheAscii(mappa, map_stats, TRUE, SO_TOP_CELLS, running_time);
    }
    else
    {
        stampaStatistiche(mappa, map_stats, TRUE, SO_TOP_CELLS, running_time);
    }

    /*attendo la morte di tutti i figli, e nel caso la forzo*/
    for (i = 0; i < SO_TAXI; i++)
    {
        kill(child_taxi_created[i], SIGKILL);
        waitpid(child_taxi_created[i], &trash_kill_signal);
    }

    for (i = 0; i < SO_SOURCES; i++)
    {
        kill(child_source_created[i], SIGINT);
        waitpid(child_source_created[i], &trash_kill_signal);
    }

    /*
        --RIMOZIONE RISORSE CONDIVISE
    */

    for (i = 0; i < SO_HEIGHT; i++)
    {
        for (j = 0; j < SO_WIDTH; j++)
        {
            semctl(mappa->matrice[i][j].availableSpace, 0, IPC_RMID, 0); /*rimuovo i semafori*/
            semctl(mappa->matrice[i][j].mutex, j, IPC_RMID, 0); /*rimuovo i semafori*/

        }
    }
    free(child_source_created);
    free(child_taxi_created);
    semctl(mappa->mutex, 0, IPC_RMID);
    semctl(taxi_semaphore_id, 0, IPC_RMID);
    semctl(taxi_statistiche->mutex, 0, IPC_RMID);
    shmdt(mappa);
    shmdt(taxi_statistiche);
    shmctl(shm_id_stats, IPC_RMID, NULL);
    shmctl(shm_id, IPC_RMID, NULL);
    msgctl(queue_id, IPC_RMID, NULL);

    exit(EXIT_SUCCESS); /*esecuzione conslusa con successo*/
}

void spawnBlocks(struct grigliaCitta *mappa, int SO_HOLES)
{
    int k, q; /*Variabili usate per ciclare*/
    int numero_buchi = 0;
    
    for (k = 0; k < SO_HEIGHT; k++) /*inizializzo il tipo di ogni cella della tabella ROAD e marco come 1(inteso come la variabile booleana TRUE) la disponibilita' di ospitare un buco*/
    {
        for (q = 0; q < SO_WIDTH; q++)
        {
            mappa->matrice[k][q].cellType = ROAD;
            mappa->matrice[k][q].availableForHoles = 1;
        }
    }

    while (numero_buchi < SO_HOLES)    /*Ciclo fino a quando non ho posizionato tutti i buchi richiesti*/
    {
        /*Randomizzo le coordinate della cella che voglio far diventare buco*/
        k = rand() % SO_HEIGHT;
        q = rand() % SO_WIDTH;
        if (mappa->matrice[k][q].availableForHoles == 1)  /*La cella con le coordinate ottenute randomicamente vengono segnate come BLOCK*/
        {
            mappa->matrice[k][q].cellType = BLOCK;
            mappa->matrice[k][q].availableForHoles = 0;

            /*La cella su cui posiziono il buco non potra' contenere altri buchi e nemmeno le 8 celle che ha intorno potranno. marco quindi come inutilizzabili per contenere buchi le celle intorno alla cella su cui ho posizionato il buco*/
            if (k > 0)
            {
                if (q > 0)/*Cella in alto a sinistra*/
                {       
                    mappa->matrice[k - 1][q - 1].availableForHoles = 0;
                }
                mappa->matrice[k - 1][q].availableForHoles = 0;  /*Cella in alto centrale*/
                if (q < SO_WIDTH - 1) /*Cella in alto a destra*/
                {       
                    mappa->matrice[k - 1][q + 1].availableForHoles = 0;
                }
            }
            if (q > 0) /*Cella centrale a sinistra*/
            {   
                mappa->matrice[k][q - 1].availableForHoles = 0;
            }
             /*Per la cella centrale non ho bisogno di un ulteriore controllo perche' e' la cella su cui siamo presenti ora*/
            if (q < SO_WIDTH - 1)/*Cella centrale a destra*/
            {   
                mappa->matrice[k][q + 1].availableForHoles = 0;
            }
            if (k < SO_HEIGHT - 1)
            {
                if (q > 0)  /*Cella in basso a sinistra*/
                {     
                    mappa->matrice[k + 1][q - 1].availableForHoles = 0;
                }
                mappa->matrice[k + 1][q].availableForHoles = 0; /*Cella in basso centrale*/
                if (q < SO_WIDTH - 1)  /*Cella in basso a destra*/
                {     
                    mappa->matrice[k + 1][q + 1].availableForHoles = 0;
                }
            }
    
            numero_buchi++; /*Se il buco e' stato posizionato allora incremento, altrimenti non ho trovato una posizione valida, continuo il ciclo*/
        }
    }
}

void spawnSources(struct grigliaCitta *mappa, int SO_SOURCES)
{
    int numero_sources = 0; /*variabile per tenere conto di quante Sources sono state piazzate nella mappa*/
    int n, l; /*variabili che determinano il posizionamento nella griglia*/

    while (numero_sources < SO_SOURCES) /*La Source puo' essere posizionata solamente in celle della griglia che prima erano ROAD, un BLOCK non puo' diventare ROAD*/
    {
        n = rand() % SO_HEIGHT;  /*Assegno alle variabili n ed l un numero random che serve per posizionarmi sulla griglia*/
        l = rand() % SO_WIDTH;
        
        if (mappa->matrice[n][l].cellType == ROAD) /*Se la cella che mi e' capitata e' di tipo ROAD allora posiziono la SOURCE e incremento il numero di SOURCE posizionate*/
        {
            mappa->matrice[n][l].cellType = SOURCE;
            numero_sources++;
        }
        /*Se la cella che mi e' capitata non e' di tipo ROAD allora continuo il ciclo senza incrementare il numero di SOURCE posizionate*/
    }
}

void setupSimulation(int *SO_TAXI, int *SO_SOURCES, int *SO_HOLES, int *SO_CAP_MIN, int *SO_CAP_MAX, int *SO_TIMENSEC_MIN, int *SO_TIMENSEC_MAX, int *SO_TOP_CELLS, int *SO_TIMEOUT, int *SO_DURATION, int argc, char *argv[], boolean *print_with_ascii)
{
    int screen_height, screen_width;
    char buffer_temp[1024]; /*buffer temporaneo per lettura dei parametri da tastiera*/
    char tmp_char; /*per leggere eventuali input non desiderati*/

    struct winsize size; /*struttura per ottenere la dimensione della finestra (api POSIX)... advanced programming for unix pagina 711. la uso per sapere se devo o meno stampare con asci o con int*/

    ioctl(STDIN_FILENO, TIOCGWINSZ, &size); /*ottengo la dimensione della finestra. la funzione ioctl serve a controlare un device (nel mio caso il terminale). con la ioctl posso volenado anche fare altre operazioni, come spostare il cursore nello schermo*/

    if (argc == 11) /*1 nome comando + 10 parametri. se ho tutti i parametri allora procedo a assegnarli e a continuare l'esecuzione*/
    {
        *SO_TAXI = abs(atoi(argv[1])); 
        *SO_SOURCES = abs(atoi(argv[2]));
        *SO_HOLES =  abs(atoi(argv[3]));
        *SO_CAP_MIN =  abs(atoi(argv[4]));
        *SO_CAP_MAX =  abs(atoi(argv[5]));
        *SO_TIMENSEC_MIN =  abs(atoi(argv[6]));
        *SO_TIMENSEC_MAX =  abs(atoi(argv[7]));
        *SO_TOP_CELLS =  abs(atoi(argv[8]));
        *SO_TIMEOUT =  abs(atoi(argv[9]));
        *SO_DURATION = abs(atoi(argv[10]));

        if(*SO_SOURCES > SO_WIDTH * SO_HEIGHT)
        {
            printf("Error: Too many sources! try again...\n");
            exit(EXIT_FAILURE);
        }

        if(*SO_HOLES > SO_WIDTH * SO_HEIGHT - *SO_SOURCES)
        {
            printf("Error: Too many holes! try again...\n");
            exit(EXIT_FAILURE);
        }

        if(*SO_TOP_CELLS > SO_WIDTH*SO_HEIGHT - *SO_HOLES - *SO_SOURCES){
            printf("Error: Too many SO_TOP_CELLS! Try again...\n");
            exit(EXIT_FAILURE);
        }

        if(*SO_DURATION < 2){
            printf("Error: SO_DURATION is too small! Try again...\n");
            exit(EXIT_FAILURE);
        }


    }
    else /*se il numero di parametri non è corretto proseguo a prelevare i parametri da stdin*/
    {
        printf("Insert number of taxi available for the simulation: ");
        fgets(buffer_temp, 20, stdin); /*leggo il testo inserito nello stdin un intero ha al massimo 10 simboli ma non fidandomi nell'utente dico che ne leggo al massimo 20 (inclus il \n alla fine)*/
        *SO_TAXI = abs(atoi(buffer_temp)); /*converto da stringa a intero*/

        do {    /*in questo caso devo assicurarmi la correttezza del valore, percui, procedo a controllare che sia giusto e se sbagliato richiedo il valore. le linee di codice successive hanno lo stesso stile percui non vengono commentate*/
            printf("Insert number of sources available for the simulation(MAX = %d): ", SO_WIDTH *SO_HEIGHT);
            fgets(buffer_temp, 20, stdin); 
            *SO_SOURCES = atoi(buffer_temp); 
            if (*SO_SOURCES >= SO_WIDTH *SO_HEIGHT)
            {
                colorPrintf("Insert again the number of SOURCES. Too many!!\n", RED, DEFAULT);
            }
        } while (*SO_SOURCES >= SO_WIDTH *SO_HEIGHT);


        do {    
            printf("Insert number of holes available for the simulation: ");
            fgets(buffer_temp, 20, stdin); 
            *SO_HOLES = abs(atoi(buffer_temp)); 
            if (*SO_HOLES >= (SO_WIDTH *SO_HEIGHT - *SO_SOURCES))
            {
                colorPrintf("Insert again the number of HOLES. Too many holes.\n", RED, DEFAULT);
            }
        } while (*SO_HOLES >= (SO_WIDTH *SO_HEIGHT - *SO_SOURCES));

        printf("Insert minimum capacity for each cell: ");
        fgets(buffer_temp, 20, stdin);
        *SO_CAP_MIN = abs(atoi(buffer_temp)); 

        printf("Insert maximum capacity for each cell: ");
        fgets(buffer_temp, 20, stdin);
        *SO_CAP_MAX = atoi(buffer_temp); 

        if(*SO_CAP_MIN >= *SO_CAP_MAX) colorPrintf("Notice: SO_CAPACITY will be set to SO_CAP_MIN\n", YELLOW, DEFAULT); /*se ho il valore di max, minore uguale a min allora avviso che il numero della capacità non sarà random ma verrà impostato a SO_CAP_MIN*/

        printf("Insert minimum crossing time for each cell (nanoseconds - max: 999999999): ");
        fgets(buffer_temp, 20, stdin); 
        *SO_TIMENSEC_MIN = atoi(buffer_temp); 

        printf("Insert maximum crossing time for each cell (nanoseconds - max: 999999999): ");
        fgets(buffer_temp, 20, stdin); 
        *SO_TIMENSEC_MAX = atoi(buffer_temp); 

        if(*SO_TIMENSEC_MIN >= *SO_TIMENSEC_MAX) colorPrintf("Notice: SO_TIMENSEC will be set to SO_TIMENSEC_MIN\n", YELLOW, DEFAULT);

        do {    
            printf("Insert number of top cells to be shown at the end of the simulation: ");
            fgets(buffer_temp, 20, stdin); 
            *SO_TOP_CELLS = atoi(buffer_temp); 
            if (*SO_TOP_CELLS >= SO_WIDTH *SO_HEIGHT - *SO_HOLES)
            {
                printf("Insert again the number of SO_TOP_CELLS. Too many!!\n");
            }
        } while (*SO_TOP_CELLS >= SO_WIDTH *SO_HEIGHT - *SO_HOLES);


        printf("Insert taxi move timeout (milliseconds): ");
        fgets(buffer_temp, 20, stdin); 
        *SO_TIMEOUT = atoi(buffer_temp);

        /*La smulazione deve durare almeno due secondi per via di un problema in cui eseguiamo un operazione di modulo, e si rischia di avere x%0 se la simulazione dura 1 secondo */
        do {    
            printf("Insert simulation duration (seconds >= 2): ");
            fgets(buffer_temp, 20, stdin); 
            *SO_DURATION = abs(atoi(buffer_temp));
            if (*SO_DURATION < 2)
            {
                printf("Insert again the number SO_DURATION value. Too small!!\n");
            }
        } while (*SO_DURATION < 2); 

    }

    /*calcolo la dimensione ottimale per potere avere la migliore esperienza*/

    if (SO_HEIGHT + 4 < 14) screen_height = 14; /*se la dimensione della tabella è 9, diventa 13 (con 2 extra sopra e 2 extra sotto*/
    else screen_height = SO_HEIGHT + 4;

    screen_width = 4 *(2 + SO_WIDTH) + 55; /*lunghezza della singola cella per la dimenzione della mappa piu la len max delle stat*/

    /*stampo un riepilogo dei parametri di esecuzione della simulazione*/
    printf("Simulation will now start with thw following parameters:\n\n\tSO_TAXI: %d\n\tSO_SOURCES: %d\n\tSO_HOLES: %d\n\tSO_CAP_MIN: %d\n\tSO_CAP_MAX: %d\n\tSO_TIMENSEC_MIN: %d\n\tSO_TIMENSEC_MAX: %d\n\tSO_TOP_CELLS: %d\n\tSO_TIMEOUT: %d\n\tSO_DURATION: %d\n\n%s For a better experience, a terminal with minimum %d char width and minimum %d character height is required %s\n\nAre you ok with the following parameters? (y/n, Default:y): ", *SO_TAXI, *SO_SOURCES, *SO_HOLES, *SO_CAP_MIN, *SO_CAP_MAX, *SO_TIMENSEC_MIN, *SO_TIMENSEC_MAX, *SO_TOP_CELLS, *SO_TIMEOUT, *SO_DURATION, C_YELLOW, screen_width, screen_height, C_DEFAULT);
    tmp_char = getc(stdin); /*leggo input per potere avviare simulazione, prima assicurandomi che la dimensione del terminale sia mantenuta delle dimansioni buone*/
    getc(stdin); /*pulisto da eventuali 7n che rompono le scatole*/
    /*verifico che la scelta sia negativa e se lo è riavvio la richiesta dei dati... con argc però a zero per farti modificare i numeri!*/
    if (tmp_char == 'n' || tmp_char == 'N') setupSimulation(SO_TAXI, SO_SOURCES, SO_HOLES, SO_CAP_MIN, SO_CAP_MAX, SO_TIMENSEC_MIN, SO_TIMENSEC_MAX, SO_TOP_CELLS, SO_TIMEOUT, SO_DURATION, 0, argv, print_with_ascii);

    if (screen_width > size.ws_col) /*se ho una dimensione dello schermo minore della dimensione raccomandata, chiedo se voglio avere una stampa in formato più compatto*/
    {
        colorPrintf("\nWarning!", YELLOW, DEFAULT);
        printf(" screen size width might be too small to fit a beautified output version... would you like to print a more compact rappresentation? (y/n - default: n): ");
        tmp_char = getc(stdin);
        getc(stdin); /*tolgo lo \n che senno fa partire subito la simulazione*/
        if (tmp_char == 'y') *print_with_ascii = TRUE;
        else *print_with_ascii = FALSE;

        printf("\nSimulation will now start. Press any key to begin....");
        getc(stdin);
    }

}

void stampaStatistiche(struct grigliaCitta *mappa, int *statistiche, boolean final_print, int SO_TOP_CELLS, int running_time)
{
    /*poichè questa funzione è identica alla stampastatisticheAscii si commenta solamente questa...*/
    int i, j, k, printed_stats = 0,  row_count = 0; /*row_count serve a contare quante righe sto stampando...*/

    char stats[13][128]; /*array di stringhe contententi il testo formattato della stampa delle statistiche*/
    const int number_of_stats = 13; /*numero di linee di statistiche da stampare*/
    char *strTmp = (char*) malloc(7); /*dichiaro una str temporanea d usare nella sprintf per poi passarla alla colorPrintf.*/

    struct winsize size; /*struttura per ottenere la dimensione della finestra... advanced programming for unix pae 711*/

    ioctl(STDIN_FILENO, TIOCGWINSZ, &size); /*ottengo la dimensione della finestra per sapere quanti \n stampare alla fine per dare impressione di avere un aggiornamento dello schermo continuo*/

    /*creo le stringhe da stampare*/
    sprintf(stats[0], "%s", " |\033[33m Statistics for running simulation \033[39m");
    sprintf(stats[1], "%s%d", " | Number of successful rides: ", statistiche[0]);

    if (!final_print) sprintf(stats[2], "%s%d", " | Number of pending rides: ", statistiche[1]); /*Faccio la differenziazione: il numero di corse unsuccesful sono il numero di corse che sono rimmaste nella coda a fine della simulazione*/
    else sprintf(stats[2], "%s%d", " | Number of unsuccessful rides: ", statistiche[1]);

    sprintf(stats[3], "%s%d", " | Number of aborted rides: ", statistiche[2]);
    sprintf(stats[4], "%s%d", " | Cumulative longest driving taxi: ", statistiche[3]);
    sprintf(stats[5], "%s%d", " | Cumulative farthest driving taxi: ", statistiche[4]);
    sprintf(stats[6], "%s%d", " | Taxi with most succesfoul rides: ", statistiche[5]);
    sprintf(stats[7], "%s %d seconds...", " | Running time :", running_time);
    sprintf(stats[8], "%s", " |\033[33m Colours legend: \033[39m");
    sprintf(stats[9], "%s", " | \033[40m  \033[49m -> Black shows blocked zones");
    sprintf(stats[10], "%s", " | \033[45m  \033[49m -> Magenta shows source points");
    sprintf(stats[11], "%s", " | \033[107m  \033[49m -> White shows roads");
    sprintf(stats[12], "%s", " | \033[43m  \033[49m -> Yellow shows SO_TOP_CELLS (only final print)");

    for (k = 0; k < 2; k++, printed_stats++, row_count++)
    {
        /*stampo il bordo superiore: 4 caratteri con sfondo grigio. nella seconda riga di intestazione stampo anche il numero di colonna in rosso*/
        colorPrintf("    ", GRAY, GRAY);
        for (i = 0; i < SO_WIDTH + 1; i++)
        {
            sprintf(strTmp, "%-4d", i);
            if (k > 0 && i < SO_WIDTH) colorPrintf(strTmp, RED, GRAY);
            else colorPrintf("    ", GRAY, GRAY);
        }
        printf("%s\n", stats[printed_stats]);
    }


    for (i = 0; i < SO_HEIGHT; i++, row_count++)/*stampo il corpo della mappa*/
    {
        sprintf(strTmp, "%3d ", i);
        colorPrintf(strTmp, RED, GRAY); /*stampo bordo laterale sx*/
        for (j = 0; j < SO_WIDTH; j++)
        {
            P(mappa->matrice[i][j].mutex);
            sprintf(strTmp, "%-4d", mappa->matrice[i][j].taxiOnThisCell); /*preparo la stringa da stampare nella cella, accedendo alla mappa in mutua esclusione*/
            V(mappa->matrice[i][j].mutex);

            if (mappa->matrice[i][j].cellType == ROAD)
            {   /*se sono alla stampa finale e sono in una SO_TOP_CELL allora vado a mostrare i vari colori nelle celle altrimenti mostro solo l'occupazione...*/
                if ((final_print == TRUE) && (mappa->matrice[i][j].isInTopSoCell == TRUE))
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
                if ((final_print == TRUE) && (mappa->matrice[i][j].isInTopSoCell == TRUE))
                {
                    colorPrintf(strTmp, MAGENTA, YELLOW);
                } 
                else
                {
                    colorPrintf(strTmp, BLACK, MAGENTA);
                } 
            }
            else /*se è una strada di tipo block*/
            {
                colorPrintf("    ", BLACK, BLACK);
            }
        }

        colorPrintf("    ", GRAY, GRAY); /*stampo bordo laterale dx, e affianco al bordo stampo anche le statistiche*/
        if (printed_stats < number_of_stats)
        {
            printf("%s\n", stats[printed_stats]);
            printed_stats++;
        }
        else
        {
            printf("\n");
        }
    }

    for (k = 0; k < 2; k++, row_count++)/*stampo bordo inferiore*/
    {
        for (i = 0; i < SO_WIDTH + 2; i++) colorPrintf("    ", GRAY, GRAY);
        if (printed_stats < number_of_stats)
        { /*stampo statistica a financo banda grigia*/
            printf("%s\n", stats[printed_stats]);
            printed_stats++;
        }
        else
        {
            printf("\n");
        }
    }

    if (printed_stats < number_of_stats) /*controllo che ho stampato tutti gli stats e se non lo ho fatto stampo quelli che mancano allineandoli agli altri sotto*/
    {
        for (i = printed_stats; i < number_of_stats; i++, row_count++)
        {
            for (j = 0; j < 7 *(SO_WIDTH + 2); j++) printf(" ");
            printf("%s\n", stats[i]);
        }
    }

    if (final_print == TRUE)/*vado a stampare tanti \n in modo da fare sembrare che la pagina si stia aggiornando in continuazione... se sono alla final print ne stampo meno...*/
    {
        for (i = 0; i < (size.ws_row - row_count - 3); i++) printf("\n"); /*stampo tanti \n fino ad ottenere di avere la finestra completamente pulita...*/
    }
    else
    {
        for (i = 0; i < (size.ws_row - row_count - 1); i++) printf("\n");
    }

    free(strTmp); /*dealloco strTmp*/
}

void stampaStatisticheAscii(struct grigliaCitta *mappa, int *statistiche, boolean final_print, int SO_TOP_CELLS, int running_time)
{
    int i, j, k, row_count = 0;

    char stats[12][128];
    const int number_of_stats = 12;
    char *strTmp = (char*) malloc(7);

    struct winsize size; 

    ioctl(STDIN_FILENO, TIOCGWINSZ, &size); 

    sprintf(stats[0], "%s", " |\033[33m Statistics for running simulation \033[39m");
    sprintf(stats[1], "%s%d", " | Number of successful rides: ", statistiche[0]);

    if (!final_print) sprintf(stats[2], "%s%d", " | Number of pending rides: ", statistiche[1]);
    else sprintf(stats[2], "%s%d", " | Number of unsuccessful rides: ", statistiche[1]);

    sprintf(stats[3], "%s%d", " | Number of aborted rides: ", statistiche[2]);
    sprintf(stats[4], "%s%d", " | Cumulative longest driving taxi: ", statistiche[3]);
    sprintf(stats[5], "%s%d", " | Cumulative farthest driving taxi: ", statistiche[4]);
    sprintf(stats[6], "%s%d", " | Taxi with most succesfoul rides: ", statistiche[5]);
    sprintf(stats[7], "%s %d seconds...", " | Running time :", running_time);
    sprintf(stats[8], "%s", " |\033[33m Colours legend: \033[39m");
    sprintf(stats[9], "%s", " | \033[44m  \033[49m -> Blue shows blocked zones");
    sprintf(stats[10], "%s", " | \033[45m  \033[49m -> Magenta shows source points");
    sprintf(stats[11], "%s", " | \033[43m  \033[49m -> Yellow shows SO_TOP_CELLS (only final print)");

    printf("    ");
    for (k = 0; k < SO_WIDTH; k++)
    {
        /*stampo il bordo superiore*/
        sprintf(strTmp, "%-4d", k);
        colorPrintf(strTmp, RED, DEFAULT);
    }
    printf("\n");
    row_count++;

    for (i = 0; i < SO_HEIGHT; i++, row_count++)
    {
        sprintf(strTmp, "%-4d", i);
        colorPrintf(strTmp, RED, DEFAULT); 
        for (j = 0; j < SO_WIDTH; j++)
        {

            P(mappa->matrice[i][j].mutex);
            sprintf(strTmp, "%-4d", mappa->matrice[i][j].taxiOnThisCell); 
            V(mappa->matrice[i][j].mutex);

            if (mappa->matrice[i][j].cellType == ROAD)
            {   
                if ((final_print == TRUE) && (mappa->matrice[i][j].isInTopSoCell == TRUE))
                {
                    colorPrintf(strTmp, BLACK, YELLOW);
                }
                else
                {
                    printf("%s", strTmp);
                } 
            }
            else if (mappa->matrice[i][j].cellType == SOURCE)
            {
                if ((final_print == TRUE) && (mappa->matrice[i][j].isInTopSoCell == TRUE))
                {
                    colorPrintf(strTmp, MAGENTA, YELLOW);
                } /*se sono alla stampa finale, mostro la most frquented cell...*/
                else
                {
                    colorPrintf(strTmp, BLACK, MAGENTA);
                } 
            }
            else
            {
                colorPrintf(strTmp, BLUE, BLUE);
            }
        }
        printf("\n");
    }

    printf("\n");
    for (i = 0; i < number_of_stats; i++, row_count++)
    {
        printf("%s\n", stats[i]);
    }
    if (final_print == TRUE)
    {
        for (i = 0; i < (size.ws_row - row_count - 4); i++) printf("\n"); 
    }
    else
    {
        for (i = 0; i < (size.ws_row - row_count - 2); i++) printf("\n");
    }
    free(strTmp);
}

void checkForDegeneresMap()
{
    if (SO_HEIGHT < 2 || SO_WIDTH < 2) /*se ho una mappa con dimensioni di altezza o larghezza minore di 2 potrei avere dei punti non raggiungibili, percui esco dalla simulazione*/
    {
        colorPrintf("\n\nWARNING: map could have some places that are not reachable due to map size. \n A change in SO_WIDTH and SO_HEIGHT is required to run the simulation...\n.", YELLOW, DEFAULT);
        exit(EXIT_FAILURE);
    }
}

void signalHandler(int signal)
{
    switch (signal)
    {
        case SIGALRM: /*se ricevo un sigalarm, risveglio il processo e dico che devo uscire dal loop principale*/
            exit_from_program = TRUE;
            break;

        default:
            break;
    }
}

void searchForTopCells(struct grigliaCitta *mappa, int SO_TOP_CELL)
{
    int i, j;
    int max_value, max_i, max_j;
    int placedTopCell = 0;

    max_value = INT_MIN;
    max_i = 0;
    max_j = 0;

    while (SO_TOP_CELL > 0) /*cerco in tutta la metrice le X top cells e le marco come da stampare top cells. dalla stampa sono escluse celle di tipo SOURCES E BLOCK*/
    {
        max_value = INT_MIN;
        max_i = 0;
        max_j = 0;

        for (i = 0; i < SO_HEIGHT; i++)
        {
            for (j = 0; j < SO_WIDTH; j++)
            {
                if ((mappa->matrice[i][j].cellType != BLOCK) && (mappa->matrice[i][j].isInTopSoCell == FALSE) && (mappa->matrice[i][j].totalNumberOfTaxiPassedHere > max_value)) /*se la cella ha un numero di taxi passato maggiore e se è di tipo road e se non è già marcata come top allora salvo le sue coordinate*/
                {
                    max_value = mappa->matrice[i][j].totalNumberOfTaxiPassedHere;
                    max_i = i;
                    max_j = j;
                }
            }
        }

        /*marco da stampare la cella a fine simulazione*/
        mappa->matrice[max_i][max_j].isInTopSoCell = TRUE; /*marco la cella come da stampare tra le top cells*/

        SO_TOP_CELL--; /*devo cercare ancora meno SO_TOP_CELL*/
    }
}

void initMap(struct grigliaCitta *mappa, int SO_CAP_MIN, int SO_CAP_MAX, int SO_TIMENSEC_MIN, int SO_TIMENSEC_MAX, int SO_HOLES, int SO_SOURCES)
{

    int i, j;
    int tmp;

    spawnBlocks(mappa, SO_HOLES); /*creo le celle di tipo block*/
    spawnSources(mappa, SO_SOURCES); /*creo le celle di tipo source*/

    /*procedo a inizializare tutte le celle della mia mappa*/
    for (i = 0; i < SO_HEIGHT; i++)
    {
        for (j = 0; j < SO_WIDTH; j++)
        { 

                do {    
                    tmp = (mappa->matrice[i][j].availableSpace = semget(IPC_PRIVATE, 1, 0666)); /*Ottengo un semaforo che indichi la quantità di posti disponibili sulla cella. è semaforo di tipo contatore. lo faccio fino a che il valore della cella è == -1 (non ho un semaforo valido)*/
                } while (tmp == -1); /*aspetto che il semaforo sia allocato*/
                
                tmp = -1;

                do {    
                    tmp = (mappa->matrice[i][j].mutex = semget(IPC_PRIVATE, 1, 0666)); /*come sopra ma ottengo un semaforo per il mutex*/
                } while (tmp == -1); /*aspetto che il semaforo sia allocato*/

                
                semctl(mappa->matrice[i][j].mutex, 0, SETVAL, 1); /*imposto mutex a 1*/


           
            if (SO_CAP_MAX > SO_CAP_MIN)  /*con questo evito errori di divisioni per 0. metto > per evitare casi in cui max<min. Inoltre così o la possibilità di forzare un valore di una cella a quello che voglio*/
            {  
                semctl(mappa->matrice[i][j].availableSpace, 0, SETVAL, SO_CAP_MIN + (rand() % (SO_CAP_MAX - SO_CAP_MIN))); /*imposto il valore random compreso tra i due estremi*/
            }
            else
            {
                semctl(mappa->matrice[i][j].availableSpace, 0, SETVAL, SO_CAP_MIN);
            }

            /*inizializzo valori della cella non semaforici*/
            mappa->matrice[i][j].taxiOnThisCell = 0;
            mappa->matrice[i][j].totalNumberOfTaxiPassedHere = 0;

            if (SO_TIMENSEC_MAX > SO_TIMENSEC_MIN) /*con questo evito errori di divisioni per 0. metto > per evitare casi in cui max<min. Inoltre così o la possibilità di forzare un valore di una cella a quello che voglio*/
            {   
                mappa->matrice[i][j].timeRequiredToCrossCell = SO_TIMENSEC_MIN + (rand() % (SO_TIMENSEC_MAX - SO_TIMENSEC_MIN)); /*imposto il valore random compreso tra i due estremi*/
            }
            else
            {
                mappa->matrice[i][j].timeRequiredToCrossCell = SO_TIMENSEC_MIN;
            }
        
            mappa->matrice[i][j].isInTopSoCell = FALSE; /*La cella non è ancora in SO_TOP_CELLS*/
        }
    }

    /*Imposto valori della mappa e non della singola cella, tra cui il mutex per potere accedere a questi due valori*/
    do {
        mappa->mutex = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | 0666);
    } while (tmp == -1); /*aspetto che il semaforo sia allocato*/

    semctl(mappa->mutex, 0, SETVAL, 1); /*imposto il mutex della mappa a 1*/

    /*Imposto i parametri della struttura dati condivisa tra i processi taxi per potere raccogliere le statistiche*/
    taxi_statistiche->strada_fatta = 0;
    taxi_statistiche->clienti_serviti = 0;
    taxi_statistiche->tempo_in_strada = 0;

    taxi_statistiche->pid_strada_fatta = 0;
    taxi_statistiche->pid_clienti_serviti = 0;
    taxi_statistiche->pid_tempo_in_strada = 0;

    taxi_statistiche->mutex = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | 0666); /*imposto il mutex della struttura delle statistiche, dopo averlo ottenuto, a 1*/
    semctl(taxi_statistiche->mutex, 0, SETVAL, 1);
}

void aggiornaStatistiche(struct grigliaCitta *mappa, int *statistiche, int msg_queue_id)
{
    struct msqid_ds buffer; /*buffer per potere salvare le statistiche della coda di messaggi*/
    msgctl(msg_queue_id, IPC_STAT, &buffer);
    statistiche[1] = buffer.msg_qnum; /*le pending / unsuccessfoul rides corrispondono al numero di messaggi nella coda*/

    P(mappa->mutex); /*in mutua esclusione accedo alla mappa per leggere i valori delle statistiche salvate in mappa*/
    statistiche[2] = mappa->aborted_rides;
    statistiche[0] = mappa->successes_rides;
    V(mappa->mutex);

    P(taxi_statistiche->mutex); /*in mutua esclusione accedo alle statistiche nella struttura dati condivisa tra i processi*/
    statistiche[3] = taxi_statistiche->pid_tempo_in_strada;
    statistiche[4] = taxi_statistiche->pid_strada_fatta;
    statistiche[5] = taxi_statistiche->pid_clienti_serviti;
    V(taxi_statistiche->mutex);
}
