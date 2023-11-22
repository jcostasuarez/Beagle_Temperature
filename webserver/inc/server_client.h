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

int ProcesarCliente(int s_aux, struct sockaddr_in *pDireccionCliente, int puerto);

#endif // SERVER_CLIENT_H
