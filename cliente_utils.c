#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "utils.h"
#include "cliente_utils.h"


// cliente comunication
void clienteEscreve(int fd, char * nome, char * texto){

	FILE * fp;
	fp = fopen (ADMINTEMP,"r");

	if(fp != NULL){
		int pid = 0;
		if(fscanf(fp, "%d", &pid) == 1){
			// printf("got admin pid.\n");
		}else {
			perror("error pid.\n");
		}

		mensagem info;
	
		strcpy(info.nome, nome);
		char pidStr[100];
		pid = getpid();
  		sprintf(pidStr, "%d", pid);

		strcpy(info.pid, pidStr);
		strcpy(info.msg, texto);

		if(write(fd, &info, sizeof(info)) != sizeof(info)){
			fprintf(stderr, "clienteEscreve write Sizeof(info)- ERROR");
		}
		
	}else {
		fprintf(stderr, "fopen (%s, r); - ERROR", ADMINTEMP);
	}
}

int clienteLe(char * fifo, int fd_cl){
	int success = 0;
	mensagem m;
	while(1){
		size_t count = read(fd_cl, &m, sizeof(mensagem));
		if (count == sizeof(mensagem)) {   
			if(m.erro == 1){
					fprintf(stderr, "\n--------Erro BadGateway--------");
				success = 0;					
			}else{
				success = 1;
			}
			fprintf(stderr, "\nserver: %s\n\n", m.msg);
			break;
		}	
	}
	return success;
}

void MenuCliente(int campC){
	printf("#mygame: dados do utilizador. \n");
	printf("#quit: desistir do jogo. \n");
	if(campC == 1){
		printf("#pjogo: jogo seguinte. \n");
	}
}
