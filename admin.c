#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include "utils.h"


int fdServer = 0;

int countCli = 0;
pcliente listaCli = NULL;

void * lerPipeAnonimo(void * arg){
	char stream[400] = {0};
	TDados * info = (TDados *) arg;

	int pip = info->pipe;
	char pid[100];
	strcpy(pid, info->pid);

	printf("THREAD info->pid %s\n",pid );	
	printf("info->pip %d\n",pip );	

	while(1){
		//fprintf(stderr,"inicia leitura info->pid %s\n", info->pid);
		ssize_t count = read(pip, stream, 399); 
		if(count > 0){
			stream[count] = '\0';
			char fifo[100];
			getFifoCliWithPid(fifo, pid);
			RES(fifo, stream);
			//fprintf(stderr,"leu %s",stream);
				
		}else {
		}
	}
}


void comecarCampeonato(){
	pcliente at = listaCli;
	

	while(at != NULL){
		int FP[2];
		int PF[2];
		pipe(FP);
		pipe(PF);

		at->pipesJogo[0] = FP[0];
		at->pipesJogo[1] = PF[1];

		int n;
		int res = fork();
		at->pidJogoAtual = res;
		if(res == 0){
			// filho 	
			close(PF[1]);// nao precisa da extremidade de escrita no pipe PF
			close(0);//fechar stdin
			dup(PF[0]);//duplica parte de leitura, para ler extremidade de leitura PF 
			close(PF[0]);// limpa duplicacao do PF[0] desnecessaria

			close(FP[0]);// nao precisa da extremidade de leitura no pipe FP
			close(1);//fechar stdout
			dup(FP[1]);//duplica parte de escrita, mete stdout apontar pra parte de escrita do FP
			close(FP[1]);// limpa duplicacao do FP[1] desnecessaria
					
			execl("jogos/g_jogo", "jogos/g_jogo", NULL);	
			printf("erro jogo\n");
			//sleep(30);
			exit(3);
		} 
		close(FP[1]);
		close(PF[0]);

	

		//listaCliente(at, stdout);
		at->leThread = malloc(sizeof (TDados));
		char pid[100];
		strcpy(pid, at->pid);
		strcpy(at->leThread->pid, pid);
		at->leThread->pipe = FP[0];

		//fprintf(stderr,"antes thread %s",pid );
		int t_leJogo = pthread_create(
		& at->leThread->tid,
		NULL,
		lerPipeAnonimo,
		(void *) at->leThread);
		if(t_leJogo == 0){
			//fprintf(stderr,"Thread Created");
		}

		//sleep(1);	
		at = at->prox;	
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

void terminar(){
	
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

void removerTodosCli(){
  	pcliente aux = listaCli;

	while(aux != NULL){
		char fifo[100] = {0};
		getFifoCliWithPid(fifo, aux->pid);

		
	 	RES(fifo, "removido");

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


void escrevePipeAnonimo(void * arg){
	TDados * info = (TDados *) arg; 
	//fprintf(stderr,"escreveu: %s\n",info->msg);
	write(info->pipe, strcat(info->msg, "\n"), strlen(info->msg) + 1);
}


void processMsg(mensagem m){
	//fprintf(stderr, "\nMensagem recebida de \"%s\".\n", m.nome);
	int count = 0 ;
	char * delim = " ";

	char msg[200] = {0};
	char pid[100] = {0};
	char nome[100] = {0};

	strcpy(msg, m.msg);
	strcpy(pid, m.pid);
	strcpy(nome, m.nome);

	char * ptr = strtok(msg, delim);
	int numCli = countCli;

	char prefixo[20] = {0};
	strcpy(prefixo, CLIPREFIXO);
	char * c_pipe = strcat(prefixo, pid);

	int i = 0;
	while(ptr != NULL){
		count++;
		// fazer login 	
		if(strcmp(ptr, "login") == 0 ){
			printf("login\n");
			ptr = strtok(NULL, delim);
			if(existe(listaCli, nome) == 0){
				listaCli = adicionarCli(listaCli, m, pid);
				if(numCli < countCli){
					fprintf(stderr, "Novo jogador \"%s\".\n", nome);
					if(countCli == 2){
						BroadCastRES("\n!!!Campeonato comecou!!!");
						comecarCampeonato();

					} else {
						RES(c_pipe, "Aguarde que campeonato comece.");
					}
					
				}else{
					//fprintf(stderr, "Mensagem enviada para \"%s\".\n", nome);
					ERROR(c_pipe,"Erro a adicionar pessoa;");
					return;
				}


			}else {
				fprintf(stderr, "Mensagem enviada para \"%s\".\n", m.nome);
				ERROR(c_pipe,"Nome ja existe.");
				return;
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
			// escreve no jogo respetivo
			pcliente c = getClienteByName(listaCli, m.nome);
			if(c == NULL){
				return;
			}else if(c->s == 0){
				TDados escreve;
				strcpy(escreve.pid, m.pid);
				escreve.pipe = c->pipesJogo[1];
				strcpy(escreve.msg, m.msg);
				escrevePipeAnonimo(&escreve);
			}	
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
	int numJogadoresMax = 30;

	// default dir
	char currentDir[100] = "/home/francisco/Desktop/so/TP";
	
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
	}else{
		strcpy(currentDir, pg);
	}

	pm = getenv(m);
	
	if(pm == NULL || *pm == '\0'){
		printf("Nao foi Possivel Encontrar [%s]\n", m);
	
	}else if(atoi(pm) > 30){
		printf("[%s] Nao pode ser maior que 30\n", m);
		terminar();
		return 0;
	}else{
		numJogadoresMax = atoi(pm);
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
		  		char nome[100]= {0};
		  		getNomeUser(nome, cmd);
		  		pcliente c =  getClienteByName(listaCli, nome);

		  		if(c == NULL){
		  			printf("Cliente \"%s\" nao existe. \n",nome );
		  			continue;
		  		}else {
		  			 
		  			char fifo[100] = {0};
					getFifoCliWithPid(fifo, c->pid);

		  			listaCli = removerCliente(listaCli, nome);

		  			RES(fifo, "removido");
		  		}

		  	}else if(cmd[0] == 's'){
		  		if(strlen(cmd) == 1){
		  			printf("faltam parametros.\n");
		  			continue;
		  		}
		  		char nome[100]= {0};
		  		getNomeUser(nome, cmd);
		  		pcliente c =  getClienteByName(listaCli, nome);
		  		c->s = 1;	

		  		if(c == NULL){
		  			printf("Cliente \"%s\" nao existe. \n",nome );
		  			continue;
		  		}else {
		  			char fifo[100] = {0};
					getFifoCliWithPid(fifo, c->pid);
		  			RES(fifo, "ligacao suspendida.");
		  		}

		  	}else if(cmd[0] == 'r'){
		  		if(strlen(cmd) == 1){
		  			printf("faltam parametros.\n");
		  			continue;
		  		}
		  		char nome[100]= {0};
		  		getNomeUser(nome, cmd);
		  		pcliente c =  getClienteByName(listaCli, nome);
				c->s = 0;	

		  		if(c == NULL){
		  			printf("Cliente \"%s\" nao existe. \n",nome );
		  			continue;
		  		}else {
		  			char fifo[100] = {0};
					getFifoCliWithPid(fifo, c->pid);
		  			RES(fifo, "ligacao retomada.");
		  		}

		  	}else if(strcmp(cmd, "games")== 0){
		  			printf("Jogos: \n");
		  			mostraJogos(currentDir);

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

