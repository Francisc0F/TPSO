#ifndef UTIL_H
#define UTIL_H



#define SERVERFIFO             "./serverFIFO"	  
#define CLIPREFIXO             "./c_"
#define ADMINTEMP              "./temp"
#define CAMPCOMECOU              "\nCampeonato comecou."
#define CAMPTERMINOU             "Campeonato terminou."

extern int countCli;


typedef struct t TDados, *pTDados;
struct t {
	int pipe;
	char msg[100];
	char pid[100];
	pthread_t tid;
	int inGame;
	int gameStatus;
	void * ret;
};


typedef struct c cliente, *pcliente;
struct c {
	char pid[100];
	char nome[100];
	char jogo[100];
	int pipesJogo[2];
	char ultimaMsg[400];
	int pidJogoAtual;
	int pontos;
	int s;
	int inGame;
	pTDados leThread;
	pTDados waitGameThread;
	pcliente prox;

};

extern pcliente listaCli;

typedef struct m mensagem;
struct m {
	char nome[100];
	char pid[100];
	char msg[400];
	int erro;
};



pcliente getClienteByName(pcliente lista, char * nome);

pcliente removerCliente(pcliente lista, char * nome);

void terminarTodosCli();

void apagarTodosCli();

void freeCliente(pcliente x);

void listaCliente(pcliente aux, FILE * p);

void listarClientes(pcliente lista);

int existe(pcliente lista,char * nome);

pcliente adicionarCli(pcliente lista, mensagem m, char * pid);

//comu

void OK(char * c_pipe);

void RES(char * c_pipe, char * info);

void ERROR(char * c_pipe, char * errorMsg);

void menu();

void getFifoCliWithPid(char fifoName[], char * pid);

void splitString(char * str, char * delim, char *dest , int * count);

int validaDirname(char * name);



void getNomeUser(char * nome, char * string);

void mostraJogos(char * dirname);

int checkRunning();

void getRandomJogo(char * jogo, char * dirname, int pid);



void BroadCastRES(char *  msg);

void BroadCastSignal(int sig);

void terminarAdmin();

#endif 
