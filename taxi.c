
#include "include_main.h"
#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}
/*
	Questa funzione spawna una taxi in una posizione casuale della mappa facendo attenzione che la posizione casuale risponda ai seguenti criteri:
	1 - ci deve essere spazio per ospitare il taxi 
	2 - la cella selezionata deve essere di tipo ROAD
*/
void spawnTaxi(struct grigliaCitta *mappa, int x, int y, int taxiSemaphore_id);

/*
    funzione che restituisce la so_source piu vicina date le coordinate taxiX e taxiY. imposta destX e desY con le coordinate della source più vicina
void closestSource(struct grigliaCitta *mappa, int taxiX, int taxiY, int *destX, int *destY);
int closestMoveUpper(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveLower(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveRight(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveLeft(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
	Questa funzione permette al taxi di muoversi verso la sua destinazione, sia SO_SOURCE che destinazione prelevata dal messaggio
*/
void moveTowards_sosource(struct grigliaCitta *mappa, int posizione_taxi_x_iniziale, int posizione_taxi_y_iniziale, int posizione_taxi_x_finale, int posizione_taxi_y_finale, int taxiSemaphore_id);


struct sembuf sops; 


boolean terminateTaxi = FALSE;

void signalHandler(int signalNo);

int SO_TIMEOUT;

int main(int argc, char * argv[]){
	  	int posizione_taxi_x, posizione_taxi_y; /*Coordinate della posizione del taxi*/
		struct grigliaCitta *mappa; /*mappa della citta*/
		
		int so_taxi = atoi(argv[2]); /*recupero il numero di taxi nella simulazione*/
		int queue_key, queue_id; /*Variabili per la coda di messaggi*/
		int taxiSemaphore_id;
		int shm_Key, shm_id, shmId_ForTaxi, shmKey_ForTaxi; /*Variabili per la memoria condivisa*/
		/*Apertura coda di messaggi*/

        /*gestisco il segnale di allarme per uscire*/
        signal(SIGALRM, signalHandler);

        SO_TIMEOUT = atoi(argv[1]); /*recupero la durata della simulazione*/

		queue_key = ftok("ipcKey.key", 1);
		if(queue_key == -1){
        	printf("Error retriving message queue key!\n");
        	exit(EXIT_FAILURE);
    	}	

		queue_id  = msgget(queue_key, IPC_CREAT | 0666);
		if(queue_id == -1){
        	printf("Error retriving queue id!\n");
        	exit(EXIT_FAILURE);
    	}

		/*Attach alla memoria condivisa*/
		shm_Key = ftok("ipcKey.key", 2);
		if(shm_Key == -1){
        	printf("Error retriving shared memory key!\n");
        	exit(EXIT_FAILURE);
    	}

		shm_id = shmget(shm_Key, sizeof(struct grigliaCitta), IPC_CREAT | 0666);
    	if(shm_id == -1) {
        	printf("Error retriving shared memory ID!\n");
        	exit(EXIT_FAILURE);
    	}

    	mappa = shmat(shm_id, NULL, 0);
    	if(mappa == (struct grigliaCitta *)(-1)){
        	printf("Error attaching memory segment!\n");
        	exit(EXIT_FAILURE);
    	}

 	  	shmKey_ForTaxi = ftok("ipcKey.key", 3);    
    	taxiSemaphore_id = semget(shmKey_ForTaxi, 1, IPC_CREAT | 0666);
		srand(getpid());
		/*fprintf(stderr, "ASPETTATTUTTI:%d\n", semctl(taxiSemaphore_id, 0, GETVAL));DEBUG*/ 
    	TEST_ERROR;
    	/*fprintf(stderr, "Posizione prima dello spawn: [%d][%d]\n", posizione_taxi_x, posizione_taxi_y);*/
    	spawnTaxi(mappa, posizione_taxi_x, posizione_taxi_y, taxiSemaphore_id);


    	/*Imposto l'operazione affinchè i processi aspettino che il valore del semafoto aspettaTutti sia 0. Quando è zero ripartono tutti da qui*/
		/*CONTINUA*/
		/*moveTowards_sosource(mappa, posizione_taxi_x, posizione_taxi_y, 10, 10);*/
    	shmdt(mappa);
    	exit(EXIT_SUCCESS);
}

void spawnTaxi(struct grigliaCitta *mappa, int posizione_taxi_x, int posizione_taxi_y, int taxiSemaphore_id) {
	/*Dubbio è la chiave o l'id del semafoto? Se è la chiave allora devo cambiare il codice perchè non sto facendo la get, se non è la chiave allora non capisco cosa sia sbagliato*/
	/*Errore ottenuto: non vengono stampati i numeri di taxi presenti nelle celle durante la simulazione*/
	int availableSpaceOnCell; 
	int i = 0, j = 0;
	/*Seleziono un punto casuale della mappa in cui spawnare, se il massimo di taxi in quella cella è stato raggiunto o non è una road cambio cella*/
	do {
		posizione_taxi_x = rand()%SO_HEIGHT;
		posizione_taxi_y = rand()%SO_WIDTH;
        availableSpaceOnCell = semctl(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, 0, GETVAL);
	} while(availableSpaceOnCell == 0 && (mappa->matrice[posizione_taxi_x][posizione_taxi_y].cellType != BLOCK));


	/*p su available space*/
	sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
	sops.sem_flg = 0; /*Comportamento di default*/
	sops.sem_op = -1; /*Decremento la variabile mutex e la variabile availableSpace*/
	/*Abbasso di uno il valore del semaforo availableSpace*/
    semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, &sops, 1);

 
	semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1);

    semop(taxiSemaphore_id, &sops, 1); /*Abbasso il valore di aspettaTutti cosi nel main è 0*/

	/*Sezione critica*/
	mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell++;
	mappa->matrice[posizione_taxi_x][posizione_taxi_y].totalNumberOfTaxiPassedHere++;

	sops.sem_op = 1; 
	/*Uscita sezione critica rilasciando la risorsa*/
	semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1);

	sops.sem_op = 0;
    semop(taxiSemaphore_id, &sops, 1);

	moveTowards_sosource(mappa, posizione_taxi_x, posizione_taxi_y, 5, 5, taxiSemaphore_id);
}


void moveTowards_sosource(struct grigliaCitta *mappa, int posizione_taxi_x, int posizione_taxi_y, int posizione_taxi_x_finale, int posizione_taxi_y_finale, int taxiSemaphore_id) {
		/*----------------------------------------------SPOSTAMENTO VERSO SINISTRA---------------------------------------------------------*/
		while(posizione_taxi_y > posizione_taxi_y_finale) 
		{
			if(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].cellType != BLOCK) {
				y = y - 1; 
				mappa->matrice[posizione_taxi_x][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
				
			} else {
				if(posizioen_taxi_x > 0) {
					x = x - 1; 
					mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;

				} else {
					x = x + 1;
					mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
				}
			}
		}

	/*------------------------------------------------SPOSTAMENTO VERSO IL BASSO-------------------------------------------------------------*/
	while(posizione_taxi_x < posizione_taxi_x_finale) 
	{
		if(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].cellType != BLOCK) {
			x = x + 1;
			mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
		} else {
			if(posizione_taxi_y > 0) {
				y = y - 1;
				mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
			} else {
				y = y + 1;
				mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
			}
		}
	}

	/*---------------------------------------------------SPOSTAMENTO VERSO L'ALTO-----------------------------------------------------------*/
	while(posizione_taxi_x > posizione_taxi_x_finale) 
	{
		if(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].cellType != BLOCK) {
			x = x - 1;
			mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
		} else {
			if(posizione_taxi_y > 0) {
				y = y - 1;
				mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
			} else {
				y = y + 1;
				mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
			}
		}
	}

	while( posizione_taxi_y < posizione_taxi_y_finale ) 
		{ 
			if(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].cellType != BLOCK) 
				{
					y = y + 1;
					mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
				} else {
					if(posizione_taxi_x > 0) 
				    /*Mi sposto in alto di uno*/
					{
						x = x - 1;
						mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;
					} else {
						/*Mi sposto in basso di uno*/
						x = x + 1;
						mappa->matrice[posizione_taxi_x][y].totalNumberOfTaxiPassedHere++;	
					}
				}	
		}
	}
	

            sops.sem_op = 1; /*Incremento la il semaforo mutex*/
            semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace, &sops, 1);
            semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex nuovo*/
            semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex, &sops, 1); /*Rilascio il mutex vecchio*/

        }


            alarm(SO_TIMEOUT);

            sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
            sops.sem_flg = 0; /*Comportamento di default*/
            sops.sem_op = -1; /*Decremento la variabile mutex e la variabile availableSpace*/
            
            semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace, &sops, 1);
            if(errno == EAGAIN){
                
                shmdt(mappa);
                exit(EXIT_FAILURE); /*skippo tutto il codice e quindi esco in quanto ho terminateTaxi a true*/
            }
            semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex vecchio*/
            if(errno == EAGAIN){
                sops.sem_op = 1; 
                semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex, &sops, 1);
                shmdt(mappa);
                exit(EXIT_FAILURE); /*skippo tutto il codice e quindi esco in quanto ho terminateTaxi a true*/
            }
            semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex nuovo*/
            if(errno == EAGAIN){
                sops.sem_op = 1; 
                semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace, &sops, 1); /*rilascio risorsa allocata*/
                semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex, &sops, 1);
                shmdt(mappa);
                exit(EXIT_FAILURE); /*skippo tutto il codice e quindi esco in quanto ho terminateTaxi a true*/
            }

            /*SEZIONE CRITICA*/
            mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
            mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/		
            mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
            posizione_taxi_x++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
            /*ESCO DALLA SEZIONE CRITICA*/

            sops.sem_op = 1; /*Incremento la il semaforo mutex*/
            semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, &sops, 1);
            semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex nuovo*/
            semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex vecchio*/
            
        }

        /*Spostamento verso l'alto*/
        while(posizione_taxi_x > posizione_taxi_x_finale && !terminateTaxi) { 

            alarm(SO_TIMEOUT);
        
            sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
            sops.sem_flg = 0;/*Comportamento di default*/
            sops.sem_op = -1; /*Decremento la variabile mutex e la variabile availableSpace*/
            
            semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace, &sops, 1);
            if(errno == EAGAIN){
                shmdt(mappa);
                exit(EXIT_FAILURE); /*skippo tutto il codice e quindi esco in quanto ho terminateTaxi a true*/
            }
            semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex vecchio*/
            if(errno == EAGAIN){
                sops.sem_op = 1; 
                semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace, &sops, 1);
                shmdt(mappa);
                exit(EXIT_FAILURE); /*skippo tutto il codice e quindi esco in quanto ho terminateTaxi a true*/
            }
            semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex nuovo*/
            if(errno == EAGAIN){
                sops.sem_op = 1; 
                semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace, &sops, 1);
                semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex vecchio*/
                shmdt(mappa);
                exit(EXIT_FAILURE); /*skippo tutto il codice e quindi esco in quanto ho terminateTaxi a true*/
            }

            /*SEZIONE CRITICA*/
            mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
            mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/		
            mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
            posizione_taxi_x--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
            /*ESCO DALLA SEZIONE CRITICA*/
            sops.sem_op = 1; /*Incremento la il semaforo mutex*/
            semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace, &sops, 1);
            semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex nuovo*/
            semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex vecchio*/

        }		
	}
}
	

void signalHandler(int signalNo){
    switch(signalNo){
        case SIGALRM: 
            terminateTaxi = TRUE;
            
    }
}