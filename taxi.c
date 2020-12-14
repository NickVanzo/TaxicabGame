
#include "include_main.h"

/*
	Questa funzione spawna una taxi in una posizione casuale della mappa facendo attenzione che la posizione casuale risponda ai seguenti criteri:
	1 - ci deve essere spazio per ospitare il taxi
	2 - la cella selezionata deve essere di tipo ROAD
*/
void spawnTaxi(struct grigliaCitta *mappa, int x, int y, int taxiSemaphore_id);

/*
    funzione che restituisce la so_source piu vicina date le coordinate taxiX e taxiY. imposta destX e desY con le coordinate della source più vicina
*/

void closestSource(struct grigliaCitta *mappa, int taxiR, int taxiC, int *destR, int *destC);



/*
	Questa funzione permette al taxi di muoversi verso la sua destinazione, sia SO_SOURCE che destinazione prelevata dal messaggio
	Ritorna il punto di arrivo
*/
void move(struct grigliaCitta *mappa, int posizione_taxi_x_iniziale, int posizione_taxi_y_iniziale, int *posizione_taxi_x_finale, int *posizione_taxi_y_finale, int taxiSemaphore_id);

boolean terminateTaxi = FALSE;

void signalHandler(int signalNo);

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

        printf("CIAOOOOOOOOOOOOOOOOOOO\n\n\n\n\n\n");
        closestSource(mappa, 0, 0, &tempx, &tempy);
        printf("closest source from 0-0 is: %d, %d\n", tempx, tempy);

 	  	shmKey_ForTaxi = ftok("ipcKey.key", 3);
    	taxiSemaphore_id = semget(shmKey_ForTaxi, 1, IPC_CREAT | 0666);
		srand(getpid());
		/*fprintf(stderr, "ASPETTATTUTTI:%d\n", semctl(taxiSemaphore_id, 0, GETVAL));DEBUG*/
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
	int *posizione_taxi_x_finale, *posizione_taxi_y_finale;
	int i = 0, j = 0;
	/*Seleziono un punto casuale della mappa in cui spawnare, se il massimo di taxi in quella cella è stato raggiunto o non è una road cambio cella*/
	do {
		posizione_taxi_x = rand()%SO_HEIGHT;
		posizione_taxi_y = rand()%SO_WIDTH;
        availableSpaceOnCell = semctl(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, 0, GETVAL);
	} while(availableSpaceOnCell == 0 && (mappa->matrice[posizione_taxi_x][posizione_taxi_y].cellType != BLOCK));


	
	/*Abbasso di uno il valore del semaforo availableSpace*/
    P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace);

	P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex);
    P(taxiSemaphore_id); /*Abbasso il valore di aspettaTutti cosi nel main è 0*/

	/*Sezione critica*/
	mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell++;
	mappa->matrice[posizione_taxi_x][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
	/*Uscita sezione critica rilasciando la risorsa*/

	V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex);
    V(taxiSemaphore_id);

    posizione_taxi_x_finale = malloc(sizeof(int));
    posizione_taxi_y_finale = malloc(sizeof(int));

    *posizione_taxi_x_finale = 15;
    *posizione_taxi_y_finale = 8;

	move(mappa, posizione_taxi_x, posizione_taxi_y, posizione_taxi_x_finale, posizione_taxi_y_finale, taxiSemaphore_id);
}


void move(struct grigliaCitta *mappa, int posizione_taxi_x, int posizione_taxi_y, int *posizione_taxi_x_finale, int *posizione_taxi_y_finale, int taxiSemaphore_id) {
		/*----------------------------------------------SPOSTAMENTO VERSO SINISTRA---------------------------------------------------------*/
		while(posizione_taxi_y > *posizione_taxi_y_finale)
		{
			if(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].cellType != BLOCK) {
				
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].availableSpace);
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].mutex); /*Ottengo il mutex dove vado*/
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/

                /*SEZIONE CRITICA*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].totalNumberOfTaxiPassedHere++;
                posizione_taxi_y--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                /*ESCO DALLA SEZIONE CRITICA*/

                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace);
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/


			} else {
				if(posizione_taxi_x > 0) {

                    P(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace);
                    P(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex); /*Ottengo il mutex dove vado*/
                    P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/


                    /*SEZIONE CRITICA*/
                    mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                    mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                    mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
                    posizione_taxi_x--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/

                    /*ESCO DALLA SEZIONE CRITICA*/
                    V(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace);
                    V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/
                    V(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/


				} else {

                    P(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace);
                    P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/
                    P(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex); /*Ottengo il mutex dove vado*/


                    /*SEZIONE CRITICA*/
                    mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                    mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                    mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
                    posizione_taxi_x++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                    /*ESCO DALLA SEZIONE CRITICA*/

                    V(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace);
                    V(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/
                    V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/

				}
			}
		}

	/*------------------------------------------------SPOSTAMENTO VERSO IL BASSO-------------------------------------------------------------*/
	while(posizione_taxi_x < *posizione_taxi_x_finale)
	{
		if(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].cellType != BLOCK) {
			
            P(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace);
            P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/
            P(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex); /*Ottengo il mutex dove vado*/


            /*SEZIONE CRITICA*/
            mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
            mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
            mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
            posizione_taxi_x++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
            /*ESCO DALLA SEZIONE CRITICA*/

            V(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace);
            V(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/
            V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/

		} else {
			if(posizione_taxi_y > 0) {
		
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].availableSpace);
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].mutex); /*Ottengo il mutex dove vado*/
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/

                /*SEZIONE CRITICA*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].totalNumberOfTaxiPassedHere++;
                posizione_taxi_y--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                /*ESCO DALLA SEZIONE CRITICA*/

                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace);
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/

			} else {

                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace);
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex); /*prendo il mutex dove vado*/
                /*SEZIONE CRITICA*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--;
                mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].taxiOnThisCell++;
                mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].totalNumberOfTaxiPassedHere++;
                posizione_taxi_y++;
                /*ESCO DALLA SEZIONE CRITICA*/

                
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].availableSpace);
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/

			}
		}
	}

	/*---------------------------------------------------SPOSTAMENTO VERSO L'ALTO-----------------------------------------------------------*/
	while(posizione_taxi_x > *posizione_taxi_x_finale)
	{
		if(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].cellType != BLOCK) {
		
            P(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace);
            P(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex); /*Ottengo il mutex dove vado*/
            P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/


            /*SEZIONE CRITICA*/
            mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
            mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
            mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
            posizione_taxi_x--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/

            /*ESCO DALLA SEZIONE CRITICA*/
            V(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace);
            V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/
            V(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/

		} else {
			if(posizione_taxi_y > 0) {

                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].availableSpace);
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].mutex); /*Ottengo il mutex dove vado*/
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/

                /*SEZIONE CRITICA*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].totalNumberOfTaxiPassedHere++;
                posizione_taxi_y--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                /*ESCO DALLA SEZIONE CRITICA*/

                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace);
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/

			} else {
				
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace);
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/
                P(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex); /*prendo il mutex dove vado*/

                /*SEZIONE CRITICA*/
                mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--;
                mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].taxiOnThisCell++;
                mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].totalNumberOfTaxiPassedHere++;
                posizione_taxi_y++;
                /*ESCO DALLA SEZIONE CRITICA*/

                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].availableSpace);
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].mutex); /*Rilascio il mutex vecchio*/
                V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/

			}
		}
	}
    /*---------------------------------------------------SPOSTAMENTO VERSO DESTRA-----------------------------------------------------------*/

	while( posizione_taxi_y < *posizione_taxi_y_finale ) {
			if(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].cellType != BLOCK) {

                    P(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace);
                    P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/
                    P(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex); /*prendo il mutex dove vado*/

                    /*SEZIONE CRITICA*/
                    mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--;
                    mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].taxiOnThisCell++;
                    mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].totalNumberOfTaxiPassedHere++;
                    posizione_taxi_y++;
                    /*ESCO DALLA SEZIONE CRITICA*/

                    V(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].availableSpace);
                    V(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].mutex); /*Rilascio il mutex vecchio*/
                    V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/

				} else {
					if(posizione_taxi_x > 0){
				
                        P(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace);
                        P(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex); /*Ottengo il mutex dove vado*/
                        P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/


                        /*SEZIONE CRITICA*/
                        mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                        mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                        mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
                        posizione_taxi_x--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/

                        /*ESCO DALLA SEZIONE CRITICA*/
                        V(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace);
                        V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/
                        V(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex); /*Rilascio il mutex nuovo*/

					} else {

                        P(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace);
                        P(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Ottengo il mutex dove sono*/
                        P(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex); /*Ottengo il mutex dove vado*/


                        /*SEZIONE CRITICA*/
                        mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
                        mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/
                        mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
                        posizione_taxi_x++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
                        /*ESCO DALLA SEZIONE CRITICA*/

                        V(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace);
                        V(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/
                        V(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex); /*Rilascio il mutex vecchio*/

					}
				}
		}
			/*se lo spostamento non è completo, riavvio lo spostamento in maniera ricorsiva*/
			if(posizione_taxi_x != *posizione_taxi_x_finale || posizione_taxi_y != *posizione_taxi_y_finale) {
				move(mappa, posizione_taxi_x, posizione_taxi_y, posizione_taxi_x_finale, posizione_taxi_y_finale , taxiSemaphore_id);
			}
	}



void signalHandler(int signalNo){
    switch(signalNo){
        case SIGALRM:
            break;

    }
}




void closestSource(struct grigliaCitta *mappa, int taxiR, int taxiC, int *destR, int *destC){
    /*
        idea: scorro la matrice in 4 sensi diversi e ottengo 4 coordinate e poi mi prendo i punti più vicini di quei quattro. lo faccio in un unico metodo

    */
   int i=0, j=0;
   int possibleRcoordinates[4];
   int possibleCcoordinates[4];
   int xTemp,yTemp;
   int countR = 0;
   int countC = 0;
   int minDistance = INT_MAX;
   int positionOfMinDistance = 0, tmp;

   boolean exitLoop = FALSE;

    /*scorro da alto a basso, da sx a dx*/
    for(i=taxiR; !exitLoop && (countR < SO_HEIGHT); i++){
        countC=0;
        for(j=taxiC; !exitLoop && (countC < SO_WIDTH); j++ ){
            if(mappa->matrice[i][j].cellType == SOURCE){
                exitLoop = TRUE;
                possibleRcoordinates[0] = i;
                possibleCcoordinates[0] = j;
            }
            countC++;
            if(j==(SO_WIDTH-1)) j = 0;
        }
        countR++;
        if(i==(SO_HEIGHT - 1)) i = 0;
    }

    exitLoop = FALSE;
    countR = 0;
    countC = 0;
    /*scorro da alto a basso, da dx a sx*/
    for(i=taxiR; !exitLoop && (countR < SO_HEIGHT); i++){
        countC = 0;
        for(j=taxiC; !exitLoop && (countC < SO_WIDTH); j--){
            if(mappa->matrice[i][j].cellType == SOURCE){
                exitLoop = TRUE;
                possibleRcoordinates[1] = i;
                possibleCcoordinates[1] = j;
            }
            countC++;
            if(j==0) j = SO_WIDTH-1;
        }
        countR++;
        if(i==(SO_HEIGHT - 1)) i = 0;
    }

      /*
        ora come sopra ma scorro prima le colonne e poi le righe
    */
    exitLoop = FALSE;
    countR = 0;
    countC = 0;
   /*scorro da alto a basso, da sx a dx*/
    for(i=taxiC; !exitLoop && (countC < SO_WIDTH); i++){
        for(j=taxiR; !exitLoop && (countR < SO_HEIGHT); j++){
            if(mappa->matrice[j][i].cellType == SOURCE){
                exitLoop = TRUE;
                possibleRcoordinates[2] = j;
                possibleCcoordinates[2] = i;
            }
            countR++;
            if(j==(SO_HEIGHT-1)) j = 0;
        }
        countC++;
        if(i==(SO_WIDTH - 1)) i = 0;
    }


    exitLoop = FALSE;
    countR = 0;
    countC = 0;
   /*scorro da alto a basso, da sx a dx*/
    for(i=taxiC; !exitLoop && (countC < SO_WIDTH); i++){
        for(j=taxiR; !exitLoop && (countR < SO_HEIGHT); j--){
            if(mappa->matrice[j][i].cellType == SOURCE){
                exitLoop = TRUE;
                possibleRcoordinates[3] = j;
                possibleCcoordinates[3] = i;
            }
            countR++;
            if(j==0) j = SO_HEIGHT-1;
        }
        countC++;
        if(i==(SO_WIDTH - 1)) i = 0;
    }
    
    
  

    for(i=0;i<4;i++){
        tmp = (int) sqrt(pow(taxiR-possibleRcoordinates[i], 2) + pow(taxiC - possibleCcoordinates[i], 2));
        printf("Close find: %d-%d\n", possibleRcoordinates[i], possibleCcoordinates[i]);
        if(tmp < minDistance){
            minDistance = tmp;
            positionOfMinDistance = i;
        }
    }

    *destR = possibleRcoordinates[positionOfMinDistance];
    *destC = possibleCcoordinates[positionOfMinDistance];


}