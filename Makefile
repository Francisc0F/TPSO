FLGS = -g -Wall 
DELETE = admin admin.o utils.o cliente cliente.o cliente_utils.o jogos/g_jogo jogos/g_jogo.o

all: admin cliente jogo

admin: admin.o utils.o
	gcc $(FLGS) -o admin admin.o utils.o -lpthread

admin.o: admin.c utils.h
	gcc -c admin.c


cliente: cliente.o cliente_utils.o utils.o
	gcc $(FLGS) -o cliente cliente.o cliente_utils.o utils.o

cliente.o: cliente.c cliente_utils.h utils.h 
	gcc -c cliente.c 
	
cliente_utils.o: cliente_utils.c utils.h
	gcc -c cliente_utils.c		

utils.o: utils.c 
	gcc -c utils.c


jogo: jogo.o
	gcc $(FLGS) -o jogos/g_jogo jogos/g_jogo.o

jogo.o: jogos/g_jogo.c
	gcc -c jogos/g_jogo.c -o jogos/g_jogo.o

clean: 
	$(RM) $(DELETE) 
