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
#include "utils.h"


int fdServer = 0;
int campComecou = 0;
int duracaoCamp = 0;
int countCli = 0;
pcliente listaCli = NULL;

char currentDirGlobal[100];

void * lerPipeAnonimo(void * arg){
	char stream[400] = {0};
	TDados * info = (TDados *) arg;

	int pip = info->pipe;
	char pid[100];
	strcpy(pid, info->pid);

	//printf("THREAD info->pid %s\n",pid );	
	//printf("info->pip %d\n",pip );	

	int tStatus = 1;
	while(1){
		//fprintf(stderr,"inicia leitura info->pid %s\n", info->pid);
		ssize_t count = read(pip, stream, 399); 
		if(count > 0){
			stream[count] = '\0';
			char fifo[100];
			getFifoCliWithPid(fifo, pid);
			RES(fifo, stream);
		}else {
		
			break;
		}
	}
	pthread_exit(&tStatus);

	return NULL;
}

void * waitGameEnd(void * arg){
	//sleep(1);
	pcliente c = (cliente *) arg;
	int tStatus;
	int x = waitpid(atoi(c->waitGameThread->pid), &c->waitGameThread->gameStatus, 0);
	if(x == -1 ){
		fprintf(stderr,"error -1 waitpid");
	}else if(WIFEXITED(c->waitGameThread->gameStatus)){
		c->pontos += WEXITSTATUS(c->waitGameThread->gameStatus);
		//fprintf(stderr,"points %d",WEXITSTATUS(info->gameStatus));
	}
	c->waitGameThread->inGame = 0;
	pthread_exit(&tStatus);

	return NULL;
}

void criarJogo(pcliente c){
	int FP[2];
	int PF[2];
	pipe(FP);
	pipe(PF);

	c->pipesJogo[0] = FP[0];
	c->pipesJogo[1] = PF[1];
	fprintf(stderr,"criarJogo\n");

	int n;
	int res = fork();
	if(res == 0){
		// filho 	
		srand(getpid());

		close(PF[1]);// nao precisa da extremidade de escrita no pipe PF
		close(0);//fechar stdin
		dup(PF[0]);//duplica parte de leitura, para ler extremidade de leitura PF 
		close(PF[0]);// limpa duplicacao do PF[1] desnecessaria

		close(FP[0]);// nao precisa da extremidade de leitura no pipe FP
		close(1);//fechar stdout
		dup(FP[1]);//duplica parte de escrita, mete stdout apontar pra parte de escrita do FP
		close(FP[1]);// limpa duplicacao do FP[1] desnecessaria

		char jogo[100];
		getRandomJogo(jogo, currentDirGlobal,  getppid());
		char diretoriaJogo[100];
		sprintf(diretoriaJogo, "%s/%s",currentDirGlobal, jogo);
		fprintf(stderr, "jogo ->%s\n",diretoriaJogo );
		execl(diretoriaJogo,diretoriaJogo, NULL);	
		//execl("jogos/g_jogo", "jogos/g_jogo", NULL);	
		
		printf("erro jogo: %s\n", diretoriaJogo);
		//sleep(30);
		exit(0);
	} 

	c->pidJogoAtual = res;

	close(FP[1]);
	close(PF[0]);

	if(c->leThread != NULL){
		free(c->leThread);
	}
	c->leThread = malloc(sizeof (TDados));
	strcpy(c->leThread->pid, c->pid);
	c->leThread->pipe = FP[0];

	//fprintf(stderr,"antes thread %s",pid );
	int t_leJogo = pthread_create(
	& c->leThread->tid,
	NULL,
	lerPipeAnonimo,
	(void *) c->leThread);
	/*if(t_leJogo == 0){
		//fprintf(stderr,"Thread Created");
	}*/
	
	if(c->waitGameThread != NULL){
		free(c->waitGameThread);
	}

	c->waitGameThread = malloc(sizeof (TDados));
	c->waitGameThread->inGame = 1;
	c->waitGameThread->gameStatus = -1;

	char pid[100];
	sprintf(pid, "%d", c->pidJogoAtual);
	strcpy(c->waitGameThread->pid, pid);

	int t_waitGameEnd = pthread_create(
	& c->waitGameThread->tid,
	NULL,
	waitGameEnd,
	(void *) c);
		fprintf(stderr,"fim CriaJogo\n");
}

void comecarCampeonato(){

	pcliente at = listaCli;
	while(at != NULL){
		criarJogo(at);
		at = at->prox;
	}
}

void escrevePipeAnonimo(void * arg){
	TDados * info = (TDados *) arg; 
	//fprintf(stderr,"escreveu: %s\n",info->msg);
	write(info->pipe, strcat(info->msg, "\n"), strlen(info->msg) + 1);
}

void terminarCampeonato(){
	pcliente aux = listaCli;
	pcliente vencedor = aux;

	// desligar jogos
	while(aux != NULL){
		if(aux->waitGameThread != NULL && aux->waitGameThread->inGame == 1){
			union sigval val = {
			//.sival_int = atoi(argv[1])
			};
			sigqueue( aux->pidJogoAtual, SIGUSR1, val);
		}

		if(aux->pontos > vencedor->pontos){
			vencedor = aux;
		}
		aux = aux->prox;
	}
	
	aux = listaCli;
	// tempo para sinais chegarem
	usleep(500);	
	while(aux != NULL){
		char fifo[100] = {0};
		getFifoCliWithPid(fifo, aux->pid);

	 	char t[100];
	 	sprintf(t,"%s Teve %d pontos.\nO vencedor foi %s com %d pontos",
	 	 CAMPTERMINOU, aux->pontos, vencedor->nome, vencedor->pontos);

		RES(fifo, t);
		aux = aux->prox;
	}

	// apagar jogadores
	apagarTodosCli();
}

void processMsg(mensagem m, int tempEspera){
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
						alarm(tempEspera);
						RES(c_pipe, "Aguardar por mais jogadores.");
						

					} else {
						printf("c_pipe %s\n", c_pipe);
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

			if(countCli == 1){
				alarm(0);
		  		terminarCampeonato();
				campComecou = 0;
			}
			
		}else if(strcmp(ptr, "#pjogo") == 0 ){
			pcliente c = getClienteByName(listaCli, m.nome);
			if(c == NULL){
				return;
			}else if(c->waitGameThread != NULL && c->waitGameThread->inGame == 0){
				criarJogo(c);	
			}else{
				RES(c_pipe, "ainda nao terminou jogo atual.");
			}
			
		}else{
			// escreve no jogo respetivo
			pcliente c = getClienteByName(listaCli, m.nome);
			if(c == NULL){
				return;
			}else if(c->s == 0){
				if(c->waitGameThread != NULL){
					if(c->waitGameThread->inGame == 1){
					TDados escreve;
					strcpy(escreve.pid, m.pid);
					escreve.pipe = c->pipesJogo[1];
					strcpy(escreve.msg, m.msg);
					escrevePipeAnonimo(&escreve);
					}else{
						fprintf(stderr, "not in game\n");	
					}	
				}
				
			}	
		}
		break;

		ptr = strtok(NULL, delim);
		i++;
	}
}

void sig_handler(int sig, siginfo_t *siginfo, void *context){
	if(sig == SIGINT){
		terminarAdmin();
		exit(EXIT_FAILURE);
	}else if(sig == SIGUSR2){

	}else if(sig == SIGALRM){
		if(campComecou == 0){
			campComecou = 1;
			BroadCastRES(CAMPCOMECOU);
			comecarCampeonato();
			
			// timer campeonato
			//printf("%d\n",duracaoCamp);
			alarm(duracaoCamp);

		}else if(campComecou == 1){
			//printf("terminar camp %d\n",duracaoCamp);
			// campeonato terminou
			terminarCampeonato();
			campComecou = 0;
		}
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
		terminarAdmin();
		return 0;
	}

	if(sigaction(SIGUSR2, &act, NULL) < 0 ){
		perror("SIGUSR2 sigaction");
		return 0;
	}

	// alarm campeonato
	if(sigaction(SIGALRM, &act, NULL) < 0 ){
		perror("SIGALRM sigaction");
		return 0;
	}


	if(checkRunning() == 1){
		perror("already running.");
		terminarAdmin();
		return 1;
	}

	// comandos
	
	
	if(argc > 3){
		printf("Argumentos a mais\n");
		terminarAdmin();
		return 0;
	}
	pg = getenv(g);
	if(pg == NULL || *pg == '\0' ){
		printf("Nao foi Possivel Encontrar [%s]\n", g);
	}else{
		strcpy(currentDir, pg);
	}

	strcpy(currentDirGlobal, currentDir);
	pm = getenv(m);
	
	if(pm == NULL || *pm == '\0'){
		printf("Nao foi Possivel Encontrar [%s]\n", m);
	
	}else if(atoi(pm) > 30){
		printf("[%s] Nao pode ser maior que 30\n", m);
		terminarAdmin();
		return 0;
	}else{
		numJogadoresMax = atoi(pm);
	}

	
	int tempEspera = 0;
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
					terminarAdmin();
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
					terminarAdmin();
					return 0;
				}
				break;
			case '?':
				fprintf( stderr,"Argumento invalido -%c. \n", optopt);
				terminarAdmin();
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
		terminarAdmin();
		return 0;
	}
	if(flagd == 0){
		printf("Falta indicar duracao do campeonato -d\n");
		terminarAdmin();
		return 0;
	}
	if(flagt == 0){
		printf("Falta indicar tempo de espera -t\n");
		terminarAdmin();
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
			   	processMsg(mLida, tempEspera);
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

		  		int s = pthread_cancel(c->leThread->tid);
		  		void * res;
		  		pthread_join(c->leThread->tid, &res);

		  		if(res == PTHREAD_CANCELED){
		  		  printf("thread was canceled\n");	
		  		}


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
		  			
		  			mostraJogos(currentDir);
		  	}
		  	else if(strcmp(cmd, "end") == 0){
		  		alarm(0);
		  		terminarCampeonato();
				campComecou = 0;
		  	}
			else if(strcmp(cmd, "exit") == 0){

		  		if(countCli == 0){
		  			terminarAdmin();
		  			exit(EXIT_SUCCESS);	
		  		}else{
		  			terminarTodosCli();
		  			terminarAdmin();	
		  			exit(EXIT_SUCCESS);
		  		}
		  	}

		}
	
	}
	
	

	fprintf(stderr,"end main\n");
  	terminarAdmin();	
	return 0;	
}

