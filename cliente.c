#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include "utils.h"
#include "cliente_utils.h"

int countCli = 0;

int fd_cl;
int fd; 

char * getFifoName( char * buffer){

  	char pidStr[100];
  	sprintf(pidStr, "%d", getpid());

	char prefixo[100] = CLIPREFIXO;

	buffer = strcat(prefixo, pidStr);
	return buffer;
}



void terminar(){
	printf("Cliente terminado.\n");
	char * fifo = NULL;
	fifo = getFifoName(fifo);

	unlink(fifo);

	close(fd_cl);
	close(fd);   
	printf("------------------------------------------------ \n");
	exit(EXIT_FAILURE);
}


void sig_handler(int sig, siginfo_t *siginfo, void *context){

	if(sig == SIGINT){
		terminar();
		exit(EXIT_FAILURE);
	}
		
	printf("Terminar Jogo. Sending PID: %ld, UID: %ld value:%d \n", 
	(long) siginfo->si_pid, (long)siginfo->si_uid, siginfo->si_value.sival_int);

}


void MenuCliente(){
	printf("#mygame: dados do utilizador. \n");
	printf("#quit: desistir do jogo. \n");
}


int main(int argc, char**argv) {

	// sinais
	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	
	act.sa_sigaction = &sig_handler;
	act.sa_flags = SA_SIGINFO;
	if(sigaction(SIGUSR2, &act, NULL) < 0 ){
		perror("SIGUSR2 sigaction");
		return 0;
	}

	if(sigaction(SIGINT, &act, NULL) < 0 ){
		perror("SIGINT sigaction");
		return 0;
	}

	// comunication
	fd = open(SERVERFIFO, O_WRONLY ); 
  	if (fd < 0){
  		fprintf(stderr, "Servidor Off. A terminar cliente.\n");
  		exit(EXIT_FAILURE);  
  	}


  	char nome[100];
  	memset(nome, '\0', sizeof(nome));

	printf("------------------------------------------------ \n");
	printf("		Programa cliente \n");

	char pidStr[100];
  	sprintf(pidStr, "%d", getpid());

	char * fifo = NULL;
	fifo = getFifoName(fifo);
	
	mkfifo(fifo, 0666); 

	fd_cl = open(fifo, O_RDWR | O_NONBLOCK); 	

	if(fd_cl == -1){
		perror("error: fd_cl = open(fifo, O_RDWR | O_NONBLOCK);");
		terminar();
	}

  	char info[100];
  	strcpy(info,"login ");

	while(1){
		printf("Qual o seu nome: \n");
		scanf("%[^\n]%*c",nome);

		clienteEscreve(fd, nome, strcat(info ,pidStr));

		if(clienteLe(fifo, fd_cl) == 1) {
			break;
		}
	}


	

  	fprintf(stderr,"----------------------------------------------------\n");
  	fprintf(stderr,"Menu cliente\n");
  	fprintf(stderr,"----------------------------------------------------\n");


	fd_set rfds;
	struct timeval timeval;
	int readyfd;

  	char cmd[100];
	while(1){

		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(fd_cl, &rfds);	

		timeval.tv_sec = 20;
		timeval.tv_usec = 0;

  		MenuCliente();

		readyfd = select(fd_cl + 1, &rfds, NULL, NULL, &timeval);

		if(readyfd == -1){
			perror("error: select()\n");

		}

		if(readyfd == 0){
			printf("Listening...\n");
			continue;
		}

		if(FD_ISSET(fd_cl, &rfds)){
			int success = 0;
			mensagem m;
			
			size_t count = read(fd_cl, &m, sizeof(mensagem));
			if(count == 0 || count == -1){
				continue;
			}

			if (count == sizeof(mensagem)) {   
				if(m.erro == 1){
					fprintf(stderr, "\n--------Erro BadGateway--------");
					success = 0;					
				}else{
					success = 1;
				}

				if(strcmp(m.msg, "removido") == 0 ){
						fprintf(stderr, "\nserver: Foi removido do campeonato.\n\n");
						printf("A sair do Jogo.\n");
						terminar();	
				}else{
					fprintf(stderr, "\nserver: %s\n\n", m.msg);
				}
			}	
			
		}
		
		if(FD_ISSET(STDIN_FILENO, &rfds)){
			scanf("%s", cmd);
	  		if(strcmp(cmd, "#mygame") == 0){
					clienteEscreve(fd, nome, cmd);
					clienteLe(fifo, fd_cl);

			}else if(strcmp(cmd, "#quit") == 0){
					clienteEscreve(fd, nome, cmd);
					clienteLe(fifo, fd_cl);
					printf("A sair do Jogo.\n");
					terminar();
					break;
				
	  		}	
		}

  		
  	};
 

	return 0;
}
