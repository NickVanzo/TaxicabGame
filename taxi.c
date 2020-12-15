
#include "include_main.h"

/*
	Questa funzione spawna una taxi in una posizione casuale della mappa facendo attenzione che la posizione casuale risponda ai seguenti criteri:
	1 - ci deve essere spazio per ospitare il taxi
	2 - la cella selezionata deve essere di tipo ROAD
*/
void spawnTaxi(struct grigliaCitta *mappa, int taxiSemaphore_id);

/*
    funzione che restituisce la so_source piu vicina date le coordinate taxiX e taxiY. 
    le coordinate della dest piu vicina sono dentro  destX e desY.
*/

void closestSource(struct grigliaCitta *mappa, int taxiR, int taxiC, int *destR, int *destC);



/*
	Questa funzione permette al taxi di muoversi verso la sua destinazione, sia SO_SOURCE che destinazione prelevata dal messaggio
	Ritorna il punto di arrivo
*/
void move(struct grigliaCitta *mappa, int taxiSemaphore_id);

boolean terminateTaxi = FALSE;

void signalHandler(int signalNo);

struct _posTaxi{
    int posR;
    int posC;
    int destR;
    int destC;
} posizioneTaxi;

int SO_TIMEOUT;

int main(int argc, char * argv[]){
	  	int posizione_taxi_x, posizione_taxi_y; /*Coordinate della posizione del taxi*/
		struct grigliaCitta *mappa; /*mappa della citta*/
        int tempx, tempy;
		int so_taxi = atoi(argv[2]); /*recupero il numero di taxi nella simulazione*/
		int queue_key, queue_id; /*Variabili per la coda di messaggi*/
		int taxiSemaphore_id;
		int shm_Key, shm_id, shmId_ForTaxi, shmKey_ForTaxi; /*Variabili per la memoria condivisa*/

        /*gestisco il segnale di allarme per uscire*/
        signal(SIGALRM, signalHandler);

        SO_TIMEOUT = atoi(argv[1]); /*recupero la durata della simulazione*/

		/*Apertura coda di messaggi*/
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
    	/*fprintf(stderr, "Posizione prima dello spawn: [%d][%d]\n", posizione_taxi_x, posizione_taxi_y);*/
    	spawnTaxi(mappa, taxiSemaphore_id);

        

    	/*Imposto l'operazione affinchè i processi aspettino che il valore del semafoto aspettaTutti sia 0. Quando è zero ripartono tutti da qui*/
		/*CONTINUA*/
		/*moveTowards_sosource(mappa, posizione_taxi_x, posizione_taxi_y, 10, 10);*/
    	shmdt(mappa);
    	exit(EXIT_SUCCESS);
}

void spawnTaxi(struct grigliaCitta *mappa, int taxiSemaphore_id) {
	/*Dubbio è la chiave o l'id del semafoto? Se è la chiave allora devo cambiare il codice perchè non sto facendo la get, se non è la chiave allora non capisco cosa sia sbagliato*/
	/*Errore ottenuto: non vengono stampati i numeri di taxi presenti nelle celle durante la simulazione*/
	int availableSpaceOnCell;
	
	int i = 0, j = 0;
	/*Seleziono un punto casuale della mappa in cui spawnare, se il massimo di taxi in quella cella è stato raggiunto o non è una road cambio cella*/
	do {
		posizioneTaxi.posR = rand()%SO_HEIGHT;
		posizioneTaxi.posC = rand()%SO_WIDTH;
        availableSpaceOnCell = semctl(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace, 0, GETVAL);
	} while(availableSpaceOnCell == 0 && (mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].cellType != BLOCK));


	
	

    P(taxiSemaphore_id); /*Abbasso il valore di aspettaTutti cosi nel main è 0*/
	P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    /*Abbasso di uno il valore del semaforo availableSpace*/
    P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].availableSpace);

	/*Sezione critica*/
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell++;
	mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
	/*Uscita sezione critica rilasciando la risorsa*/

	V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex);
    V(taxiSemaphore_id);

   

    posizioneTaxi.destR = 15;
    posizioneTaxi.destC = 8;

	move(mappa, taxiSemaphore_id);
}


void move(struct grigliaCitta *mappa, int taxiSemaphore_id) {
		/*----------------------------------------------SPOSTAMENTO VERSO SINISTRA---------------------------------------------------------*/
		while(posizioneTaxi.posC > posizioneTaxi.destC)
		{
			if(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].cellType != BLOCK) {
				
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].availableSpace);
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].mutex); /*Ottengo il mutex dove vado*/
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/

                /*SEZIONE CRITICA*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].totalNumberOfTaxiPassedHere++;
                posizioneTaxi.posC--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                /*ESCO DALLA SEZIONE CRITICA*/

                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].availableSpace);
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/


			} else {
				if(posizioneTaxi.posR > 0) {

                    P(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].availableSpace);
                    P(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove vado*/
                    P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/


                    /*SEZIONE CRITICA*/
                    mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                    mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                    mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
                    posizioneTaxi.posR--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/

                    /*ESCO DALLA SEZIONE CRITICA*/
                    V(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].availableSpace);
                    V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
                    V(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/


				} else {

                    P(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].availableSpace);
                    P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/
                    P(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove vado*/


                    /*SEZIONE CRITICA*/
                    mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                    mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                    mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
                    posizioneTaxi.posR++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                    /*ESCO DALLA SEZIONE CRITICA*/

                    V(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].availableSpace);
                    V(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
                    V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/

				}
			}
		}

	/*------------------------------------------------SPOSTAMENTO VERSO IL BASSO-------------------------------------------------------------*/
	while(posizioneTaxi.posR < posizioneTaxi.destR)
	{
		if(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].cellType != BLOCK) {
			
            P(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].availableSpace);
            P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/
            P(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove vado*/


            /*SEZIONE CRITICA*/
            mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
            mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
            mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
            posizioneTaxi.posR++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
            /*ESCO DALLA SEZIONE CRITICA*/

            V(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].availableSpace);
            V(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
            V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/

		} else {
			if(posizioneTaxi.posC > 0) {
		
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].availableSpace);
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].mutex); /*Ottengo il mutex dove vado*/
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/

                /*SEZIONE CRITICA*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].totalNumberOfTaxiPassedHere++;
                posizioneTaxi.posC--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                /*ESCO DALLA SEZIONE CRITICA*/

                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].availableSpace);
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

			} else {

                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].availableSpace);
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].mutex); /*prendo il mutex dove vado*/
                /*SEZIONE CRITICA*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].taxiOnThisCell++;
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].totalNumberOfTaxiPassedHere++;
                posizioneTaxi.posC++;
                /*ESCO DALLA SEZIONE CRITICA*/

                
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].availableSpace);
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

			}
		}
	}

	/*---------------------------------------------------SPOSTAMENTO VERSO L'ALTO-----------------------------------------------------------*/
	while(posizioneTaxi.posR > posizioneTaxi.destR)
	{
		if(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].cellType != BLOCK) {
		
            P(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].availableSpace);
            P(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove vado*/
            P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/


            /*SEZIONE CRITICA*/
            mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
            mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
            mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
            posizioneTaxi.posR--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/

            /*ESCO DALLA SEZIONE CRITICA*/
            V(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].availableSpace);
            V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
            V(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

		} else {
			if(posizioneTaxi.posC > 0) {

                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].availableSpace);
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].mutex); /*Ottengo il mutex dove vado*/
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/

                /*SEZIONE CRITICA*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].totalNumberOfTaxiPassedHere++;
                posizioneTaxi.posC--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                /*ESCO DALLA SEZIONE CRITICA*/

                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].availableSpace);
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

			} else {
				
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].availableSpace);
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/
                P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].mutex); /*prendo il mutex dove vado*/

                /*SEZIONE CRITICA*/
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].taxiOnThisCell++;
                mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].totalNumberOfTaxiPassedHere++;
                posizioneTaxi.posC++;
                /*ESCO DALLA SEZIONE CRITICA*/

                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].availableSpace);
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

			}
		}
	}
    /*---------------------------------------------------SPOSTAMENTO VERSO DESTRA-----------------------------------------------------------*/

	while( posizioneTaxi.posC < posizioneTaxi.destC ) {
			if(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].cellType != BLOCK) {

                    P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].availableSpace);
                    P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/
                    P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].mutex); /*prendo il mutex dove vado*/

                    /*SEZIONE CRITICA*/
                    mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--;
                    mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].taxiOnThisCell++;
                    mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC+1].totalNumberOfTaxiPassedHere++;
                    posizioneTaxi.posC++;
                    /*ESCO DALLA SEZIONE CRITICA*/

                    V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].availableSpace);
                    V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC-1].mutex); /*Rilascio il mutex vecchio*/
                    V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

				} else {
					if(posizioneTaxi.posR > 0){
				
                        P(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].availableSpace);
                        P(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove vado*/
                        P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/


                        /*SEZIONE CRITICA*/
                        mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                        mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                        mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
                        posizioneTaxi.posR--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/

                        /*ESCO DALLA SEZIONE CRITICA*/
                        V(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].availableSpace);
                        V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
                        V(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].mutex); /*Rilascio il mutex nuovo*/

					} else {

                        P(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].availableSpace);
                        P(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove sono*/
                        P(mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].mutex); /*Ottengo il mutex dove vado*/


                        /*SEZIONE CRITICA*/
                        mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                        mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                        mappa->matrice[posizioneTaxi.posR+1][posizioneTaxi.posC].totalNumberOfTaxiPassedHere++;
                        posizioneTaxi.posR++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                        /*ESCO DALLA SEZIONE CRITICA*/

                        V(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].availableSpace);
                        V(mappa->matrice[posizioneTaxi.posR-1][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/
                        V(mappa->matrice[posizioneTaxi.posR][posizioneTaxi.posC].mutex); /*Rilascio il mutex vecchio*/

					}
				}
		}
			/*se lo spostamento non è completo, riavvio lo spostamento in maniera ricorsiva*/
			if(posizioneTaxi.posR != posizioneTaxi.destR || posizioneTaxi.posC != posizioneTaxi.destC) {
				move(mappa, taxiSemaphore_id);
			}else{
                posizioneTaxi.posR = posizioneTaxi.destR;
                posizioneTaxi.posC = posizioneTaxi.destC;
            }
	}



void signalHandler(int signalNo){
    switch(signalNo){
        case SIGALRM:
            break;

    }
}




void closestSource(struct grigliaCitta *mappa, int taxiR, int taxiC, int *destR, int *destC){
   
   int i=0, j=0;
   int minDistance = INT_MAX;
   int positionOfMinDistance = 0, tmp;

   if(mappa->matrice[taxiR][taxiC].cellType == SOURCE) return;

    for(i=0;i<SO_HEIGHT;i++){
        for(j=0;j<SO_WIDTH;j++){
            if(mappa->matrice[i][j].cellType == SOURCE && i!=taxiR && j!=taxiC){
                tmp = (int) sqrt(pow(taxiR-i, 2) + pow(taxiC - j, 2));
                if(tmp < minDistance){
                    minDistance = tmp;
                    *destR = i;
                    *destC = j;
                }
            }
        }
    }

    


}