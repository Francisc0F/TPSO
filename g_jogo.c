#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define TAM 10

int pontos = 0 ;

void sig_handler(int sig, siginfo_t *siginfo, void *context){
	
	printf("Terminar Jogo. Sending PID: %ld, UID: %ld value:%d \n", 
	(long) siginfo->si_pid, (long)siginfo->si_uid, siginfo->si_value.sival_int);
	printf("Pontos: %d\n", pontos);
	exit(pontos);
	
}

int main(int argc, char**argv) {
	// sinais



	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	int pid = getpid();
	printf("Pid: %d \n", pid);
	
	act.sa_sigaction = &sig_handler;
	act.sa_flags = SA_SIGINFO;
	if(sigaction(SIGUSR1, &act, NULL) < 0 ){
		perror("SIGUSR1 sigaction");
		return 0;
	}
	

	// jogo
	char listaP[][20] = {"arroz", "feijao", "couve", "lebre", "coelho", "gato", "zebra", "cama", "lisboa", "trump"};
	char listaInc[][20] = {"ar-oz", "f-i-ao", "cou-e", "le-re", "c-elho", "ga-o", "-ebra", "cam-", "l-s-oa", "t-ump"};
	char res[30];
	int nP = 0;
	int vidas = 3;
	printf("------------------------------------------------------------------------");
	printf("\n \t\t Jogo Das Palavras \n");
	printf("Objetivo: \n");
	printf("Cada Iteracao vai ser apresentada uma palavra incompleta  \n");
	printf("e o user tera de adivinhar a palavra em questao. \n\n");
	printf("\tex:(b-tat-) -> babata \n");

	exit(1);
	while(1){
		printf("Palavra: %s \t\t\t Sair -> q",listaInc[nP]);
		printf("\nResposta: ");
		scanf("%s", res);
	
		if( res[0] == 'q')
			break;
	
		if(strcmp(listaP[nP], res)==0){
			pontos++;
			nP++;
			if(nP == TAM){
				printf("\n Acabou o Jogo.");
				break;
			}else{
				printf("\nCorreto.\n");
			}
			
		}
		else {
			vidas--;
			if(vidas == 0){
				printf("Acabaram as vidas o Jogo Acabou");
				break;
			}else{
				printf("\nErrado Tente novamente \t vidas: %d \n", vidas);
			}
		}
	}
	printf("\n Pontos: %d \n", pontos);
	printf("------------------------------------------------------------------------\n");
	exit(pontos);
}
