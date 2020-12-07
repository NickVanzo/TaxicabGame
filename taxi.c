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
void spawnTaxi(struct grigliaCitta *mappa, int x, int y);

/*
	Questa funzione permette di fare attach alla memoria condivisa e di aprire la coda di messaggi
*/
void settingIpcs(struct grigliaCitta *mappa);

void spostamentoVersoDestinazione(struct grigliaCitta *mappa);

struct sembuf sops; 

int main(int argc, char * argv[]){
	  	int posizione_taxi_x, posizione_taxi_y; /*Coordinate della posizione del taxi*/
		struct grigliaCitta *mappa; /*mappa della citta*/
		int so_duration = atoi(argv[1]);

		srand(getpid());
		settingIpcs(mappa);

    	spawnTaxi(mappa,posizione_taxi_x,posizione_taxi_y);
    	shmdt(mappa);


    	exit(EXIT_SUCCESS);
}

void spawnTaxi(struct grigliaCitta *mappa, int posizione_taxi_x, int posizione_taxi_y) {
	/*Dubbio è la chiave o l'id del semafoto? Se è la chiave allora devo cambiare il codice perchè non sto facendo la get, se non è la chiave allora non capisco cosa sia sbagliato*/
	/*Errore ottenuto: non vengono stampati i numeri di taxi presenti nelle celle durante la simulazione*/
	int availableSpaceOnCell; 
	/*Seleziono un punto casuale della mappa in cui spawnare, se il massimo di taxi in quella cella è stato raggiunto o non è una road cambio cella*/
	do {
		/*Seleziono un punto random*/
		posizione_taxi_x = rand()%SO_WIDTH;
		posizione_taxi_y = rand()%SO_HEIGHT;
	} while((availableSpaceOnCell = semctl(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, 0, GETVAL)) == 0 || mappa->matrice[posizione_taxi_x][posizione_taxi_y].cellType != ROAD);
	/*La condizione fa si' che il taxi non spawni dove non gli è consentito, ossia in una cella non ROAD oppure in una cella con availableSpace = taxiOnThisCell*/
	/*Incremento il numero di semafori presenti nella cella in cui sono spawnato*/

	/*p su available space*/
	sops.sem_num = 0; /*Ho un solo semaforo in ogni cella*/
	sops.sem_flg = 0; /*Comportamento di default*/
	sops.sem_op = -1; /*Decremento la variabile mutex e la variabile availableSpace*/
	/*Abbasso di uno il valore del semaforo availableSpace*/
	semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, &sops, 1);
	semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1);
	/*Sezione critica*/
	semctl(mappa->matrice[posizione_taxi_x][posizione_taxi_y].availableSpace, 0, availableSpaceOnCell--);
	mappa->matrice[posizione_taxi_x][posizione_taxi_y].taxiOnThisCell++;
	mappa->matrice[posizione_taxi_x][posizione_taxi_y].totalNumberOfTaxiPassedHere++;
	sops.sem_op = 1; 
	/*Uscita sezione critica rilasciando la risorsa*/
	semop(mappa->matrice[posizione_taxi_x][posizione_taxi_y].mutex, &sops, 1);
	/*continua*/
}

void settingIpcs(struct grigliaCitta *mappa) {
	int queue_key, queue_id; /*Variabili per la coda di messaggi*/
	int shm_Key, shm_id; /*Variabili per la memoria condivisa*/


	/*Apertura coda di messaggi*/	
		queue_key = ftok("ipcKey.key", 1);
		if(queue_key == -1){
        	printf("Error retriving message queue key!\n");
        	exit(EXIT_FAILURE);
    	}	

		queue_id  = msgget(queue_key, 0);
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

		shm_id = shmget(shm_Key, sizeof(struct grigliaCitta), 0);
    	if(shm_id == -1) {
        	printf("Error retriving shared memory ID!\n");
        	exit(EXIT_FAILURE);
    	}

    	mappa = shmat(shm_id, NULL, 0);
    	if(mappa == (struct grigliaCitta *)(-1)){
        	printf("Error attaching memory segment!\n");
        	exit(EXIT_FAILURE);
    	}
}



















