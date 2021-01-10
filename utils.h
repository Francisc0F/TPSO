#ifndef UTIL_H
#define UTIL_H



#define SERVERFIFO             "./serverFIFO"	  
#define CLIPREFIXO             "./c_"
#define ADMINTEMP              "./temp"

extern int countCli;

typedef struct c cliente, *pcliente;
struct c {
	char pid[100];
	char nome[100];
	char jogo[100];
	int pipesJogo[2];
	char ultimaMsg[400];
	int pontos;
	pcliente prox;
};

typedef struct m mensagem;
struct m {
	char nome[100];
	char pid[100];
	char msg[400];
	int erro;
};


typedef struct t TDados;
struct t {
	int pipe;
	char pid[100];
	pthread_t tid;
	void * ret;
};







pcliente getClienteByName(pcliente lista, char * nome);

pcliente removerCliente(pcliente lista, char * nome);

void listaCliente(pcliente aux, FILE * p);

void listarClientes(pcliente lista);

int existe(pcliente lista,char * nome);

pcliente adicionarCli(pcliente lista, mensagem m, char * pid);

//comu

void OK(char * c_pipe);

void RES(char * c_pipe, char * info);

void ERROR(char * c_pipe, char * errorMsg);



char * getFifoCliWithPid(char * pid);

void splitString(char * str, char * delim, char *dest , int * count);

void menu();


#endif 
