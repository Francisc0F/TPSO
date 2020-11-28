#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"



// admin manager clientes
extern int countCli;

void listaCliente(pcliente aux, FILE * p){
		fprintf(p, "\n--------------------------------------\n");
		fprintf(p, "Pid: %s\n", aux->pid);
		fprintf(p, "Nome: %s\n", aux->nome);
		fprintf(p, "Ultima msg: %s\n", aux->ultimaMsg);
		fprintf(p,"--------------------------------------\n");
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

    memset(n, '\0', sizeof(cliente));

    if (n == NULL) {
    	perror("adiconarCli - ERROR\n");
    	return NULL;
    }
    countCli++;
	strcpy(n->pid , pid);

    strcpy(n->nome, m.nome);

	strcpy(n->ultimaMsg, m.msg);

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


pcliente removerCliente(pcliente lista, char * nome){

	pcliente aux =  lista;
	pcliente prev = aux;
	while(aux != NULL){
		if(strcmp(aux->nome, nome) == 0) {
			if(aux == lista){
				lista = aux->prox;
				free(aux);
				countCli--;
				return lista;
			}else{
				prev->prox = aux->prox;
				countCli--;
				free(aux);
				return lista;
			}
		} 
		prev = aux;	
		aux = aux->prox;
	}

	return lista;
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

void splitString(char * str, 
				char * delim, 
				char * dest,
				int * count){

	char * ptr = strtok(str, delim);
	int i = 0;
	while(ptr != NULL){
		(*count)++;
		strcpy(&dest[i], "coisas");
		//printf("%s\n", ptr); 
		printf("%s\n", ptr); 
		ptr = strtok(NULL, delim);
		i++;
	}

}
