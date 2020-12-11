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



	Questa funzione permette al taxi di muoversi verso la sua destinazione, sia SO_SOURCE che destinazione prelevata dal messaggio
*/
void closestSource(struct grigliaCitta *mappa, int taxiX, int taxiY, int *destX, int *destY);
int closestMoveUpper(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveLower(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveRight(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);
int closestMoveLeft(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY);

void moveTowards_sosource(struct grigliaCitta *mappa, int posizione_taxi_x_iniziale, int posizione_taxi_y_iniziale, int posizione_taxi_x_finale, int posizione_taxi_y_finale, int taxiSemaphore_id);

struct sembuf sops; 

int main(int argc, char * argv[]){
	  	int posizione_taxi_x, posizione_taxi_y; /*Coordinate della posizione del taxi*/
		struct grigliaCitta *mappa; /*mappa della citta*/
		int so_duration = atoi(argv[1]); /*recupero la durata della simulazione*/
		int so_taxi = atoi(argv[2]); /*recupero il numero di taxi nella simulazione*/
		int queue_key, queue_id; /*Variabili per la coda di messaggi*/
		int taxiSemaphore_id;
		int shm_Key, shm_id, shmId_ForTaxi, shmKey_ForTaxi; /*Variabili per la memoria condivisa*/
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
		/*Seleziono un punto random*/
		posizione_taxi_x = rand()%SO_HEIGHT;
		posizione_taxi_y = rand()%SO_WIDTH;	
        availableSpaceOnCell = semctl(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, 0, GETVAL);
	} while(availableSpaceOnCell == 0 && (mappa->matrice[posizione_taxi_x][posizione_taxi_y].cellType != BLOCK));
	/*fprintf(stderr, "Sono spawnato in posizione: [%d][%d]\n", posizione_taxi_x, posizione_taxi_y);*/
	/*La condizione fa si' che il taxi non spawni dove non gli è consentito, ossia in una cella non ROAD oppure in una cella con availableSpace = taxiOnThisCell*/
	/*Incremento il numero di semafori presenti nella cella in cui sono spawnato*/

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

	fprintf(stderr, "Posizione dopo dello spawn: [%d][%d]\n", posizione_taxi_x, posizione_taxi_y);
	/*moveTowards_sosource(mappa, posizione_taxi_x, posizione_taxi_y, 0, 2, taxiSemaphore_id);*/
}


void moveTowards_sosource(struct grigliaCitta *mappa, int posizione_taxi_x, int posizione_taxi_y, int posizione_taxi_x_finale, int posizione_taxi_y_finale, int taxiSemaphore_id) {
	/*Il taxi è fermo nella sua posizione, la posizione gli viene passata come parametro, ho l'idea di farla chiamare da spawnTaxi*/
	/*mi sposto inizialmente verso la SO_SOURCE più vicina*/
	/*Ottengo le coordinate della SO_SOURCE più vicina con la funzione closestSource*/
	/*Questa implementazione non tiene conto delle celle HOLES*/
	/*Spostamento verso destra*/
	while(posizione_taxi_y < posizione_taxi_y_finale) { 
		fprintf(stderr, "La mia y è minore di dove voglio andare\n");
		fprintf(stderr, "[%d][%d]\n", posizione_taxi_x, posizione_taxi_y);	
		/*fprintf(stderr, "[%d][%d]", *posizione_taxi_x, *posizione_taxi_y);*/		
		/*p su available space*/
		sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
		sops.sem_flg = 0; /*Comportamento di default*/
		sops.sem_op = -1; /*Decremento la variabile mutex e la variabile availableSpace*/
		fprintf(stderr, "Checkpoint I\n");
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace, &sops, 1);
		posizione_taxi_y++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex nuovo*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].mutex, &sops, 1); /*Ottengo il mutex vecchio*/
		/*SEZIONE CRITICA*/
		mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
		mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/		
		mappa->matrice[posizione_taxi_x][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
		/*ESCO DALLA SEZIONE CRITICA*/
		sops.sem_op = 1; /*Incremento la il semaforo mutex*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].availableSpace, &sops, 1);
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex nuovo*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].mutex, &sops, 1); /*Rilascio il mutex vecchio*/
	


	}
	/*Spostamento verso sinistra*/
	while(posizione_taxi_y > posizione_taxi_y_finale) { 
		fprintf(stderr, "La mia y è maggiore di dove voglio andare\n");
		fprintf(stderr, "[%d][%d]\n", posizione_taxi_x, posizione_taxi_y);		
		/*Entro nella sezione critica del semaforo in cui mi sto spostando*/
		sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
		sops.sem_flg = 0; /*Comportamento di default*/
		sops.sem_op = -1; /*Decremento la variabile mutex e la variabile availableSpace*/
		fprintf(stderr, "Checkpoint II\n");

		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y-1].availableSpace, &sops, 1);
		posizione_taxi_y--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex nuovo*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex, &sops, 1); /*Ottengo il mutex vecchio*/
		/*SEZIONE CRITICA*/
		mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
		mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/		
		mappa->matrice[posizione_taxi_x][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
		/*ESCO DALLA SEZIONE CRITICA*/
		sops.sem_op = 1; /*Incremento la il semaforo mutex*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].availableSpace, &sops, 1);
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex nuovo*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y+1].mutex, &sops, 1); /*Rilascio il mutex vecchio*/
	}
	/*Spostamento verso destra*/
	while(posizione_taxi_x < posizione_taxi_x_finale) {
		fprintf(stderr, "La mia x è minore di dove voglio andare\n"); 
		fprintf(stderr, "[%d][%d]\n", posizione_taxi_x, posizione_taxi_y);		

		sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
		sops.sem_flg = 0; /*Comportamento di default*/
		sops.sem_op = -1; /*Decremento la variabile mutex e la variabile availableSpace*/
				fprintf(stderr, "Checkpoint III\n");
		semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].availableSpace, &sops, 1);
		semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex nuovo*/
		posizione_taxi_x++; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
		semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex vecchio*/
		/*SEZIONE CRITICA*/
		mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
		mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/		
		mappa->matrice[posizione_taxi_x][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
		/*ESCO DALLA SEZIONE CRITICA*/
		sops.sem_op = 1; /*Incremento la il semaforo mutex*/
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, &sops, 1);
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex nuovo*/
		semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex vecchio*/
	
	}
	/*Spostamento verso l'alto*/
	while(posizione_taxi_x > posizione_taxi_x_finale) { 
		fprintf(stderr, "La mia x è maggiore di dove voglio andare\n");
		fprintf(stderr, "[%d][%d]", posizione_taxi_x, posizione_taxi_y);		

		sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
		sops.sem_flg = 0; /*Comportamento di default*/
		sops.sem_op = -1; /*Decremento la variabile mutex e la variabile availableSpace*/
		fprintf(stderr, "Checkpoint IV\n");
		semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace, &sops, 1);
		semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex nuovo*/
		posizione_taxi_x--; /*Lo spostamento avviene quando sono sicuro che il taxi si possa spostare*/
		semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex, &sops, 1); /*Ottengo il mutex vecchio*/
		/*SEZIONE CRITICA*/
		mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].taxiOnThisCell--; /*Abbandonando la cella diminuisco il numero di taxi in quella cella*/
		mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell++; /*Entrando nella nuova cella aumento il numero di taxi in quella cella*/		
		mappa->matrice[posizione_taxi_x][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
		/*ESCO DALLA SEZIONE CRITICA*/
		sops.sem_op = 1; /*Incremento la il semaforo mutex*/
		semop(mappa->matrice[posizione_taxi_x-1][posizione_taxi_y].availableSpace, &sops, 1);
		semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex nuovo*/
		semop(mappa->matrice[posizione_taxi_x+1][posizione_taxi_y].mutex, &sops, 1); /*Rilascio il mutex vecchio*/
		
	}
	fprintf(stderr, "Posizione finale: [%d][%d]\n", posizione_taxi_x, posizione_taxi_y);
}



/*
    idea: mi sposto a spirale nel cercare la so source piu vicina. per rendere il codice dell'algoritmo piu semplice però, mi immagino che il mio campo di 
    ricerca sia effettivamente piu grande della dimensione della mappa: ovvero mi immagino di creare una griglia abbastanza grande da centr
*/





void closestSource(struct grigliaCitta *mappa, int taxiX, int taxiY, int *destX, int *destY){
    int tempX = taxiX, tempY= taxiY;
    int rangeX = 3, rangeY = 3;
    int sourceFound = 0;
    while(sourceFound == 0){

        tempX = taxiX - ((rangeX-1)/2); /*mi sposto come punto di inzio della ricerca nella cella in alto a sx nella diagonale*/
        tempY = taxiY - ((rangeY -1)/2);

        sourceFound = closestMoveUpper(mappa, &destX, &destY, rangeX, rangeY, &tempX, &tempY);

        if(sourceFound == 0)  sourceFound = closestMoveRight(mappa, &destX, &destY, rangeX, rangeY, &tempX, &tempY);
        
        if(sourceFound == 0)  sourceFound = closestMoveLower(mappa, &destX, &destY, rangeX, rangeY, &tempX, &tempY);

        if(sourceFound == 0)  sourceFound = closestMoveLeft(mappa, &destX, &destY, rangeX, rangeY, &tempX, &tempY);


        /*incremento il range su cui devo controllare los corrimento*/
        rangeX += 2;
        rangeY += 2;
    }

}

int closestMoveUpper(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY){
    int i = 0;
    *tempX ++;/*mi sposto di una cella a destra di quella in diagonale per potere mantenere uniforme il codice*/
        for(i=0;i<rangeX-1;i++){ /*scorro la linea superiore*/    
           if(*tempX>=0 && *tempY>=0 && *tempY<SO_HEIGHT && *tempY < SO_WIDTH){ //vado a controllare la matrice solamente se mi trovo dentro l'intervallo delle dimensioni della mappa
               if(mappa->matrice[tempX][tempY].cellType == SOURCE){
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
     tempY++; /*ho già controllato che la cella non sia una source. mi sopsto sotto di uno*/
        
        for(i=0;i<rangeY-1;i++){ /*controllo la linea a destra*/
            if(tempX>=0 && tempY>=0 && tempY<SO_HEIGHT && tempY < SO_WIDTH){
               if(mappa->matrice[tempX][tempY].cellType == SOURCE){
                    *destX = tempX;
                    *destY = tempY;
                    return 1;
                }
           }
           tempY++;/*mi sposto di sotto*/
        }
        return 0;
}


int closestMoveLeft(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY){
    int i = 0;
     tempX --; /*mi sposto indietro di uno*/

        for(i=0;i<rangeX-1;i++){ /*scorro la linea inferiore*/    
           if(tempX>=0 && tempY>=0 && tempY<SO_HEIGHT && tempY < SO_WIDTH){
               if(mappa->matrice[tempX][tempY].cellType == SOURCE){
                    *destX = tempX;
                    *destY = tempY;
                    return 1;
                }
           }
           tempX--; /*mi sposto a destra*/
        }
        return 0;
}


int closestMoveRight(struct grigliaCitta *mappa, int *destX, int *destY, int rangeX, int rangeY, int *tempX, int *tempY){
    int i = 0;
      tempY--; /*mi sposto di una cella in sopra*/

        for(i=0;i<rangeY-1;i++){ /*scorro la linea a sinistra*/    
           if(tempX>=0 && tempY>=0 && tempY<SO_HEIGHT && tempY < SO_WIDTH){
               if(mappa->matrice[tempX][tempY].cellType == SOURCE){
                    *destX = tempX;
                    *destY = tempY;
                    return 1;
                }
           }
           tempY--; /*mi sposto a destra*/
        }
        return 0;
}












