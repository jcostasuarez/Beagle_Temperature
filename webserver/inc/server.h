#ifndef __SERVER_H
#define __SERVER_H
    
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <errno.h>
    #define MAX_CONN 10 //Nro maximo de conexiones en espera

    void ProcesarCliente(int s_aux, struct sockaddr_in *pDireccionCliente, int puerto);

#endif