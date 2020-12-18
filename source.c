#include "include_main.h"

/*
    Dichiarazione variabili. 
    Sono dichiarate globalmente per aggevolare l'utilizzo delle variabili necessarie alla funzione signalHandler

    Dichiarazione variabili che riguardano la memoria condivisa e la coda di messaggi. 
    Il prefisso nel nome della variabile ne indica l'appartenenza. (queue riguarda la coda di messaggi, shm la memoria condivisa).
*/
int queue_key, queue_id;
int shm_key, shm_id;

/*
    La variabile "exit_from_loop" viene utilizzata per smettere di creare messaggi in coda e per eseguire il detach dalla memoria condivisa.
*/
int exit_from_loop = 0;

/*
    Dichiarazione structs.
    my_message viene usata per costruire il corpo del messaggio da mandare nella coda di messaggi.
    mappa viene usata per eseguire l'attach alla memoria condivisa che contiene la mappa creata dal processo master.
*/
struct msgBuf my_message;
int SO_SOURCE, SO_DURATION;
struct grigliaCitta * mappa;

/*
    La funzione "signalHandler" riceve come parametro un intero che indica quale segnale stiamo considerando all'interno dell'handler da noi personalizzatp.
    La funzione non ha valore di ritorno.
    Per approfondimenti sui segnali digitare "man 7 signal" nel cmd.
*/
void signalHandler(int signal);

int main(int argc, char *argv[])
{
    /*
        Controllo che il numero di parametri ricevuto dal source.c sia corretto.
    */
    if (argc != 4)
    {
        printf("Error parametrs not matching!\n");
        exit(EXIT_FAILURE);
    }
    srand(getpid());
    /*
        Converto in interi i parametri che vengono passati dalla execlp (dal processo master)
    */
    SO_SOURCE = atoi(argv[1]);
    SO_DURATION = atoi(argv[2]);

    /*
        Ottengo la chiave e l'id della coda di messaggi per eseguire l'attach alla coda ed eseguo relativi controlli per verificare che non ci siano problemi.
    */
    queue_key = ftok("ipcKey.key", 1);
    if (queue_key == -1)
    {
        printf("Error retriving message queue key!\n");
        exit(EXIT_FAILURE);
    }

    queue_id = msgget(queue_key, IPC_CREAT | 0666);
    if (queue_id == -1)
    {
        printf("Error retriving queue id!\n");
        exit(EXIT_FAILURE);
    }
    /*
        Ottengo la chiave e l'id della memoria condivisa che contiene la mappa.
        La mappa viene usata per eseguire un controllo sulla cella che vado a selezionare come destinazione all'atto di creazione del corpo del messaggio.
        Per ulteriori informazioni riguardo il controllo appena citato si guardi al funzione signalHandler.
    */
    shm_key = ftok("ipcKey.key", 2);
    if (shm_key == -1)
    {
        printf("Error retriving shared memory key!\n");
        exit(EXIT_FAILURE);
    }

    shm_id = shmget(shm_key, sizeof(struct grigliaCitta), 0);
    if (shm_id == -1)
    {
        printf("Error retriving shared memory ID!\n");
        exit(EXIT_FAILURE);
    }

    mappa = shmat(shm_id, NULL, 0);
    if (mappa == (struct grigliaCitta *)(-1))
    {
        printf("Error attaching memory segment!\n");
        exit(EXIT_FAILURE);
    }
    /*
        Memoria condivisa e coda di messaggi ottenute con successo.
    */

    /*
        Imposto l'handler di SIGALRM. 
        Si potrebbe fare con la sigaction ma in questo modo il procedimento è meno macchinoso.
    */
    signal(SIGALRM, signalHandler);
    signal(SIGUSR1, signalHandler);

    /*
        Eseguo un loop che continua fino a quando la variabile exit_from_loop viene settata ad 1 da ?????????????????????????????
    */
    while (exit_from_loop == 0)
    {
        alarm(rand() % 5 / SO_SOURCE + 1);
        pause();
    }
    /*
        Eseguo il detach dalla memoria condivisa
    */
    shmdt(mappa);
    exit(EXIT_SUCCESS);
}

void signalHandler(int signal)
{
    switch (signal)
    {
        case SIGALRM:
            /*
                Costruzione del corpo del messaggio da mandare nella coda di messaggi.
                La source da cui viene generata la richiesta è generata randomicamente tra le celle di tipo SOURCE disponibili nella mappa.
                La destinazione può essere una cella di tipo ROAD o una cella di tipo SOURCE. NON UNA BLOCK.
            */
            my_message.mtype = (rand() % SO_SOURCE) + 1;
            do {    
                my_message.xDest = rand() % SO_HEIGHT;
                my_message.yDest = rand() % SO_WIDTH;
            } while (mappa->matrice[my_message.xDest][my_message.yDest].cellType == BLOCK);

            /*
                Mando il messaggio nella coda di messaggi con il corpo impostato con valori generati qui sopra.
            */
            msgsnd(queue_id, &my_message, 2* sizeof(int), 0);
            break;
        case SIGINT:
            exit_from_loop = 1;
            break;
        default:
            break;
    }
}