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
#include <unistd.h>
#include <stdbool.h>

#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SHM_KEY 9230
#define SEM_KEY 456
#define MAX 15

//---------------------------------------VARIABILI GLOBALI E STRUCT-------------------------------------------------------------------------

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

int semid, shmid;

//---------------------------------------DICHIARAZIONE FUNZIONI--------------------------------------------------------------

void semOp (int semid, unsigned short sem_num, short sem_op);
void chiudi_client();
void connessione();
void stampaCampo();
void mossa();
bool mossaNonValida(int mossa);
void gestoreSegnale(int sig);
void errExit(const char *messaggio);

//---------------------------------------MAIN---------------------------------------------------------------------------------
int main(int argc, char *argv[]){

	srand(time(NULL));
	
	connessione();
	memoria_condivisa->giocatore2 = getpid();
  	semOp(semid,2,-1); //si pone in attesa e verrà liberato quando avrà finito l'inizializzazione del giocatore
  	memoria_condivisa->numGiocatori ++; //Rappresenta il bot
  	semOp(semid,0,1); //libera il server così che gli assegna il gettone
  	semOp(semid,1,1); //Libera il client così viene notificato
  	
  	int mossa;
  	while(memoria_condivisa->mossa>=0){ 
  		do{
	  		mossa=rand()%memoria_condivisa->colonne;
	  	}while(memoria_condivisa->campo[0][mossa] != ' ');
	  	
	  	memoria_condivisa->mossa=mossa;
	  	semOp(semid,0,1);
		semOp(semid,2,-1); 
  	}
	chiudi_client();	 
}

//---------------------------------------SEMOP------------------------------------------------------------

void semOp (int semid, unsigned short sem_num, short sem_op) {

	struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};
	bool here = false;
		
    	while (semop(semid, &sop, 1) == -1 && errno == EINTR){ //Il processo continua ad aspettare finchè viene interrotto da un segnale
    		here = true; //Questa assegnazione viene fatta per assicurarmi che se avviene un errore è perchè è successo qualcosa con la system call del semaforo.
        }
        
        if(errno != EINTR && here)
        	errExit("semop non ha avuto successo");
        	     	
}

//---------------------------------------CONNESSIONE--------------------------------------------------------

void connessione(){	

		//Inizializza i segnali interessati 
    	if (signal(SIGUSR2, gestoreSegnale) == SIG_ERR || signal(SIGINT, gestoreSegnale) == SIG_ERR ||signal(SIGUSR1, gestoreSegnale) == SIG_ERR || signal(SIGTERM, gestoreSegnale) == SIG_ERR){
			perror("inizializzaSegnali non ha avuto successo!");
			exit(EXIT_SUCCESS);
		}	

        // Ottieni l'ID della memoria condivisa
    	shmid = shmget(SHM_KEY, sizeof(struct CampoDaGioco), IPC_CREAT | 0666);
    	if (shmid == -1)
        	errExit("shmget non ha avuto successo!");

    	// Collega la memoria condivisa al processo
    	memoria_condivisa = (struct CampoDaGioco*)shmat(shmid, NULL, 0);
    	if (memoria_condivisa == (struct CampoDaGioco *)-1)
        	errExit("shmat non ha avuto successo!");
    	
    	semid = semget(SEM_KEY, 4, IPC_CREAT | 0666);
    	if (semid == -1) {
        	errExit("semget non ha avuto successo!");
    	}
    	
    	struct semid_ds sem_info;
    	if(semctl(semid, 0, IPC_STAT, &sem_info) == -1) {
        	errExit("semctl non ha avuto successo!");
    	}
    
		
}

//-------------------------------------GESTORE SEGNALE--------------------------------------
void gestoreSegnale(int sig){
	switch(sig){
		case SIGTERM:
		case SIGINT: 
			kill(memoria_condivisa->server, SIGUSR1);
			chiudi_client();
			break;
		case SIGUSR2:
			chiudi_client();
			break;
	}

}

//---------------------------------------CHIUDI CLIENT--------------------------------------------------------

void chiudi_client(){

	memoria_condivisa->numGiocatori--;
			
	if(shmdt(memoria_condivisa)== -1){
		errExit("shmdt non ha avuto successo!");
	}

	semOp(semid,0,1);
	exit(EXIT_SUCCESS);
}

//---------------------------------------ERREXIT--------------------------------------------------------

void errExit(const char *messaggio) {
    	perror(messaggio);
    	kill(memoria_condivisa->server, SIGUSR1);  	
    	chiudi_client();
}

