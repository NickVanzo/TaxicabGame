#include "include_main.h"

/*
	Questa funzione spawna una taxi in una posizione casuale della mappa facendo attenzione che la posizione casuale risponda ai seguenti criteri:
	1 - ci deve essere spazio per ospitare il taxi
	2 - la cella selezionata deve essere di tipo ROAD
*/
void spawnTaxi(struct grigliaCitta * mappa, int taxiSemaphore_id);

/*
    funzione che restituisce la so_source piu vicina date le coordinate taxiX e taxiY. 
    le coordinate della dest piu vicina sono dentro  destX e desY.
*/

void closestSource(struct grigliaCitta * mappa);

/*
    questa funzione permette di ottenere il tipo di messaggio su cui mettersi in ascolto, data la posizione del taxi
    il tipo di messaggio corrsiponde al numero in cui compare la SO_SOURCE con una letttura sequenziale della mappa
    ovvero per esempio se ho una so source in 0,0 e una in 0,3 , la prima avrà mtype = 1  e la seconda avrà mtype=2 e così via...
    la funzione ritorna -1 in caso di errore
*/
int enumSoSources(struct grigliaCitta *mappa);

/*
	Questa funzione permette al taxi di muoversi verso la sua destinazione, sia SO_SOURCE che destinazione prelevata dal messaggio
	Ritorna il punto di arrivo
*/
void move(struct grigliaCitta * mappa);
void moveUp(struct grigliaCitta * mappa);
void moveDown(struct grigliaCitta * mappa);
void moveLeft(struct grigliaCitta * mappa);
void moveRight(struct grigliaCitta * mappa);


/*
    questa funzione permette di prelevare un messaggio, dato un taxi presente in una so_source...
*/
void getRide(int msg_queue_id, long so_source);

void signalHandler(int signalNo);

struct taxiStatistiche * taxi_statistiche;

int Ptemp(int semaphore, long time) {
	struct timespec timeout_taxi;
	struct sembuf sops;
	sops.sem_flg = 0;
	sops.sem_num = 0;
    sops.sem_op = -1;
    timeout_taxi.tv_sec = 0;
    timeout_taxi.tv_nsec = time;
 	return semtimedop(semaphore, &sops, 1, &timeout_taxi);
}

/*Variabili e struct*/
struct _posTaxi {
    int posR;
    int posC;
    int destR;
    int destC;
}
posizioneTaxi;

int so_timeout;
int strada_percorsa;
int richieste_finite;
unsigned long int tempo_in_strada;
struct timespec time_struct;
boolean exit_program;
boolean ride_taken;


int main(int argc, char * argv[]) {
    int posizione_taxi_x, posizione_taxi_y; /*Coordinate della posizione del taxi*/
    struct grigliaCitta * mappa; /*mappa della citta*/
    int tempx, tempy;
    int queue_key, queue_id; /*Variabili per la coda di messaggi*/
    int taxiSemaphore_id;
    int time_duration;
    int shm_Key, shm_id, shmId_ForTaxi, shmKey_ForTaxi, shmKey_stats, shmID_stats; /*Variabili per la memoria condivisa*/
    srand(getpid());
    /*gestisco il segnale di allarme per uscire*/
    signal(SIGALRM, signalHandler);
    exit_program = FALSE;
    ride_taken = FALSE;

    /*500000000 sono 0,5 secondi*/
    so_timeout = atol(argv[1]);
    time_duration = atoi(argv[2]);
    alarm(time_duration);

    /*Apertura coda di messaggi*/
    queue_key = ftok("ipcKey.key", 1);
    if (queue_key == -1) {
        printf("Error retriving message queue key!\n");
        exit(EXIT_FAILURE);
    }

    queue_id = msgget(queue_key, IPC_CREAT | 0666);
    if (queue_id == -1) {
        printf("Error retriving queue id!\n");
        exit(EXIT_FAILURE);
    }

    /*Attach alla memoria condivisa*/
    shm_Key = ftok("ipcKey.key", 2);
    if (shm_Key == -1) {
        printf("Error retriving shared memory key!\n");
        exit(EXIT_FAILURE);
    }

    shm_id = shmget(shm_Key, sizeof(struct grigliaCitta), IPC_CREAT | 0666);
    if (shm_id == -1) {
        printf("Error retriving shared memory ID!\n");
        exit(EXIT_FAILURE);
    }

    mappa = shmat(shm_id, NULL, 0);
    if (mappa == (struct grigliaCitta * )(-1)) {
        printf("Error attaching memory segment!\n");
        exit(EXIT_FAILURE);
    }

    shmKey_ForTaxi = ftok("ipcKey.key", 3);
    taxiSemaphore_id = semget(shmKey_ForTaxi, 1, IPC_CREAT | 0666);
    /*Statistiche taxi memoria condivisa attach*/
    shmKey_stats = ftok("ipcKey.key", 5);
    shmID_stats = shmget(shmKey_stats, sizeof(struct taxiStatistiche), IPC_CREAT | 0666);
    taxi_statistiche = shmat(shmID_stats, NULL, 0);
   	
    spawnTaxi(mappa, taxiSemaphore_id);
    while(!exit_program) {
	    closestSource(mappa);
	    move(mappa);
	    getRide(queue_id, (long)enumSoSources(mappa));
	    move(mappa);
    }


    /*Imposto l'operazione affinchè i processi aspettino che il valore del semafoto aspettaTutti sia 0. Quando è zero ripartono tutti da qui*/
    /*CONTINUA*/
    shmdt(mappa);
    exit(EXIT_SUCCESS);
}

void spawnTaxi(struct grigliaCitta * mappa, int taxiSemaphore_id) {
    /*Dubbio è la chiave o l'id del semafoto? Se è la chiave allora devo cambiare il codice perchè non sto facendo la get, se non è la chiave allora non capisco cosa sia sbagliato*/
    /*Errore ottenuto: non vengono stampati i numeri di taxi presenti nelle celle durante la simulazione*/
    int availableSpaceOnCell;

    int i = 0, j = 0;
    /*Seleziono un punto casuale della mappa in cui spawnare, se il massimo di taxi in quella cella è stato raggiunto o non è una road cambio cella*/
    do {
        posizioneTaxi.posR = rand() % SO_HEIGHT;
        posizioneTaxi.posC = rand() % SO_WIDTH;
        availableSpaceOnCell = semctl(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace, 0, GETVAL);
    } while (availableSpaceOnCell == 0 && (mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].cellType != BLOCK));
    P(taxiSemaphore_id); /*Abbasso il valore di aspettaTutti cosi nel main è 0*/
    P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    /*Abbasso di uno il valore del semaforo availableSpace*/
    P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

    /*Sezione critica*/
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell++;
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
    /*Uscita sezione critica rilasciando la risorsa*/

    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    V(taxiSemaphore_id);
}



void move(struct grigliaCitta * mappa) {
    /*----------------------------------------------SPOSTAMENTO VERSO SINISTRA---------------------------------------------------------*/
    while (posizioneTaxi.posC > posizioneTaxi.destC) {
        if (mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].cellType != BLOCK) {
            moveLeft(mappa);
        } else {
            if (posizioneTaxi.posR > 0) {
                moveUp(mappa);
            } else {
                moveDown(mappa);
            }
        }
    }
    /*------------------------------------------------SPOSTAMENTO VERSO IL BASSO-------------------------------------------------------------*/
    while (posizioneTaxi.posR < posizioneTaxi.destR) {
        if (mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].cellType != BLOCK) {
            moveDown(mappa);
        } else {
            if (posizioneTaxi.posC > 0) {
                moveLeft(mappa);
            } else {
                moveRight(mappa);
            }
        }
    }
    /*---------------------------------------------------SPOSTAMENTO VERSO L'ALTO-----------------------------------------------------------*/
    while (posizioneTaxi.posR > posizioneTaxi.destR) {
        if (mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].cellType != BLOCK) {
            moveUp(mappa);
        } else {
            if (posizioneTaxi.posC > 0) {
                moveLeft(mappa);
            } else {
                moveRight(mappa);
            }
        }
    }
    /*---------------------------------------------------SPOSTAMENTO VERSO DESTRA-----------------------------------------------------------*/
    while (posizioneTaxi.posC < posizioneTaxi.destC) {
        if (mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].cellType != BLOCK) {
            moveRight(mappa);
        } else {
            if (posizioneTaxi.posR > 0) {
                moveUp(mappa);
            } else {
                moveDown(mappa);
            }
        }
    }
    /*se lo spostamento non è completo, riavvio lo spostamento in maniera ricorsiva*/
    if (posizioneTaxi.posR != posizioneTaxi.destR || posizioneTaxi.posC != posizioneTaxi.destC) {
        move(mappa);
    } else {
        posizioneTaxi.posR = posizioneTaxi.destR;
        posizioneTaxi.posC = posizioneTaxi.destC;
        if(ride_taken) {
	        P(mappa -> mutex);
	        mappa -> successes_rides++;
	        V(mappa -> mutex);

	        richieste_finite++;
	    	P(taxi_statistiche -> mutex);
	    	if(richieste_finite > taxi_statistiche -> clienti_serviti) {
	    		taxi_statistiche -> clienti_serviti = richieste_finite;
	    		taxi_statistiche -> pid_clienti_serviti = getpid();
	    	}
	    	V(taxi_statistiche -> mutex);
	    	ride_taken = FALSE;

	    	}
        }
}



void moveUp(struct grigliaCitta * mappa) {
    time_struct.tv_sec = 0;
    time_struct.tv_nsec = mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].timeRequiredToCrossCell; 
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].availableSpace, so_timeout) == -1 && errno == EAGAIN) {
    	/*Il semaforo non è riuscito ad ottenere il semaforo, aumento corse abortite e rilascio eventuali risorse ottenute*/
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	exit(EXIT_FAILURE);
    }
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN) {
	    P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
	    V(mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].availableSpace);
	    P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
	    exit(EXIT_FAILURE);
    } 
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN) {
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	V(mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].availableSpace);
    	V(mappa -> matrice[posizioneTaxi.posR -1][posizioneTaxi.posC].mutex);
    	P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	exit(EXIT_FAILURE);
    }

    /*SEZIONE CRITICA*/
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
    mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
    mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
    posizioneTaxi.posR--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/

    /*ESCO DALLA SEZIONE CRITICA*/
    V(mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].availableSpace);
    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
    V(mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

    /*blocco di gestione delle risorse da stampare a terminale*/
    strada_percorsa++;
    tempo_in_strada = tempo_in_strada + time_struct.tv_nsec;
    P(taxi_statistiche -> mutex);
   	if(strada_percorsa > taxi_statistiche -> strada_fatta) {
   		taxi_statistiche -> pid_strada_fatta = getpid();
   		taxi_statistiche -> strada_fatta = strada_percorsa;
   	}
   	if(tempo_in_strada > taxi_statistiche -> tempo_in_strada) {
   		taxi_statistiche -> pid_tempo_in_strada = getpid();
   		taxi_statistiche -> tempo_in_strada = tempo_in_strada;
   	}
    V(taxi_statistiche -> mutex);
    nanosleep(&time_struct, NULL);
}


void moveDown(struct grigliaCitta * mappa) {
    time_struct.tv_sec = 0;
    time_struct.tv_nsec = mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].timeRequiredToCrossCell; 
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].availableSpace, so_timeout) == -1 && errno == EAGAIN) {
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	exit(EXIT_FAILURE);
    }
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN) {
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	V(mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].availableSpace);
	    P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
	    exit(EXIT_FAILURE);
    }
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN) {
	    P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
	    V(mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].availableSpace);
	    V(mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
	    P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
	    exit(EXIT_FAILURE);
    }

    /*SEZIONE CRITICA*/
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
    mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
    mappa -> matrice[posizioneTaxi.posR + 1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
    posizioneTaxi.posR++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
    /*ESCO DALLA SEZIONE CRITICA*/

    V(mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].availableSpace);
    V(mappa -> matrice[posizioneTaxi.posR - 1][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
	

    strada_percorsa++;
    tempo_in_strada = tempo_in_strada + time_struct.tv_nsec;
    P(taxi_statistiche -> mutex);
   	if(strada_percorsa > taxi_statistiche -> strada_fatta) {
   		taxi_statistiche -> pid_strada_fatta = getpid();
   		taxi_statistiche -> strada_fatta = strada_percorsa;
   	}
   	if(tempo_in_strada > taxi_statistiche -> tempo_in_strada) {
   		taxi_statistiche -> pid_tempo_in_strada = getpid();
   		taxi_statistiche -> tempo_in_strada = tempo_in_strada;
   	}
    V(taxi_statistiche -> mutex);
    nanosleep(&time_struct, NULL);
}


void moveLeft(struct grigliaCitta * mappa) {
    time_struct.tv_sec = 0;
    time_struct.tv_nsec = mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].timeRequiredToCrossCell; 
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].availableSpace, so_timeout) == -1 && errno == EAGAIN) {
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	exit(EXIT_FAILURE);
    } 
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].mutex, so_timeout) == -1 && errno == EAGAIN) {
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].availableSpace);
    	P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	exit(EXIT_FAILURE);
    } /*Ottengo il mutex dove vado*/
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN) {
		P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
		V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].availableSpace);
		V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].mutex);
		P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
		exit(EXIT_FAILURE);    	
    }

    /*SEZIONE CRITICA*/
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].totalNumberOfTaxiPassedHere++;
    posizioneTaxi.posC--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
    /*ESCO DALLA SEZIONE CRITICA*/

    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].availableSpace);
    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].mutex); /*Rilascio il mutex vecchio*/
    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

	
	strada_percorsa++;
    tempo_in_strada = tempo_in_strada + time_struct.tv_nsec;
    P(taxi_statistiche -> mutex);
   	if(strada_percorsa > taxi_statistiche -> strada_fatta) {
   		taxi_statistiche -> pid_strada_fatta = getpid();
   		taxi_statistiche -> strada_fatta = strada_percorsa;
   	}
   	if(tempo_in_strada > taxi_statistiche -> tempo_in_strada) {
   		taxi_statistiche -> pid_tempo_in_strada = getpid();
   		taxi_statistiche -> tempo_in_strada = tempo_in_strada;
   	}
    V(taxi_statistiche -> mutex);
    nanosleep(&time_struct, NULL);    
}


void moveRight(struct grigliaCitta * mappa) {
    time_struct.tv_sec = 0;
    time_struct.tv_nsec = mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].timeRequiredToCrossCell; 
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].availableSpace, so_timeout) == -1 && errno == EAGAIN) {
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	exit(EXIT_FAILURE);
    }
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex, so_timeout) == -1 && errno == EAGAIN) {
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].availableSpace);
    	P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	exit(EXIT_FAILURE);
    } 
    if(Ptemp(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].mutex, so_timeout) == -1 && errno == EAGAIN) {
    	P(mappa -> mutex);
    	mappa -> aborted_rides++;
    	V(mappa -> mutex);
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].availableSpace);
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	P(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    	V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    	exit(EXIT_FAILURE);
    } 

    /*SEZIONE CRITICA*/
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].taxiOnThisCell++;
    mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC + 1].totalNumberOfTaxiPassedHere++;
    posizioneTaxi.posC++;
    /*ESCO DALLA SEZIONE CRITICA*/

    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].availableSpace);
    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC - 1].mutex); /*Rilascio il mutex vecchio*/
    V(mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

	strada_percorsa++;
    tempo_in_strada = tempo_in_strada + time_struct.tv_nsec;
    P(taxi_statistiche -> mutex);
   	if(strada_percorsa > taxi_statistiche -> strada_fatta) {
   		taxi_statistiche -> pid_strada_fatta = getpid();
   		taxi_statistiche -> strada_fatta = strada_percorsa;
   	}
   	if(tempo_in_strada > taxi_statistiche -> tempo_in_strada) {
   		taxi_statistiche -> pid_tempo_in_strada = getpid();
   		taxi_statistiche -> tempo_in_strada = tempo_in_strada;
   	}
    V(taxi_statistiche -> mutex);	
    nanosleep(&time_struct, NULL);
}



void signalHandler(int signalNo) {
    switch (signalNo) {
    case SIGALRM:
	    exit_program = TRUE;
        break;

    }
}

void closestSource(struct grigliaCitta * mappa) {

    int i = 0, j = 0;
    int minDistance = INT_MAX;
    int positionOfMinDistance = 0, tmp;

    if (mappa -> matrice[posizioneTaxi.posR][posizioneTaxi.posC].cellType == SOURCE) {
        posizioneTaxi.destR = posizioneTaxi.posR;
        posizioneTaxi.destC = posizioneTaxi.posC;
        return;
    }

    for (i = 0; i < SO_HEIGHT; i++) {
        for (j = 0; j < SO_WIDTH; j++) {
            if (mappa -> matrice[i][j].cellType == SOURCE) {
                tmp = (int) sqrt(pow(posizioneTaxi.posR - i, 2) + pow(posizioneTaxi.posC - j, 2));
                if (tmp < minDistance) {
                    minDistance = tmp;
                    posizioneTaxi.destR = i;
                    posizioneTaxi.destC = j;
                }
            }
        }
    }

}

int enumSoSources(struct grigliaCitta *mappa) {
	int i, j;
	int count_source = 0;
	for(i = 0; i < SO_HEIGHT; i++) {
		for(j = 0; j < SO_WIDTH; j++) {
			if(mappa->matrice[i][j].cellType == SOURCE) {
				count_source++;
				if(i == posizioneTaxi.posR && j == posizioneTaxi.posC) return count_source;
			}
		}
	}
	return -1;
}

void getRide(int msg_queue_id, long so_source){
    struct msgBuf myMessage;
	if(msgrcv(msg_queue_id, &myMessage, 2*sizeof(int), so_source, 0) == -1) {
		exit(EXIT_FAILURE);
	}
    posizioneTaxi.destR = myMessage.xDest;
    posizioneTaxi.destC = myMessage.yDest;
    ride_taken = TRUE;
}
