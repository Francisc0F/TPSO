#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>



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


void OK(char * c_pipe){
	mensagem m;
	int fd_cl = open(c_pipe, O_WRONLY ); 	
				
	strcpy(m.msg, "OK");
	m.erro = 0;
	write(fd_cl, &m, sizeof(m)); // == sizeof(mensagem)
	 
	close(fd_cl);		
}

void RES(char * c_pipe, char * info){
	mensagem m;
	int fd_cl = open(c_pipe, O_WRONLY ); 	
				
	strcpy(m.msg, info);
	m.erro = 0;
	write(fd_cl, &m, sizeof(m)); // == sizeof(mensagem)
	 
	close(fd_cl);	
}

void ERROR(char * c_pipe, char * errorMsg){
	mensagem m;
	int fd_cl = open(c_pipe, O_WRONLY ); 	
	strcpy(m.msg, errorMsg);
	m.erro = 1;
	write(fd_cl, &m, sizeof(m)); // == sizeof(mensagem)
	close(fd_cl);
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
			//fprintf(stderr, "A registar jogador.\n");
			ptr = strtok(NULL, delim);
			if(existe(listaCli, m.nome) == 0){
				listaCli = adicionarCli(listaCli, m, ptr);
				if(numCli < countCli){
					fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
					OK(c_pipe);
					break;
				}else{
					fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
					ERROR(c_pipe,"Erro: adicionar pessoa;");
				}

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
		terminar();
		exit(EXIT_FAILURE);
	}else if(sig == SIGUSR2){
/*
		printf("\nSinal SIGUSR2. %ld, UID: %ld value:%d \n", 
		(long) siginfo->si_pid, 
		(long)siginfo->si_uid, 
		siginfo->si_value.sival_int);*/

		if(siginfo->si_value.sival_int == START_READ){
			//fprintf(stderr, "\nSTART_READ\n");
			
		}
		else if(siginfo->si_value.sival_int == STOP_READ){
			//fprintf(stderr, "\nSTOP_READ\n");
			//int m =	menu();
		}

	//validar de e' o admin a enviar sinal (long) siginfo->si_pid
		
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
	//int pid = getpid();
	/// printf("Pid: %d \n", pid);
	
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
  	if (fdServer < 0)
		exit(EXIT_FAILURE); 



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

		timeval.tv_sec = 20;
		timeval.tv_usec = 0;


		menu();


		readyfd = select(fdServer + 1, &rfds, NULL, NULL, &timeval);

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
		  		// todo make function
		  		int i ;
		  		for (i = 1; i < strlen(cmd); i++)
		  		{
		  			nome[(i-1)] = cmd[i];
		  		}
		  		printf("NOME %s\n",nome );


		  		pcliente c =  getClienteByName(listaCli, nome);

		  		if(c == NULL){
		  			printf("Cliente \"%s\" nao existe. \n",nome );
		  			continue;
		  		}else {
		  			char prefixo[20];
					strcpy(prefixo, CLIPREFIXO);
					char * c_pipe = strcat(prefixo, c->pid);
		  			RES(c_pipe, "Foi removido do campeonato.");
		  			listaCli = removerCliente(listaCli, nome);	
		  		}
		  	}	
		}
	
	}
	
	

	fprintf(stderr,"end main\n");
  	terminar();	
	return 0;	
}
