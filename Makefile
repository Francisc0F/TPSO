#todo better version
all: cliente.c  admin.c g_jogo.c
	gcc -g -Wall -o cliente cliente.c cliente_utils.c utils.c
	gcc -g -Wall -o admin admin.c utils.c
	gcc -g -Wall -o g_jogo g_jogo.c

cliente: cliente.c 
	gcc -g -Wall -o cliente cliente.c cliente_utils.c utils.c

jogo: g_jogo.c 
	gcc -g  -Wall -o g_jogo g_jogo.c

admin: admin.c
	gcc -g -Wall -o admin admin.c utils.c 
clean: 
	$(RM) cliente admin g_jogo




