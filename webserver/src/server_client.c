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

#include <arpa/inet.h> // Add this line to include the header file

#include "../inc/server_client.h"

#define BUFFER_COMUNIC_SIZE 16384
#define IP_ADDR_SIZE 20
#define HTML_SIZE 8192
#define HTML_HEADER_SIZE 4096

#define FILE_HTML_HEADER_ADDR  "public/html/header.html\0"
//#define FILE_CSS_ADDR "public/css/style.css\0"
//#define FILE_HTML_BODY_ADDR "public/html/body.html\0"
//#define FILE_HTML_FOOTER_ADDR "public/html/footer.html\0"

/*Funciones privadas*/

static int get_string_from_file(char *file_name, char *string);

/*Funciones de la biblioteca*/

/**
 * @brief 
 * 
 * @param s_aux 
 * @param pDireccionCliente 
 * @param puerto 
 * @param buffer 
 * @return int 
 */
int ProcesarCliente(int s_aux, struct sockaddr_in *pDireccionCliente, int puerto, shared_buffer *buffer)
{
    char bufferComunic[BUFFER_COMUNIC_SIZE];
    char ipAddr[IP_ADDR_SIZE];
    int Port;
    float tempCelsius;
    int Mensaje_Valido = 0;
    char HTML[HTML_SIZE];
    char encabezadoHTML[HTML_HEADER_SIZE];

    strcpy(ipAddr, inet_ntoa(pDireccionCliente->sin_addr));
    Port = ntohs(pDireccionCliente->sin_port);

    // Recibe el mensaje del cliente
    if (recv(s_aux, bufferComunic, sizeof(bufferComunic), 0) == -1)
    {
        fprintf(stderr, "Error en recv");
        return -1;
    }
    printf("* Recibido del navegador Web %s:%d:\n%s\n",
           ipAddr, Port, bufferComunic);

    printf("* Obteniendo la temperatura del buffer...\n");
    printf("direccion de buffer: %p\n", buffer);

    // Obtiene la temperatura del buffer

    print_buffer(buffer);
        
    if (buffer_get(buffer, &tempCelsius))
    {
        fprintf(stderr, "Error en buffer_get");
        return -1;
    }
    printf("* Temperatura obtenida del buffer: %f\n", tempCelsius);

    // Obtengo el promedio de los datos del buffer

    if (buffer_avg(buffer, &tempCelsius))
    {
        fprintf(stderr, "Error en buffer_avg");
        return -1;
    }
    printf("* Promedio de los datos del buffer: %f\n", tempCelsius);

    // Valida el mensaje recibido

    if(strstr(bufferComunic, "GET /gradosCelsius HTTP/1.1") != NULL)
    {
        Mensaje_Valido = 1;
    }

    // Genera el mensaje a enviar al cliente

    if (get_string_from_file(FILE_HTML_HEADER_ADDR, encabezadoHTML))
    {
        fprintf(stderr, "Error en get_sting_from_file");
        return -1;
    }

    if (Mensaje_Valido)
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
        fprintf(stderr, "Error en send");
        return -1;
    }

    // Cierra la conexion con el cliente actual
    close(s_aux);

    return 0;
}


static int get_string_from_file(char *file_name, char *string)
{
  FILE *file = NULL;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  file = fopen(file_name, "r");

  if (file == NULL)
  {
    fprintf(stderr, "Error al abrir el archivo %s\n", file_name);
    return -1;
  }

  while ((read = getline(&line, &len, file)) != -1)
  {
    strcat(string, line);
  }

  fclose(file);

  if (line)
  {
    free(line);
  }

  return 0;
}