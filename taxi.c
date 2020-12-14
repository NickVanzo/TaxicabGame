
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


void closestSource(struct grigliaCitta *mappa, int taxiX, int taxiY, int *destX, int *destY);
int closestMoveUpper(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveLower(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveRight(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveLeft(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);



/*
	Questa funzione permette al taxi di muoversi verso la sua destinazione, sia SO_SOURCE che destinazione prelevata dal messaggio
*/
void move(struct grigliaCitta *mappa, int posizione_taxi_x_iniziale, int posizione_taxi_y_iniziale, int posizione_taxi_x_finale, int posizione_taxi_y_finale, int taxiSemaphore_id);

void moveUp();
void moveDown();
void moveRight();
void moveLeft();


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

	move(mappa, posizione_taxi_x, posizione_taxi_y, 15, 8, taxiSemaphore_id);
}


void move(struct grigliaCitta *mappa, int posizione_taxi_x, int posizione_taxi_y, int posizione_taxi_x_finale, int posizione_taxi_y_finale, int taxiSemaphore_id) {
		/*----------------------------------------------SPOSTAMENTO VERSO SINISTRA---------------------------------------------------------*/
		while(posizione_taxi_y > posizione_taxi_y_finale)
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
	while(posizione_taxi_x < posizione_taxi_x_finale)
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
	while(posizione_taxi_x > posizione_taxi_x_finale)
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

	while( posizione_taxi_y < posizione_taxi_y_finale ) {
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
			if(posizione_taxi_x != posizione_taxi_x_finale || posizione_taxi_y != posizione_taxi_y_finale) {
				move(mappa, posizione_taxi_x, posizione_taxi_y, posizione_taxi_x_finale,posizione_taxi_y_finale , taxiSemaphore_id);
			}
	}



void signalHandler(int signalNo){
    switch(signalNo){
        case SIGALRM:
            break;

    }
}




void closestSource(struct grigliaCitta *mappa, int taxiX, int taxiY, int *destX, int *destY){
    int tempX = taxiX, tempY= taxiY;
    int rangeX = 3, rangeY = 3;
    int sourceFound = 0;
    while(sourceFound == 0){

        tempX = taxiX - ((rangeX-1)/2); /*mi sposto come punto di inzio della ricerca nella cella in alto a sx nella diagonale*/
        tempY = taxiY - ((rangeY -1)/2);

        sourceFound = closestMoveUpper(mappa, destX, destY, rangeX, rangeY, &tempX, &tempY);

        if(sourceFound == 0)  sourceFound = closestMoveRight(mappa, destX, destY, rangeX, rangeY, &tempX, &tempY);
        
        if(sourceFound == 0)  sourceFound = closestMoveLower(mappa, destX, destY, rangeX, rangeY, &tempX, &tempY);

        if(sourceFound == 0)  sourceFound = closestMoveLeft(mappa, destX, destY, rangeX, rangeY, &tempX, &tempY);


        /*incremento il range su cui devo controllare los corrimento*/
        rangeX += 2;
        rangeY += 2;
    }

}

int closestMoveUpper(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY){
    int i = 0;
    *tempX ++;/*mi sposto di una cella a destra di quella in diagonale per potere mantenere uniforme il codice*/
        for(i=0;i<rangeX-1;i++){ /*scorro la linea superiore*/    
           if(*tempX>=0 && *tempY>=0 && *tempY<SO_HEIGHT && *tempY < SO_WIDTH){ /*vado a controllare la matrice solamente se mi trovo dentro l'intervallo delle dimensioni della mappa*/
               if(mappa->matrice[*tempX][*tempY].cellType == SOURCE){
                    *destX = *tempX;
                    *destY = *tempY;
                    return 1;
                }
           }
           *tempX++; /*mi sposto a destra*/
        }
        return 0;
}


int closestMoveLower(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY){
    int i = 0;
     *tempY++; /*ho già controllato che la cella non sia una source. mi sopsto sotto di uno*/
        
        for(i=0;i<rangeY-1;i++){ /*controllo la linea a destra*/
            if(*tempX>=0 && *tempY>=0 && *tempY<SO_HEIGHT && *tempY < SO_WIDTH){
               if(mappa->matrice[*tempX][*tempY].cellType == SOURCE){
                    *destX = *tempX;
                    *destY = *tempY;
                    return 1;
                }
           }
           *tempY++;/*mi sposto di sotto*/
        }
        return 0;
}


int closestMoveLeft(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY){
    int i = 0;
     *tempX --; /*mi sposto indietro di uno*/

        for(i=0;i<rangeX-1;i++){ /*scorro la linea inferiore*/    
           if(*tempX>=0 && *tempY>=0 && *tempY<SO_HEIGHT && *tempY < SO_WIDTH){
               if(mappa->matrice[*tempX][*tempY].cellType == SOURCE){
                    *destX = *tempX;
                    *destY = *tempY;
                    return 1;
                }
           }
           *tempX--; /*mi sposto a destra*/
        }
        return 0;
}


int closestMoveRight(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY){
    int i = 0;
      *tempY--; /*mi sposto di una cella in sopra*/

        for(i=0;i<rangeY-1;i++){ /*scorro la linea a sinistra*/    
           if(*tempX>=0 && *tempY>=0 && *tempY<SO_HEIGHT && *tempY < SO_WIDTH){
               if(mappa->matrice[*tempX][*tempY].cellType == SOURCE){
                    *destX = *tempX;
                    *destY = *tempY;
                    return 1;
                }
           }
           *tempY--; /*mi sposto a destra*/
        }
        return 0;
}
