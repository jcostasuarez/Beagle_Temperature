/**
 * @file server_client.c
 * @author Juan Costa Suárez (jcostasurez@frba.utn.edu.ar)
 * @brief 
 * @version 0.1
 * @date 2023-11-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "../inc/server_client.h"

#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_COMUNIC_SIZE 16384
#define IP_ADDR_SIZE 20
#define HTML_SIZE 8192
#define HTML_HEADER_SIZE 4096


int ProcesarCliente(int s_aux, struct sockaddr_in *pDireccionCliente, int puerto, struct shared_buffer *buffer)
{
    char bufferComunic[BUFFER_COMUNIC_SIZE];
    char ipAddr[IP_ADDR_SIZE];
    int Port;
    float tempCelsius;
    int tempValida = 0;
    char HTML[HTML_SIZE];
    char encabezadoHTML[HTML_HEADER_SIZE];

    char file_html_header[] = "../public/server_header.html";
    char file_css[] = "../public/css/style.css/0";

    strcpy(ipAddr, inet_ntoa(pDireccionCliente->sin_addr));
    Port = ntohs(pDireccionCliente->sin_port);

    // Recibe el mensaje del cliente
    if (recv(s_aux, bufferComunic, sizeof(bufferComunic), 0) == -1)
    {
        perror("Error en recv");
        return -1;
    }
    printf("* Recibido del navegador Web %s:%d:\n%s\n",
           ipAddr, Port, bufferComunic);

    // Obtener la temperatura desde la ruta.
    if (memcmp(bufferComunic, "GET /", 5) == 0)
    {
        printf("%c\n\n", bufferComunic[5]);
        if (sscanf(&bufferComunic[5], "%f", &tempCelsius) == 1)
        {
            // Conversion done successfully.
            tempValida = 1;
        }
    }

    if (get_sting_from_file(file_html_header, encabezadoHTML))
    {
        perror("Error en get_sting_from_file");
        return -1;
    }

    if (tempValida)
    {
        sprintf(HTML,
                "%s<p>%f grados Celsius equivale a %f grados Fahrenheit</p>",
                encabezadoHTML, tempCelsius, tempCelsius * 1.8 + 32);
    }
    else
    {
        sprintf(HTML,
                "%s<p>El URL debe ser http://dominio:%d/gradosCelsius.</p>",
                encabezadoHTML, puerto);
    }

    sprintf(bufferComunic,
            "HTTP/1.1 200 OK\n"
            "Content-Length: %ld\n"
            "Content-Type: text/html; charset=utf-8\n"
            "Connection: Closed\n\n%s",
            strlen(HTML), HTML);

    printf("* Enviado al navegador Web %s:%d:\n%s\n",
           ipAddr, Port, bufferComunic);

    // Envia el mensaje al cliente
    if (send(s_aux, bufferComunic, strlen(bufferComunic), 0) == -1)
    {
        perror("Error en send");
        return -1;
    }

    // Cierra la conexion con el cliente actual
    close(s_aux);
}