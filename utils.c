#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include "utils.h"



// admin manager clientes
int countCli;
pcliente listaCli;



void BroadCastSignal(int sig){
	pcliente aux = listaCli;

	while(aux != NULL){
		char fifo[100];
		union sigval val = {
			.sival_int = sig
		};
		sigqueue( atoi(aux->pid), SIGUSR2, val);
		usleep(20);
		aux = aux->prox;
	}
} 

void listaCliente(pcliente aux, FILE * p){
		fprintf(p, "--------------------------------------\n");
		fprintf(p, "Pid: %s\n", aux->pid);
		fprintf(p, "Nome: %s\n", aux->nome);
		fprintf(p, "Ultima msg: %s\n", aux->ultimaMsg);
		fprintf(p, "pontos: %d\n", aux->pontos);
		//fprintf(p,"--------------------------------------\n");
}

pcliente getClienteByName(pcliente lista, char * nome){
	pcliente aux = lista;
	while( aux != NULL){
		//printf("nome na lista: %s\n", aux->nome);
		if(strcmp(nome, aux->nome) == 0){
			return aux;
		}	

		aux = aux->prox;
	}
	return NULL;
}	

void listarClientes(pcliente lista){
	pcliente aux = lista;
	if(aux == NULL){
		printf("NAO HA CLIENTES.\n");
	}

	while( aux != NULL){
		listaCliente(aux, stdout);
		aux = aux->prox;
	}
}
int existe(pcliente lista,char * nome){
	pcliente at = lista;
	while(at != NULL){
		if(strcmp(at->nome, nome) == 0){
			printf("ja existe este jogador.\n");
			return 1;
		}
		at = at->prox;	
	}
	return 0;
}

pcliente adicionarCli(pcliente lista, mensagem m, char * pid){
	pcliente n = malloc(sizeof (cliente));

    if (n == NULL) {
    	perror("adiconarCli - ERROR\n");
    	return NULL;
    }
    countCli++;	
	strcpy(n->pid , pid);
    strcpy(n->nome, m.nome);
	strcpy(n->ultimaMsg, m.msg);
	n->leThread = NULL;
	n->waitGameThread = NULL;
	n->s = 0;
	n->prox = NULL;
	n->pontos = 0;
	n->pidJogoAtual = -1;
	n->inGame = 0;
 	//fprintf(stderr, "pid \"%s\"\n",n->pid);

	if(lista == NULL){
	 n->prox = lista;
	 lista = n;
	 	//	fprintf(stderr, "Novo jogador \"%s\" registado.\n",n->nome);
	 return lista;
	} else {
		pcliente at = lista;
		while(at->prox != NULL){
			at = at->prox;	
		}
		at->prox = n;
		//fprintf(stderr, "Novo jogador \"%s\" registado.\n",n->nome);
		return lista;
	}
}

void freeCliente(pcliente x){

	free(x->leThread);
	free(x->waitGameThread);
	free(x);
}

pcliente removerCliente(pcliente lista, char * nome){

	pcliente aux =  lista;
	pcliente prev = aux;
	while(aux != NULL){
		if(strcmp(aux->nome, nome) == 0) {
			if(aux == lista){
				lista = aux->prox;
				freeCliente(aux);
				countCli--;
				return lista;
			}else{
				prev->prox = aux->prox;
				countCli--;
				freeCliente(aux);
				return lista;
			}
		} 
		prev = aux;	
		aux = aux->prox;
	}

	return lista;
}



//comu
void OK(char * c_pipe){
	mensagem m;
	int fd_cl = open(c_pipe, O_WRONLY ); 	
	if(fd_cl == -1){
		perror("error: OK(char * c_pipe)");
		return;
	}

	strcpy(m.msg, "OK");
	m.erro = 0;
	write(fd_cl, &m, sizeof(m)); 
	 
	close(fd_cl);		
}

void RES(char * c_pipe, char * info){
	mensagem m;
	int fd_cl = open(c_pipe, O_WRONLY ); 	
	if(fd_cl == -1){
		perror("error: RES(char * c_pipe, char * info)");
		return;
	}
				
	strcpy(m.msg, info);
	m.erro = 0;
	if(write(fd_cl, &m, sizeof(m))){
		//fprintf(stderr, "error RES write\n");
	} else{
	 	fprintf(stderr, "error RES write\n");
	}

	close(fd_cl);	
}

void ERROR(char * c_pipe, char * errorMsg){
	mensagem m;
	int fd_cl = open(c_pipe, O_WRONLY );
	if(fd_cl == -1){
		perror("error: ERROR(char * c_pipe, char * errorMsg)");
		return;
	}

	strcpy(m.msg, errorMsg);
	m.erro = 1;
	write(fd_cl, &m, sizeof(m)); 
	close(fd_cl);
}



// menu



void menu() {
    puts("\nplayers - Listar jogadores em jogo.");
    puts("games- Listar jogos disponiveis. ");
    puts("k<user name> - Remover jogador de campeonato.");
    puts("s<user name> - Suspender ligacao de jogador.");
    puts("r<user name> - Retomar ligacao de jogador.");
    puts("end - Termina campeonato.");
    puts("exit - Desligar server.");
 
}


// string utils

void getFifoCliWithPid(char fifoName[], char * pid){

	strcpy(fifoName, CLIPREFIXO) ;
	strcat(fifoName, pid);
}


void getNomeUser(char * nome, char * string){
	for (int i = 1; i < strlen(string); i++){
		nome[(i-1)] = string[i];
	}
}

int validaDirname(char * name){
	char pre[2]= "g_";
	// nao funciona com pontos no nome do jogo 
	char * dot = strrchr(name, '.');
	if(pre[0] == *name){
		name++;
		if(pre[1] == *name ){
			if(dot == NULL){
				return 1;
			}	
		}
	}
	return 0;
}


void mostraJogos(char * dirname){
	printf("Jogos: \n");
	int n = 0;	
	DIR * dir;
	struct dirent * e;
	if((dir = opendir(dirname)) == NULL){
		perror("\n erro opendir");
	}else{
		while( (e = readdir(dir)) != NULL){
			if(validaDirname(e->d_name) == 1){
				printf("%s\n",e->d_name);
				n++;
			}
		}
		closedir(dir);
	}
	if(n==0)
		printf("Nao ha jogos na diretoria indicada.\n");
}

void getRandomJogo(char * jogo, char * dirname, int pid){
	int n = 0;	
	DIR * dir;
	struct dirent * e;

	if((dir = opendir(dirname)) == NULL){
		perror("\n erro opendir");
	}else{
		while( (e = readdir(dir)) != NULL){
			if(validaDirname(e->d_name) == 1){
				n++;
			}
		}
		closedir(dir);
	}
	int random = rand() % (n-1);
	int s = 0;
	//fprintf(stderr,"rand %d\n", random);
	if((dir = opendir(dirname)) == NULL){
		perror("\n erro opendir");
	}else{
		while( (e = readdir(dir)) != NULL){
			if(validaDirname(e->d_name) == 1){
				if(s == random){
					strcpy(jogo, e->d_name);
					return;
				}
				s++;
			}
		}
		closedir(dir);
	}
}




void terminarAdmin(){
	FILE * fp;
	fp = fopen (ADMINTEMP,"r");
	int pid = 0;

	if(fp){
		fscanf(fp, "%d", &pid);
		//printf("Admin PID : %d \n", pid);	
   		if (pid == getpid()){
   			int del = remove(ADMINTEMP);
   			if(!del){
      			//printf("temp file apagada.\n");
   			}
      		printf("Admin Terminado.\n");
  		}	
	}
	BroadCastSignal(2);
	
	unlink(SERVERFIFO);
		
}




int checkRunning(){
   	FILE * fp;
	if( access(ADMINTEMP, F_OK ) != -1 ) {
		fp = fopen (ADMINTEMP,"r");
		int pid = 0;
		if((fscanf(fp, "%d", &pid) == 1))
		return 1;			
		
	} else {
        fp = fopen (ADMINTEMP,"w");
		if(fp != NULL){
			fprintf (fp, "%d",getpid());
		}
		fclose(fp);
	}
	return 0;
}

void terminarTodosCli(){
  	pcliente aux = listaCli;

	while(aux != NULL){
		char fifo[100] = {0};
		getFifoCliWithPid(fifo, aux->pid);
	 	RES(fifo, "removido");
		listaCli = removerCliente(listaCli, aux->nome);	
		aux = aux->prox;
	}
}

void apagarTodosCli(){
  	pcliente aux = listaCli;

	while(aux != NULL){
		char fifo[100] = {0};
		getFifoCliWithPid(fifo, aux->pid);
		listaCli = removerCliente(listaCli, aux->nome);	
		aux = aux->prox;
	}

}

void BroadCastRES(char *  msg){
	pcliente aux = listaCli;

	while(aux != NULL){
		char fifo[100] = {0};
		getFifoCliWithPid(fifo, aux->pid);

	 	RES(fifo, msg);
		aux = aux->prox;
	}
}

