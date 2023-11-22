#ifndef __SERVER_H
#define __SERVER_H
    
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <errno.h>
    #include <semaphore.h>

    #include <netinet/in.h>
    #include <arpa/inet.h>
    
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/ipc.h>

    #define MAX_CONN 10 //Nro maximo de conexiones en espera
    #define BUFFER_TIME_SLEEP 1 //Tiempo de espera entre cada carga de buffer
    
#endif