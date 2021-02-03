#ifndef CLIENTE_UTILS
#define CLIENTE_UTILS


// cliente comunication
void clienteEscreve(int fd, char * nome, char * texto);

int clienteLe(char * fifo, int fd_cl);

//UI
void MenuCliente();

#endif 