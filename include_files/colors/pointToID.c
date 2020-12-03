#include "../../include_main.h"
/*Il prototipo della funzione e' contenuto nel file include_main.h */
/*
	Funzione che prende come parametri le coordinate di x e y e la mappa
	Ritorna il tipo di messaggio da ascoltare
*/


/*
	idea della funzione: scorro la mappa per sapere dove sono le sources. 
	Costruisco lo stesso identico array che uso per costruire i messaggi in source.c. L'array contiene le celle sources.
	Il numero ritornato e' l'indice dell'array a cui siamo arrivati. Dato che hanno lo stesso ordine nell'array hanno lo stesso tipo di messaggio
	Questa e' l'idea, verificare se funziona o meno. 
*/
int pointToID(map_cell ** mappa, int x, int y) {

	int i,j,k;/*Variabili iteratrici*/
	map_cell sources[SO_SOURCES]; /*Array contenente le celle SOURCE*/

	k=1;

	for(i=0; i<SO_WIDTH;i++) {
		for(j=0;j<SO_HEIGHT;j++) {
			if((&mappa[i][j])->cellType == SOURCE) {
				/*Aggiungo la cella SOURCE all'array di SOURCES e incremento la variabile k per spostarmi nell'array delle SO_SOURCES*/
				sources[k] = (&mappa[i][j]); 
				k++;
			}
			if((i == x) && (j == y)) {
				if((&mappa[i][j])->cellType != SOURCE) {
					return -1;
				} else {
					return k;
				}
			}
		}
	}

	return -1; /*La cella non e' stata trovata, aggiustare le coordinate richieste da terminale*/

}