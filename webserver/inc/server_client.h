/**
 * @file server_client.h
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include "../inc/buffer.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

int ProcesarCliente(int s_aux, struct sockaddr_in *pDireccionCliente, int puerto, shared_buffer *buffer) ;

#endif // SERVER_CLIENT_H
