/**
VR477731
*Laura Salinetti

VR471377
Michael Testini

VR471982
ALessandro Comai
2/2/2024
**/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SHM_KEY 9230
#define SEM_KEY 456
#define MAX 15

//---------------------------------------VARIABILI GLOBALI E STRUCT---------------------------------------------------------------------//

struct CampoDaGioco{
	char campo[MAX][MAX];
	char gettone;
	int numGiocatori;
	int righe;
	int colonne;
	int mossa;
	pid_t giocatore1;
	pid_t giocatore2;
	pid_t server;
};

struct CampoDaGioco *memoria_condivisa;
struct sembuf sem_buf;

union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

bool turno=true, statoSIGINT = true;
int semid, shmid;

//---------------------------------------DICHIARAZIONE FUNZIONI-------------------------------------------------------------------------//

void semOp (int semid, unsigned short sem_num, short sem_op);
void inizializzaCampoDaGioco(int righe, int colonne,char gettone1, char gettone2);
void connessione();
int controllaPartita();
void inserisceGettone();
void scambioTurnoEGettone(char gettone1, char gettone2);
void controlloParametri(int argc, char *argv[]);
void chiudi_server();
void errExit(const char *messaggio);
void gestoreSegnale(int sig);
void stringaErrore(const char *messaggio, bool modalita);

//-----------------------------------------------------MAIN-------------------------------------------------------------------------//

int main(int argc, char *argv[]){
	
	controlloParametri(argc,argv);
	
	//Salvataggio dei parametri passati
	int righe = atoi (argv[1]);
	int colonne = atoi (argv[2]);
	char gettone1 = *argv[3];
	char gettone2 = *argv[4];

	connessione(); //Crea o ottiene la memoria condivisa e i semafori. Inizializza i segnali.

	printf("FORZA 4\n");
	inizializzaCampoDaGioco(righe,colonne,gettone1,gettone2); //Inizializzazione del campo da gioco
	memoria_condivisa->gettone = gettone1; //Affida al primo client che si collega il primo gettone
	printf("Aspettando che si connetta il primo client...\n");
	semOp(semid,3,1);//Libera il primo client che stava aspettando che il server si connettesse
	semOp(semid,0,-1); //Si pone in attesa finchè non si collega il primo client
	printf("Il primo client si è connesso!\n");
	
	memoria_condivisa->gettone = gettone2; //Si collega il secondo client e gli affida il secondo gettone
	semOp(semid,3,1);//Libera il secondo client che stava aspettando che il server si connettesse
	printf("Aspettando che si connetta il secondo client...\n");
	semOp(semid, 0, -1); //Il server aspetta che si connetti un secondo client
	
	printf("Il secondo client si è connesso!\nPartita iniziata!\nAspettando la prima mossa del giocatore...\n");
	
	fflush(stdout);
	int StatoFine=0;
        
        while(StatoFine == 0){ 
        	semOp(semid, 0, -1); //Il server aspetta che venga fatta la mossa
		inserisceGettone();  //Inserisce il gettone 
		StatoFine=controllaPartita(); //Controlla se ha vinto
		scambioTurnoEGettone(gettone1,gettone2); //Scambia i turni e i gettoni
	} 
	
	//La partita è finita, dice chi ha vinto 
	if(StatoFine==2){
		printf("Pareggio!\n");
		memoria_condivisa->mossa=-3;
	}else{
		if(turno){
		  	memoria_condivisa->mossa=-1;
		  	printf("Giocatore '%c' ha vinto\n",gettone1);
		}else{  
		  	memoria_condivisa->mossa=-2;
		  	printf("Giocatore '%c' ha vinto\n",gettone2);
		}
	}
	
	
	//Il server sblocca entrambi i client così che possano chiudersi...
	semOp(semid,2,1);
	semOp(semid,1,1);
	
	semOp(semid,0,-1); //...per poi aspettare che si chiudano
  	chiudi_server(); //Chiude il server, semafori e memoria condivisa	

}

//------------------------------CONTROLLI PARAMETRI--------------------------
void controlloParametri(int argc, char *argv[]){

	//Stampa un messaggio di errore se vengono inseriti meno o più di quattro parametri
	if(argc != 5){
		stringaErrore("Hai scritto il comando in modo errato, segui il seguente aiuto:\n./F4Server numero1 numero2 gettone1 gettone2\n",false);
	}
	
	//Stampa un messaggio di errore se viene dichiarato un campo da gioco più piccolo di 5x5
	if(atoi(argv[1]) < 5 || atoi(argv[2]) < 5){
		stringaErrore("Hai provato a inizializzare un campo da gioco troppo piccolo! Dichiarane uno almeno da 5x5!\n",false);
	}
	
	//Stampa un messaggio di errore se viene dichiarato un campo da gioco più grande di 15x15
	if(atoi(argv[1]) > 15 || atoi(argv[2]) > 15){
		stringaErrore("Hai provato a inizializzare un campo da gioco troppo grande!\n",false);
	}
	
	//Stampa un messaggio di errore se vengono dichiarati due gettoni uguali
	if(argv[3]==argv[4]){
         	stringaErrore("Errore, hai inserito due gettoni uguali!\n",false);
     	}
}
//------------------------------SCAMBIO TURNO---------------------------------

void scambioTurnoEGettone(char gettone1, char gettone2){
	//se turno è true, vuol dire che è il turno del primo giocatore
	//se turno è false, vuol dire che è il turno del secondo giocatore
	if(turno)
	{
		memoria_condivisa->gettone = gettone1;
	   	semOp(semid, 1, 1);
 	}else{
 		memoria_condivisa->gettone = gettone2;
	    	semOp(semid, 2, 1);
	}

	turno = !turno;
}

//---------------------------------------SEMOP-------------------------------------------------------------------------

void semOp (int semid, unsigned short sem_num, short sem_op) {
    	struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};
	bool here = false;
		
    	while (semop(semid, &sop, 1) == -1 && errno == EINTR){ //Il processo continua ad aspettare finchè viene interrotto da un segnale
    		here = true; //Questa assegnazione viene fatta per assicurarmi che se avviene un errore è perchè è successo qualcosa con la system call del semaforo.
        }
        
        if(errno != EINTR && here)
        	errExit("semop non ha avuto successo");    	
        	
}

//---------------------------------------INIZIALIZZAZIONE CAMPO-------------------------------------------------------------------------

void inizializzaCampoDaGioco(int righe, int colonne,char gettone1, char gettone2) {
	for (int i = 0; i < righe; i++) {
    		for (int j = 0; j < colonne; j++) {
      			memoria_condivisa->campo[i][j] = ' '; 
    		}
  	}
  	
  	memoria_condivisa->numGiocatori = 0;
  	memoria_condivisa->righe=righe;
  	memoria_condivisa->colonne=colonne;
  	if((memoria_condivisa->server = getpid()) == -1){
  		errExit("getpid non ha avuto successo!");
  	}
}

//---------------------------------------CONTROLLO VINCITA-------------------------------------------------------------------------//

int controllaPartita() {

	bool pareggio = true; 

	for(int i = 0; i < memoria_condivisa->righe; i++){
		for(int j = 0; j < memoria_condivisa->colonne; j++){
			if(memoria_condivisa->campo[i][j] == ' '){
				pareggio = false;
			}
		}
	}
	
	if(pareggio){
		return 2; //Pareggio
	}else{
		// controllo per riga
		for (int i = 0; i < memoria_condivisa->righe; i++) {
			for (int j = 0; j <= memoria_condivisa->colonne - 4; j++) { //Ottimizzazione: cicla sulle colonne di cui effettivamente bisogna fare un controllo
				if (memoria_condivisa->campo[i][j] == memoria_condivisa->gettone && memoria_condivisa->campo[i][j + 1] == memoria_condivisa->gettone &&
			 	memoria_condivisa->campo[i][j + 2] == memoria_condivisa->gettone && memoria_condivisa->campo[i][j + 3] == memoria_condivisa->gettone) {
					return 1; //Vincita
		      		}
		    	}
		}

		// controllo per colonna
		for (int i = 0; i <= memoria_condivisa->righe - 4; i++) { //Ottimizzazione: cicla sulle righe di cui effettivamente bisogna fare un controllo
			for (int j = 0; j < memoria_condivisa->colonne; j++) {
		      		if (memoria_condivisa->campo[i][j] == memoria_condivisa->gettone && memoria_condivisa->campo[i + 1][j] == memoria_condivisa->gettone &&
			  	memoria_condivisa->campo[i + 2][j] == memoria_condivisa->gettone && memoria_condivisa->campo[i + 3][j] == memoria_condivisa->gettone) {
					return 1; //Vincita
		      		}
		    	}
		}

		  // controllo diagonale (verso l'alto e verso il basso)
		for (int i = 0; i <= memoria_condivisa->righe - 4; i++) { //Ottimizzazione: cicla sulle colonne di cui effettivamente bisogna fare un controllo
		    	for (int j = 0; j <= memoria_condivisa->colonne - 4; j++) { //Ottimizzazione: cicla sulle righe di cui effettivamente bisogna fare un controllo
		      		if (memoria_condivisa->campo[i][j] == memoria_condivisa->gettone && memoria_condivisa->campo[i + 1][j + 1] == memoria_condivisa->gettone &&
			  	memoria_condivisa->campo[i + 2][j + 2] == memoria_condivisa->gettone && memoria_condivisa->campo[i + 3][j + 3] == memoria_condivisa->gettone) {
					return 1; //Vincita
		      		}

		      		if (memoria_condivisa->campo[i + 3][j] == memoria_condivisa->gettone && memoria_condivisa->campo[i + 2][j + 1] == memoria_condivisa->gettone &&
			  	memoria_condivisa->campo[i + 1][j + 2] == memoria_condivisa->gettone && memoria_condivisa->campo[i][j + 3] == memoria_condivisa->gettone) {
					return 1; //Vincita
		      		}
		    	}
		}
	}
       
         
	return 0; //Non è ancora finita la partita
}

//-----------------------------------------------------INSERISCI GETTONE------------------------------------------------//

void inserisceGettone(){

  //Il controllo della riga piena viene fatta da parte del client
  int riga = memoria_condivisa->righe - 1;
  while (riga >= 0 && memoria_condivisa->campo[riga][memoria_condivisa->mossa] != ' ') {
      riga--;
  }
  
  memoria_condivisa->campo[riga][memoria_condivisa->mossa] = memoria_condivisa->gettone;
  
  printf("Giocatore '%c' -> Mossa = %i\n",memoria_condivisa->gettone, memoria_condivisa->mossa+1);

}

//------------------------------GESTORE SEGNALE----------------------------------------------------------------------//
void gestoreSegnale(int sig){

    switch(sig){ 
        case SIGTERM:
        case SIGINT: //Questo segnale viene mandato quando si preme ctrl+c. Dopo due volte che viene premuto, si chiude tutto.
            if(statoSIGINT){
                printf("Se premi un'altra volta Ctrl + C la partita viene terminata!\n");
                statoSIGINT = false;
            }else{
                if(memoria_condivisa->numGiocatori==0){ //Se il numero di giocatori è pari a 0, viene chiuso direttamente il server
                	printf("Chiusura server forzata!\n");
                        chiudi_server();
		}else{
			if(memoria_condivisa->numGiocatori == 1){ //Altrimenti se ci sta uno solo, allora viene mandato un segnale solo a lui
				kill(memoria_condivisa->giocatore1, SIGUSR2);
				printf("Chiusura server forzata!\n");
				chiudi_server();
			}else{ //Altrimenti ad entrambi
				kill(memoria_condivisa->giocatore1, SIGUSR2);
				kill(memoria_condivisa->giocatore2, SIGUSR2);
				printf("Partita è stata forzatamente chiusa.\n");
				chiudi_server();
			}
		}
            }
            break;
        case SIGUSR1:
                if(memoria_condivisa->numGiocatori==0) //Se il numero di giocatori è pari a zero, vuol dire che si deve chiudere direttamente il server
                {
		        chiudi_server();
                	
                }else{
		    kill(memoria_condivisa->giocatore2, SIGUSR1);
		    chiudi_server();
		}
            	break;
        case SIGUSR2:
                if(memoria_condivisa->numGiocatori==0) //Se il numero di giocatori è pari a zero, vuol dire che si deve chiudere direttamente il server
                {
		        chiudi_server();
                }else{
		    kill(memoria_condivisa->giocatore1, SIGUSR1);
		    chiudi_server();
		}
		break;
    }
}

//---------------------------------------CONNESSIONE-------------------------------------------------------------------------//

void connessione(){

        // Ottieni l'ID della memoria condivisa
    	shmid = shmget(SHM_KEY, sizeof(struct CampoDaGioco), IPC_EXCL | IPC_CREAT | 0666); //aggiungere flag
    	if (shmid == -1){
        	stringaErrore("shmget non ha avuto successo!\n",true);
        }
	
    	// Collega la memoria condivisa al processo
    	memoria_condivisa = (struct CampoDaGioco*)shmat(shmid, NULL, 0);
    	if (memoria_condivisa == (struct CampoDaGioco *)-1){
        	stringaErrore("shmat non ha avuto successo!\n",true);
        }
        
         //Inizializza i segnali interessati 
    	if (signal(SIGUSR2, gestoreSegnale) == SIG_ERR || signal(SIGINT, gestoreSegnale) == SIG_ERR ||signal(SIGUSR1, gestoreSegnale) == SIG_ERR || signal(SIGTERM, gestoreSegnale) == SIG_ERR){
		stringaErrore("la inizializzazione dei segnali non ha avuto successo!",true);
	}
    	
    	//Crea o ottiene l'ID dei semafori
    	unsigned short semInitVal[] = {0,0,0,0};	//0 = server, 1 = client1, 2 = client2
    	//Il quarto semaforo serve per far aspettare il client nel mentre il server inizializza tutto
    	union semun arg;
   	arg.array = semInitVal;
    	semid = semget(SEM_KEY, 4, IPC_CREAT | 0666);
    	if (semid == -1)
        	errExit("semget non ha avuto successo!");
        	
    	if(semctl(semid, 0, SETALL, arg) == -1)
    		errExit("semctl non ha avuto successo!");

}

//---------------------------------------CHIUDI SERVER-------------------------------------------------------------------------

void chiudi_server(){

	//Si stacca dal segmento di memoria condivisa
	if (shmdt(memoria_condivisa) == -1) {
        	errExit("shmdt non ha avuto successo!\n");
    	}

	//Rimuove il segmento di memoria condivisa
	if(shmctl(shmid, IPC_RMID, NULL) == -1)
		errExit("shmctl non ha avuto successo!\n");

	//Rimuove i semafori
	if(semctl(semid, 0, IPC_RMID, 0) == -1)
		errExit("semctl non ha avuto successo!\n");

	printf("Partita finita.\n");
	exit(EXIT_SUCCESS);
			
}

//---------------------------------STRINGA ERRORE------------------------------------------------
void stringaErrore(const char *messaggio, bool modalita){
	if(modalita){
		perror(messaggio);
		exit(EXIT_SUCCESS);
	}else{
		printf("%s",messaggio);
		exit(EXIT_SUCCESS);
	}
}

//---------------------------------------ERREXIT-------------------------------------------------------------------------

void errExit(const char *messaggio) {
    perror(messaggio);
    kill(memoria_condivisa->giocatore1, SIGUSR2);
    kill(memoria_condivisa->giocatore2, SIGUSR2);
    chiudi_server();
}
