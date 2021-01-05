#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include "utils.h"


int fdServer = 0;

int countCli = 0;
pcliente listaCli = NULL;


void terminar(){
	
	FILE * fp;
	fp = fopen (ADMINTEMP,"r");

	if(fp){
		int pid = 0;
		fscanf(fp, "%d", &pid);
		printf("Admin PID : %d \n", pid);	
   		if (pid == getpid()){
   			int del = remove(ADMINTEMP);
   			if(!del)
      			printf("temp file apagada.\n");
      		printf("Admin Terminado.\n");
  		}	

	}


	unlink("./serverFIFO");
		
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

void removerTodosCli(){
  	pcliente aux = listaCli;

	while(aux != NULL){
		char fifo[100] = "";
		strcpy(fifo, getFifoCliWithPid(aux->pid));

		
	 	RES(fifo, "removido");

		listaCli = removerCliente(listaCli, aux->nome);	
		
		aux = aux->prox;
	}
}

void processMsg(mensagem m){
	fprintf(stderr, "\nMensagem recebida de \"%s\".\n", m.nome);
	int count = 0 ;
	char * delim = " ";
	char * ptr = strtok(m.msg, delim);
	

	int numCli = countCli;

	char prefixo[20];
	strcpy(prefixo, CLIPREFIXO);
	char * c_pipe = strcat(prefixo, m.pid);

	int i = 0;
	while(ptr != NULL){
		count++;
		// fazer login 	
		if(strcmp(ptr, "login") == 0 ){
		printf("login\n");
			ptr = strtok(NULL, delim);
			if(existe(listaCli, m.nome) == 0){
				listaCli = adicionarCli(listaCli, m, ptr);
				if(numCli < countCli){
					fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
					OK(c_pipe);
				
				}else{
					fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
					ERROR(c_pipe,"Erro: adicionar pessoa;");
				}

				// executa jogo 
			fprintf(stderr, "executa jogo \n");
				int FP[2];
				int PF[2];

				char jogo_stream[400]= {0};
				// 0 leitura / 1 escrita
				//FDs 0  stdin / 1 stdout / 2 stderr / 3 canal[0] / 4 canal[1]
				pipe(FP);
				pipe(PF);
				
				int n;	

				int res = fork();
				if(res == 0){
					// filho 	
					close(PF[1]);
					close(0);//fechar stdin
					dup(PF[0]);
					close(PF[0]);

					close(FP[0]);
					close(1);
					dup(FP[1]);
					close(FP[1]);
				
					//FDs 0  stdin / canal[1] * / 2 stderr / 3 canal[0] / 4 canal[1]
					execl("g_jogo", "g_jogo", NULL);	
					printf("erro jogo\n");
					//sleep(30);
					exit(3);
				} 
				close(FP[1]);
				close(PF[0]);

				char resJogo[30] = {0};
				fprintf(stderr, "fifo %d \n", FP[0]);
				while(1){
				ssize_t count = read(FP[0], jogo_stream, 399); 
					fprintf(stderr,"count %d", count);
					if(count > 0){
						jogo_stream[count] = '\0';
						fprintf(stderr,"%s",jogo_stream);
						
					}else {
						scanf("%s", resJogo);
						fprintf(stderr,"resJogo %s",resJogo);
						write(PF[1], resJogo, count);
					}
					/*if(count == -1){
						continue;		
					}else if(count == 0){
						break;
					}else{
						jogo_stream[count] = '\0';
						//write(STDIN_FILENO, jogo_stream, count);
						printf("leu\n");
					}	
					*/
					//fprintf(stderr,"LEU :%s\n", jogo_stream);
				//write(PF[1], "testar enviu", 12);
				}
				//	RES(c_pipe, jogo_stream);
				
			
				int cStatus = 0;
				waitpid(res, &cStatus, 0);
				printf("status :%d\n", cStatus);

			}else {
				fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
				ERROR(c_pipe,"Nome ja existe.");
			}

		}else if(strcmp(ptr, "#mygame") == 0 ){
				char * buffer = NULL;
				size_t bufferSize = 0;
				FILE * ss = open_memstream(&buffer, &bufferSize);
				
				pcliente c = getClienteByName(listaCli, m.nome);
				if(c == NULL){
					fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
					ERROR(c_pipe,"Erro: pessoa nao esta registada no sistema.");
				}else{
					strcpy(c->ultimaMsg, ptr);	
					fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
				
					listaCliente(c, ss);
					
					fclose(ss);
					RES(c_pipe, buffer);

					free(buffer);
				}

		}else if(strcmp(ptr, "#quit") == 0 ){
		listaCli = removerCliente(listaCli, m.nome);
			if(numCli > countCli){
				fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
				OK(c_pipe);
				listarClientes(listaCli);
			}else{
				fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
				ERROR(c_pipe,"Erro: Nao foi possivel remover.");
			}
			
		}else{
			perror("mensagem invalida lida.\n");
		}
		break;

		ptr = strtok(NULL, delim);
		i++;
	}

}

void sig_handler(int sig, siginfo_t *siginfo, void *context){
		
		
	if(sig == SIGINT){
		if(countCli > 0){
			removerTodosCli();	
		}
		
		terminar();
		exit(EXIT_FAILURE);
	}else if(sig == SIGUSR2){

		
	}

}



int main(int argc , char **argv) {
	char *g = "GAMEDIR";
	char *pg = NULL;
	char *m = "MAXPLAYERS";
	char *pm = NULL;
	
	// sinais
	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	
	act.sa_sigaction = &sig_handler;
	act.sa_flags = SA_SIGINFO;
	if(sigaction(SIGINT, &act, NULL) < 0 ){
		perror("SIGINT sigaction");
		terminar();
		return 0;
	}

	if(sigaction(SIGUSR2, &act, NULL) < 0 ){
		perror("SIGUSR2 sigaction");
		return 0;
	}


	if(checkRunning() == 1){
		perror("already running.");
		terminar();
		return 1;
	}

	// comandos
	
	
	if(argc > 3){
		printf("Argumentos a mais\n");
		terminar();
		return 0;
	}
	pg = getenv(g);
	if(pg == NULL || *pg == '\0' ){
		printf("Nao foi Possivel Encontrar [%s]\n", g);
		terminar();
		return 0;
	}
	pm = getenv(m);
	
	if(pm == NULL || *pm == '\0'){
		printf("Nao foi Possivel Encontrar [%s]\n", m);
		terminar();
		return 0;
	}else if(atoi(pm) > 30){
		printf("[%s] Nao pode ser maior que 30\n", m);
		terminar();
		return 0;
	}

	int duracaoCamp = 0, tempEspera = 0;
	int opt;
	int flagd = 0;
	int flagt = 0;

	while((opt =  getopt(argc, argv, "d:t:")) != -1 ){
		switch(opt)
		{
			case 'd':
				flagd = 1;
				duracaoCamp = atoi(optarg);
				if(duracaoCamp != 0){
					fprintf(stderr,"Duracao Campeonato: %d \n", duracaoCamp);
				}else{
					fprintf(stderr,"Duracao Campeonato Invalida \n");
					terminar();
					return 0 ;
				}
					
				break;
			break;
			case 't':
				flagt = 1;
				tempEspera = atoi(optarg);
				if(tempEspera != 0){
					fprintf(stderr,"Tempo de espera: %d \n", tempEspera);
				}else {
					fprintf(stderr,"Tempo de espera Invalido\n");
					terminar();
					return 0;
				}
				break;
			case '?':
				fprintf( stderr,"Argumento invalido -%c. \n", optopt);
				terminar();
				return 0;
				break;
			default:	
				return 1;
					fprintf(stderr, "getopt");
				break;
		}
	}
	if(flagd == 0 && flagt == 0){
		printf("Falta indicar tempo de espera -t\n");
		printf("Falta indicar duracao do campeonato -d\n");
		terminar();
		return 0;
	}
	if(flagd == 0){
		printf("Falta indicar duracao do campeonato -d\n");
		terminar();
		return 0;
	}
	if(flagt == 0){
		printf("Falta indicar tempo de espera -t\n");
		terminar();
		return 0;
	}



	// start comunicacao 
	// read only 
	mkfifo(SERVERFIFO, 0666); 
  	
  	fdServer = open(SERVERFIFO, O_RDWR | O_NONBLOCK);
  	if (fdServer < 0){
  		perror("SERVERFIFO");
  		exit(EXIT_FAILURE); 
  	}
		



  	fprintf(stderr,"----------------------------------------------------\n");
  	fprintf(stderr,"A espera de clientes. standby\n");
  	fprintf(stderr,"----------------------------------------------------\n");
  	  	// wait 

  	// setup select()
	fd_set rfds;
	struct timeval timeval;

	int readyfd;

	char cmd[100];


	while(1){

		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(fdServer, &rfds);	

		//timeval.tv_sec = 20;
		//timeval.tv_usec = 0;


		menu();


		readyfd = select(fdServer + 1, &rfds, NULL, NULL, NULL);

		if(readyfd == -1){
			perror("error: select()\n");
		}

		if(readyfd == 0){
			printf("Listening...\n");
			continue;
		}
		
		
		if(FD_ISSET(fdServer, &rfds)){
			mensagem mLida;
			size_t count = read(fdServer, &mLida, sizeof(mensagem));
	    	if (count == sizeof(mensagem)) {    
			   	processMsg(mLida);
			}
			if(count == 0 || count == -1){
				continue;	
			}

		}

  	
		if(FD_ISSET(STDIN_FILENO, &rfds)){
			scanf("%[^\n]%*c",cmd);
			
			if(strcmp(cmd, "players") == 0){

  				listarClientes(listaCli);

		  	}else if(cmd[0] == 'k'){
		  		if(strlen(cmd) == 1){
		  			printf("faltam parametros.\n");
		  			continue;
		  		}
		  		char nome[100];
		  		memset(nome, '\0',100);
		  		
		  		int i ;
		  		for (i = 1; i < strlen(cmd); i++)
		  		{
		  			nome[(i-1)] = cmd[i];
		  		}
		  	

		  		pcliente c =  getClienteByName(listaCli, nome);

		  		if(c == NULL){
		  			printf("Cliente \"%s\" nao existe. \n",nome );
		  			continue;
		  		}else {
		  			 
		  			char fifo[100] = "";
					strcpy(fifo, getFifoCliWithPid(c->pid));

		  			listaCli = removerCliente(listaCli, nome);

		  			RES(fifo, "removido");
		  		}

		  	}else if(strcmp(cmd, "games")== 0){
		  			printf("Jogos: \n");
		  			printf("g_jogos \n");

		  	}
			else if(strcmp(cmd, "exit") == 0){

		  		if(countCli == 0){
		  			terminar();
		  			exit(EXIT_SUCCESS);	
		  		}else{
		  			removerTodosCli();
		  			terminar();	
		  			exit(EXIT_SUCCESS);
		  		}
		  	}

		}
	
	}
	
	

	fprintf(stderr,"end main\n");
  	terminar();	
	return 0;	
}

