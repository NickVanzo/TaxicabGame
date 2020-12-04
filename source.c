#include "include_main.h"
/*
	Il processo deve essere in grado di mandare messaggi in una coda di messaggi
	I messaggi verranno poi letti e consumati dai processi lettori taxi
*/

/*
	Funzione usata per gestire segnali
*/
void handle_signal(int signal);

/*TI HO TOLTO IL SECONDO PARAMETRO PECHè HANDLE SIGNAL RICHIEDE AL MAZ 1 SIGNAL tutte le var che vuoi accedre dall'handler devono essere glibali!*/
struct msgBuf my_msg; /*Struttua del messaggio da inviare nella coda di messaggi*/ /*DEVE ESSERE GLOBALE SE LA VUOI ACCEDERE DA HANDLE SIGNAL!*/
struct grigliaCitta* mappa;
int queue_ID; /*ID della coda di messaggi*/
int SO_SOURCES;



void main(int argc, char * argv[]) {
	/*Ancora non c'e' la memoria condivisa ma suppongo di avere accesso alla memoria e di avere quindi accesso alla griglia*/
	int i,j,k; /*Variabili iteratrici*/
	
	
	int pid;
	struct sigaction sa; /*Struttura per impostare il nuovo handler*/
	int shmKey, msgQeueueKey;
	int shm_id; /*ID della memoria condivisa*/
    int SO_DURATION; 


	/*Variabili per memoria condivisa*/
    struct grigliaCitta* mappa;

    /*PROBLEMA SO_SOURCES VA PASSATO COME PARAMETRO ALTRIMETNI NON FUNZIONA. MA QUINDI SOURCES DEVE ESSERE ARRAY DINAMICO... QUICK FIX:*/ 
    map_cell sources[SO_SOURCES]; /*Array contenente le celle SO_SOURCES*/



    /*Prima cosa, creo memoria condivisa*/
    shmKey = ftok("msgQueue.key", 2);
    shm_id = shmget(shmKey, sizeof(struct grigliaCitta), IPC_CREAT | IPC_EXCL |  0600);
    mappa = shmat(shm_id, NULL, 0); /*SHARED MEMORY FOR GRIGLIA*/


	

	


	/*Inizializzazione della struct sigaction con relativo assegnamento del nuovo handler*/
	bzero(&sa, sizeof(sa));
	sa.sa_handler = handle_signal;

	/*Costruisco l'array contenente i SO_SOURCES processi*/
	for(i=0; i < SO_WIDTH; i++) {
		for(j=0; j < SO_HEIGHT; j++) {
			if((mappa-> matrice[i][j]).cellType == SOURCE) {
				/*Aggiungo la cella SOURCE all'array di SOURCES e incremento la variabile k per spostarmi nell'array delle SO_SOURCES*/
				sources[k] = (mappa->matrice[i][j]); 
				k++;
			}
		}
	}

	/*Ora conosco le coordinate delle SO_SOURCES celle, comincio a costruire il messaggio da mandare in coda*/
	/*Ottengo la chiave della coda di messaggi*/
	msgQeueueKey = ftok("msgQueue.key", 1);

	/*Ottengo l'ID della coda di messaggi*/
	if(queue_ID = msgget(msgQeueueKey, IPC_CREAT) == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
	}

	srand(getpid()); /*Il messaggio viene mandato dopo X secondi che variano da 1 a SO_DURATION*/

	i = 0;
	/*Creo SO_SOURCES processi che immetterano SO_SOURCES messaggi nelle celle SOURCES*/
	while(i < SO_SOURCES) {
		switch(pid = fork()) {
			case -1:
				fprintf(stderr, "PID: [%d]\nErrore: %s", getpid(), strerror(errno));
				break;
			case 0:
				alarm(rand()%SO_DURATION+1); /*Dico al SO di mandarmi un segnale SIGALRM dopo un numero casuale di secondi*/
				pause(); /*Resto in attesa di un segnale, quello che mi interessa e' SIGALRM, cosi' parte l'handler e mando un messaggio in coda*/
				exit(EXIT_SUCCESS);
			default: 
				/*Do nothing*/
				break;
		}
 	i++;
 	}

	/*	
 		PROBLEMA: cosa succede se non tutti i figli sono riusciti a terminare entro lo scadere del tempo di gioco? Ragionarci su
 		PROBLEMA2: i processi passano a running anche se ricevono segnali diversi da SIGALRM, procedere con mask? Cambiare strategia?
                        -> usa sigprocmask()
 	*/
 	while(wait(NULL) != -1) { /*Aspetto che tutti i figli abbiano terminato, volevo farlo con waitpid ma e' tardi e sono stanco (la modifico domani)*/
 		
 	}
}

void signalHandler(int signal) {
	srand(getpid());
	switch(signal) {
		case SIGALRM:
			srand(getpid()); /*Non so se sia necessario in quanto c'e' gia' nel main ma nel dubbio lo metto*/
			/*Il figlio deve mandare un messaggio nella coda di messaggi*/
			my_msg.mtype = rand()%SO_SOURCES+1; /*Il tipo di messaggio riguarda quale delle celle SOURCE fara' richiesta*/
			do {
				my_msg.xDest = rand()%SO_WIDTH; /*Genero randomicamente una coordinata x*/
				my_msg.yDest = rand()%SO_HEIGHT; /*Genero randomicamente una coordinata y*/
			} while((mappa-> matrice[my_msg.xDest][my_msg.yDest]).cellType == BLOCK || (mappa->matrice[my_msg.xDest][my_msg.yDest]).cellType == SOURCE);
			/*La condizione per uscire dal ciclo e' che la cella selezionata come destinazione sia di tipo ROAD*/
			msgsnd(queue_ID, &my_msg, sizeof(int)*2 - sizeof(long), 0);
			break;
	}
}